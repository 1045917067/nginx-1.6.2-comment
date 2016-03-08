
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这个文件为符合posix标准的unix系统定义了一些通用的全局变量和通用的初始化函数。

#include <ngx_config.h>
#include <ngx_core.h>
#include <nginx.h>


// 机器cpu核心数量,
ngx_int_t   ngx_ncpu;
// 这个变量只见到赋值的地方，没见到使用的地方
ngx_int_t   ngx_max_sockets;
// 没有明白这个变量的用处
ngx_uint_t  ngx_inherited_nonblocking;
// 这个变量在linux等于0，在freebsd和darwin下等于1.
ngx_uint_t  ngx_tcp_nodelay_and_tcp_nopush;


// nginx进程一些资源的限值，如最大可打开的fd数量。
struct rlimit  rlmt;

// 初始化一些机器和操作系统相关的全局变量以及设置master进程的随机数种子
ngx_os_io_t ngx_os_io = {
    ngx_unix_recv,
    ngx_readv_chain,
    ngx_udp_unix_recv,
    ngx_unix_send,
    ngx_writev_chain,
    0
};


// 初始化一些机器和操作系统相关的全局变量以及设置master进程的随机数种子
ngx_int_t
ngx_os_init(ngx_log_t *log)
{
    ngx_uint_t  n;

#if (NGX_HAVE_OS_SPECIFIC_INIT)
    if (ngx_os_specific_init(log) != NGX_OK) {
        return NGX_ERROR;
    }
#endif

    ngx_init_setproctitle(log);

    ngx_pagesize = getpagesize();
    ngx_cacheline_size = NGX_CPU_CACHE_LINE;

    for (n = ngx_pagesize; n >>= 1; ngx_pagesize_shift++) { /* void */ }

#if (NGX_HAVE_SC_NPROCESSORS_ONLN)
    // 获取系统cpu数量
    if (ngx_ncpu == 0) {
        ngx_ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    }
#endif

    if (ngx_ncpu < 1) {
        ngx_ncpu = 1;
    }

    // 读取CPU的cacheline赋给ngx_cacheline_size
    ngx_cpuinfo();

    if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
        ngx_log_error(NGX_LOG_ALERT, log, errno,
                      "getrlimit(RLIMIT_NOFILE) failed)");
        return NGX_ERROR;
    }

    ngx_max_sockets = (ngx_int_t) rlmt.rlim_cur;

#if (NGX_HAVE_INHERITED_NONBLOCK || NGX_HAVE_ACCEPT4)
    ngx_inherited_nonblocking = 1;
#else
    ngx_inherited_nonblocking = 0;
#endif

    // 用当前时间设置master进程的随机数种子
    srandom(ngx_time());

    return NGX_OK;
}

// 在日志里打印nginx版本号，编译器类型及版本号，操作系统类型及版本号和启动时最多可打开fd数量上限。
// 打印的fd数量上限只是启动时的，启动后可能会被重新设置。
void
ngx_os_status(ngx_log_t *log)
{
    ngx_log_error(NGX_LOG_NOTICE, log, 0, NGINX_VER);

#ifdef NGX_COMPILER
    ngx_log_error(NGX_LOG_NOTICE, log, 0, "built by " NGX_COMPILER);
#endif

#if (NGX_HAVE_OS_SPECIFIC_INIT)
    ngx_os_specific_status(log);
#endif

    ngx_log_error(NGX_LOG_NOTICE, log, 0,
                  "getrlimit(RLIMIT_NOFILE): %r:%r",
                  rlmt.rlim_cur, rlmt.rlim_max);
}


#if 0

ngx_int_t
ngx_posix_post_conf_init(ngx_log_t *log)
{
    ngx_fd_t  pp[2];

    if (pipe(pp) == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "pipe() failed");
        return NGX_ERROR;
    }

    if (dup2(pp[1], STDERR_FILENO) == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, errno, "dup2(STDERR) failed");
        return NGX_ERROR;
    }

    if (pp[1] > STDERR_FILENO) {
        if (close(pp[1]) == -1) {
            ngx_log_error(NGX_LOG_EMERG, log, errno, "close() failed");
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}

#endif
