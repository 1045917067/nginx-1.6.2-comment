
/*
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件和实现文件实现了一个将当前进程与某些CPU绑定的函数。

#include <ngx_config.h>
#include <ngx_core.h>

// 根据是否支持cpuset_setaffinity()和sched_setaffinity()来决定ngx_setaffinity()的实现方式。
// linux下使用sched_setaffinity()实现。
#if (NGX_HAVE_CPUSET_SETAFFINITY)

#include <sys/cpuset.h>

// 将当前进程绑定到参数cpu_affinity对应比特位为1的位置的CPU核心。
// cpu_affinity[in]: 如果为1将绑定到第0个CPU，如果为2将绑定到第1个CPU，如果为4将绑定到第2个CPU，以此类推。
//                   同时支持绑定到多个CPU，比如为3可以绑定到第0和第1个CPU，为5可以绑定到第0和第2个CPU。
void
ngx_setaffinity(uint64_t cpu_affinity, ngx_log_t *log)
{
    cpuset_t    mask;
    ngx_uint_t  i;

    ngx_log_error(NGX_LOG_NOTICE, log, 0,
                  "cpuset_setaffinity(0x%08Xl)", cpu_affinity);

    CPU_ZERO(&mask);
    i = 0;
    do {
        if (cpu_affinity & 1) {
            CPU_SET(i, &mask);
        }
        i++;
        cpu_affinity >>= 1;
    } while (cpu_affinity);

    if (cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1,
                           sizeof(cpuset_t), &mask) == -1)
    {
        ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                      "cpuset_setaffinity() failed");
    }
}

#elif (NGX_HAVE_SCHED_SETAFFINITY)

// 将当前进程绑定到参数cpu_affinity对应比特位为1的位置的CPU核心。
// cpu_affinity[in]: 如果为1将绑定到第0个CPU，如果为2将绑定到第1个CPU，如果为4将绑定到第2个CPU，以此类推。
//                   同时支持绑定到多个CPU，比如为3可以绑定到第0和第1个CPU，为5可以绑定到第0和第2个CPU。
void
ngx_setaffinity(uint64_t cpu_affinity, ngx_log_t *log)
{
    cpu_set_t   mask;
    ngx_uint_t  i;

    ngx_log_error(NGX_LOG_NOTICE, log, 0,
                  "sched_setaffinity(0x%08Xl)", cpu_affinity);

    CPU_ZERO(&mask);
    i = 0;
    do {
        if (cpu_affinity & 1) {
            CPU_SET(i, &mask);
        }
        i++;
        cpu_affinity >>= 1;
    } while (cpu_affinity);

    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
        ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                      "sched_setaffinity() failed");
    }
}

#endif
