
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件和实现文件实现了解析字节数字符串或时间间隔字符串的函数。

#ifndef _NGX_PARSE_H_INCLUDED_
#define _NGX_PARSE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


ssize_t ngx_parse_size(ngx_str_t *line);
off_t ngx_parse_offset(ngx_str_t *line);
ngx_int_t ngx_parse_time(ngx_str_t *line, ngx_uint_t is_sec);


#endif /* _NGX_PARSE_H_INCLUDED_ */
