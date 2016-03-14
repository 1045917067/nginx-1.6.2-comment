
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件和实现文件声明了ngx_peer_connection_t结构体，并实现了相关的操作函数。
// 这个结构体和相关函数用来把nginx作为客户端连接外部服务器。

#ifndef _NGX_EVENT_CONNECT_H_INCLUDED_
#define _NGX_EVENT_CONNECT_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


#define NGX_PEER_KEEPALIVE           1
#define NGX_PEER_NEXT                2
#define NGX_PEER_FAILED              4


typedef struct ngx_peer_connection_s  ngx_peer_connection_t;

typedef ngx_int_t (*ngx_event_get_peer_pt)(ngx_peer_connection_t *pc,
    void *data);
typedef void (*ngx_event_free_peer_pt)(ngx_peer_connection_t *pc, void *data,
    ngx_uint_t state);
#if (NGX_SSL)

typedef ngx_int_t (*ngx_event_set_peer_session_pt)(ngx_peer_connection_t *pc,
    void *data);
typedef void (*ngx_event_save_peer_session_pt)(ngx_peer_connection_t *pc,
    void *data);
#endif


struct ngx_peer_connection_s {
    ngx_connection_t                *connection;

    // 这三个成员可以在ngx_event_connect_peer()之前赋好值，
    // 也可以在get成员对应的回调函数里赋值。
    struct sockaddr                 *sockaddr;
    socklen_t                        socklen;
    ngx_str_t                       *name;

    ngx_uint_t                       tries;

    // 获取地址用的回调函数，不可以为空,
    // 如果地址以提前设置好，可以设置为ngx_event_get_peer()
    // pc[out]: 这个结构体对象
    // data[in]: 这个结构体对象的data成员
    // get(ngx_peer_connection_t *pc, void *data)
    ngx_event_get_peer_pt            get;
    // 没有地方使用
    ngx_event_free_peer_pt           free;
    // 会传递给get回调函数
    void                            *data;

#if (NGX_SSL)
    ngx_event_set_peer_session_pt    set_session;
    ngx_event_save_peer_session_pt   save_session;
#endif

#if (NGX_THREADS)
    ngx_atomic_t                    *lock;
#endif

    ngx_addr_t                      *local;

    // 在ngx_event_connect_peer函数参数里作为输入，
    // 如果不为零会用来设置套接字的SO_RCVBUF属性
    int                              rcvbuf;

    ngx_log_t                       *log;

    unsigned                         cached:1;

                                     /* ngx_connection_log_error_e */
    unsigned                         log_error:2;
};


ngx_int_t ngx_event_connect_peer(ngx_peer_connection_t *pc);
ngx_int_t ngx_event_get_peer(ngx_peer_connection_t *pc, void *data);


#endif /* _NGX_EVENT_CONNECT_H_INCLUDED_ */
