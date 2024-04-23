/***********************
 * @file: os_core.h
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2022-09-10     Feijie Luo   First version
 * 2023-10-11     Feijie Luo   Add sp pointer
 * 2023-10-13     Feijie Luo   Add OS_TASK_SLEEP_TIME_OUT.
 * @note:
 ***********************/

#ifndef _OS_CORE_H_
#define _OS_CORE_H_

#include "os_config.h"
#include "os_def.h"
#include "os_list.h"

typedef void (*__task_fn_)(void *_arg);

typedef unsigned int os_task_stack_t;
typedef unsigned char os_task_priority_t;

enum os_task_state {
    OS_TASK_NEW = 1,
    OS_TASK_READY = 2,
    OS_TASK_RUNNING = 3,
    OS_TASK_TERMINATED = 4,
    OS_TASK_BLOCKING = 5
};

enum os_task_block_state {
    OS_TASK_BLOCK_NONE = 0,
    OS_TASK_BLOCK_TICKING = 1,
    OS_TASK_BLOCK_TIMEOUT = 2,
    OS_TASK_BLOCK_EARLY_WAKEUP = 3,
};

typedef struct task_control_block {
    // pointer of task stack top
    os_task_stack_t *_stack_top;
    // sp register
    void *sp;
    // task priority
    os_task_priority_t _task_priority;

    // task state
    enum os_task_state _task_state;
    // block state
    enum os_task_block_state _task_block_state;
    const char *_task_name;
    unsigned int _task_id;
    unsigned int _task_timeslice;

    struct os_block_object *_block_mount;

    // mount to the TICK with BLOCK
    struct list_head _bt_nd;
    // mount to the BLOCK
    struct list_head _slot_nd;
} tcb_t;

os_handle_state_t os_task_create(struct task_control_block *_task_tcb,
                                 unsigned int *_stack_addr,
                                 unsigned int _stack_size,
                                 unsigned char _prio,
                                 __task_fn_ _entry_fn,
                                 void *_entry_fn_arg,
                                 const char *_task_name);

bool os_task_state_is_new(struct task_control_block *_task_tcb);
void os_task_state_set_new(struct task_control_block *_task_tcb);
bool os_task_state_is_ready(struct task_control_block *_task_tcb);
void os_task_state_set_ready(struct task_control_block *_task_tcb);
bool os_task_state_is_running(struct task_control_block *_task_tcb);
void os_task_state_set_running(struct task_control_block *_task_tcb);
bool os_task_state_is_stopped(struct task_control_block *_task_tcb);
void os_task_state_set_stopped(struct task_control_block *_task_tcb);
bool os_task_state_is_blocking(struct task_control_block *_task_tcb);
void os_task_state_set_blocking(struct task_control_block *_task_tcb);

#endif
