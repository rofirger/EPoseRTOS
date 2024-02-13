/***********************
 * @file: os_def.h
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2024-02-08     Feijie Luo   First version
 * @note:
 ***********************/
#ifndef _OS_DEF_H_
#define _OS_DEF_H_
#include "os_config.h"
#if defined(__GNUC__)
#define __OS_OPTIMIZE_O0__ __attribute__((optimize("O0")))
#define __OS_OPTIMIZE_O1__ __attribute__((optimize("O1")))
#define __OS_OPTIMIZE_O2__ __attribute__((optimize("O2")))
#define __OS_OPTIMIZE_O3__ __attribute__((optimize("O3")))
#define __OS_OPTIMIZE_Os__ __attribute__((optimize("Os")))
#else
#define __OS_OPTIMIZE_O0__
#define __OS_OPTIMIZE_O1__
#define __OS_OPTIMIZE_O2__
#define __OS_OPTIMIZE_O3__
#define __OS_OPTIMIZE_Os__
#endif

#ifndef bool
#define bool _Bool
#endif

#ifndef NULL
#define NULL ((void *)(0))
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#define __os_inline inline
#define __os_static static

#define OS_NEVER_TIME_OUT (0xFFFFFFFF)

typedef int os_ssize_t;
typedef long os_off_t;
typedef unsigned long os_size_t;

typedef enum os_handle_state {
    OS_HANDLE_FAIL = 0,
    OS_HANDLE_SUCCESS = 1
} os_handle_state_t;

#ifdef CONFIG_OS_DEBUG
void os_printk_flush();
void os_printk(const char *fmt, ...);
#define OS_ASSERT(expr)                                                                      \
    if (!(expr))                                                                             \
        do {                                                                                 \
            os_printk("Assertion failed:%s, file %s, line %d\n", #expr, __FILE__, __LINE__); \
            while(1);                                                                        \
    } while (0)
#else
#define ASSERT_VOID_CAST (void)
#define OS_ASSERT(expr)  ASSERT_VOID_CAST(0)
#endif

#endif
