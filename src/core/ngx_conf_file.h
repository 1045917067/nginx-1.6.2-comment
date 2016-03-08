
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这个文件是一个NGX_CONF_MODULE类型的模块，主要功能是提供解析配置文件的函数接口。

#ifndef _NGX_CONF_FILE_H_INCLUDED_
#define _NGX_CONF_FILE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/*
 *        AAAA  number of arguments
 *      FF      command flags
 *    TT        command type, i.e. HTTP "location" or "server" command
 */

#define NGX_CONF_NOARGS      0x00000001
#define NGX_CONF_TAKE1       0x00000002
#define NGX_CONF_TAKE2       0x00000004
#define NGX_CONF_TAKE3       0x00000008
#define NGX_CONF_TAKE4       0x00000010
#define NGX_CONF_TAKE5       0x00000020
#define NGX_CONF_TAKE6       0x00000040
#define NGX_CONF_TAKE7       0x00000080

#define NGX_CONF_MAX_ARGS    8

#define NGX_CONF_TAKE12      (NGX_CONF_TAKE1|NGX_CONF_TAKE2)
#define NGX_CONF_TAKE13      (NGX_CONF_TAKE1|NGX_CONF_TAKE3)

#define NGX_CONF_TAKE23      (NGX_CONF_TAKE2|NGX_CONF_TAKE3)

#define NGX_CONF_TAKE123     (NGX_CONF_TAKE1|NGX_CONF_TAKE2|NGX_CONF_TAKE3)
#define NGX_CONF_TAKE1234    (NGX_CONF_TAKE1|NGX_CONF_TAKE2|NGX_CONF_TAKE3   \
                              |NGX_CONF_TAKE4)

#define NGX_CONF_ARGS_NUMBER 0x000000ff
#define NGX_CONF_BLOCK       0x00000100
#define NGX_CONF_FLAG        0x00000200
#define NGX_CONF_ANY         0x00000400
#define NGX_CONF_1MORE       0x00000800
#define NGX_CONF_2MORE       0x00001000
#define NGX_CONF_MULTI       0x00000000  /* compatibility */

#define NGX_DIRECT_CONF      0x00010000

#define NGX_MAIN_CONF        0x01000000
#define NGX_ANY_CONF         0x0F000000



#define NGX_CONF_UNSET       -1
#define NGX_CONF_UNSET_UINT  (ngx_uint_t) -1
#define NGX_CONF_UNSET_PTR   (void *) -1
#define NGX_CONF_UNSET_SIZE  (size_t) -1
#define NGX_CONF_UNSET_MSEC  (ngx_msec_t) -1


#define NGX_CONF_OK          NULL
#define NGX_CONF_ERROR       (void *) -1

#define NGX_CONF_BLOCK_START 1
#define NGX_CONF_BLOCK_DONE  2
#define NGX_CONF_FILE_DONE   3

#define NGX_CORE_MODULE      0x45524F43  /* "CORE" */
#define NGX_CONF_MODULE      0x464E4F43  /* "CONF" */


#define NGX_MAX_CONF_ERRSTR  1024

// 这个结构体的一个对象对应配置文件里的一条指令。
// 配置文件中每个';'和'{'标识一条指令的结束。
struct ngx_command_s {
    // 指令的第一个字段字符串
    ngx_str_t             name;
    // 配置项在配置文件里的允许位置及配置项允许的字段个数
    ngx_uint_t            type;
    // 解析这个配置项时的回调函数
    // cf[in]: 配置项的具体内容保存在cf->args里，存放的方式与main()参数argv相同。
    // cmd[in]: 指向set函数所属的ngx_command_t对象。
    // conf[out]: 根据成员type指向ngx_cycle->conf四级指针的某一级。
    char               *(*set)(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
    // 用于决定配置项设置模块的哪个结构体成员。
    ngx_uint_t            conf;
    // 用于决定配置项设置上面结构体的哪个成员。
    ngx_uint_t            offset;
    // 一般用于给回调函数传送些信息。经常指向一个ngx_conf_post_t对象。
    void                 *post;
};

#define ngx_null_command  { ngx_null_string, 0, NULL, 0, 0, NULL }


// 在reopen操作时需要重新打开的文件
struct ngx_open_file_s {
    ngx_fd_t              fd;
    ngx_str_t             name;

    void                (*flush)(ngx_open_file_t *file, ngx_log_t *log);
    void                 *data;
};


#define NGX_MODULE_V1          0, 0, 0, 0, 0, 0, 1
#define NGX_MODULE_V1_PADDING  0, 0, 0, 0, 0, 0, 0, 0

// 这个结构体的每一个对象对应一个nginx模块。
// 这个结构体里有很多流程相关的回调函数。
// 在全局数组ngx_modules包含指向程序的所有模块指针。
struct ngx_module_s {
    // 这个模块在type相同的一类模块中的序号。
    ngx_uint_t            ctx_index;
    // 这个模块在ngx_modules全局数组（即所有模块）中的序号。
    ngx_uint_t            index;

