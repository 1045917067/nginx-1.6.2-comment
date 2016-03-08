
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件与实现文件实现了nginx 6个基本容器之一的ngx_queue_t双向链表及相关操作函数。

#include <ngx_config.h>
#include <ngx_core.h>


#ifndef _NGX_QUEUE_H_INCLUDED_
#define _NGX_QUEUE_H_INCLUDED_


typedef struct ngx_queue_s  ngx_queue_t;

// 双向链表核心结构体。
// 当链表有n个节点时，需要n+1个ngx_queue_t对象，n个代表每个节点，另一个代表整个链表。
// 使用双向链表时，往往
struct ngx_queue_s {
    // 代表节点的ngx_queue_t对象，prev指向上一个节点ngx_queue_t对象，
    // 代表整个链表的ngx_queue_t对象，prev指向链表最后一个元素。
    ngx_queue_t  *prev;
    // 代表节点的ngx_queue_t对象，next指向下一个节点ngx_queue_t对象，
    // 代表整个链表的ngx_queue_t对象，next指向链表第一个元素。
    ngx_queue_t  *next;
};


// 初始化双向链表q
#define ngx_queue_init(q)                                                     \
    (q)->prev = q;                                                            \
    (q)->next = q


// 双向链表h为空反会1，不为空返回0.
#define ngx_queue_empty(h)                                                    \
    (h == (h)->prev)


// 在双向链表h头部插入节点x
#define ngx_queue_insert_head(h, x)                                           \
    (x)->next = (h)->next;                                                    \
    (x)->next->prev = x;                                                      \
    (x)->prev = h;                                                            \
    (h)->next = x



#define ngx_queue_insert_after   ngx_queue_insert_head


// 在双向链表尾部插入节点x
#define ngx_queue_insert_tail(h, x)                                           \
    (x)->prev = (h)->prev;                                                    \
    (x)->prev->next = x;                                                      \
    (x)->next = h;                                                            \
    (h)->prev = x


// 取出双向链表h首节点
#define ngx_queue_head(h)                                                     \
    (h)->next

// 取出双向链表h尾节点
#define ngx_queue_last(h)                                                     \
    (h)->prev

// 运行层面没有作用，这样写强调h代表链表容器指针
#define ngx_queue_sentinel(h)                                                 \
    (h)

// 获取双向链表节点q的后一个节点
#define ngx_queue_next(q)                                                     \
    (q)->next

// 获取双向链表节点q的前一个节点
#define ngx_queue_prev(q)                                                     \
    (q)->prev


#if (NGX_DEBUG)

// 将x节点从双向链表里移除
#define ngx_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next

#endif


// 将h双向链表从q节点分开成两个链表，分开后前一个链表是h,后一个链表是n, n的首节点是q.
#define ngx_queue_split(h, q, n)                                              \
    (n)->prev = (h)->prev;                                                    \
    (n)->prev->next = n;                                                      \
    (n)->next = q;                                                            \
    (h)->prev = (q)->prev;                                                    \
    (h)->prev->next = h;                                                      \
    (q)->prev = n;

// 合并链表，将双向链表n添加到双向链表h的末尾。
#define ngx_queue_add(h, n)                                                   \
    (h)->prev->next = (n)->next;                                              \
    (n)->next->prev = (h)->prev;                                              \
    (h)->prev = (n)->prev;                                                    \
    (h)->prev->next = h;


// 获取q节点对象所隶属的结构体对象的指针
#define ngx_queue_data(q, type, link)                                         \
    (type *) ((u_char *) q - offsetof(type, link))


ngx_queue_t *ngx_queue_middle(ngx_queue_t *queue);
void ngx_queue_sort(ngx_queue_t *queue,
    ngx_int_t (*cmp)(const ngx_queue_t *, const ngx_queue_t *));


#endif /* _NGX_QUEUE_H_INCLUDED_ */
