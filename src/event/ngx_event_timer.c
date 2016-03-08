
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件和实现文件实现了定时器的相关功能。
// 定时器的实现流程：
// 1. 将所有定时器的到期时间值放到一个红黑树中.
// 2. 在进入等待事件的函数时，用所有定时器中最近的到期时间作为事件模型等待函数的超时时间.
// 3. 等待事件函数返回时，执行所有已到期的定时器事件.

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


#if (NGX_THREADS)
ngx_mutex_t  *ngx_event_timer_mutex;
#endif


// 存放所有定时器的红黑树。
ngx_thread_volatile ngx_rbtree_t  ngx_event_timer_rbtree;
// ngx_event_timer_rbtree红黑树的哨兵节点。
static ngx_rbtree_node_t          ngx_event_timer_sentinel;

/*
 * the event timer rbtree may contain the duplicate keys, however,
 * it should not be a problem, because we use the rbtree to find
 * a minimum timer value only
 */

// 定时器初始化函数，初始化全局变量ngx_event_timer_rbtree。
ngx_int_t
ngx_event_timer_init(ngx_log_t *log)
{
    ngx_rbtree_init(&ngx_event_timer_rbtree, &ngx_event_timer_sentinel,
                    ngx_rbtree_insert_timer_value);

#if (NGX_THREADS)

    if (ngx_event_timer_mutex) {
        ngx_event_timer_mutex->log = log;
        return NGX_OK;
    }

    ngx_event_timer_mutex = ngx_mutex_init(log, 0);
    if (ngx_event_timer_mutex == NULL) {
        return NGX_ERROR;
    }

#endif

    return NGX_OK;
}


// 计算再过多少毫秒会有定时器过期。
ngx_msec_t
ngx_event_find_timer(void)
{
    ngx_msec_int_t      timer;
    ngx_rbtree_node_t  *node, *root, *sentinel;

    if (ngx_event_timer_rbtree.root == &ngx_event_timer_sentinel) {
        return NGX_TIMER_INFINITE;
    }

    ngx_mutex_lock(ngx_event_timer_mutex);

    root = ngx_event_timer_rbtree.root;
    sentinel = ngx_event_timer_rbtree.sentinel;

    node = ngx_rbtree_min(root, sentinel);

    ngx_mutex_unlock(ngx_event_timer_mutex);

    timer = (ngx_msec_int_t) (node->key - ngx_current_msec);

    return (ngx_msec_t) (timer > 0 ? timer : 0);
}


// 处理已经过期的定时器事件。
void
ngx_event_expire_timers(void)
{
    ngx_event_t        *ev;
    ngx_rbtree_node_t  *node, *root, *sentinel;

    sentinel = ngx_event_timer_rbtree.sentinel;

    for ( ;; ) {

        ngx_mutex_lock(ngx_event_timer_mutex);

        root = ngx_event_timer_rbtree.root;

        if (root == sentinel) {
            return;
        }

        node = ngx_rbtree_min(root, sentinel);

        /* node->key <= ngx_current_time */

        if ((ngx_msec_int_t) (node->key - ngx_current_msec) <= 0) {
            ev = (ngx_event_t *) ((char *) node - offsetof(ngx_event_t, timer));

#if (NGX_THREADS)

            if (ngx_threaded && ngx_trylock(ev->lock) == 0) {

                /*
                 * We cannot change the timer of the event that is being
                 * handled by another thread.  And we cannot easy walk
                 * the rbtree to find next expired timer so we exit the loop.
                 * However, it should be a rare case when the event that is
                 * being handled has an expired timer.
                 */

                ngx_log_debug1(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                               "event %p is busy in expire timers", ev);
                break;
            }
#endif

            ngx_log_debug2(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                           "event timer del: %d: %M",
                           ngx_event_ident(ev->data), ev->timer.key);

            ngx_rbtree_delete(&ngx_event_timer_rbtree, &ev->timer);

            ngx_mutex_unlock(ngx_event_timer_mutex);

#if (NGX_DEBUG)
            ev->timer.left = NULL;
            ev->timer.right = NULL;
            ev->timer.parent = NULL;
#endif

            ev->timer_set = 0;

#if (NGX_THREADS)
            if (ngx_threaded) {
                ev->posted_timedout = 1;

                ngx_post_event(ev, &ngx_posted_events);

                ngx_unlock(ev->lock);

                continue;
            }
#endif

            ev->timedout = 1;

            ev->handler(ev);

            continue;
        }

        break;
    }

    ngx_mutex_unlock(ngx_event_timer_mutex);
}
