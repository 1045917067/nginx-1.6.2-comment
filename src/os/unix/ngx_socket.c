
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件和实现文件对socket相关函数和数据类型进行了封装。

#include <ngx_config.h>
#include <ngx_core.h>


/*
 * ioctl(FIONBIO) sets a non-blocking mode with the single syscall
 * while fcntl(F_SETFL, O_NONBLOCK) needs to learn the current state
 * using fcntl(F_GETFL).
 *
 * ioctl() and fcntl() are syscalls at least in FreeBSD 2.x, Linux 2.2
 * and Solaris 7.
 *
 * ioctl() in Linux 2.4 and 2.6 uses BKL, however, fcntl(F_SETFL) uses it too.
 */

// 根据系统socket fd是否支持ioctl FIONBIO标识位来选择用ioctl还是用fcntl来设置socket fd非阻塞，
// 优先选择ioctl，因为fcntl需要用两个系统调用实现设置一个套接字非阻塞，ioctl只需要一个。
#if (NGX_HAVE_FIONBIO)

// 设置s为非阻塞套接字
int
ngx_nonblocking(ngx_socket_t s)
{
    int  nb;

    nb = 1;

    return ioctl(s, FIONBIO, &nb);
}


// 设置s为阻塞套接字
int
ngx_blocking(ngx_socket_t s)
{
    int  nb;

    nb = 0;

    return ioctl(s, FIONBIO, &nb);
}

#endif


#if (NGX_FREEBSD)

// freebsd下打开套接字s的TCP_NOPUSH选项
int
ngx_tcp_nopush(ngx_socket_t s)
{
    int  tcp_nopush;

    tcp_nopush = 1;

    return setsockopt(s, IPPROTO_TCP, TCP_NOPUSH,
                      (const void *) &tcp_nopush, sizeof(int));
}


// freebsd下关闭套接字s的TCP_NOPUSH选项
int
ngx_tcp_push(ngx_socket_t s)
{
    int  tcp_nopush;

    tcp_nopush = 0;

    return setsockopt(s, IPPROTO_TCP, TCP_NOPUSH,
                      (const void *) &tcp_nopush, sizeof(int));
}

#elif (NGX_LINUX)


// linux下打开套接字s的TCP_CORK选项
int
ngx_tcp_nopush(ngx_socket_t s)
{
    int  cork;

    cork = 1;

    return setsockopt(s, IPPROTO_TCP, TCP_CORK,
                      (const void *) &cork, sizeof(int));
}


// linux下关闭套接字s的TCP_CORK选项
int
ngx_tcp_push(ngx_socket_t s)
{
    int  cork;

    cork = 0;

    return setsockopt(s, IPPROTO_TCP, TCP_CORK,
                      (const void *) &cork, sizeof(int));
}

#else

int
ngx_tcp_nopush(ngx_socket_t s)
{
    return 0;
}


int
ngx_tcp_push(ngx_socket_t s)
{
    return 0;
}

#endif
