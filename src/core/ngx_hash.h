
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件与实现文件定义了nginx 6个基本容器之一的散列表容器并实现了相关操作函数。

#ifndef _NGX_HASH_H_INCLUDED_
#define _NGX_HASH_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


// 这个结构体是ngx_hash_t结构体的一部分。
typedef struct {
    // 指向用户自定义数据的指针。
    void             *value;
    // 元素关键字的长度。
    u_short           len;
    // 元素关键字的首地址。
    u_char            name[1];
} ngx_hash_elt_t;


// 一个ngx_hash_t对象对应一个常规散列表
typedef struct {
    // ngx_hash_elt_t元素组成的二级指针
    ngx_hash_elt_t  **buckets;
    // 桶的个数，不是每个桶中的成员个数，
    ngx_uint_t        size;
} ngx_hash_t;


// 结构体的每个对象对应一个带通配符的散列表
typedef struct {
    ngx_hash_t        hash;
    void             *value;
} ngx_hash_wildcard_t;


// 初始化哈希表时使用，也是ngx_hash_keys_arrays_t中成员
// ngx_hash_init()第二个参数是这个结构体对象数组
typedef struct {
    // 一个字符串
    ngx_str_t         key;
    // 上面key的hash值
    ngx_uint_t        key_hash;
    // 用户自定义数据指针
    void             *value;
} ngx_hash_key_t;


typedef ngx_uint_t (*ngx_hash_key_pt) (u_char *data, size_t len);


typedef struct {
    ngx_hash_t            hash;
    ngx_hash_wildcard_t  *wc_head;
    ngx_hash_wildcard_t  *wc_tail;
} ngx_hash_combined_t;


// 初始化散列表的专用结构体。
typedef struct {
    // 要被初始化的散列表。
    // 在ngx_hash_init()用于输出
    ngx_hash_t       *hash;
    // 计算字符串哈希值用的回调函数。
    // 在ngx_hash_init()用于输入
    ngx_hash_key_pt   key;

    // 在ngx_hash_init()用于输入
    ngx_uint_t        max_size;
    // 在ngx_hash_init()用于输入
    // 哈希表存的最长字符串对齐后加上一个指针的长度后应小于这个值
    ngx_uint_t        bucket_size;

    // 在ngx_hash_init()用于输入
    char             *name;
    // 在ngx_hash_init()用于输入
    ngx_pool_t       *pool;
    // 在ngx_hash_init()用于输入
    ngx_pool_t       *temp_pool;
} ngx_hash_init_t;


#define NGX_HASH_SMALL            1
#define NGX_HASH_LARGE            2

#define NGX_HASH_LARGE_ASIZE      16384
#define NGX_HASH_LARGE_HSIZE      10007

#define NGX_HASH_WILDCARD_KEY     1
#define NGX_HASH_READONLY_KEY     2


typedef struct {
    ngx_uint_t        hsize;

    ngx_pool_t       *pool;
    ngx_pool_t       *temp_pool;

    ngx_array_t       keys;         // ngx_hash_key_t
    ngx_array_t      *keys_hash;    // ngx_str_t

    ngx_array_t       dns_wc_head;
    ngx_array_t      *dns_wc_head_hash;

    ngx_array_t       dns_wc_tail;
    ngx_array_t      *dns_wc_tail_hash;
} ngx_hash_keys_arrays_t;


typedef struct {
    ngx_uint_t        hash;
    ngx_str_t         key;
    ngx_str_t         value;
    u_char           *lowcase_key;
} ngx_table_elt_t;


void *ngx_hash_find(ngx_hash_t *hash, ngx_uint_t key, u_char *name, size_t len);
void *ngx_hash_find_wc_head(ngx_hash_wildcard_t *hwc, u_char *name, size_t len);
void *ngx_hash_find_wc_tail(ngx_hash_wildcard_t *hwc, u_char *name, size_t len);
void *ngx_hash_find_combined(ngx_hash_combined_t *hash, ngx_uint_t key,
    u_char *name, size_t len);

ngx_int_t ngx_hash_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names,
    ngx_uint_t nelts);
ngx_int_t ngx_hash_wildcard_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names,
    ngx_uint_t nelts);

#define ngx_hash(key, c)   ((ngx_uint_t) key * 31 + c)
ngx_uint_t ngx_hash_key(u_char *data, size_t len);
ngx_uint_t ngx_hash_key_lc(u_char *data, size_t len);
ngx_uint_t ngx_hash_strlow(u_char *dst, u_char *src, size_t n);


ngx_int_t ngx_hash_keys_array_init(ngx_hash_keys_arrays_t *ha, ngx_uint_t type);
ngx_int_t ngx_hash_add_key(ngx_hash_keys_arrays_t *ha, ngx_str_t *key,
    void *value, ngx_uint_t flags);


#endif /* _NGX_HASH_H_INCLUDED_ */
