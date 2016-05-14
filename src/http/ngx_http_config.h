
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这个文件声明了http配置相关的结构体

#ifndef _NGX_HTTP_CONFIG_H_INCLUDED_
#define _NGX_HTTP_CONFIG_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


// 解析http相关配置的关键结构体。
// 每个'http{'，'server{', 'location{'都会对应一个ngx_http_conf_ctx_t对象。
typedef struct {
    // 第一级指针指向的数组元素个数是ngx_http_max_module。
	// 每个"http{"，"server{"，"location{"指令对应的main_conf都指向同一份内存。
    void        **main_conf;
    // 第一级指针指向的数组元素个数是ngx_http_max_module。
	// 每个"http{"对应的srv_conf指向一份单独的内存，
    // "server{"和其内部的"location{"指令对应的src_conf都指向同一份内存。
    void        **srv_conf;
    // 第一级指针指向的数组元素个数是ngx_http_max_module。
	// 每个"http{"，"server{"，"location{"指令对应的main_conf都指向单独一份内存。
    void        **loc_conf;
} ngx_http_conf_ctx_t;


// 声明了NGX_HTTP_MODULE类型模块的解析配置文件时相关回调函数。
// 当ngx_module_t::type==NGX_HTTP_MODULE时，ngx_module_t::ctx将指向ngx_http_module_t的一个对象。
typedef struct {
	// 这个回调函数在一个模块里定义后，在解析配置文件前会被调用一次。
    ngx_int_t   (*preconfiguration)(ngx_conf_t *cf);
    // 这个回调函数在一个模块里定义后，在解析配置文件后会被调用一次。
    ngx_int_t   (*postconfiguration)(ngx_conf_t *cf);

	// 用于创建和初始化一个模块的 main层配置结构体对象。
    // 这个回调函数在一个模块里定义后，在解析配置文件前会被调用一次。
    void       *(*create_main_conf)(ngx_conf_t *cf);
	// 用于给一个模块的 main层配置结构体对象中配置文件没配置的字段付初始值。
    // 这个回调函数在一个模块里定义后，在解析配置文件前会被调用一次。
    char       *(*init_main_conf)(ngx_conf_t *cf, void *conf);

	// 用于创建和初始化一个"server{"对应的配置结构体对象。
    // 这个回调函数在一个模块里定义后，调用次数等于配置文件中"http{"字段个数与"server{"字段个数相加。
    void       *(*create_srv_conf)(ngx_conf_t *cf);
	// 用于给一个"server{"对应的配置结构体对象中配置文件没配置的字段付初始值。
    // 这个回调函数在一个模块里定义后，调用次数等于配置文件中"server{"字段个数。
    char       *(*merge_srv_conf)(ngx_conf_t *cf, void *prev, void *conf);

	// 用于创建和初始化一个"location{"对应的配置结构体对象。
    // 这个回调函数在一个模块里定义后，调用次数等于配置文件中"http{"字段个数、"server{"字段个数和"location{"个数相加。
    void       *(*create_loc_conf)(ngx_conf_t *cf);
	// 用于给一个"location{"对应的配置结构体对象中配置文件没配置的字段付初始值。
    // 这个回调函数在一个模块里定义后，调用次数等于配置文件中"server{"字段个数和"location{"个数相加。
    char       *(*merge_loc_conf)(ngx_conf_t *cf, void *prev, void *conf);
} ngx_http_module_t;


#define NGX_HTTP_MODULE           0x50545448   /* "HTTP" */

#define NGX_HTTP_MAIN_CONF        0x02000000
#define NGX_HTTP_SRV_CONF         0x04000000
#define NGX_HTTP_LOC_CONF         0x08000000
#define NGX_HTTP_UPS_CONF         0x10000000
#define NGX_HTTP_SIF_CONF         0x20000000
#define NGX_HTTP_LIF_CONF         0x40000000
#define NGX_HTTP_LMT_CONF         0x80000000


#define NGX_HTTP_MAIN_CONF_OFFSET  offsetof(ngx_http_conf_ctx_t, main_conf)
#define NGX_HTTP_SRV_CONF_OFFSET   offsetof(ngx_http_conf_ctx_t, srv_conf)
#define NGX_HTTP_LOC_CONF_OFFSET   offsetof(ngx_http_conf_ctx_t, loc_conf)


// 获取ngx_http_request_t对象在某个模块的main层配置。
#define ngx_http_get_module_main_conf(r, module)                             \
    (r)->main_conf[module.ctx_index]
// 获取ngx_http_request_t对象在某个模块的server层配置。
#define ngx_http_get_module_srv_conf(r, module)  (r)->srv_conf[module.ctx_index]
// 获取ngx_http_request_t对象在某个模块的location层配置。
#define ngx_http_get_module_loc_conf(r, module)  (r)->loc_conf[module.ctx_index]


#define ngx_http_conf_get_module_main_conf(cf, module)                        \
    ((ngx_http_conf_ctx_t *) cf->ctx)->main_conf[module.ctx_index]
#define ngx_http_conf_get_module_srv_conf(cf, module)                         \
    ((ngx_http_conf_ctx_t *) cf->ctx)->srv_conf[module.ctx_index]
#define ngx_http_conf_get_module_loc_conf(cf, module)                         \
    ((ngx_http_conf_ctx_t *) cf->ctx)->loc_conf[module.ctx_index]

// 获取某个NGX_HTTP_MODULE类型模块的main级别配置
// 因为所有NGX_HTTP_MODULE类型模块只对应一份main级别配置，所以可以直接用cycle获取。
#define ngx_http_cycle_get_module_main_conf(cycle, module)                    \
    (cycle->conf_ctx[ngx_http_module.index] ?                                 \
        ((ngx_http_conf_ctx_t *) cycle->conf_ctx[ngx_http_module.index])      \
            ->main_conf[module.ctx_index]:                                    \
        NULL)


#endif /* _NGX_HTTP_CONFIG_H_INCLUDED_ */
