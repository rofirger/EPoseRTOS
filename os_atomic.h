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

#include "os_def.h"
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
os_base_t os_atomic_compare_exchange_strong(volatile os_base_t *ptr, os_base_t *old, os_base_t desired);

#endif /* _OS_ATOMIC_H_ */
