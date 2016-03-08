
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

// 这组头文件和实现文件主要定义了一些初始化、死循环、关闭清理等各个进程运行的流程性函数。

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_channel.h>


static void ngx_start_worker_processes(ngx_cycle_t *cycle, ngx_int_t n,
    ngx_int_t type);
static void ngx_start_cache_manager_processes(ngx_cycle_t *cycle,
    ngx_uint_t respawn);
static void ngx_pass_open_channel(ngx_cycle_t *cycle, ngx_channel_t *ch);
static void ngx_signal_worker_processes(ngx_cycle_t *cycle, int signo);
static ngx_uint_t ngx_reap_children(ngx_cycle_t *cycle);
static void ngx_master_process_exit(ngx_cycle_t *cycle);
static void ngx_worker_process_cycle(ngx_cycle_t *cycle, void *data);
static void ngx_worker_process_init(ngx_cycle_t *cycle, ngx_int_t worker);
static void ngx_worker_process_exit(ngx_cycle_t *cycle);
static void ngx_channel_handler(ngx_event_t *ev);
#if (NGX_THREADS)
static void ngx_wakeup_worker_threads(ngx_cycle_t *cycle);
static ngx_thread_value_t ngx_worker_thread_cycle(void *data);
#endif
static void ngx_cache_manager_process_cycle(ngx_cycle_t *cycle, void *data);
static void ngx_cache_manager_process_handler(ngx_event_t *ev);
static void ngx_cache_loader_process_handler(ngx_event_t *ev);


// 记录当前进程类型的全局变量。
// 可能的取值是NGX_PROCESS_SINGLE等，声明在ngx_process_cycle.h中 
ngx_uint_t    ngx_process;
// 记录当前进程pid的全局变量，需要时直接使用这个全局变量，
// 不再需要用系统调用getpid()来获取。
ngx_pid_t     ngx_pid;
ngx_uint_t    ngx_threaded;

// 当master进程的子进程意外退出时，这个全局变量会被置1.
sig_atomic_t  ngx_reap;
// 当nignx任意进程收到SIGIO信号是，这个全局变量会被置1,nginx不处理这个信号
sig_atomic_t  ngx_sigio;
// master进程会使用这个变量
sig_atomic_t  ngx_sigalrm;
// master进程会在收到SIGINT信号后将这个全局变量置1，
// master进程子进程收到SIGINT信号或收到父进程的关闭命令后会被置1
sig_atomic_t  ngx_terminate;
// master进程和master进程的子进程都会使用这个变量
sig_atomic_t  ngx_quit;
// 跟调试点相关的一个变量
sig_atomic_t  ngx_debug_quit;
// worker进程等master进程子进程会使用这个变量 
ngx_uint_t    ngx_exiting;
// master进程会使用这个变量，reload操作
sig_atomic_t  ngx_reconfigure;
// master进程和子进程都会使用这个变量,重新打开日志文件
sig_atomic_t  ngx_reopen;

// master进程会使用这个全局变量
sig_atomic_t  ngx_change_binary;
// 平滑升级时新的master进程的pid
ngx_pid_t     ngx_new_binary;
// 这个变量为1说明是平滑升级启动
ngx_uint_t    ngx_inherited;
// nignx是否是守护进程模式
ngx_uint_t    ngx_daemonized;

// master进程会使用这个全局变量
// 接收到sigwinch信号，使worker进程不再接收新请求
sig_atomic_t  ngx_noaccept;
// master进程会使用这个全局变量
ngx_uint_t    ngx_noaccepting;
// master进程会使用这个全局变量
// 平滑升级时老的worker进入不接受请求状态，新的nignx意外退出，
// 需要重启老nginx的进程
ngx_uint_t    ngx_restart;


#if (NGX_THREADS)
volatile ngx_thread_t  ngx_threads[NGX_MAX_THREADS];
ngx_int_t              ngx_threads_n;
#endif


static u_char  master_process[] = "master process";


static ngx_cache_manager_ctx_t  ngx_cache_manager_ctx = {
    ngx_cache_manager_process_handler, "cache manager process", 0
};

static ngx_cache_manager_ctx_t  ngx_cache_loader_ctx = {
    ngx_cache_loader_process_handler, "cache loader process", 60000
};


static ngx_cycle_t      ngx_exit_cycle;
static ngx_log_t        ngx_exit_log;
static ngx_open_file_t  ngx_exit_log_file;


