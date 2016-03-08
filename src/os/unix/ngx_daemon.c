
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这个文件实现了一个使前台进程进入守护进程的函数

#include <ngx_config.h>
#include <ngx_core.h>


// 将进程转为守护进程。
// 与常规转为守护进程函数的区别是stderr仍可以向控制台打印
ngx_int_t
ngx_daemon(ngx_log_t *log)
{
    int  fd;

    // 产生新进程
    switch (fork()) {
    case -1:
    // 出错的情况，nginx启动将失败
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "fork() failed");
        return NGX_ERROR;

    case 0:
    // 子进程进入这个分支
        break;

    default:
    // 父进程进入这个分支退出
        exit(0);
    }

    // 重新给全局变量ngx_pid赋值
    ngx_pid = ngx_getpid();

    // 创建新会话，使当前进程成为这个会话的领头进程
    if (setsid() == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "setsid() failed");
        return NGX_ERROR;
    }

    umask(0);

    // 将标准输入和标准输出都重定向到/dev/null, 但标准出错没有重定向，
    fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      "open(\"/dev/null\") failed");
        return NGX_ERROR;
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "dup2(STDIN) failed");
        return NGX_ERROR;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "dup2(STDOUT) failed");
        return NGX_ERROR;
    }

#if 0
    if (dup2(fd, STDERR_FILENO) == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "dup2(STDERR) failed");
        return NGX_ERROR;
    }
#endif

    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "close() failed");
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}
