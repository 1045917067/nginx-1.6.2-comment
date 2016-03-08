
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件与实现文件实现了nginx 6个基本容器之一的ngx_list_t及相关操作函数。

#ifndef _NGX_LIST_H_INCLUDED_
#define _NGX_LIST_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_list_part_s  ngx_list_part_t;

// ngx_list_t链表的一个节点, 节点中存放了一个元素数组。
struct ngx_list_part_s {
    // 元素数组的首地址
    void             *elts;
    // 元素数组已经使用了的元素个数
    ngx_uint_t        nelts;
    // 指向下一个节点的指针
    ngx_list_part_t  *next;
};


// ngx_list_t是一个单向链表，单向链表的一个节点存放着一个元素数组。
typedef struct {
    // 指向链表的最后一个节点的指针。
    ngx_list_part_t  *last;
    // 链表的首节点。
    ngx_list_part_t   part;
    // 每个元素的大小。
    size_t            size;
    // 每个元素数组的最大元素个数。
    ngx_uint_t        nalloc;
    // 申请内存用的内存池。
    ngx_pool_t       *pool;
} ngx_list_t;



ngx_list_t *ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size);

// 初始化一个ngx_list_t变量。
// list[out]: 指向一个待初始化ngx_list_t变量的指针。
// pool[in]: 指向申请内存用的内存池变量的指针。
// n[in]: 每个节点的元素数组的元素数量。
// size[in]: 每个元素的字节数。
static ngx_inline ngx_int_t
ngx_list_init(ngx_list_t *list, ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    list->part.elts = ngx_palloc(pool, n * size);
    if (list->part.elts == NULL) {
        return NGX_ERROR;
    }

    list->part.nelts = 0;
    list->part.next = NULL;
    list->last = &list->part;
    list->size = size;
    list->nalloc = n;
    list->pool = pool;

    return NGX_OK;
}


/*
 *
 *  the iteration through the list:
 *
 *  part = &list.part;
 *  data = part->elts;
 *
 *  for (i = 0 ;; i++) {
 *
 *      if (i >= part->nelts) {
 *          if (part->next == NULL) {
 *              break;
 *          }
 *
 *          part = part->next;
 *          data = part->elts;
 *          i = 0;
 *      }
 *
 *      ...  data[i] ...
 *
 *  }
 */


void *ngx_list_push(ngx_list_t *list);


#endif /* _NGX_LIST_H_INCLUDED_ */