// 启动后master进程在这个函数进入死循环
void
ngx_master_process_cycle(ngx_cycle_t *cycle)
{
    char              *title;
    u_char            *p;
    size_t             size;
    ngx_int_t          i;
    ngx_uint_t         n, sigio;
    sigset_t           set;
    struct itimerval   itv;
    ngx_uint_t         live;
    ngx_msec_t         delay;
    ngx_listening_t   *ls;
    ngx_core_conf_t   *ccf;

    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGIO);
    sigaddset(&set, SIGINT);
    sigaddset(&set, ngx_signal_value(NGX_RECONFIGURE_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_REOPEN_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_NOACCEPT_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_TERMINATE_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_SHUTDOWN_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_CHANGEBIN_SIGNAL));

    // 让master进程只有在sigsuspend()里才会捕捉参数set里的信号。
    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "sigprocmask() failed");
    }

    sigemptyset(&set);


    // 设置ps命令显示的进程标题
    size = sizeof(master_process);

    for (i = 0; i < ngx_argc; i++) {
        size += ngx_strlen(ngx_argv[i]) + 1;
    }

    title = ngx_pnalloc(cycle->pool, size);

    p = ngx_cpymem(title, master_process, sizeof(master_process) - 1);
    for (i = 0; i < ngx_argc; i++) {
        *p++ = ' ';
        p = ngx_cpystrn(p, (u_char *) ngx_argv[i], size);
    }

    ngx_setproctitle(title);


    ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module);

    // 启动worker进程
    ngx_start_worker_processes(cycle, ccf->worker_processes,
                               NGX_PROCESS_RESPAWN);
    // 配置文件配置了相关功能时打开cache manager或cache loader进程,
    ngx_start_cache_manager_processes(cycle, 0);

    ngx_new_binary = 0;
    delay = 0;
    sigio = 0;
    live = 1;

    for ( ;; ) {
        if (delay) {
        // 收到退出信号后会使用这个分支，
        // 作用是定时检查是否还有活跃子进程，如果没有就退出master进程
            if (ngx_sigalrm) {
                sigio = 0;
                delay *= 2;
                ngx_sigalrm = 0;
            }

            ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                           "termination cycle: %d", delay);

            itv.it_interval.tv_sec = 0;
            itv.it_interval.tv_usec = 0;
            itv.it_value.tv_sec = delay / 1000;
            itv.it_value.tv_usec = (delay % 1000 ) * 1000;

            if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
                ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                              "setitimer() failed");
            }
        }

        ngx_log_debug0(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "sigsuspend");

        // 当没有收到信号时进程会在这个函数里休眠, 当收到信号后如果有信号处理函数先运行信号处理函数，
        // 然后向下运行。
        sigsuspend(&set);

        // 更新记录当前时间的全局变量。
        ngx_time_update();

        ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                       "wake up, sigio %i", sigio);

        if (ngx_reap) {
        // master进程的子进程结束时，master进程收到SIGCHLD信号后，ngx_reap被置1。
            ngx_reap = 0;
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "reap children");

            live = ngx_reap_children(cycle);
        }

        if (!live && (ngx_terminate || ngx_quit)) {
        // 收到退出信号且已经没有活跃子进程后便退出
            ngx_master_process_exit(cycle);
        }

        if (ngx_terminate) {
        // 收到SIGINT,SIGTERM信号后，直接关闭nginx,
        // 首先给子进程发送关闭信号，1秒还没有关完就强制关闭所有子进程
            if (delay == 0) {
                delay = 50;
            }

            if (sigio) {
                sigio--;
                continue;
            }

            sigio = ccf->worker_processes + 2 /* cache processes */;

            if (delay > 1000) {
                ngx_signal_worker_processes(cycle, SIGKILL);
            } else {
                ngx_signal_worker_processes(cycle,
                                       ngx_signal_value(NGX_TERMINATE_SIGNAL));
            }

            continue;
        }

        if (ngx_quit) {
        // 收到SIGQUIT信号后，优雅关闭Nginx，
        // 待所有worker进程将已经连接的请求处理完后在退出
            ngx_signal_worker_processes(cycle,
                                        ngx_signal_value(NGX_SHUTDOWN_SIGNAL));

            ls = cycle->listening.elts;
            for (n = 0; n < cycle->listening.nelts; n++) {
                if (ngx_close_socket(ls[n].fd) == -1) {
                    ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_socket_errno,
                                  ngx_close_socket_n " %V failed",
                                  &ls[n].addr_text);
                }
            }
            cycle->listening.nelts = 0;

            continue;
        }

        if (ngx_reconfigure) {
        // 收到SIGHUP信号后，ngx_reconfigure在信号处理函数中被置1，执行让新配置文件生效的操作。
            ngx_reconfigure = 0;

            if (ngx_new_binary) {
            // 在不重新载入配置文件的情况下启动工作进程。
                ngx_start_worker_processes(cycle, ccf->worker_processes,
                                           NGX_PROCESS_RESPAWN);
                ngx_start_cache_manager_processes(cycle, 0);
                ngx_noaccepting = 0;

                continue;
            }

            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "reconfiguring");

            cycle = ngx_init_cycle(cycle);
            if (cycle == NULL) {
                cycle = (ngx_cycle_t *) ngx_cycle;
                continue;
            }

            ngx_cycle = cycle;
            ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx,
                                                   ngx_core_module);
            ngx_start_worker_processes(cycle, ccf->worker_processes,
                                       NGX_PROCESS_JUST_RESPAWN);
            ngx_start_cache_manager_processes(cycle, 1);

            /* allow new processes to start */
            ngx_msleep(100);

            live = 1;
            ngx_signal_worker_processes(cycle,
                                        ngx_signal_value(NGX_SHUTDOWN_SIGNAL));
        }

        if (ngx_restart) {
        // 好像是平滑升级时，新起的nginx意外退出了，要在把之前nginx的工作进程在重新启动
            ngx_restart = 0;
            ngx_start_worker_processes(cycle, ccf->worker_processes,
                                       NGX_PROCESS_RESPAWN);
            ngx_start_cache_manager_processes(cycle, 0);
            live = 1;
        }

        if (ngx_reopen) {
        // 重新打开日志文件
            ngx_reopen = 0;
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "reopening logs");
            ngx_reopen_files(cycle, ccf->user);
            ngx_signal_worker_processes(cycle,
                                        ngx_signal_value(NGX_REOPEN_SIGNAL));
        }

        if (ngx_change_binary) {
        // 启动新的nignx,使新旧nginx同时工作
            ngx_change_binary = 0;
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "changing binary");
            ngx_new_binary = ngx_exec_new_binary(cycle, ngx_argv);
        }

        if (ngx_noaccept) {
        // 收到sigwinch信号后优雅关闭所有子进程
            ngx_noaccept = 0;
            ngx_noaccepting = 1;
            ngx_signal_worker_processes(cycle,
                                        ngx_signal_value(NGX_SHUTDOWN_SIGNAL));
        }
    }
}


