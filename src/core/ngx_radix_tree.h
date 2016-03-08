
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件与实现文件实现了nginx 6个基本容器之一的ngx_radix_tree_t（基数树）及相关操作函数。

#ifndef _NGX_RADIX_TREE_H_INCLUDED_
#define _NGX_RADIX_TREE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


#define NGX_RADIX_NO_VALUE   (uintptr_t) -1

typedef struct ngx_radix_node_s  ngx_radix_node_t;

// 基数树的一个节点
struct ngx_radix_node_s {
    ngx_radix_node_t  *right;
    ngx_radix_node_t  *left;
    ngx_radix_node_t  *parent;
    // 这个节点的数据指针，32位系统为32位，64位系统为64位。
    uintptr_t          value;
};

// 基数树，除第一个成员外，其他成员都为管理内存使用。
typedef struct {
    // 指向根节点。
    ngx_radix_node_t  *root;
    // 负则给基数树节点分配内存的内存池。
    ngx_pool_t        *pool;
    // 这个成员是一个单链表，当释放一个基数树的节点时，不会把一个节点真正释放而是挂到这个单链表里，
    // 需要申请节点时，如果这个单链表不为空，先复用这个单链表里的节点。
    // free->right被用作单链表的next指针。
    ngx_radix_node_t  *free;
    // 已在分配内存还未使用的首地址。
    // 给节点分配内存时，不是一次分配一个节点的内存，而是一次分配pagesize大小的内存，剩下的部分留给下次使用。
    char              *start;
    size_t             size;
} ngx_radix_tree_t;


ngx_radix_tree_t *ngx_radix_tree_create(ngx_pool_t *pool,
    ngx_int_t preallocate);

ngx_int_t ngx_radix32tree_insert(ngx_radix_tree_t *tree,
    uint32_t key, uint32_t mask, uintptr_t value);
ngx_int_t ngx_radix32tree_delete(ngx_radix_tree_t *tree,
    uint32_t key, uint32_t mask);
uintptr_t ngx_radix32tree_find(ngx_radix_tree_t *tree, uint32_t key);

#if (NGX_HAVE_INET6)
ngx_int_t ngx_radix128tree_insert(ngx_radix_tree_t *tree,
    u_char *key, u_char *mask, uintptr_t value);
ngx_int_t ngx_radix128tree_delete(ngx_radix_tree_t *tree,
    u_char *key, u_char *mask);
uintptr_t ngx_radix128tree_find(ngx_radix_tree_t *tree, u_char *key);
#endif


#endif /* _NGX_RADIX_TREE_H_INCLUDED_ */
