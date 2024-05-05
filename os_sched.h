/***********************
 * @file: os_sched.h
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2022-09-10     Feijie Luo   First version
 * @note:
 ***********************/

#ifndef _OS_SCHED_H_
#define _OS_SCHED_H_

#include "os_core.h"

#define OS_SCHED_TIMESLICE_NULL 0xFFFFFFFF

struct os_ready_queue {
    unsigned int _highest_priority;
    struct list_head _queue[OS_READY_LIST_SIZE];
};

struct os_sched_timeslice_pos {
    unsigned int _last_priority;
    struct list_head *_last_task_node;
};

void __insert_task_priority(unsigned char _priority);
void __del_task_priority(unsigned char _priority);
void update_ready_queue_priority(void);
unsigned char __get_highest_ready_priority(void);
void os_rq_add_task(struct task_control_block *_task);
void os_rq_del_task(struct task_control_block *_task);
struct task_control_block *os_rq_get_highest_prio_task(void);
void os_sys_ready_queue_init(void);
os_handle_state_t os_sched_lock(void);
bool os_sched_is_lock(void);
os_handle_state_t os_sched_unlock(void);
void os_sched_timeslice_init(void);
void os_sched_timeslice_set(unsigned int _prio, unsigned int _new_timeslice);
unsigned int os_sched_timeslice_get(unsigned int _prio);
void os_sched_timeslice_reload(struct task_control_block *_task_tcb);
void os_sched_timeslice_poll(void);
void os_diable_sched(void);
void os_enable_sched(void);
struct task_control_block *os_get_current_task_tcb(void);
void os_set_current_task_tcb(struct task_control_block *tcb);
struct task_control_block *os_get_ready_task_tcb(void);
void os_sched_insert_ready(void);
bool __os_sched_status_called_by_sw(void);
int __os_sched_called_by_sw(void);
int __os_sched(void);
os_handle_state_t os_task_yield(void);
void os_sched_halt(void);
bool os_sched_is_running(void);
void os_sched_set_running(void);

#endif