// 单进程启动时在这个函数进入死循环
void
ngx_single_process_cycle(ngx_cycle_t *cycle)
{
    ngx_uint_t  i;

    if (ngx_set_environment(cycle, NULL) == NULL) {
        /* fatal */
        exit(2);
    }

    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->init_process) {
            if (ngx_modules[i]->init_process(cycle) == NGX_ERROR) {
                /* fatal */
                exit(2);
            }
        }
    }

    for ( ;; ) {
        ngx_log_debug0(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "worker cycle");

        ngx_process_events_and_timers(cycle);

        if (ngx_terminate || ngx_quit) {

            for (i = 0; ngx_modules[i]; i++) {
                if (ngx_modules[i]->exit_process) {
                    ngx_modules[i]->exit_process(cycle);
                }
            }

            ngx_master_process_exit(cycle);
        }

        if (ngx_reconfigure) {
            ngx_reconfigure = 0;
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "reconfiguring");

            cycle = ngx_init_cycle(cycle);
            if (cycle == NULL) {
                cycle = (ngx_cycle_t *) ngx_cycle;
                continue;
            }

            ngx_cycle = cycle;
        }

        if (ngx_reopen) {
            ngx_reopen = 0;
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "reopening logs");
            ngx_reopen_files(cycle, (ngx_uid_t) -1);
        }
    }
}


// 启动所有worker进程,并同步所有已启动进程的ngx_processes全局数组。
// n[in]: worker进程的数量。
static void
ngx_start_worker_processes(ngx_cycle_t *cycle, ngx_int_t n, ngx_int_t type)
{
    ngx_int_t      i;
    ngx_channel_t  ch;

    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "start worker processes");

    ngx_memzero(&ch, sizeof(ngx_channel_t));

    ch.command = NGX_CMD_OPEN_CHANNEL;

    for (i = 0; i < n; i++) {

        ngx_spawn_process(cycle, ngx_worker_process_cycle,
                          (void *) (intptr_t) i, "worker process", type);

        ch.pid = ngx_processes[ngx_process_slot].pid;
        ch.slot = ngx_process_slot;
        ch.fd = ngx_processes[ngx_process_slot].channel[0];

        ngx_pass_open_channel(cycle, &ch);
    }
}


// 配置文件配置了相关功能时打开cache manager或cache loader进程,
// 并同步所有已启动进程的ngx_processes全局变量
static void
ngx_start_cache_manager_processes(ngx_cycle_t *cycle, ngx_uint_t respawn)
{
    ngx_uint_t       i, manager, loader;
    ngx_path_t     **path;
    ngx_channel_t    ch;

    manager = 0;
    loader = 0;

    path = ngx_cycle->paths.elts;
    for (i = 0; i < ngx_cycle->paths.nelts; i++) {

        if (path[i]->manager) {
            manager = 1;
        }

        if (path[i]->loader) {
            loader = 1;
        }
    }

    if (manager == 0) {
        return;
    }

    ngx_spawn_process(cycle, ngx_cache_manager_process_cycle,
                      &ngx_cache_manager_ctx, "cache manager process",
                      respawn ? NGX_PROCESS_JUST_RESPAWN : NGX_PROCESS_RESPAWN);

    ngx_memzero(&ch, sizeof(ngx_channel_t));

    ch.command = NGX_CMD_OPEN_CHANNEL;
    ch.pid = ngx_processes[ngx_process_slot].pid;
    ch.slot = ngx_process_slot;
    ch.fd = ngx_processes[ngx_process_slot].channel[0];

    ngx_pass_open_channel(cycle, &ch);

    if (loader == 0) {
        return;
    }

    ngx_spawn_process(cycle, ngx_cache_manager_process_cycle,
                      &ngx_cache_loader_ctx, "cache loader process",
                      respawn ? NGX_PROCESS_JUST_SPAWN : NGX_PROCESS_NORESPAWN);

    ch.command = NGX_CMD_OPEN_CHANNEL;
    ch.pid = ngx_processes[ngx_process_slot].pid;
    ch.slot = ngx_process_slot;
    ch.fd = ngx_processes[ngx_process_slot].channel[0];

    ngx_pass_open_channel(cycle, &ch);
}


// master进程通过这个函数通知所有子进程，进程组里又添加了某个进程。
static void
ngx_pass_open_channel(ngx_cycle_t *cycle, ngx_channel_t *ch)
{
    ngx_int_t  i;

    for (i = 0; i < ngx_last_process; i++) {

        if (i == ngx_process_slot
            || ngx_processes[i].pid == -1
            || ngx_processes[i].channel[0] == -1)
        {
            continue;
        }

        ngx_log_debug6(NGX_LOG_DEBUG_CORE, cycle->log, 0,
                      "pass channel s:%d pid:%P fd:%d to s:%i pid:%P fd:%d",
                      ch->slot, ch->pid, ch->fd,
                      i, ngx_processes[i].pid,
                      ngx_processes[i].channel[0]);

        /* TODO: NGX_AGAIN */

        ngx_write_channel(ngx_processes[i].channel[0],
                          ch, sizeof(ngx_channel_t), cycle->log);
    }
}


