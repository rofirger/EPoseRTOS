/*
 * @File: soft_timer.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2022-09-6     Feijie Luo   First version
 * 2023-10-13    Feijie Luo   Decouple
 * @note:
 */

#include "board/os_board.h"
#include "os_soft_timer.h"

static uint32_t _irq_times_s;
static struct jiffies_structure jiffies;

static soft_timer_t soft_timer[SOFT_TIMER_NUM];

inline void os_soft_timer_systick_handle(void)
{
    jiffies.c++;
    if (jiffies.c == 0)
        jiffies.bc++;
}

inline void os_soft_timer_set_systick_times(const uint32_t new_st)
{
    jiffies.c = new_st;
}

struct jiffies_structure os_get_timestamp(void)
{
    return jiffies;
}

void os_soft_timer_init(void)
{
    _irq_times_s = jiffies.c;
    for (uint32_t _i = 0; _i < SOFT_TIMER_NUM; ++_i) {
        soft_timer[_i]._soft_timer_state = SOFT_TIMER_STOPPED;
        soft_timer[_i]._soft_timer_mode = SOFT_TIMER_MODE_ONE_SHOT;
        soft_timer[_i]._due._ms = 0;
        soft_timer[_i]._due._us = 0;
        soft_timer[_i]._period = 0;
        soft_timer[_i]._callback = NULL;
        soft_timer[_i]._argv = NULL;
    }
}

bool os_soft_timer_start(const uint32_t _id, const soft_timer_mode _mode, const uint32_t _period, soft_timer_callback _callback, void *_argv)
{
    if (_id >= SOFT_TIMER_NUM ||
        (_mode != SOFT_TIMER_MODE_ONE_SHOT &&
         _mode != SOFT_TIMER_MODE_CYCLE) ||
        soft_timer[_id]._soft_timer_state != SOFT_TIMER_STOPPED)
        return false;
    soft_timer[_id]._soft_timer_state = SOFT_TIMER_RUNNING;
    soft_timer[_id]._soft_timer_mode = _mode;
    soft_timer[_id]._due = soft_timer_add_us(soft_timer_get_time(), _period);
    soft_timer[_id]._period = _period;
    soft_timer[_id]._callback = _callback;
    soft_timer[_id]._argv = _argv;
    return true;
}

void os_soft_timer_update(void)
{
    for (uint32_t _i = 0; _i < SOFT_TIMER_NUM; ++_i) {
        switch (soft_timer[_i]._soft_timer_state) {
        case SOFT_TIMER_STOPPED:
            break;
        case SOFT_TIMER_RUNNING:
            if (soft_timer_get_2_time_diff_us(soft_timer_get_time(), soft_timer[_i]._due) >= 0) {
                soft_timer[_i]._soft_timer_state = SOFT_TIMER_TIMEOUT;
                soft_timer[_i]._callback(soft_timer[_i]._argv);
            }
            break;
        case SOFT_TIMER_TIMEOUT:
            if (soft_timer[_i]._soft_timer_mode == SOFT_TIMER_MODE_ONE_SHOT)
                soft_timer[_i]._soft_timer_state = SOFT_TIMER_STOPPED;
            else {
                soft_timer[_i]._due = soft_timer_add_us(soft_timer_get_time(), soft_timer[_i]._period);
                soft_timer[_i]._soft_timer_state = SOFT_TIMER_RUNNING;
            }
            break;
        default:
            break;
        }
    }
}

inline bool os_soft_timer_stop(const uint32_t _id)
{
    if (_id >= SOFT_TIMER_NUM)
        return false;
    soft_timer[_id]._soft_timer_state = SOFT_TIMER_STOPPED;
    return true;
}

inline soft_timer_state os_soft_timer_get_state(const uint32_t _id)
{
    if (_id >= SOFT_TIMER_NUM)
        return SOFT_TIMER_NO_STATE;
    return soft_timer[_id]._soft_timer_state;
}

// 获取从开始到现在的时间(单位:us). 不建议使用
inline unsigned int sys_tick_get_us(void)
{
    return (CLOCK_COUNT_TO_US(os_hw_systick_get_reload() - os_hw_systick_get_val(), CONFIG_SYSTICK_CLOCK_FREQUENCY) + (jiffies.c - _irq_times_s) * CLOCK_COUNT_TO_US(os_hw_systick_get_reload(), CONFIG_SYSTICK_CLOCK_FREQUENCY));
}

soft_timer_time_t soft_timer_get_time(void)
{
    soft_timer_time_t _ret;
    float _one_iqr_ms = CLOCK_COUNT_TO_US(os_hw_systick_get_reload(), CONFIG_SYSTICK_CLOCK_FREQUENCY) / 1000.0f;
    float _ret_ms = CLOCK_COUNT_TO_US(os_hw_systick_get_reload() - os_hw_systick_get_val(), CONFIG_SYSTICK_CLOCK_FREQUENCY) / 1000.0f;
    float _total_ms = _one_iqr_ms * (jiffies.c - _irq_times_s) + _ret_ms;
    _ret._ms = (uint32_t)_total_ms;
    _ret._us = (uint32_t)((_total_ms - (uint32_t)_total_ms) * 1000.0f);
    return _ret;
}

inline soft_timer_time_t soft_timer_add_2_time(const soft_timer_time_t _fst, const soft_timer_time_t _scd)
{
    soft_timer_time_t _ret;
    unsigned int _add_us = _fst._us + _scd._us;
    _ret._us = _add_us % 1000;
    _ret._ms = _fst._ms + _scd._ms + _add_us / 1000;
    return _ret;
}

inline soft_timer_time_t soft_timer_add_us(const soft_timer_time_t _time, const uint32_t _us)
{
    soft_timer_time_t _ret;
    _ret._us = _time._us + _us;
    _ret._ms = _time._ms + (_ret._us / 1000);
    _ret._us = _ret._us % 1000;
    return _ret;
}

inline int soft_timer_get_2_time_diff_us(const soft_timer_time_t _e, const soft_timer_time_t _s)
{
    return ((_e._ms - _s._ms) * 1000U + _e._us - _s._us);
}
