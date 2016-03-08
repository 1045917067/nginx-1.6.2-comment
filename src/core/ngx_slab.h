
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件和实现实现了进程间共享内存上分配内存的内存池，使用slab内存分配算法。

#ifndef _NGX_SLAB_H_INCLUDED_
#define _NGX_SLAB_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_slab_page_s  ngx_slab_page_t;

// 一个内存页的链表节点
struct ngx_slab_page_s {
    // 保存一个内存页
    uintptr_t         slab; 
    // 链表下一个节点
    ngx_slab_page_t  *next;
    // 链表上一个节点
    uintptr_t         prev;
};


// 由slab内存分配算法实现的内存池。
typedef struct {
    // mutex成员加锁需要使用的两个原子变量
    ngx_shmtx_sh_t    lock;

    // 最小分配单元。在ngx_slab_init()中由min_shift生成
    size_t            min_size;
    // 最小分配单元对应的位移。即min_size是2的多少次方
    // 在作为参数输入给ngx_slab_init()时作为输入
    size_t            min_shift;

    // 页数组
    ngx_slab_page_t  *pages;
    // 空闲页链表
    ngx_slab_page_t   free;

    // 可分配内存的起始地址
    // 在作为参数输入给ngx_slab_init()时作为输入
    u_char           *start;
    // 可分配内存的结束地址
    // 在作为参数输入给ngx_slab_init()时作为输入
    u_char           *end;

    // 线程安全的申请或释放内存时的锁
    // 锁在初始化后要自己初始化
    ngx_shmtx_t       mutex;

    u_char           *log_ctx;
    u_char            zero;

    unsigned          log_nomem:1;

    void             *data;
    // 指向这个结构体对象的开头
    void             *addr;
} ngx_slab_pool_t;


void ngx_slab_init(ngx_slab_pool_t *pool);
void *ngx_slab_alloc(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_alloc_locked(ngx_slab_pool_t *pool, size_t size);
void ngx_slab_free(ngx_slab_pool_t *pool, void *p);
void ngx_slab_free_locked(ngx_slab_pool_t *pool, void *p);


#endif /* _NGX_SLAB_H_INCLUDED_ */
