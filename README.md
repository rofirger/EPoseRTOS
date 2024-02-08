# Toy RTOS

## 简介

### 概述

Toy-RTOS非常简易，目前有效代码(C , asm)行数为三千余行，支持ARM Cortex-M系列与RISC-V MCU。由于水平有限以及编写时间之短（前后一共花费6周），Toy-RTOS目前不可避免存在一些稚嫩的设计以及BUG。Toy-RTOS目前仍存在标准库的依赖，如`strcmp`/`strncmp`/`snprintf`/`memset`/`memcpy`.

Toy-RTOS目前具备RTOS内核所具备的基本功能：线程管理、内存管理、时钟管理、线程间同步、线程间通信、中断管理。此外还具备一层简易的Linux风格的设备抽象层。暂不支持SMP。本着“高内聚，低耦合”的设计原则，Toy-RTOS的各个功能模块之间的依赖性小，有着较高的可维护性、可测试性和可重用性，易于理解、修改和拓展。

## 线程管理

有别于 Free-RTOS ，Toy-RTOS数值越小的优先级系数代表越高的优先级。优先级系数范围可配置，最大支持256个优先级，0为最高优先级。在Toy-RTOS中，内核的IDLE线程被赋予最低优先级。Toy-RTOS支持同一等优先级包含多个线程。调度策略是：同等优先级的线程之间采用非抢占式时间片轮转调度策略，不同等优先级的线程之间采用抢占式调度策略。调度器的时间复杂度为O(1).RISC-V内核在软件中断SWI中进行上下文切换，ARM Cortex-M系列内核在异常PendSV中进行上下文切换。在Toy-RTOS内核中，用于进行上下文切换的异常的优先级被设置为最低。对于线程栈FPU部分，目前Toy-RTOS在RISC-V内核下如果开启FPU则每次进行上下文切换无论FPU相关的DIRTY位有没有被置位，即，不判断Mstatus.FS域，直接全保存FPU部分的寄存器。而在ARM Cortex-M3/4/7内核下，如果开启FPU，不是SoftVFP且!EXC_RETURN[4]才会保存FPU部分的寄存器。对于线程状态，Toy-RTOS目前只有5种线程状态，分别是：NEW(新的)，READY(就绪)，RUNNING(正在运行)，BLOCKING(正在被阻塞)，TERMINATED(终止的)。状态划分标准与RT-Thread基本相同，而Free-RTOS则是把阻塞态细分为了挂起态和延时态。虽然Toy-RTOS将延时态和挂起态融合为阻塞态，但是内核依旧有所细分，只不过将细分任务交给了TICK，即在内核函数`os_add_tick_task`中根据参数tick的数值来判断是延时态，还是挂起态，如果是延时态，则内核会将线程，既挂载到延时队列，也会挂载到挂起队列。如果延时结束并且线程依旧是挂起态，那么线程的BLOCKING状态将被置为TIME_OUT。当正在执行的线程被阻塞，内核就会立刻将就绪队列中优先级最高的线程进行恢复上下文。

## 内存管理

目前，Toy-RTOS只具备小内存管理算法。当然，由于其具备“高内聚，低耦合”的特点，Toy-RTOS易于添加其他效率更高、实时性更强的内存管理算法。初始时，它是一块大的内存。当需要分配内存块时，将从这个大的内存块上分割出大小相匹配的内存块，然后把分割出来的空闲内存块还回给内存管理系统中。每个内存块都包含一个管理用的数据头，通过这个头把使用块与空闲块用双向链表的方式链接起来。Toy-RTOS调用os_malloc均是从这块大内存中进行内存分配。当然，Toy-RTOS也支持分配用户给定的一块大内存。但是要求用户给定的这块内存的起始地址必须是4字节对齐。在本次课程设计中，当音频、视频和图片等需要较大块连续内存时，均是使用用户自定义的内存块来进行分配。因为Toy-RTOS在运行的过程中会不断调用os_malloc/os_free进行内核小对象分配/释放，其产生的外部内存碎片容易导致本就内存紧张的系统难以分配一块大且连续的内存块。

## 时钟管理

需要一个稳定且触发时间间隔相等（一般是1ms）的定时器来作为Toy-RTOS的心跳信号。目前，Toy-RTOS还没有定时激活一个任务的功能以及其他软定时器功能。

## 线程间同步

Toy-RTOS目前仅具备信号量和互斥量。计划添加事件集。这两种同步方式均具备限时/不限时(有没有等待时间)挂起处理功能，均继承自TICK。如果是限时处理并且已经超时，那么线程的BLOCKING状态将被置为TIME_OUT。信号量有优先级反转的风险，而互斥量在内核层面上避免了优先级反转现象的发生。Toy-RTOS的互斥量目前有两种类型，非递归互斥量和递归互斥量。

## 线程间通信

Toy-RTOS目前仅具备消息队列(mqueue)这一种线程间通信方式。消息队列的限时/不限时(有没有等待时间)挂起处理功能也继承自TICK。如果是限时处理并且已经超时，那么线程的BLOCKING状态将被置为TIME_OUT。

关于阻塞队列的唤醒：为了保证内核调度的实时性，任何阻塞队列出队顺序都是高优先级的线程先出队，不支持FIFO（FIFO属于非实时调度）。因为内核所有具备阻塞功能的类(mutex, semaphore, mqueue, delay…)都继承自同一个内置的BLOCK类(结构体)，这个内置的BLOCK类(结构体)内队列的出队顺序都是高优先级的线程先出队。

## 中断管理

Toy-RTOS提供在中断中切换栈（双堆栈。为什么这里要切换栈？是为了将中断栈与用户栈区分开，不给用户栈带来不确定性，因为中断具有不确定性。）以及切换MCU工作模式（目前在Toy-RTOS下的系统代码和用户代码没有明显界限，没有如RTX5通过SVC将内核和应用隔离开，需要用户代码和系统代码均运行在具备最高权限的工作模式下）的接口，提供全局中断开关，提供调度器开关，提供记录中断嵌套层数功能。暂无其他中断管理功能。

## 其他功能

比如控制台，类似与RT-Thread的FinSH控制台，支持命令注册。命令注册的策略是：编程人员通过一个宏能将一个命令字符串与一个函数绑定。例如，一行代码`OS_CMD_EXPORT(ps, ps_fn)`，就能够将“ps”这个命令与“`ps_fn`”这个函数绑定起来，当用户在控制台输入“`ps`”并且内核的FISH线程能够获取MCU执行权，那么，“`ps_fn`”就会被立刻执行。也就说与命令绑定的函数均是在内核的FISH线程上执行。在Toy-RTOS中，FISH线程被默认赋予的优先级系数为2。



