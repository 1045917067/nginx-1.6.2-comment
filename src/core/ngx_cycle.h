
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件和实现文件声明了ngx_cycle_t结构体，并实现了相关的操作函数。

#ifndef _NGX_CYCLE_H_INCLUDED_
#define _NGX_CYCLE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


#ifndef NGX_CYCLE_POOL_SIZE
#define NGX_CYCLE_POOL_SIZE     NGX_DEFAULT_POOL_SIZE
#endif


#define NGX_DEBUG_POINTS_STOP   1
#define NGX_DEBUG_POINTS_ABORT  2


typedef struct ngx_shm_zone_s  ngx_shm_zone_t;

typedef ngx_int_t (*ngx_shm_zone_init_pt) (ngx_shm_zone_t *zone, void *data);

// 共享内存的内存池
struct ngx_shm_zone_s {
    // 自定义使用
    void                     *data;
    // 共享内存池使用的共享内存
    ngx_shm_t                 shm;
    // 初始化共享内存区时的回调函数。
    ngx_shm_zone_init_pt      init;
    void                     *tag;
};


struct ngx_cycle_s {
    // 指向所有配置文件解析到的所有配置结构体对象。
    // 第一级指针指向一个指针数组，这个数组的元素个数等于模块个数，ngx_module_t::index对应这个模块在数组中的index。
    // 第二级指针数组中，ngx_http_module位置的指针指向一个ngx_http_conf_ctx_t对象，也就是一个有三个元素的指针数组。
    void                  ****conf_ctx;
    ngx_pool_t               *pool;

    ngx_log_t                *log;
    ngx_log_t                 new_log;

    // 日志打印到终端，而不打印到log文件。
    ngx_uint_t                log_use_stderr;  /* unsigned  log_use_stderr:1; */

    // 这个成员在epoll事件模型下无用。
    ngx_connection_t        **files;
    ngx_connection_t         *free_connections;
    // 成员connections数组中空闲可用的元素个数。
    ngx_uint_t                free_connection_n;

    // 成员connections数组中可重用的元素链表。
    ngx_queue_t               reusable_connections_queue;

    // 正常运行时会监听的端口。 
    ngx_array_t               listening;      // ngx_listening_t
    // 配置文件中所使用的path
    ngx_array_t               paths;          // ngx_path_t *
    // 在reopen操作时需要重新打开的文件
    ngx_list_t                open_files;     // ngx_open_file_t
    // 正常运行时会使用的共享内存。
    ngx_list_t                shared_memory;  // ngx_shm_zone_t

    // 成员connections数组的元素个数。
    ngx_uint_t                connection_n;
    // 这个成员在epoll事件模型下无用。
    ngx_uint_t                files_n;

    // 指向一个ngx_connection_t数组，数组元素个数是上面的connection_n成员.
    ngx_connection_t         *connections;
    // 指向一个ngx_event_t数组，数组元素个数是上面的connection_n成员，与上面的connections成员一一对应。
    ngx_event_t              *read_events;
    // 指向一个ngx_event_t数组，数组元素个数是上面的connection_n成员，与上面的connections成员一一对应。
    ngx_event_t              *write_events;

    ngx_cycle_t              *old_cycle;

    // 配置文件绝对路径。
    ngx_str_t                 conf_file;
    // 存储命令行参数-g携带的一些信息。
    ngx_str_t                 conf_param;
    // config文件目录的绝对路径。
    ngx_str_t                 conf_prefix;
    // 进程运行CWD目录的绝对路径。 
    ngx_str_t                 prefix;
    // 用于进程间同步的文件锁。
    ngx_str_t                 lock_file;
    // 由glibc函数gethostname得到的hostname。
    ngx_str_t                 hostname;
};

// ngx_core_module的配置信息
typedef struct {
     ngx_flag_t               daemon;
     ngx_flag_t               master;

     ngx_msec_t               timer_resolution;

     ngx_int_t                worker_processes;
     ngx_int_t                debug_points;

     ngx_int_t                rlimit_nofile;
     ngx_int_t                rlimit_sigpending;
     off_t                    rlimit_core;

     int                      priority;

     ngx_uint_t               cpu_affinity_n;
     uint64_t                *cpu_affinity;

     char                    *username;
     ngx_uid_t                user;
     ngx_gid_t                group;

     ngx_str_t                working_directory;
     ngx_str_t                lock_file;

     ngx_str_t                pid;
     ngx_str_t                oldpid;

     ngx_array_t              env;
     // 指向进程的环境变量
     char                   **environment;

#if (NGX_THREADS)
     ngx_int_t                worker_threads;
     size_t                   thread_stack_size;
#endif

} ngx_core_conf_t;


typedef struct {
     ngx_pool_t              *pool;   /* pcre's malloc() pool */
} ngx_core_tls_t;


#define ngx_is_init_cycle(cycle)  (cycle->conf_ctx == NULL)


ngx_cycle_t *ngx_init_cycle(ngx_cycle_t *old_cycle);
ngx_int_t ngx_create_pidfile(ngx_str_t *name, ngx_log_t *log);
void ngx_delete_pidfile(ngx_cycle_t *cycle);
ngx_int_t ngx_signal_process(ngx_cycle_t *cycle, char *sig);
void ngx_reopen_files(ngx_cycle_t *cycle, ngx_uid_t user);
char **ngx_set_environment(ngx_cycle_t *cycle, ngx_uint_t *last);
ngx_pid_t ngx_exec_new_binary(ngx_cycle_t *cycle, char *const *argv);
uint64_t ngx_get_cpu_affinity(ngx_uint_t n);
ngx_shm_zone_t *ngx_shared_memory_add(ngx_conf_t *cf, ngx_str_t *name,
    size_t size, void *tag);


extern volatile ngx_cycle_t  *ngx_cycle;
extern ngx_array_t            ngx_old_cycles;
extern ngx_module_t           ngx_core_module;
extern ngx_uint_t             ngx_test_config;
extern ngx_uint_t             ngx_quiet_mode;
#if (NGX_THREADS)
extern ngx_tls_key_t          ngx_core_tls_key;
#endif


#endif /* _NGX_CYCLE_H_INCLUDED_ */