// master进程通过这个函数向所有master进程的子进程发送signo信号。
static void
ngx_signal_worker_processes(ngx_cycle_t *cycle, int signo)
{
    ngx_int_t      i;
    ngx_err_t      err;
    ngx_channel_t  ch;

    ngx_memzero(&ch, sizeof(ngx_channel_t));

#if (NGX_BROKEN_SCM_RIGHTS)

    ch.command = 0;

#else

    switch (signo) {

    case ngx_signal_value(NGX_SHUTDOWN_SIGNAL):
        ch.command = NGX_CMD_QUIT;
        break;

    case ngx_signal_value(NGX_TERMINATE_SIGNAL):
        ch.command = NGX_CMD_TERMINATE;
        break;

    case ngx_signal_value(NGX_REOPEN_SIGNAL):
        ch.command = NGX_CMD_REOPEN;
        break;

    default:
        ch.command = 0;
    }

#endif

    ch.fd = -1;


    for (i = 0; i < ngx_last_process; i++) {

        ngx_log_debug7(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                       "child: %d %P e:%d t:%d d:%d r:%d j:%d",
                       i,
                       ngx_processes[i].pid,
                       ngx_processes[i].exiting,
                       ngx_processes[i].exited,
                       ngx_processes[i].detached,
                       ngx_processes[i].respawn,
                       ngx_processes[i].just_spawn);

        if (ngx_processes[i].detached || ngx_processes[i].pid == -1) {
            continue;
        }

        if (ngx_processes[i].just_spawn) {
            ngx_processes[i].just_spawn = 0;
            continue;
        }

        if (ngx_processes[i].exiting
            && signo == ngx_signal_value(NGX_SHUTDOWN_SIGNAL))
        {
            continue;
        }

        if (ch.command) {
            if (ngx_write_channel(ngx_processes[i].channel[0],
                                  &ch, sizeof(ngx_channel_t), cycle->log)
                == NGX_OK)
            {
                if (signo != ngx_signal_value(NGX_REOPEN_SIGNAL)) {
                    ngx_processes[i].exiting = 1;
                }

                continue;
            }
        }

        ngx_log_debug2(NGX_LOG_DEBUG_CORE, cycle->log, 0,
                       "kill (%P, %d)", ngx_processes[i].pid, signo);

        if (kill(ngx_processes[i].pid, signo) == -1) {
            err = ngx_errno;
            ngx_log_error(NGX_LOG_ALERT, cycle->log, err,
                          "kill(%P, %d) failed", ngx_processes[i].pid, signo);

            if (err == NGX_ESRCH) {
                ngx_processes[i].exited = 1;
                ngx_processes[i].exiting = 0;
                ngx_reap = 1;
            }

            continue;
        }

        if (signo != ngx_signal_value(NGX_REOPEN_SIGNAL)) {
            ngx_processes[i].exiting = 1;
        }
    }
}


// master进程收到SIGCHILD信号也就是有子进程退出时调用
// return: 如果子进程都正常退出返回0，否则返回1.
static ngx_uint_t
ngx_reap_children(ngx_cycle_t *cycle)
{
    ngx_int_t         i, n;
    ngx_uint_t        live;
    ngx_channel_t     ch;
    ngx_core_conf_t  *ccf;

    ngx_memzero(&ch, sizeof(ngx_channel_t));

    ch.command = NGX_CMD_CLOSE_CHANNEL;
    ch.fd = -1;

    live = 0;
    for (i = 0; i < ngx_last_process; i++) {

        ngx_log_debug7(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                       "child: %d %P e:%d t:%d d:%d r:%d j:%d",
                       i,
                       ngx_processes[i].pid,
                       ngx_processes[i].exiting,
                       ngx_processes[i].exited,
                       ngx_processes[i].detached,
                       ngx_processes[i].respawn,
                       ngx_processes[i].just_spawn);

        if (ngx_processes[i].pid == -1) {
            continue;
        }

        if (ngx_processes[i].exited) {

            if (!ngx_processes[i].detached) {
                ngx_close_channel(ngx_processes[i].channel, cycle->log);

                ngx_processes[i].channel[0] = -1;
                ngx_processes[i].channel[1] = -1;

                ch.pid = ngx_processes[i].pid;
                ch.slot = i;

                for (n = 0; n < ngx_last_process; n++) {
                    if (ngx_processes[n].exited
                        || ngx_processes[n].pid == -1
                        || ngx_processes[n].channel[0] == -1)
                    {
                        continue;
                    }

                    ngx_log_debug3(NGX_LOG_DEBUG_CORE, cycle->log, 0,
                                   "pass close channel s:%i pid:%P to:%P",
                                   ch.slot, ch.pid, ngx_processes[n].pid);

                    /* TODO: NGX_AGAIN */

                    ngx_write_channel(ngx_processes[n].channel[0],
                                      &ch, sizeof(ngx_channel_t), cycle->log);
                }
            }

            if (ngx_processes[i].respawn
                && !ngx_processes[i].exiting
                && !ngx_terminate
                && !ngx_quit)
            {
                if (ngx_spawn_process(cycle, ngx_processes[i].proc,
                                      ngx_processes[i].data,
                                      ngx_processes[i].name, i)
                    == NGX_INVALID_PID)
                {
                    ngx_log_error(NGX_LOG_ALERT, cycle->log, 0,
                                  "could not respawn %s",
                                  ngx_processes[i].name);
                    continue;
                }


                ch.command = NGX_CMD_OPEN_CHANNEL;
                ch.pid = ngx_processes[ngx_process_slot].pid;
                ch.slot = ngx_process_slot;
                ch.fd = ngx_processes[ngx_process_slot].channel[0];

                ngx_pass_open_channel(cycle, &ch);

                live = 1;

                continue;
            }

            if (ngx_processes[i].pid == ngx_new_binary) {

                ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx,
                                                       ngx_core_module);

                if (ngx_rename_file((char *) ccf->oldpid.data,
                                    (char *) ccf->pid.data)
                    == NGX_FILE_ERROR)
                {
                    ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                                  ngx_rename_file_n " %s back to %s failed "
                                  "after the new binary process \"%s\" exited",
                                  ccf->oldpid.data, ccf->pid.data, ngx_argv[0]);
                }

                ngx_new_binary = 0;
                if (ngx_noaccepting) {
                // 好像是平滑升级时，新起的nginx意外退出了，要在把之前nginx的工作进程在重新启动
                    ngx_restart = 1;
                    ngx_noaccepting = 0;
                }
            }

            if (i == ngx_last_process - 1) {
                ngx_last_process--;

            } else {
                ngx_processes[i].pid = -1;
            }

        } else if (ngx_processes[i].exiting || !ngx_processes[i].detached) {
            live = 1;
        }
    }

    return live;
}


