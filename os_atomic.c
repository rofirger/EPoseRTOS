/***********************
 * @file: os_atomic.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2024-04-27     Feijie Luo   First version
 * @note:
 ***********************/
#include "os_headfile.h"

#ifndef CONFIG_USING_HW_ATOMIC
os_base_t os_atomic_load(volatile os_base_t *ptr)
{
    os_base_t result = 0;
    OS_ENTER_CRITICAL
    result = *ptr;
    OS_EXIT_CRITICAL
    return result;
}

/*
 * *ptr = val, ret = *ptr
 */
os_base_t os_atomic_exchange(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
    OS_ENTER_CRITICAL
    result = *ptr;
    *ptr = val;
    OS_EXIT_CRITICAL
    return result;
}

os_base_t os_atomic_add(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
    OS_ENTER_CRITICAL
    result = *ptr;
    *ptr += val;
    OS_EXIT_CRITICAL
    return result;
}

os_base_t os_atomic_sub(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
    OS_ENTER_CRITICAL
    result = *ptr;
    *ptr -= val;
    OS_EXIT_CRITICAL
    return result;
}

os_base_t os_atomic_xor(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
    OS_ENTER_CRITICAL
    result = *ptr;
    *ptr = (*ptr) ^ val;
    OS_EXIT_CRITICAL
    return result;
}

os_base_t os_atomic_and(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
    OS_ENTER_CRITICAL
    result = *ptr;
    *ptr = (*ptr) & val;
    OS_EXIT_CRITICAL
    return result;
}

os_base_t os_atomic_or(volatile os_base_t *ptr, os_base_t val)
{
    os_base_t result = 0;
    OS_ENTER_CRITICAL
    result = *ptr;
    *ptr = (*ptr) | val;
    OS_EXIT_CRITICAL
    return result;
}

void os_atomic_store(volatile os_base_t *ptr, os_base_t val)
{
    OS_ENTER_CRITICAL
    *ptr = val;
    OS_EXIT_CRITICAL
}

os_base_t os_atomic_flag_test_and_set(volatile os_base_t *ptr)
{
    os_base_t result = 0;
    OS_ENTER_CRITICAL
    if (*ptr == 0) {
        result = *ptr;
        *ptr = 1;
    }
    else
        result = *ptr;
    OS_EXIT_CRITICAL
    return result;
}

void os_atomic_flag_clear(volatile os_base_t *ptr)
{
    OS_ENTER_CRITICAL
    *ptr = 0;
    OS_EXIT_CRITICAL
}

os_base_t os_atomic_compare_exchange_strong(volatile os_base_t *ptr,
                                            os_base_t *old,
                                            volatile os_base_t desired)
{
    volatile os_base_t result = 0;
    OS_ENTER_CRITICAL
    if ((*ptr) != (*old)){
        *old = *ptr;
        result = 0;
    }else{
        *ptr = desired;
        result = 1;
    }
    OS_EXIT_CRITICAL
    return result;
}

os_base_t os_atomic_bge_set_strong(volatile os_base_t *ptr,
                                   volatile os_base_t limit,
                                   volatile os_base_t desired)
{
    volatile os_base_t result = 0;
    OS_ENTER_CRITICAL
    if ((*ptr) >= limit) {
        *ptr = desired;
        result = 0;
    } else {
        result = 1;
    }
    OS_EXIT_CRITICAL
    return result;
}

os_base_t os_atomic_add_bge_set_strong(volatile os_base_t *ptr,
                                       volatile os_base_t add_num,
                                       volatile os_base_t limit,
                                       volatile os_base_t desired)
{
    OS_ENTER_CRITICAL
    os_base_t result = *ptr;
    os_base_t tmp = result;
    tmp += add_num;
    if (tmp >= limit) {
        *ptr = desired;
    } else {
        *ptr = tmp;
    }
    OS_EXIT_CRITICAL
    return result;
}
#endif


