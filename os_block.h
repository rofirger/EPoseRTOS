/***********************
 * @file: os_block.h
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2022-09-10     Feijie Luo   First version
 * 2023-10-13     Feijie Luo   decouple from os_task_state
 * @note:
 ***********************/
#ifndef _OS_BLOCK_H_
#define _OS_BLOCK_H_
#include "os_core.h"

typedef enum os_block_type {
    OS_BLOCK_NONE = (0),
    OS_BLOCK_MUTEX = (1),
    OS_BLOCK_TICK = (2),
    OS_BLOCK_SEM = (3),
    OS_BLOCK_MQUEUE = (4),
} os_block_type_t;

struct os_block_object {
    os_block_type_t _type;
    struct list_head _list;
};

bool os_task_is_block(struct task_control_block *_task_tcb);
void os_block_init(struct os_block_object *_block_obj, os_block_type_t _block_type);
void os_block_deinit(struct os_block_object *_block_obj);
bool os_block_list_is_empty(struct os_block_object *_block_obj);
os_handle_state_t os_add_block_task(struct task_control_block *_task_tcb,
                                    struct os_block_object *_block_obj);
os_handle_state_t os_add_tick_block_task(struct task_control_block *_task_tcb,
                                         struct os_block_object *_block_obj);
os_handle_state_t os_block_wakeup_task(struct task_control_block *_task_tcb);
void os_block_wakeup_first_task(struct os_block_object *_block_obj,
                                void (*callback)(struct task_control_block *task));
void os_block_wakeup_all_task(struct os_block_object *_block_obj);

#endif
