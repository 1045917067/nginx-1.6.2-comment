
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这是一个正则表达式相关的NGX_CORE_MODULE模块

#ifndef _NGX_REGEX_H_INCLUDED_
#define _NGX_REGEX_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

#include <pcre.h>


#define NGX_REGEX_NO_MATCHED  PCRE_ERROR_NOMATCH   /* -1 */

#define NGX_REGEX_CASELESS    PCRE_CASELESS


typedef struct {
    pcre        *code;
    pcre_extra  *extra;
} ngx_regex_t;


typedef struct {
    // 正则表达式的模版字符串
    // ngx_regex_compile()中作为输入
    ngx_str_t     pattern;
    ngx_pool_t   *pool;
    ngx_int_t     options;

    // ngx_regex_compile()中作为主要输出
    ngx_regex_t  *regex;
    int           captures;
    int           named_captures;
    int           name_size;
    u_char       *names;
    ngx_str_t     err;
} ngx_regex_compile_t;


typedef struct {
    ngx_regex_t  *regex;
    // 正则表达式原字符串
    u_char       *name;
} ngx_regex_elt_t;


void ngx_regex_init(void);
ngx_int_t ngx_regex_compile(ngx_regex_compile_t *rc);

// 检查一个具体字符串是否和之前定义的一个正则表达式模版匹配
#define ngx_regex_exec(re, s, captures, size)                                \
    pcre_exec(re->code, re->extra, (const char *) (s)->data, (s)->len, 0, 0, \
              captures, size)
#define ngx_regex_exec_n      "pcre_exec()"

ngx_int_t ngx_regex_exec_array(ngx_array_t *a, ngx_str_t *s, ngx_log_t *log);


#endif /* _NGX_REGEX_H_INCLUDED_ */
