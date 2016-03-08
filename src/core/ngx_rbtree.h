
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件与实现文件实现了nginx 6个基本容器之一的ngx_rbtree_t红黑树及相关操作函数。
// 使用红黑树的流程，1.创建一个红黑树对象和一个哨兵节点,定义一个插入节点回调函数，
//                   2.调用ngx_rbtree_init()创建红黑树
//                   3.调用ngx_rbtree_insert()插入节点
//                   4.ngx_rbtree_delete()删除节点
//                   5.查询函数往往需要自己实现,根据ngx_rbtree_node_t::key查询
//                   注：红黑树的节点结构体往往嵌入在需要查询的结构体里

#ifndef _NGX_RBTREE_H_INCLUDED_
#define _NGX_RBTREE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef ngx_uint_t  ngx_rbtree_key_t;
typedef ngx_int_t   ngx_rbtree_key_int_t;


typedef struct ngx_rbtree_node_s  ngx_rbtree_node_t;

// 红黑树的一个节点
// 红黑树节点使用时往往是把红黑树的一个节点嵌入到一个结构体中，
// 然后根据这个红黑树节点对象反求结构体对象
struct ngx_rbtree_node_s {
    // 无符号整型关键字
    ngx_rbtree_key_t       key;
    // 左子节点
    ngx_rbtree_node_t     *left;
    // 右子节点
    ngx_rbtree_node_t     *right;
    // 父节点
    ngx_rbtree_node_t     *parent;
    // 节点颜色，0表示黑色，1表示红色
    u_char                 color;
    u_char                 data;
};


typedef struct ngx_rbtree_s  ngx_rbtree_t;

typedef void (*ngx_rbtree_insert_pt) (ngx_rbtree_node_t *root,
    ngx_rbtree_node_t *node, ngx_rbtree_node_t *sentinel);


// 红黑树
// 红黑树是一种平衡二叉查找树
struct ngx_rbtree_s {
    // 指向树的根节点。
    ngx_rbtree_node_t     *root;
    // 指向NIL哨兵节点。
    ngx_rbtree_node_t     *sentinel;
    // 表示红黑树添加节点的函数指针。
    ngx_rbtree_insert_pt   insert;
};
#define ngx_rbtree_init(tree, s, i)                                           \
    ngx_rbtree_sentinel_init(s);                                              \
    (tree)->root = s;                                                         \
    (tree)->sentinel = s;                                                     \
    (tree)->insert = i


void ngx_rbtree_insert(ngx_thread_volatile ngx_rbtree_t *tree,
    ngx_rbtree_node_t *node);
void ngx_rbtree_delete(ngx_thread_volatile ngx_rbtree_t *tree,
    ngx_rbtree_node_t *node);
void ngx_rbtree_insert_value(ngx_rbtree_node_t *root, ngx_rbtree_node_t *node,
    ngx_rbtree_node_t *sentinel);
void ngx_rbtree_insert_timer_value(ngx_rbtree_node_t *root,
    ngx_rbtree_node_t *node, ngx_rbtree_node_t *sentinel);


#define ngx_rbt_red(node)               ((node)->color = 1)
#define ngx_rbt_black(node)             ((node)->color = 0)
#define ngx_rbt_is_red(node)            ((node)->color)
#define ngx_rbt_is_black(node)          (!ngx_rbt_is_red(node))
#define ngx_rbt_copy_color(n1, n2)      (n1->color = n2->color)


/* a sentinel must be black */

#define ngx_rbtree_sentinel_init(node)  ngx_rbt_black(node)


// 查找红黑树中数值最小的节点，查找最先到期的定时器时会使用
static ngx_inline ngx_rbtree_node_t *
ngx_rbtree_min(ngx_rbtree_node_t *node, ngx_rbtree_node_t *sentinel)
{
    while (node->left != sentinel) {
        node = node->left;
    }

    return node;
}


#endif /* _NGX_RBTREE_H_INCLUDED_ */
