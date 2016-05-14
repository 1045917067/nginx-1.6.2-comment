
### 简介

开始学习nginx的时候看了很多人分享的有价值的资料，回馈一份自己的到社区。

期望每个函数定义、全局变量定义和结构体声明都能有说明，但不会注释：

1. spdy相关代码,因为spdy已经被http2取代.
2. 线程相关代码,因为至少在1.6.2版本nginx的线程还处在实验性阶段.
3. linux之外其他平台的代码.
4. epoll之外其他事件模型的代码
5. gcc之外其他编译器的代码.
6. mail相关的代码.

### 网址

这篇注释的网址：https://github.com/langsim/nginx-1.6.2-comment

推荐的读写工具：https://github.com/langsim/vim-ide


### 完成程度

1. core目录    60%
2. event目录   60%
3. http目录    30%
4. os目录      80%


### 函数链表

1. http filter模块之间，ngx_http_top_header_filter和ngx_http_top_body_filter会形成两个函数链表, 链表中函数的调用顺序会和对应模块在ngx_modules数组的顺序相反。
2. http请求处理的11个阶段中很多阶段都可以有多个回调函数，同一个阶段的回调函数之间调用顺序和对应模块在ngx_modules数组里的顺序相同。　


### 日志

nginx默认有两个log文件:

1. access.log: 每一个http请求会在里面产生一行记录。
2. error.log: nginx运行时的一些常规，错误或调试日志。


### 进程模型

nginx启动后可能出现四种进程状态:

1. 单进程前台运行
2. 单进程守护进程模式
3. 一个master进程,一个(或多个)worker进程,一个(或0个)cachemanager进程和一个(或0个)cacheloader进程共同在前台运行
4. 一个master进程,一个(或多个)worker进程,一个(或0个)cachemanager进程和一个(或0个)cacheloader进程在守护进程状态下运行。
    * master进程在启动是用于启动其他进程，启动完成后用于管理其他进程。
    * woker进程，用于实际处理链接请求。
    * cachemanager进程用于按要求清理cache文件。
    * cacheloader进程在启动时会将cache目录里符合条件的cache文件在内存里建立索引，不符合要求的cache文件清除,然后退出。
5. 启动后给正在运行的nginx的主进程发信号，然后退出。


### 进程运行状态的切换

1. 重载配置文件（reload）
2. 热升级可执行文件
3. master进程子进程意外关闭，master进程检测到后将其重启


### 编译

nginx的编译与linux平台常规开源软件的编译方式相同，都是运行./configure && make && make install, 在./configure后可加入自己的自定义选项。


### nginx使用信号

1. TERM, INT 快速关闭
2. QUIT 从容关闭
3. HUP 平滑重启，重新加载配置文件
4. USR1 重新打开日志文件，在切割日志时用途较大
5. USR2 平滑升级可执行程序
6. WINCH 从容关闭工作进程


### nginx内存池

nginx有两个内存池

1. 在共享内存上分配内存用的内存池
2. 在普通内存上申请内存用的内存池,目的是高效的利用内存


### nginx使用到的hash算法

1. crc32
2. crc16
3. sha1
4. murmurhash
5. md5


### nignx进程通信与同步

1. 共享内存的使用
2. 信号
3. unix套接字
