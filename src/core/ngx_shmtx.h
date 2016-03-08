
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 进程间互斥锁的实现。
// 用于解决多进程tcp连接accept时的惊群问题及同步共享内存操作。

#ifndef _NGX_SHMTX_H_INCLUDED_
#define _NGX_SHMTX_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct {
    ngx_atomic_t   lock;
#if (NGX_HAVE_POSIX_SEM)
    ngx_atomic_t   wait;
#endif
} ngx_shmtx_sh_t;



typedef struct {
#if (NGX_HAVE_ATOMIC_OPS)
    // 自旋锁使用的原子变量
    ngx_atomic_t  *lock;
#if (NGX_HAVE_POSIX_SEM)
    // 标识有几个信号量在等待
    ngx_atomic_t  *wait;
    // 如果为1信号量初始化成功可以使用
    ngx_uint_t     semaphore;
    // 信号量
    sem_t          sem;
#endif
#else
    // 文件锁才用
    ngx_fd_t       fd;
    // 文件锁才用
    u_char        *name;
#endif
    // 如果为-1说明是自旋锁,不为-1将决定自旋多少次进入信号量等待
    ngx_uint_t     spin;
} ngx_shmtx_t;


ngx_int_t ngx_shmtx_create(ngx_shmtx_t *mtx, ngx_shmtx_sh_t *addr,
    u_char *name);
void ngx_shmtx_destroy(ngx_shmtx_t *mtx);
ngx_uint_t ngx_shmtx_trylock(ngx_shmtx_t *mtx);
void ngx_shmtx_lock(ngx_shmtx_t *mtx);
void ngx_shmtx_unlock(ngx_shmtx_t *mtx);
ngx_uint_t ngx_shmtx_force_unlock(ngx_shmtx_t *mtx, ngx_pid_t pid);


#endif /* _NGX_SHMTX_H_INCLUDED_ */
