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

// enable FPU
#define CONFIG_ARCH_FPU
// SysTick frequency
#define CONFIG_SYSTICK_CLOCK_FREQUENCY (144000000UL)
// enable friendly-interface-shell
#define CONFIG_FISH
// reserve command description
#define CONFIG_FISH_CMD_DESC
// enable os_printk
#define CONFIG_PRINTK
// enable debug
#define CONFIG_OS_DEBUG
// the size of system heap, unit: byte
#define CONFIG_HEAP_SIZE (30720)
// the size of buffer in os_printk, unit: byte
#define CONFIG_OS_PRINTK_BUF_SIZE  (512)

#ifdef CONFIG_FISH
// the priority of FISH thread
#define OS_KTHREAD_FISH_PRIO (2)
#endif

// MAX priority
#define OS_TASK_MAX_PRIORITY (32)
// MAX ID
#define OS_TASK_MAX_ID (64)
// time slice, unit: ms
#define OS_TIMESLICE_STD (10)

#define OS_READY_LIST_SIZE  ((OS_TASK_MAX_PRIORITY) + 1)
#define OS_TASK_MAX_ID_SIZE ((OS_TASK_MAX_ID) + 1)

#if (OS_TASK_MAX_PRIORITY > 255 || OS_TASK_MAX_PRIORITY < 0)
#error "OS_TASK_MAX_PRIORITY should be maintained between 0 and 255."
#endif

#endif
