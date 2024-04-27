/***********************
 * @file: os_atomic.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2024-04-27     Feijie Luo   First version
 * @note:
 ***********************/
#include "../../os_config.h"
#include "../../os_def.h"
#include "os_port_c.h"

__FORCE_INLINE__ os_base_t os_atomic_load(volatile os_base_t *ptr)
{
    os_base_t result = 0;
#ifdef CONFIG_USING_HW_ATOMIC
    // reg:x0 === 0
    asm volatile ("amoxor.w %0, x0, (%1)" : "=r"(result) : "r"(ptr) : "memory");
#else
    OS_ENTER_CRITICAL
    result = *ptr;
    OS_EXIT_CRITICAL
#endif
    return result;
}

/*
 * *ptr = val, ret = *ptr
 */
__FORCE_INLINE__ os_base_t os_atomic_exchange(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
#ifdef CONFIG_USING_HW_ATOMIC
    asm volatile ("amoswap.w %0, %1, (%2)" : "=r"(result) : "r"(val), "r"(ptr) : "memory");
#else
    OS_ENTER_CRITICAL
    result = *ptr;
    *ptr = val;
    OS_EXIT_CRITICAL
#endif
    return result;
}

__FORCE_INLINE__ os_base_t os_atomic_add(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
#ifdef CONFIG_USING_HW_ATOMIC
    asm volatile ("amoadd.w %0, %1, (%2)" : "=r"(result) : "r"(val), "r"(ptr) : "memory");
#else
    OS_ENTER_CRITICAL
    result = *ptr;
    *ptr += val;
    OS_EXIT_CRITICAL
#endif
    return result;
}

os_base_t os_atomic_sub(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
#ifdef CONFIG_USING_HW_ATOMIC
    val = -val;
    asm volatile ("amoadd.w %0, %1, (%2)" : "=r"(result) : "r"(val), "r"(ptr) : "memory");
#else
    OS_ENTER_CRITICAL
    result = *ptr;
    *ptr -= val;
    OS_EXIT_CRITICAL
#endif
    return result;
}

__FORCE_INLINE__ os_base_t os_atomic_xor(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
#ifdef CONFIG_USING_HW_ATOMIC
    asm volatile ("amoxor.w %0, %1, (%2)" : "=r"(result) : "r"(val), "r"(ptr) : "memory");
#else
    OS_ENTER_CRITICAL
    result = *ptr;
    *ptr = (*ptr) ^ val;
    OS_EXIT_CRITICAL
#endif
    return result;
}

__FORCE_INLINE__ os_base_t os_atomic_and(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
#ifdef CONFIG_USING_HW_ATOMIC
    asm volatile ("amoand.w %0, %1, (%2)" : "=r"(result) : "r"(val), "r"(ptr) : "memory");
#else
    OS_ENTER_CRITICAL
    result = *ptr;
    *ptr = (*ptr) & val;
    OS_EXIT_CRITICAL
#endif
    return result;
}

__FORCE_INLINE__ os_base_t os_atomic_or(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
#ifdef CONFIG_USING_HW_ATOMIC
    asm volatile ("amoor.w %0, %1, (%2)" : "=r"(result) : "r"(val), "r"(ptr) : "memory");
#else
    OS_ENTER_CRITICAL
    result = *ptr;
    *ptr = (*ptr) | val;
    OS_EXIT_CRITICAL
#endif
    return result;
}

__FORCE_INLINE__ void os_atomic_store(volatile os_base_t *ptr, os_base_t val)
{
#ifdef CONFIG_USING_HW_ATOMIC
    os_base_t result = 0;
    asm volatile ("amoswap.w %0, %1, (%2)" : "=r"(result) : "r"(val), "r"(ptr) : "memory");
#else
    OS_ENTER_CRITICAL
    *ptr = val;
    OS_EXIT_CRITICAL
#endif
}

os_base_t os_atomic_flag_test_and_set(volatile os_base_t *ptr)
{
    os_base_t result = 0;
#ifdef CONFIG_USING_HW_ATOMIC
    os_base_t temp = 1;
    asm volatile ("amoor.w %0, %1, (%2)" : "=r"(result) : "r"(temp), "r"(ptr) : "memory");
#else
    OS_ENTER_CRITICAL
    if (*ptr == 0) {
        result = *ptr;
        *ptr = 1;
    }
    else
        result = *ptr;
    OS_EXIT_CRITICAL
#endif
    return result;
}

__FORCE_INLINE__ void os_atomic_flag_clear(volatile os_base_t *ptr)
{
#ifdef CONFIG_USING_HW_ATOMIC
    os_base_t result = 0;
    asm volatile ("amoand.w %0, x0, (%1)" : "=r"(result) :"r"(ptr) : "memory");
#else
    OS_ENTER_CRITICAL
    *ptr = 0;
    OS_EXIT_CRITICAL
#endif
}

os_base_t os_atomic_compare_exchange_strong(volatile os_base_t *ptr, os_base_t *old, os_base_t desired)
{
    os_base_t result = 0;
#ifdef CONFIG_USING_HW_ATOMIC
    os_base_t tmp = *old;
    asm volatile(
            " fence iorw, ow\n"
            "1: lr.w.aq  %[result], (%[ptr])\n"
            "   bne      %[result], %[tmp], 2f\n"
            "   sc.w.rl  %[tmp], %[desired], (%[ptr])\n"
            "   bnez     %[tmp], 1b\n"
            "   li  %[result], 1\n"
            "   j 3f\n"
            " 2:sw  %[result], (%[old])\n"
            "   li  %[result], 0\n"
            " 3:\n"
            : [result]"+r" (result), [tmp]"+r" (tmp), [ptr]"+r" (ptr)
            : [desired]"r" (desired), [old]"r"(old)
            : "memory");
#else
    OS_ENTER_CRITICAL
    if ((*ptr) != (*old))
    {
        *old = *ptr;
        result = 0;
    }
    else
    {
        *ptr = desired;
        result = 1;
    }
    OS_EXIT_CRITICAL
#endif
    return result;
}
