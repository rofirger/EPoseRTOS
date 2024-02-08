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

#define CONFIG_ARCH_FPU
#define CONFIG_CLOCK_FREQUENCY  144000000
// 开启终端
#define CONFIG_FISH
// 开启os_printk
#define CONFIG_PRINTK

#ifdef CONFIG_FISH
// 内核终端线程的优先级
#define OS_KTHREAD_FISH_PRIO (2)
#endif

// 可支持的最高优先级
#define OS_TASK_MAX_PRIORITY	(32)
// 可支持的最大id值
#define OS_TASK_MAX_ID			(64)
// 时间片
#define OS_TIMESLICE_STD		(10)

#define OS_READY_LIST_SIZE		((OS_TASK_MAX_PRIORITY) + 1)
#define OS_TASK_MAX_ID_SIZE		((OS_TASK_MAX_ID) + 1)

#if (OS_TASK_MAX_PRIORITY > 255 || OS_TASK_MAX_PRIORITY < 0)
	#error "OS_TASK_MAX_PRIORITY should be maintained between 0 and 255."
#endif

#if defined (__GNUC__)
#define __OS_OPTIMIZE_O0__   __attribute__((optimize("O0")))
#define __OS_OPTIMIZE_O1__   __attribute__((optimize("O1")))
#define __OS_OPTIMIZE_O2__   __attribute__((optimize("O2")))
#define __OS_OPTIMIZE_O3__   __attribute__((optimize("O3")))
#define __OS_OPTIMIZE_Os__   __attribute__((optimize("Os")))
#else
#define __OS_OPTIMIZE_O0__
#define __OS_OPTIMIZE_O1__
#define __OS_OPTIMIZE_O2__
#define __OS_OPTIMIZE_O3__
#define __OS_OPTIMIZE_Os__
#endif

#ifndef bool
	#define bool unsigned char
#endif

#ifndef NULL
	#define NULL ((void*)(0))
#endif

#ifndef true
	#define true 1
#endif
	
#ifndef false
	#define false 0
#endif
	
#define __os_inline inline
#define __os_static	static

typedef int           os_ssize_t;
typedef long          os_off_t;
typedef unsigned long os_size_t;

typedef enum os_handle_state
{
	OS_HANDLE_FAIL = 0,
	OS_HANDLE_SUCCESS = 1
}os_handle_state_t;

#define OS_NEVER_TIME_OUT  (0xFFFFFFFF)

#endif