// master进程退出时的一些清理和关闭行为
static void
ngx_master_process_exit(ngx_cycle_t *cycle)
{
    ngx_uint_t  i;

    ngx_delete_pidfile(cycle);

    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "exit");

    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->exit_master) {
            ngx_modules[i]->exit_master(cycle);
        }
    }

    ngx_close_listening_sockets(cycle);

    /*
     * Copy ngx_cycle->log related data to the special static exit cycle,
     * log, and log file structures enough to allow a signal handler to log.
     * The handler may be called when standard ngx_cycle->log allocated from
     * ngx_cycle->pool is already destroyed.
     */

    ngx_exit_log_file.fd = ngx_cycle->log->file->fd;

    ngx_exit_log = *ngx_cycle->log;
    ngx_exit_log.file = &ngx_exit_log_file;
    ngx_exit_log.next = NULL;

    ngx_exit_cycle.log = &ngx_exit_log;
    ngx_exit_cycle.files = ngx_cycle->files;
    ngx_exit_cycle.files_n = ngx_cycle->files_n;
    ngx_cycle = &ngx_exit_cycle;

    ngx_destroy_pool(cycle->pool);

    exit(0);
}


// 启动后worker进程在这个函数进入死循环
static void
ngx_worker_process_cycle(ngx_cycle_t *cycle, void *data)
{
    ngx_int_t worker = (intptr_t) data;

    ngx_uint_t         i;
    ngx_connection_t  *c;

    ngx_process = NGX_PROCESS_WORKER;

    ngx_worker_process_init(cycle, worker);

    ngx_setproctitle("worker process");

#if (NGX_THREADS)
    {
    ngx_int_t         n;
    ngx_err_t         err;
    ngx_core_conf_t  *ccf;

    ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module);

    if (ngx_threads_n) {
        if (ngx_init_threads(ngx_threads_n, ccf->thread_stack_size, cycle)
            == NGX_ERROR)
        {
            /* fatal */
            exit(2);
        }

        err = ngx_thread_key_create(&ngx_core_tls_key);
        if (err != 0) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, err,
                          ngx_thread_key_create_n " failed");
            /* fatal */
            exit(2);
        }

        for (n = 0; n < ngx_threads_n; n++) {

            ngx_threads[n].cv = ngx_cond_init(cycle->log);

            if (ngx_threads[n].cv == NULL) {
                /* fatal */
                exit(2);
            }

            if (ngx_create_thread((ngx_tid_t *) &ngx_threads[n].tid,
                                  ngx_worker_thread_cycle,
                                  (void *) &ngx_threads[n], cycle->log)
                != 0)
            {
                /* fatal */
                exit(2);
            }
        }
    }
    }
#endif

    for ( ;; ) {

        if (ngx_exiting) {

            c = cycle->connections;

            for (i = 0; i < cycle->connection_n; i++) {

                /* THREAD: lock */

                if (c[i].fd != -1 && c[i].idle) {
                    c[i].close = 1;
                    c[i].read->handler(c[i].read);
                }
            }

            if (ngx_event_timer_rbtree.root == ngx_event_timer_rbtree.sentinel)
            {
                ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "exiting");

                ngx_worker_process_exit(cycle);
            }
        }

        ngx_log_debug0(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "worker cycle");

        ngx_process_events_and_timers(cycle);

        if (ngx_terminate) {
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "exiting");

            ngx_worker_process_exit(cycle);
        }

        if (ngx_quit) {
            ngx_quit = 0;
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0,
                          "gracefully shutting down");
            ngx_setproctitle("worker process is shutting down");

            if (!ngx_exiting) {
                ngx_close_listening_sockets(cycle);
                ngx_exiting = 1;
            }
        }

        if (ngx_reopen) {
            ngx_reopen = 0;
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "reopening logs");
            ngx_reopen_files(cycle, -1);
        }
    }
}


