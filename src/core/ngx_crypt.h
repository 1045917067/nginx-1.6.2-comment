
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件和实现文件实现了验证用户名密码的一些函数。

#ifndef _NGX_CRYPT_H_INCLUDED_
#define _NGX_CRYPT_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


ngx_int_t ngx_crypt(ngx_pool_t *pool, u_char *key, u_char *salt,
    u_char **encrypted);


#endif /* _NGX_CRYPT_H_INCLUDED_ */