    // 保留字段
    ngx_uint_t            spare0;
    ngx_uint_t            spare1;
    ngx_uint_t            spare2;
    ngx_uint_t            spare3;

    // 在这个版本中都为1。
    ngx_uint_t            version;

    // 一般类型的模块会把ctx指向一个由回调函数组成的结构体。
    // 这些回调函数往往是解析配置相关的。
    void                 *ctx;
    // 这个模块会处理的配置项数组。
    ngx_command_t        *commands;
    // 模块的类型，有NGX_CORE_MODULE, NGX_EVENT_MODULE, NGX_HTTP_MODULE和NGX_CONF_MODULE等。
    ngx_uint_t            type;

    // 这个回调函数没有使用。
    ngx_int_t           (*init_master)(ngx_log_t *log);
    // 在解析好配置文件之后调用。
    ngx_int_t           (*init_module)(ngx_cycle_t *cycle);
    // 在master/worker模式下，worker、cachemanager、cacheloader进程会在进程初始化函数里调用。
    ngx_int_t           (*init_process)(ngx_cycle_t *cycle);
    // 这个回调函数没有使用。
    ngx_int_t           (*init_thread)(ngx_cycle_t *cycle);
    // 这个回调函数没有使用。
    void                (*exit_thread)(ngx_cycle_t *cycle);
    // 在master/worker模式下，worker进程会在进程即将结束时调用。
    void                (*exit_process)(ngx_cycle_t *cycle);
    // 在master进程退出时调用。
    void                (*exit_master)(ngx_cycle_t *cycle);

    // 保留字段。
    uintptr_t             spare_hook0;
    uintptr_t             spare_hook1;
    uintptr_t             spare_hook2;
    uintptr_t             spare_hook3;
    uintptr_t             spare_hook4;
    uintptr_t             spare_hook5;
    uintptr_t             spare_hook6;
    uintptr_t             spare_hook7;
};

// NGX_CORE_MODULE类型模块在ngx_module_t结构体ctx中填充对象。
typedef struct {
    ngx_str_t             name;
    // 这个回调函数在init_cycle()中调用, 一般用做申请配置结构体对象，并做一些初始化工作。
    void               *(*create_conf)(ngx_cycle_t *cycle);
    // 这个回调函数在init_cycle()中调用，一般用做给未配置的配置结构体对象成员赋默认值，及一些其他初始化工作。
    char               *(*init_conf)(ngx_cycle_t *cycle, void *conf);
} ngx_core_module_t;


// 这个结构体只在ngx_conf_t里定义。
typedef struct {
    // 配置文件
    ngx_file_t            file;
    // 用于缓存配置文件的某一段
    ngx_buf_t            *buffer;
    // 当前读到的行数
    ngx_uint_t            line;
} ngx_conf_file_t;


typedef char *(*ngx_conf_handler_pt)(ngx_conf_t *cf,
    ngx_command_t *dummy, void *conf);


// 作为ngx_conf_parse()和ngx_command_t::set()的参数。
struct ngx_conf_s {
    char                 *name;
    // 在传递给ngx_command_t::set()时，存储着这条指令的每一个字段
    ngx_array_t          *args;

    ngx_cycle_t          *cycle;
    ngx_pool_t           *pool;
    //用该pool的空间都是临时空间，在ngx_init_cycle()结尾会释放
    ngx_pool_t           *temp_pool;
    // 当前解析的配置文件，或某段内存
    ngx_conf_file_t      *conf_file;
    ngx_log_t            *log;

    // 解析到不同的位置有不同的含义
    void                 *ctx;
    // 模块类型,取值为NGX_HTTP_MODULE、NGX_CORE_MODULE、NGX_CONF_MODULE等
    ngx_uint_t            module_type;
    // 命令类型,如NGX_HTTP_SRV_CONF NGX_HTTP_LOC_CONF等
    ngx_uint_t            cmd_type;

    // 如果这个不为空解析时掉用这个函数，而不调用ngx_command_t::set()
    ngx_conf_handler_pt   handler;
    // 调用上面的handler回调函数时的第三个参数
    char                 *handler_conf;
};


typedef char *(*ngx_conf_post_handler_pt) (ngx_conf_t *cf,
    void *data, void *conf);

// (*ngx_command_t)->post经常指向这个结构体对象。
typedef struct {
    ngx_conf_post_handler_pt  post_handler;
} ngx_conf_post_t;


typedef struct {
    ngx_conf_post_handler_pt  post_handler;
    char                     *old_name;
    char                     *new_name;
} ngx_conf_deprecated_t;


typedef struct {
    ngx_conf_post_handler_pt  post_handler;
    ngx_int_t                 low;
    ngx_int_t                 high;
} ngx_conf_num_bounds_t;


typedef struct {
    ngx_str_t                 name;
    ngx_uint_t                value;
} ngx_conf_enum_t;


#define NGX_CONF_BITMASK_SET  1

typedef struct {
    ngx_str_t                 name;
    ngx_uint_t                mask;
} ngx_conf_bitmask_t;



char * ngx_conf_deprecated(ngx_conf_t *cf, void *post, void *data);
char *ngx_conf_check_num_bounds(ngx_conf_t *cf, void *post, void *data);


#define ngx_get_conf(conf_ctx, module)  conf_ctx[module.index]



#define ngx_conf_init_value(conf, default)                                   \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = default;                                                      \
    }