// worker进程，cachemanager进程和cacheloader进程的初始化函数
static void
ngx_worker_process_init(ngx_cycle_t *cycle, ngx_int_t worker)
{
    sigset_t          set;
    uint64_t          cpu_affinity;
    ngx_int_t         n;
    ngx_uint_t        i;
    struct rlimit     rlmt;
    ngx_core_conf_t  *ccf;
    ngx_listening_t  *ls;


    if (ngx_set_environment(cycle, NULL) == NULL) {
        /* fatal */
        exit(2);
    }

    ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module);

    // 设置worker进程的nice值，cachemanager cacheloader进程不会设置
    if (worker >= 0 && ccf->priority != 0) {
        if (setpriority(PRIO_PROCESS, 0, ccf->priority) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "setpriority(%d) failed", ccf->priority);
        }
    }

    // 设置进程最多可以打开的fd数量
    if (ccf->rlimit_nofile != NGX_CONF_UNSET) {
        rlmt.rlim_cur = (rlim_t) ccf->rlimit_nofile;
        rlmt.rlim_max = (rlim_t) ccf->rlimit_nofile;

        if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "setrlimit(RLIMIT_NOFILE, %i) failed",
                          ccf->rlimit_nofile);
        }
    }

    // 设置这个进程独立于系统的coredump属性
    if (ccf->rlimit_core != NGX_CONF_UNSET) {
        rlmt.rlim_cur = (rlim_t) ccf->rlimit_core;
        rlmt.rlim_max = (rlim_t) ccf->rlimit_core;

        if (setrlimit(RLIMIT_CORE, &rlmt) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "setrlimit(RLIMIT_CORE, %O) failed",
                          ccf->rlimit_core);
        }
    }

#ifdef RLIMIT_SIGPENDING
    if (ccf->rlimit_sigpending != NGX_CONF_UNSET) {
        rlmt.rlim_cur = (rlim_t) ccf->rlimit_sigpending;
        rlmt.rlim_max = (rlim_t) ccf->rlimit_sigpending;

        if (setrlimit(RLIMIT_SIGPENDING, &rlmt) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "setrlimit(RLIMIT_SIGPENDING, %i) failed",
                          ccf->rlimit_sigpending);
        }
    }
#endif

    // 如果设置了user指令,而且使用root权限启动会在这里改变进程所属的用户和组
    if (geteuid() == 0) {
        if (setgid(ccf->group) == -1) {
            ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                          "setgid(%d) failed", ccf->group);
            /* fatal */
            exit(2);
        }

        if (initgroups(ccf->username, ccf->group) == -1) {
            ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                          "initgroups(%s, %d) failed",
                          ccf->username, ccf->group);
        }

        if (setuid(ccf->user) == -1) {
            ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                          "setuid(%d) failed", ccf->user);
            /* fatal */
            exit(2);
        }
    }

    if (worker >= 0) {
        // 获取这个worker对应的CPU号
        cpu_affinity = ngx_get_cpu_affinity(worker);

        // 绑定这个worker对应的CPU。
        if (cpu_affinity) {
            ngx_setaffinity(cpu_affinity, cycle->log);
        }
    }

#if (NGX_HAVE_PR_SET_DUMPABLE)

    /* allow coredump after setuid() in Linux 2.4.x */

    if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "prctl(PR_SET_DUMPABLE) failed");
    }

#endif

    // 设置工作目录
    if (ccf->working_directory.len) {
        if (chdir((char *) ccf->working_directory.data) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "chdir(\"%s\") failed", ccf->working_directory.data);
            /* fatal */
            exit(2);
        }
    }

    sigemptyset(&set);

    // 把父进程设置为阻塞的信号重新设置为可接收状态。
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "sigprocmask() failed");
    }

    // 设置随机数种子。
    srandom((ngx_pid << 16) ^ ngx_time());

    /*
     * disable deleting previous events for the listening sockets because
     * in the worker processes there are no events at all at this point
     */
    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) {
        ls[i].previous = NULL;
    }

    // 调用每个模块的init_process函数
    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->init_process) {
            if (ngx_modules[i]->init_process(cycle) == NGX_ERROR) {
                /* fatal */
                exit(2);
            }
        }
    }

    // 关闭其他子进程与maser进程通信的unix套接字
    for (n = 0; n < ngx_last_process; n++) {

        if (ngx_processes[n].pid == -1) {
            continue;
        }

        if (n == ngx_process_slot) {
            continue;
        }

        if (ngx_processes[n].channel[1] == -1) {
            continue;
        }

        if (close(ngx_processes[n].channel[1]) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "close() channel failed");
        }
    }

    // 关闭unix套接字对的父进程端套接字
    if (close(ngx_processes[ngx_process_slot].channel[0]) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "close() channel failed");
    }

#if 0
    ngx_last_process = 0;
#endif

    // 将这个进程接收父进程消息套接字的接收事件放到事件模型中，
    // 并将回调函数设置为ngx_channel_handler
    if (ngx_add_channel_event(cycle, ngx_channel, NGX_READ_EVENT,
                              ngx_channel_handler)
        == NGX_ERROR)
    {
        /* fatal */
        exit(2);
    }
}


