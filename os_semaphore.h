/***********************
 * @file: os_semaphore.h
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2023-10-19     Feijie Luo   First version
 * @note:
 ***********************/

#ifndef _OS_SEMAPHORE_H_
#define _OS_SEMAPHORE_H_

#include "os_block.h"

#define OS_SEM_NO_WAIT          (0)
#define OS_SEM_NEVER_TIMEOUT    (OS_NEVER_TIME_OUT)

struct os_sem {
    unsigned short _value;
    struct os_block_object _block_obj;
};

os_handle_state_t os_sem_init(struct os_sem* sem, unsigned short value);
os_handle_state_t os_sem_try_take(struct os_sem* sem);
os_handle_state_t os_sem_take(struct os_sem* sem, unsigned int time_out);
os_handle_state_t os_sem_release(struct os_sem* sem);

#endif
