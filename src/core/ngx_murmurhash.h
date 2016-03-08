
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件和实现文件实现了mururhash2算法。
// mururhash2一种非加密的32位哈希算法，具有高效和低碰撞率的特点。

#ifndef _NGX_MURMURHASH_H_INCLUDED_
#define _NGX_MURMURHASH_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


uint32_t ngx_murmur_hash2(u_char *data, size_t len);


#endif /* _NGX_MURMURHASH_H_INCLUDED_ */
