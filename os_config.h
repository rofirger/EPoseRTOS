/***********************
 * @file: os_cofig.h
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2022-09-10     Feijie Luo   First version
 * 2023-11-05     Feijie Luo   Add  optimize attribute
 * @note:
 ***********************/

#ifndef _OS_CONFIG_H_
#define _OS_CONFIG_H_

// 开启FPU
#define CONFIG_ARCH_FPU
// 提供给RTOS的滴答时钟频率, 24MHz
#define CONFIG_SYSTICK_CLOCK_FREQUENCY (24000000UL)
// 开启终端
#define CONFIG_FISH
// 开启命令描述
#define CONFIG_FISH_CMD_DESC
// 开启os_printk
#define CONFIG_PRINTK
// 开启调试
#define CONFIG_OS_DEBUG
// 堆大小
#define CONFIG_HEAP_SIZE (5120)

#ifdef CONFIG_FISH
// 内核终端线程的优先级
#define OS_KTHREAD_FISH_PRIO (2)
#endif

// 可支持的最高优先级
#define OS_TASK_MAX_PRIORITY (32)
// 可支持的最大id值
#define OS_TASK_MAX_ID (64)
// 时间片
#define OS_TIMESLICE_STD (10)

#define OS_READY_LIST_SIZE  ((OS_TASK_MAX_PRIORITY) + 1)
#define OS_TASK_MAX_ID_SIZE ((OS_TASK_MAX_ID) + 1)

#if (OS_TASK_MAX_PRIORITY > 255 || OS_TASK_MAX_PRIORITY < 0)
#error "OS_TASK_MAX_PRIORITY should be maintained between 0 and 255."
#endif

#endif
