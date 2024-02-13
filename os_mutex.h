/***********************
 * @file: os_mutex.h
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2022-09-10     Feijie Luo   First version
 * @note:
 ***********************/

#ifndef _OS_MUTEX_H_
#define _OS_MUTEX_H_

#include "os_config.h"
#include "os_core.h"

#define OS_MUTEX_MAX_RECURSIVE 0xFF
#define OS_MUTEX_PRIO_LOWEST   OS_TASK_MAX_PRIORITY

typedef enum os_mutex_type {
    OS_MUTEX_NO_RECURSIVE = 0,
    OS_MUTEX_RECURSIVE = 1
} os_mutex_type_t;

typedef enum os_mutex_handle_state {
    OS_MUTEX_HANDLE_LOCK_FAIL = 0,
    OS_MUTEX_HANDLE_LOCK_SUECCESS = 1,
    OS_MUTEX_HANDLE_GET_OWNER = 2,
    OS_MUTEX_HANDLE_RECURSIVE_FAIL = 3,
    OS_MUTEX_HANDLE_RECURSIVE_SUECCESS = 4,
    OS_MUTEX_HANDLE_OTHER_OWNER = 5,
    OS_MUTEX_HANDLE_INIT_FAIL = 6,
    OS_MUTEX_HANDLE_INIT_SUCCESS = 7,
    OS_MUTEX_HANDLE_UNLOCK_FAIL = 8,
    OS_MUTEX_HANDLE_UNLOCK_SUCCESS = 9,
    OS_MUTEX_HANDLE_DESTORY_FAIL = 10,
    OS_MUTEX_HANDLE_DESTORY_SUCCESS = 11
} os_mutex_handle_state_t;

typedef struct os_mutex {
    struct os_block_object _block_obj;
    struct task_control_block *_mutex_owner;
    unsigned int _owner_prio;
    unsigned int _lock_nesting;
    os_mutex_type_t _mutex_type;
} os_mutex_t;

bool os_mutex_is_recursive(struct os_mutex *_mutex);
bool os_mutex_block_is_empty(struct os_mutex *_mutex);
bool os_mutex_is_owned(struct os_mutex *_mutex);
bool os_mutex_is_self(struct os_mutex *_mutex);
os_mutex_handle_state_t os_mutex_try_lock(struct os_mutex *_mutex);
os_handle_state_t os_mutex_lock(struct os_mutex *_mutex, unsigned int time_out);
os_handle_state_t os_mutex_unlock(struct os_mutex *_mutex);
os_mutex_handle_state_t os_mutex_init(struct os_mutex *_mutex, os_mutex_type_t _type);
os_mutex_handle_state_t os_mutex_destory(struct os_mutex *_mutex);

#endif
