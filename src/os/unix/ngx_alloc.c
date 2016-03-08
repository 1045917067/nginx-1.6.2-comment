
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件和实现文件封装了unix平台下的一些内存申请相关函数

#include <ngx_config.h>
#include <ngx_core.h>


// 内存的页大小, 在ngx_os_init()中初始化
// 申请内存时按照这个大小的倍数申请, 可以更有效利用内存
ngx_uint_t  ngx_pagesize;
// 内存的页大小是2的次方数, 在ngx_os_init()中初始化
ngx_uint_t  ngx_pagesize_shift;
// cpu的cacheline,在ngx_cpuinfo()中初始化
// 一些内存的申请以cpu的cacheline对齐，可以更高效的利用cpucache
ngx_uint_t  ngx_cacheline_size;


// 对malloc()的封装
void *
ngx_alloc(size_t size, ngx_log_t *log)
{
    void  *p;

    p = malloc(size);
    if (p == NULL) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      "malloc(%uz) failed", size);
    }

    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, log, 0, "malloc: %p:%uz", p, size);

    return p;
}


// 相当于calloc()
void *
ngx_calloc(size_t size, ngx_log_t *log)
{
    void  *p;

    p = ngx_alloc(size, log);

    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}


// 根据系统支持情况选择ngx_memalign()的实现方式
#if (NGX_HAVE_POSIX_MEMALIGN)

// 按参数alignment对齐申请一块大小是参数size的内存
void *
ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
{
    void  *p;
    int    err;

    err = posix_memalign(&p, alignment, size);

    if (err) {
        ngx_log_error(NGX_LOG_EMERG, log, err,
                      "posix_memalign(%uz, %uz) failed", alignment, size);
        p = NULL;
    }

    ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0,
                   "posix_memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#elif (NGX_HAVE_MEMALIGN)

// 按参数alignment对齐申请一块大小是参数size的内存
void *
ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
{
    void  *p;

    p = memalign(alignment, size);
    if (p == NULL) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      "memalign(%uz, %uz) failed", alignment, size);
    }

    ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0,
                   "memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#endif