// worker进程退出时做的一些清理或关闭任务。
static void
ngx_worker_process_exit(ngx_cycle_t *cycle)
{
    ngx_uint_t         i;
    ngx_connection_t  *c;

#if (NGX_THREADS)
    ngx_terminate = 1;

    ngx_wakeup_worker_threads(cycle);
#endif

    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->exit_process) {
            ngx_modules[i]->exit_process(cycle);
        }
    }

    if (ngx_exiting) {
        c = cycle->connections;
        for (i = 0; i < cycle->connection_n; i++) {
            if (c[i].fd != -1
                && c[i].read
                && !c[i].read->accept
                && !c[i].read->channel
                && !c[i].read->resolver)
            {
                ngx_log_error(NGX_LOG_ALERT, cycle->log, 0,
                              "*%uA open socket #%d left in connection %ui",
                              c[i].number, c[i].fd, i);
                ngx_debug_quit = 1;
            }
        }

        if (ngx_debug_quit) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, 0, "aborting");
            ngx_debug_point();
        }
    }

    /*
     * Copy ngx_cycle->log related data to the special static exit cycle,
     * log, and log file structures enough to allow a signal handler to log.
     * The handler may be called when standard ngx_cycle->log allocated from
     * ngx_cycle->pool is already destroyed.
     */

    ngx_exit_log_file.fd = ngx_cycle->log->file->fd;

    ngx_exit_log = *ngx_cycle->log;
    ngx_exit_log.file = &ngx_exit_log_file;
    ngx_exit_log.next = NULL;

    ngx_exit_cycle.log = &ngx_exit_log;
    ngx_exit_cycle.files = ngx_cycle->files;
    ngx_exit_cycle.files_n = ngx_cycle->files_n;
    ngx_cycle = &ngx_exit_cycle;

    ngx_destroy_pool(cycle->pool);

    ngx_log_error(NGX_LOG_NOTICE, ngx_cycle->log, 0, "exit");

    exit(0);
}


// master进程的子进程通过unix套接字接收父进程消息时回调函数
// 子进程会在这个函数里接收到消息并执行消息对应的命令。
static void
ngx_channel_handler(ngx_event_t *ev)
{
    ngx_int_t          n;
    ngx_channel_t      ch;
    ngx_connection_t  *c;

    if (ev->timedout) {
        ev->timedout = 0;
        return;
    }

    c = ev->data;

    ngx_log_debug0(NGX_LOG_DEBUG_CORE, ev->log, 0, "channel handler");

    for ( ;; ) {

        n = ngx_read_channel(c->fd, &ch, sizeof(ngx_channel_t), ev->log);

        ngx_log_debug1(NGX_LOG_DEBUG_CORE, ev->log, 0, "channel: %i", n);

        if (n == NGX_ERROR) {

            if (ngx_event_flags & NGX_USE_EPOLL_EVENT) {
                ngx_del_conn(c, 0);
            }

            ngx_close_connection(c);
            return;
        }

        if (ngx_event_flags & NGX_USE_EVENTPORT_EVENT) {
            if (ngx_add_event(ev, NGX_READ_EVENT, 0) == NGX_ERROR) {
                return;
            }
        }

        if (n == NGX_AGAIN) {
            return;
        }

        ngx_log_debug1(NGX_LOG_DEBUG_CORE, ev->log, 0,
                       "channel command: %d", ch.command);

        switch (ch.command) {

        case NGX_CMD_QUIT:
            ngx_quit = 1;
            break;

        case NGX_CMD_TERMINATE:
            ngx_terminate = 1;
            break;

        case NGX_CMD_REOPEN:
            ngx_reopen = 1;
            break;

        case NGX_CMD_OPEN_CHANNEL:

            ngx_log_debug3(NGX_LOG_DEBUG_CORE, ev->log, 0,
                           "get channel s:%i pid:%P fd:%d",
                           ch.slot, ch.pid, ch.fd);

            ngx_processes[ch.slot].pid = ch.pid;
            ngx_processes[ch.slot].channel[0] = ch.fd;
            break;

        case NGX_CMD_CLOSE_CHANNEL:

            ngx_log_debug4(NGX_LOG_DEBUG_CORE, ev->log, 0,
                           "close channel s:%i pid:%P our:%P fd:%d",
                           ch.slot, ch.pid, ngx_processes[ch.slot].pid,
                           ngx_processes[ch.slot].channel[0]);

            if (close(ngx_processes[ch.slot].channel[0]) == -1) {
                ngx_log_error(NGX_LOG_ALERT, ev->log, ngx_errno,
                              "close() channel failed");
            }

            ngx_processes[ch.slot].channel[0] = -1;
            break;
        }
    }
}


#if (NGX_THREADS)

// 多线程才会用到
static void
ngx_wakeup_worker_threads(ngx_cycle_t *cycle)
{
    ngx_int_t   i;
    ngx_uint_t  live;

    for ( ;; ) {

        live = 0;

        for (i = 0; i < ngx_threads_n; i++) {
            if (ngx_threads[i].state < NGX_THREAD_EXIT) {
                if (ngx_cond_signal(ngx_threads[i].cv) == NGX_ERROR) {
                    ngx_threads[i].state = NGX_THREAD_DONE;

                } else {
                    live = 1;
                }
            }

            if (ngx_threads[i].state == NGX_THREAD_EXIT) {
                ngx_thread_join(ngx_threads[i].tid, NULL);
                ngx_threads[i].state = NGX_THREAD_DONE;
            }
        }

        if (live == 0) {
            ngx_log_debug0(NGX_LOG_DEBUG_CORE, cycle->log, 0,
                           "all worker threads are joined");

            /* STUB */
            ngx_done_events(cycle);
            ngx_mutex_destroy(ngx_event_timer_mutex);
            ngx_mutex_destroy(ngx_posted_events_mutex);

            return;
        }

        ngx_sched_yield();
    }
}


