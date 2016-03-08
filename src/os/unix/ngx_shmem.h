
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件和实现文件实现申请和释放共享内存的函数。
// nginx在启动时从master进程中申请指定几块指定大小的共享内存,
// 启动完成后在几个worker进程中共享使用。

#ifndef _NGX_SHMEM_H_INCLUDED_
#define _NGX_SHMEM_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


// nginx共享内存,主要是在master进程初始化好后在各个worker进程共享使用,以后无法改变大小
typedef struct {
    // 指向共享内存的首地址
    u_char      *addr;
    // 共享内存的大小
    size_t       size;
    // 共享内存的名称,每一块共享内存对应唯一的名字
    ngx_str_t    name;
    ngx_log_t   *log;
    ngx_uint_t   exists;   /* unsigned  exists:1;  */
} ngx_shm_t;


ngx_int_t ngx_shm_alloc(ngx_shm_t *shm);
void ngx_shm_free(ngx_shm_t *shm);


#endif /* _NGX_SHMEM_H_INCLUDED_ */
