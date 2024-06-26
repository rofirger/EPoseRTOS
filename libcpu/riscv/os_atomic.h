/***********************
 * @file: os_atomic.h
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2024-04-27     Feijie Luo   First version
 * @note:
 ***********************/

#ifndef _OS_ATOMIC_H_
#define _OS_ATOMIC_H_

#include "../../os_def.h"

#ifdef CONFIG_USING_HW_ATOMIC
__FORCE_INLINE__ os_private os_base_t os_atomic_load(volatile os_base_t *ptr)
{
    os_base_t result = 0;
    // reg:x0 === 0
    asm volatile ("amoxor.w %0, x0, (%1)" : "=r"(result) : "r"(ptr) : "memory");
    return result;
}

__FORCE_INLINE__ os_private os_base_t os_atomic_exchange(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
    asm volatile ("amoswap.w %0, %1, (%2)" : "=r"(result) : "r"(val), "r"(ptr) : "memory");
    return result;
}

__FORCE_INLINE__ os_private os_base_t os_atomic_add(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
    asm volatile ("amoadd.w %0, %1, (%2)" : "=r"(result) : "r"(val), "r"(ptr) : "memory");
    return result;
}

__FORCE_INLINE__ os_private os_base_t os_atomic_sub(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
    val = -val;
    asm volatile ("amoadd.w %0, %1, (%2)" : "=r"(result) : "r"(val), "r"(ptr) : "memory");
    return result;
}

__FORCE_INLINE__ os_private os_base_t os_atomic_xor(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
    asm volatile ("amoxor.w %0, %1, (%2)" : "=r"(result) : "r"(val), "r"(ptr) : "memory");
    return result;
}

__FORCE_INLINE__ os_private os_base_t os_atomic_and(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
    asm volatile ("amoand.w %0, %1, (%2)" : "=r"(result) : "r"(val), "r"(ptr) : "memory");
    return result;
}

__FORCE_INLINE__ os_private os_base_t os_atomic_or(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
    asm volatile ("amoor.w %0, %1, (%2)" : "=r"(result) : "r"(val), "r"(ptr) : "memory");
    return result;
}

__FORCE_INLINE__ os_private void os_atomic_store(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
    asm volatile ("amoswap.w %0, %1, (%2)" : "=r"(result) : "r"(val), "r"(ptr) : "memory");
}

__FORCE_INLINE__ os_private os_base_t os_atomic_flag_test_and_set(volatile os_base_t *ptr)
{
    os_base_t result = 0;
    os_base_t temp = 1;
    asm volatile ("amoor.w %0, %1, (%2)" : "=r"(result) : "r"(temp), "r"(ptr) : "memory");
    return result;
}

__FORCE_INLINE__ os_private void os_atomic_flag_clear(volatile os_base_t *ptr)
{
    os_base_t result = 0;
    asm volatile ("amoand.w %0, x0, (%1)" : "=r"(result) :"r"(ptr) : "memory");
}

__FORCE_INLINE__ os_private os_base_t os_atomic_compare_exchange_strong(volatile os_base_t *ptr,
                                                                        os_base_t *old,
                                                                        volatile os_base_t desired)
{
    volatile os_base_t result = 0;
    volatile os_base_t tmp = *old;
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
    return result;
}

__FORCE_INLINE__ os_private os_base_t os_atomic_bge_set_strong(volatile os_base_t *ptr,
                                                               volatile os_base_t limit,
                                                               volatile os_base_t desired)
{
    volatile os_base_t result = 0;
    asm volatile(
            " fence iorw, ow\n"
            "1: lr.w.aq  %[result], (%[ptr])\n"
            "   blt      %[result], %[limit], 2f\n"
            "   sc.w.rl  %[result], %[desired], (%[ptr])\n"
            "   bnez     %[result], 1b\n"
            "   j 3f\n"
            " 2: li  %[result], 1\n"
            " 3:\n"
            : [result]"+r" (result),  [ptr]"+r" (ptr)
            : [desired]"r" (desired), [limit]"r" (limit)
            : "memory");
    return result;
}

__FORCE_INLINE__ os_private os_base_t os_atomic_add_bge_set_strong(volatile os_base_t *ptr,
                                                                   volatile os_base_t add_num,
                                                                   volatile os_base_t limit,
                                                                   volatile os_base_t desired)
{
    volatile os_base_t result = 0;
    volatile os_base_t tmp = 0;
    volatile os_base_t flag = 0;
    asm volatile(
            " fence iorw, ow\n"
            "1: lr.w.aq  %[result], (%[ptr])\n"
            "   add %[tmp], %[result], %[add_num]\n"
            "   blt      %[tmp], %[limit], 2f\n"
            "   sc.w.rl  %[flag], %[desired], (%[ptr])\n"
            "   bnez     %[flag], 1b\n"
            "   j 3f\n"
            "2: sc.w.rl  %[flag], %[tmp], (%[ptr])\n"
            "   bnez     %[flag], 1b\n"
            "3:\n"
            : [result]"+r" (result),  [ptr]"+r" (ptr), [flag]"+r" (flag), [tmp]"+r" (tmp)
            : [desired]"r" (desired), [limit]"r" (limit), [add_num]"r" (add_num)
            : "memory");
    return result;
}
#else
os_base_t os_atomic_load(volatile os_base_t *ptr);
void os_atomic_store(volatile os_base_t *ptr, os_base_t val);
os_base_t os_atomic_exchange(volatile os_base_t *ptr, os_base_t val);
os_base_t os_atomic_add(volatile os_base_t *ptr, os_base_t val);
os_base_t os_atomic_sub(volatile os_base_t *ptr, os_base_t val);
os_base_t os_atomic_xor(volatile os_base_t *ptr, os_base_t val);
os_base_t os_atomic_and(volatile os_base_t *ptr, os_base_t val);
os_base_t os_atomic_or(volatile os_base_t *ptr, os_base_t val);
os_base_t os_atomic_flag_test_and_set(volatile os_base_t *ptr);
void os_atomic_flag_clear(volatile os_base_t *ptr);
os_base_t os_atomic_compare_exchange_strong(volatile os_base_t *ptr,
                                            os_base_t *old,
                                            volatile os_base_t desired);
os_base_t os_atomic_bge_set_strong(volatile os_base_t *ptr,
                                   volatile os_base_t limit,
                                   volatile os_base_t desired);
os_base_t os_atomic_add_bge_set_strong(volatile os_base_t *ptr,
                                       volatile os_base_t add_num,
                                       volatile os_base_t limit,
                                       volatile os_base_t desired);
#endif
#endif /* _OS_ATOMIC_H_ */
