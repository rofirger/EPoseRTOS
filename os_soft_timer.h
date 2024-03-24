/*
 * @File: soft_timer.h
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2022-09-10     Feijie Luo   First version
 * @note:
 */

#ifndef _SOFT_TIMER_H_
#define _SOFT_TIMER_H_
#include "os_config.h"
#include "os_def.h"
#include "stdint.h"
#include "stdlib.h"

#define SOFT_TIMER_NUM 2

#define CLOCK_COUNT_TO_US(_count, _freq) ((uint64_t)((uint64_t)(_count) * 1000000U / (_freq)))
#define CLOCK_COUNT_TO_MS(_count, _freq) ((uint64_t)((uint64_t)(_count) * 1000U / (_freq)))

#define US_TO_CLOCK_COUNT(_us, _freq) ((uint64_t)((_us) * (_freq) / ((uint64_t)1000000U)))
#define MS_TO_CLOCK_COUNT(_ms, _freq) ((uint64_t)((_ms) * (_freq) / ((uint64_t)1000U)))

typedef void (*soft_timer_callback)(void *_argv);

typedef struct soft_timer_time_t {
    uint32_t _ms;
    int32_t _us;
} soft_timer_time_t;

typedef enum soft_timer_state {
    SOFT_TIMER_STOPPED,
    SOFT_TIMER_RUNNING,
    SOFT_TIMER_TIMEOUT,
    SOFT_TIMER_NO_STATE
} soft_timer_state;

typedef enum soft_timer_mode {
    SOFT_TIMER_MODE_ONE_SHOT,
    SOFT_TIMER_MODE_CYCLE
} soft_timer_mode;

typedef struct soft_timer_t {
    soft_timer_state _soft_timer_state;
    soft_timer_mode _soft_timer_mode;
    soft_timer_time_t _due;
    uint32_t _period;
    soft_timer_callback _callback;
    void *_argv;
} soft_timer_t;

struct jiffies_structure {
    uint32_t c; 
    // bc plus 1 when c reaches the max value of uint32_t
    uint32_t bc;
};

struct jiffies_structure os_get_timestamp(void);
void os_soft_timer_set_systick_times(const uint32_t new_st);
void os_soft_timer_systick_handle(void);
void os_soft_timer_init(void);
void soft_timer_systick_irq(void);
bool os_soft_timer_start(const uint32_t _id, const soft_timer_mode _mode, const uint32_t _period, soft_timer_callback _callback, void *_argv);
void os_soft_timer_update(void);
bool os_soft_timer_stop(const uint32_t _id);
soft_timer_state os_soft_timer_get_state(const uint32_t _id);
soft_timer_time_t soft_timer_get_time(void);
unsigned int sys_tick_get_us(void);
soft_timer_time_t soft_timer_add_us(const soft_timer_time_t _time, const uint32_t _us);
int soft_timer_get_2_time_diff_us(const soft_timer_time_t _e, const soft_timer_time_t _s);

#define os_soft_timer_reset_systick_times() os_soft_timer_set_systick_times(0)

extern struct jiffies_structure jiffies;

#endif
