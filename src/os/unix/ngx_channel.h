
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件和实现文件声明了ngx_channel_t结构体，并实现了相关的操作函数。

#ifndef _NGX_CHANNEL_H_INCLUDED_
#define _NGX_CHANNEL_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


// 这个结构体对象是父进程给子进程通过unix套接字发送消息时的载体
typedef struct {
     // 命令类型，在ngx_process_cycle.h定义了可能的取值。
     ngx_uint_t  command;
     // 当ngx_channel_t::command等与NGX_CMD_OPEN_CHANNEL或NGX_CMD_CLOSE_CHANNEL时，
     // 代表状态改变进程的ngx_pid。
     ngx_pid_t   pid;
     // 当ngx_channel_t::command等与NGX_CMD_OPEN_CHANNEL或NGX_CMD_CLOSE_CHANNEL时，
     // 代表状态改变进程的ngx_process_slot。
     ngx_int_t   slot;
     // 当ngx_channel_t::command等与NGX_CMD_OPEN_CHANNEL或NGX_CMD_CLOSE_CHANNEL时，
     // 代表状态改变进程unix套接字对的父进程套接字。
     ngx_fd_t    fd;
} ngx_channel_t;


ngx_int_t ngx_write_channel(ngx_socket_t s, ngx_channel_t *ch, size_t size,
    ngx_log_t *log);
ngx_int_t ngx_read_channel(ngx_socket_t s, ngx_channel_t *ch, size_t size,
    ngx_log_t *log);
ngx_int_t ngx_add_channel_event(ngx_cycle_t *cycle, ngx_fd_t fd,
    ngx_int_t event, ngx_event_handler_pt handler);
void ngx_close_channel(ngx_fd_t *fd, ngx_log_t *log);


#endif /* _NGX_CHANNEL_H_INCLUDED_ */
