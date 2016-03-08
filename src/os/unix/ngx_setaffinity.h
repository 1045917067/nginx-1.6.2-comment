
/*
 * Copyright (C) Nginx, Inc.
 */

#ifndef _NGX_SETAFFINITY_H_INCLUDED_
#define _NGX_SETAFFINITY_H_INCLUDED_

// 这组头文件和实现文件实现了一个将当前进程与某些CPU绑定的函数。

#if (NGX_HAVE_SCHED_SETAFFINITY || NGX_HAVE_CPUSET_SETAFFINITY)

#define NGX_HAVE_CPU_AFFINITY 1

void ngx_setaffinity(uint64_t cpu_affinity, ngx_log_t *log);

#else

#define ngx_setaffinity(cpu_affinity, log)

#endif


#endif /* _NGX_SETAFFINITY_H_INCLUDED_ */
