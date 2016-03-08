
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件和实现文件声明了ngx_connection_t和ngx_listening_t结构体，并实现了相关的操作函数。

#ifndef _NGX_CONNECTION_H_INCLUDED_
#define _NGX_CONNECTION_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_listening_s  ngx_listening_t;

// 这个结构体的一个对象对应一个监听套接字的相关信息。
struct ngx_listening_s {
    // 监听socket的文件描述符。
    ngx_socket_t        fd;

    // 以下两个成员是结构体形式的监听socket的端口和IP。
    struct sockaddr    *sockaddr;
    socklen_t           socklen;    /* size of sockaddr */
    // 以下两个成员是字符串形式的监听socket的端口和IP。
    size_t              addr_text_max_len;
    ngx_str_t           addr_text;

    // 监听socket类型，如SOCK_STREAM，即socket()第二个参数。
    int                 type;

    int                 backlog;
    // 这个套接字在内核中的接收缓冲区大小。
    int                 rcvbuf;
    // 这个套接字在内核中的发送缓冲区大小。
    int                 sndbuf;
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
    int                 keepidle;
    int                 keepintvl;
    int                 keepcnt;
#endif

    /* handler of accepted connection */
    // 这个监听socket收到accept请求时的回调函数。
    ngx_connection_handler_pt   handler;

    void               *servers;  /* array of ngx_http_in_addr_t, for example */

    ngx_log_t           log;
    ngx_log_t          *logp;

    size_t              pool_size;
    /* should be here because of the AcceptEx() preread */
    size_t              post_accept_buffer_size;
    /* should be here because of the deferred accept */
    ngx_msec_t          post_accept_timeout;

    // 在平滑升级或reload操作时会用到。
    ngx_listening_t    *previous;
    // connection->read被用来做监听端口的accept等待事件。
    ngx_connection_t   *connection;

    unsigned            open:1;
    unsigned            remain:1;
    unsigned            ignore:1;

    unsigned            bound:1;       /* already bound */
    // 这个成员没有使用
    unsigned            inherited:1;   /* inherited from previous process */
    unsigned            nonblocking_accept:1;
    unsigned            listen:1;
    // 这个成员没有使用
    unsigned            nonblocking:1;
    // 这个成员没有使用
    unsigned            shared:1;    /* shared between threads or processes */
    unsigned            addr_ntop:1;

#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
    unsigned            ipv6only:1;
#endif
    unsigned            keepalive:2;

#if (NGX_HAVE_DEFERRED_ACCEPT)
    unsigned            deferred_accept:1;
    unsigned            delete_deferred:1;
    unsigned            add_deferred:1;
#ifdef SO_ACCEPTFILTER
    char               *accept_filter;
#endif
#endif
#if (NGX_HAVE_SETFIB)
    int                 setfib;
#endif

#if (NGX_HAVE_TCP_FASTOPEN)
    int                 fastopen;
#endif

};


typedef enum {
     NGX_ERROR_ALERT = 0,
     NGX_ERROR_ERR,
     NGX_ERROR_INFO,
     NGX_ERROR_IGNORE_ECONNRESET,
     NGX_ERROR_IGNORE_EINVAL
} ngx_connection_log_error_e;


typedef enum {
     NGX_TCP_NODELAY_UNSET = 0,
     NGX_TCP_NODELAY_SET,
     NGX_TCP_NODELAY_DISABLED
} ngx_connection_tcp_nodelay_e;


typedef enum {
     NGX_TCP_NOPUSH_UNSET = 0,
     NGX_TCP_NOPUSH_SET,
     NGX_TCP_NOPUSH_DISABLED
} ngx_connection_tcp_nopush_e;


#define NGX_LOWLEVEL_BUFFERED  0x0f
#define NGX_SSL_BUFFERED       0x01
#define NGX_SPDY_BUFFERED      0x02


