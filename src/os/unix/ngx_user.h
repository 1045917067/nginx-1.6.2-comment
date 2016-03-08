
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这个头文件和实现文件将libc crypt相关数据类型和函数进行了封装。 

#ifndef _NGX_USER_H_INCLUDED_
#define _NGX_USER_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef uid_t  ngx_uid_t;
typedef gid_t  ngx_gid_t;


ngx_int_t ngx_libc_crypt(ngx_pool_t *pool, u_char *key, u_char *salt,
    u_char **encrypted);


#endif /* _NGX_USER_H_INCLUDED_ */