#define ngx_conf_init_ptr_value(conf, default)                               \
    if (conf == NGX_CONF_UNSET_PTR) {                                        \
        conf = default;                                                      \
    }

#define ngx_conf_init_uint_value(conf, default)                              \
    if (conf == NGX_CONF_UNSET_UINT) {                                       \
        conf = default;                                                      \
    }

#define ngx_conf_init_size_value(conf, default)                              \
    if (conf == NGX_CONF_UNSET_SIZE) {                                       \
        conf = default;                                                      \
    }

#define ngx_conf_init_msec_value(conf, default)                              \
    if (conf == NGX_CONF_UNSET_MSEC) {                                       \
        conf = default;                                                      \
    }

#define ngx_conf_merge_value(conf, prev, default)                            \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = (prev == NGX_CONF_UNSET) ? default : prev;                    \
    }

#define ngx_conf_merge_ptr_value(conf, prev, default)                        \
    if (conf == NGX_CONF_UNSET_PTR) {                                        \
        conf = (prev == NGX_CONF_UNSET_PTR) ? default : prev;                \
    }

#define ngx_conf_merge_uint_value(conf, prev, default)                       \
    if (conf == NGX_CONF_UNSET_UINT) {                                       \
        conf = (prev == NGX_CONF_UNSET_UINT) ? default : prev;               \
    }

#define ngx_conf_merge_msec_value(conf, prev, default)                       \
    if (conf == NGX_CONF_UNSET_MSEC) {                                       \
        conf = (prev == NGX_CONF_UNSET_MSEC) ? default : prev;               \
    }

#define ngx_conf_merge_sec_value(conf, prev, default)                        \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = (prev == NGX_CONF_UNSET) ? default : prev;                    \
    }

#define ngx_conf_merge_size_value(conf, prev, default)                       \
    if (conf == NGX_CONF_UNSET_SIZE) {                                       \
        conf = (prev == NGX_CONF_UNSET_SIZE) ? default : prev;               \
    }

#define ngx_conf_merge_off_value(conf, prev, default)                        \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = (prev == NGX_CONF_UNSET) ? default : prev;                    \
    }

#define ngx_conf_merge_str_value(conf, prev, default)                        \
    if (conf.data == NULL) {                                                 \
        if (prev.data) {                                                     \
            conf.len = prev.len;                                             \
            conf.data = prev.data;                                           \
        } else {                                                             \
            conf.len = sizeof(default) - 1;                                  \
            conf.data = (u_char *) default;                                  \
        }                                                                    \
    }

#define ngx_conf_merge_bufs_value(conf, prev, default_num, default_size)     \
    if (conf.num == 0) {                                                     \
        if (prev.num) {                                                      \
            conf.num = prev.num;                                             \
            conf.size = prev.size;                                           \
        } else {                                                             \
            conf.num = default_num;                                          \
            conf.size = default_size;                                        \
        }                                                                    \
    }

#define ngx_conf_merge_bitmask_value(conf, prev, default)                    \
    if (conf == 0) {                                                         \
        conf = (prev == 0) ? default : prev;                                 \
    }


char *ngx_conf_param(ngx_conf_t *cf);
char *ngx_conf_parse(ngx_conf_t *cf, ngx_str_t *filename);
char *ngx_conf_include(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


ngx_int_t ngx_conf_full_name(ngx_cycle_t *cycle, ngx_str_t *name,
    ngx_uint_t conf_prefix);
ngx_open_file_t *ngx_conf_open_file(ngx_cycle_t *cycle, ngx_str_t *name);
void ngx_cdecl ngx_conf_log_error(ngx_uint_t level, ngx_conf_t *cf,
    ngx_err_t err, const char *fmt, ...);


// 14个解析配置文件的通用回调函数，会被赋值给ngx_command_t::set.
char *ngx_conf_set_flag_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_str_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_str_array_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
char *ngx_conf_set_keyval_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_num_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_size_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_off_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_msec_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_sec_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_bufs_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_enum_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_bitmask_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


extern ngx_uint_t     ngx_max_module;
extern ngx_module_t  *ngx_modules[];


#endif /* _NGX_CONF_FILE_H_INCLUDED_ */