// 这个结构体每个对象对应一个外部请求连入的套接字。
struct ngx_connection_s {
    // 不同场景有不同的用图，
    // 在ngx_cycle->free_connectionss链表中充当next指针。
    // 在http框架中指向这个连接对应的ngx_http_request_t对象。
    void               *data;
    // 连接对应的读事件，在程序启动时初始化好。
    ngx_event_t        *read;
    // 连接对应的写事件，在程序启动时以初始化好。
    ngx_event_t        *write;

    // 连接对应的文件描述符
    ngx_socket_t        fd;

    // linux下指向ngx_unix_recv
    ngx_recv_pt         recv;
    // linux下指向ngx_unix_send
    ngx_send_pt         send;
    // linux下指向ngx_readv_chain
    ngx_recv_chain_pt   recv_chain;
    // 在非ssl相关发送下指向ngx_ssl_send_chain(), 除此之外，
    // 在支持sendfile()系统调用的情况下指向ngx_linux_sendfile_chaini(),
    // 不支持的情况下指向ngx_writev_chain()。
    ngx_send_chain_pt   send_chain;

    // 这个连接由套接字listening->fd所建立。
    ngx_listening_t    *listening;

    // 这个连接已发送出的字节数。
    off_t               sent;

    ngx_log_t          *log;

    ngx_pool_t         *pool;

    // 客户端的地址。
    struct sockaddr    *sockaddr;
    // sockaddr的长度。
    socklen_t           socklen;
    // 字符串形式的客户端地址。
    ngx_str_t           addr_text;

    ngx_str_t           proxy_protocol_addr;

#if (NGX_SSL)
    ngx_ssl_connection_t  *ssl;
#endif

    struct sockaddr    *local_sockaddr;
    socklen_t           local_socklen;

    ngx_buf_t          *buffer;

    ngx_queue_t         queue;

    ngx_atomic_uint_t   number;

    ngx_uint_t          requests;

    unsigned            buffered:8;

    unsigned            log_error:3;     /* ngx_connection_log_error_e */

    unsigned            unexpected_eof:1;
    unsigned            timedout:1;
    unsigned            error:1;
    unsigned            destroyed:1;

    unsigned            idle:1;
    unsigned            reusable:1;
    unsigned            close:1;

    unsigned            sendfile:1;
    unsigned            sndlowat:1;
    unsigned            tcp_nodelay:2;   /* ngx_connection_tcp_nodelay_e */
    unsigned            tcp_nopush:2;    /* ngx_connection_tcp_nopush_e */

    unsigned            need_last_buf:1;

#if (NGX_HAVE_IOCP)
    unsigned            accept_context_updated:1;
#endif

#if (NGX_HAVE_AIO_SENDFILE)
    unsigned            aio_sendfile:1;
    unsigned            busy_count:2;
    ngx_buf_t          *busy_sendfile;
#endif

#if (NGX_THREADS)
    ngx_atomic_t        lock;
#endif
};


ngx_listening_t *ngx_create_listening(ngx_conf_t *cf, void *sockaddr,
    socklen_t socklen);
ngx_int_t ngx_set_inherited_sockets(ngx_cycle_t *cycle);
ngx_int_t ngx_open_listening_sockets(ngx_cycle_t *cycle);
void ngx_configure_listening_sockets(ngx_cycle_t *cycle);
void ngx_close_listening_sockets(ngx_cycle_t *cycle);
void ngx_close_connection(ngx_connection_t *c);
ngx_int_t ngx_connection_local_sockaddr(ngx_connection_t *c, ngx_str_t *s,
    ngx_uint_t port);
ngx_int_t ngx_connection_error(ngx_connection_t *c, ngx_err_t err, char *text);

ngx_connection_t *ngx_get_connection(ngx_socket_t s, ngx_log_t *log);
void ngx_free_connection(ngx_connection_t *c);

void ngx_reusable_connection(ngx_connection_t *c, ngx_uint_t reusable);

#endif /* _NGX_CONNECTION_H_INCLUDED_ */
