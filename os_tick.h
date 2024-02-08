/***********************
 * @file: os_tick.h
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2022-09-10     Feijie Luo   First version
 * @note:
 ***********************/

#ifndef	_OS_TICK_H_
#define _OS_TICK_H_

#include "os_config.h"
#include "os_core.h"
#include "os_list.h"
#include "os_block.h"

enum os_tick_state {
    OS_TICK_RUNNING    = (1 << 0L),
    OS_TICK_TIME_OUT   = (1 << 1L)
};

struct os_tick {
    unsigned int                          _tick_count;
    enum os_tick_state                    _tick_state;
    struct list_head                      _tick_list_nd;
    struct os_block_object                _block_head;
};

os_handle_state_t os_add_tick_task(struct task_control_block* _task_tcb, unsigned int _tick,
                                   struct os_block_object* _block_obj);
void os_task_tick_poll(void);
void os_task_delay_ms(unsigned int _tick_ms);
#endif
