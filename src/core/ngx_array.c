
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件与实现文件实现了nginx 6个基本容器之一的ngx_array_t及相关操作函数。

#include <ngx_config.h>
#include <ngx_core.h>


// 创建一个ngx_array_t对象,并预先申请好n个元素的内存。
// p[in]: 申请内存用的内存池对象
// n[in]: 创建时申请可以存放多少个元素的内存。
//        当数组扩容时会重新为所有元素重新申请内存，原来的内存不会被释放，所以尽量创建时将元素个数申请够。
// size[in]: 每个元素的大小
ngx_array_t *
ngx_array_create(ngx_pool_t *p, ngx_uint_t n, size_t size)
{
    ngx_array_t *a;

    // 申请ngx_array_t对象的内存
    a = ngx_palloc(p, sizeof(ngx_array_t));
    if (a == NULL) {
        return NULL;
    }

    // 给a赋初值,并申请n块size大小的内存
    if (ngx_array_init(a, p, n, size) != NGX_OK) {
        return NULL;
    }

    return a;
}


// 销毁一个ngx_array_t实例，如果这个对象扩容过，将不能完全释放内存。
// 这个函数在nginx中没有使用过
void
ngx_array_destroy(ngx_array_t *a)
{
    ngx_pool_t  *p;

    p = a->pool;

    if ((u_char *) a->elts + a->size * a->nalloc == p->d.last) {
        p->d.last -= a->size * a->nalloc;
    }

    if ((u_char *) a + sizeof(ngx_array_t) == p->d.last) {
        p->d.last = (u_char *) a;
    }
}


// 在参数a中增加一个元素，并返回其首地址。
// return: 返回指向申请到元素的内存首地址
void *
ngx_array_push(ngx_array_t *a)
{
    void        *elt, *new;
    size_t       size;
    ngx_pool_t  *p;

    // 如果分配的空间已用完就申请
    if (a->nelts == a->nalloc) {

        /* the array is full */

        size = a->size * a->nalloc;

        p = a->pool;

        if ((u_char *) a->elts + size == p->d.last
            && p->d.last + a->size <= p->d.end)
        {
            /*
             * the array allocation is the last in the pool
             * and there is space for new allocation
             */

            p->d.last += a->size;
            a->nalloc++;

        } else {
            /* allocate a new array */
            // 如果这个数组不是这个内存池最后一块元素或者内存池已没有空闲空间
            // 需要重新申请一块两倍于原来总大小的内存, 然后把原来的内存拷贝到新内存
            // 之前的内存不会被释放

            new = ngx_palloc(p, 2 * size);
            if (new == NULL) {
                return NULL;
            }

            ngx_memcpy(new, a->elts, size);
            a->elts = new;
            a->nalloc *= 2;
        }
    }

    //将elt指向到的申请元素地址,将整个数组的元素个数加一
    elt = (u_char *) a->elts + a->size * a->nelts;
    a->nelts++;

    return elt;
}


// 在参数a中增加n个元素，并返回其首地址
// return: 返回指向第一个元素的指针
void *
ngx_array_push_n(ngx_array_t *a, ngx_uint_t n)
{
    void        *elt, *new;
    size_t       size;
    ngx_uint_t   nalloc;
    ngx_pool_t  *p;

    size = n * a->size;

    if (a->nelts + n > a->nalloc) {

        /* the array is full */

        p = a->pool;

        if ((u_char *) a->elts + a->size * a->nalloc == p->d.last
            && p->d.last + size <= p->d.end)
        {
            /*
             * the array allocation is the last in the pool
             * and there is space for new allocation
             */

            p->d.last += size;
            a->nalloc += n;

        } else {
            /* allocate a new array */

            nalloc = 2 * ((n >= a->nalloc) ? n : a->nalloc);

            new = ngx_palloc(p, nalloc * a->size);
            if (new == NULL) {
                return NULL;
            }

            ngx_memcpy(new, a->elts, a->nelts * a->size);
            a->elts = new;
            a->nalloc = nalloc;
        }
    }

    elt = (u_char *) a->elts + a->size * a->nelts;
    a->nelts += n;

    return elt;
}