// 多线程才会用到
static ngx_thread_value_t
ngx_worker_thread_cycle(void *data)
{
    ngx_thread_t  *thr = data;

    sigset_t          set;
    ngx_err_t         err;
    ngx_core_tls_t   *tls;
    ngx_cycle_t      *cycle;

    cycle = (ngx_cycle_t *) ngx_cycle;

    sigemptyset(&set);
    sigaddset(&set, ngx_signal_value(NGX_RECONFIGURE_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_REOPEN_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_CHANGEBIN_SIGNAL));

    err = ngx_thread_sigmask(SIG_BLOCK, &set, NULL);
    if (err) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, err,
                      ngx_thread_sigmask_n " failed");
        return (ngx_thread_value_t) 1;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, cycle->log, 0,
                   "thread " NGX_TID_T_FMT " started", ngx_thread_self());

    ngx_setthrtitle("worker thread");

    tls = ngx_calloc(sizeof(ngx_core_tls_t), cycle->log);
    if (tls == NULL) {
        return (ngx_thread_value_t) 1;
    }

    err = ngx_thread_set_tls(ngx_core_tls_key, tls);
    if (err != 0) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, err,
                      ngx_thread_set_tls_n " failed");
        return (ngx_thread_value_t) 1;
    }

    ngx_mutex_lock(ngx_posted_events_mutex);

    for ( ;; ) {
        thr->state = NGX_THREAD_FREE;

        if (ngx_cond_wait(thr->cv, ngx_posted_events_mutex) == NGX_ERROR) {
            return (ngx_thread_value_t) 1;
        }

        if (ngx_terminate) {
            thr->state = NGX_THREAD_EXIT;

            ngx_mutex_unlock(ngx_posted_events_mutex);

            ngx_log_debug1(NGX_LOG_DEBUG_CORE, cycle->log, 0,
                           "thread " NGX_TID_T_FMT " is done",
                           ngx_thread_self());

            return (ngx_thread_value_t) 0;
        }

        thr->state = NGX_THREAD_BUSY;

        if (ngx_event_thread_process_posted(cycle) == NGX_ERROR) {
            return (ngx_thread_value_t) 1;
        }

        if (ngx_event_thread_process_posted(cycle) == NGX_ERROR) {
            return (ngx_thread_value_t) 1;
        }

        if (ngx_process_changes) {
            if (ngx_process_changes(cycle, 1) == NGX_ERROR) {
                return (ngx_thread_value_t) 1;
            }
        }
    }
}

#endif


// 启动后cachemanager进程在这个函数进入死循环。
// 启动后cacheloader进程在这个函数运行一次handler函数，在handler函数里会退出进程。
static void
ngx_cache_manager_process_cycle(ngx_cycle_t *cycle, void *data)
{
    ngx_cache_manager_ctx_t *ctx = data;

    void         *ident[4];
    ngx_event_t   ev;

    /*
     * Set correct process type since closing listening Unix domain socket
     * in a master process also removes the Unix domain socket file.
     */
    ngx_process = NGX_PROCESS_HELPER;

    ngx_close_listening_sockets(cycle);

    /* Set a moderate number of connections for a helper process. */
    cycle->connection_n = 512;

    ngx_worker_process_init(cycle, -1);

    ngx_memzero(&ev, sizeof(ngx_event_t));
    ev.handler = ctx->handler;
    ev.data = ident;
    ev.log = cycle->log;
    ident[3] = (void *) -1;

    ngx_use_accept_mutex = 0;

    ngx_setproctitle(ctx->name);

    ngx_add_timer(&ev, ctx->delay);

    for ( ;; ) {

        if (ngx_terminate || ngx_quit) {
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "exiting");
            exit(0);
        }

        if (ngx_reopen) {
            ngx_reopen = 0;
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "reopening logs");
            ngx_reopen_files(cycle, -1);
        }

        ngx_process_events_and_timers(cycle);
    }
}


// cache manager进程的执行函数，执行已经超时的cycle->paths对应的manager函数
static void
ngx_cache_manager_process_handler(ngx_event_t *ev)
{
    time_t        next, n;
    ngx_uint_t    i;
    ngx_path_t  **path;

    next = 60 * 60;

    path = ngx_cycle->paths.elts;
    for (i = 0; i < ngx_cycle->paths.nelts; i++) {

        if (path[i]->manager) {
            n = path[i]->manager(path[i]->data);

            next = (n <= next) ? n : next;

            ngx_time_update();
        }
    }

    if (next == 0) {
        next = 1;
    }

    ngx_add_timer(ev, next * 1000);
}


// cache loader进程的执行函数，执行各个cycle->paths对应的loader函数，然后退出进程。
static void
ngx_cache_loader_process_handler(ngx_event_t *ev)
{
    ngx_uint_t     i;
    ngx_path_t   **path;
    ngx_cycle_t   *cycle;

    cycle = (ngx_cycle_t *) ngx_cycle;

    path = cycle->paths.elts;
    for (i = 0; i < cycle->paths.nelts; i++) {

        if (ngx_terminate || ngx_quit) {
            break;
        }

        if (path[i]->loader) {
            path[i]->loader(path[i]->data);
            ngx_time_update();
        }
    }

    exit(0);
}
