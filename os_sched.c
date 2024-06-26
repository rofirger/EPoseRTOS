/***********************
 * @file: os_sched.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2022-09-10     Feijie Luo   First version
 * @note:
 ***********************/

#include "os_config.h"
#include "os_core.h"
#include "os_list.h"
#include "os_sched.h"
#include "os_sys.h"
#include "os_tick.h"

#include "board/libcpu_headfile.h"

static unsigned char os_ready_table[32];
static unsigned int os_ready_priority_group;
static struct os_ready_queue _os_rq;
static unsigned char _os_sched_lock_nesting;
// 时间片管理
static unsigned int _sched_prio_timeslice[OS_READY_LIST_SIZE];
// 时间片轮转位置记录
static struct os_sched_timeslice_pos _os_sched_timeslice_pos;

struct task_control_block *os_task_current = NULL;
struct task_control_block *os_task_ready = NULL;

static unsigned char _os_sched_flag = 0;

const unsigned char _lowest_bitmap[] =
    {
        0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};

/* 向bitmap中登记优先级 */
inline void __insert_task_priority(unsigned char _priority)
{
    unsigned char _group_bit_index = _priority >> 3;
    os_ready_table[_group_bit_index] |= (1L << (_priority & 0x07));
    os_ready_priority_group |= (1L << _group_bit_index);
}

/* 从bitmap中注销优先级 */
inline void __del_task_priority(unsigned char _priority)
{
    unsigned char _group_bit_index = _priority >> 3;
    os_ready_table[_group_bit_index] &= (~(1L << (_priority & 0x07)));
    if (0 == os_ready_table[_group_bit_index])
        os_ready_priority_group &= (~(1L << _group_bit_index));
}

os_private unsigned int __ffb(unsigned int _word)
{
    if (0 == _word)
        return 0;
    if (_word & 0xFF)
        return _lowest_bitmap[_word & 0xFF];
    if (_word & 0xFF00)
        return _lowest_bitmap[(_word & 0xFF00) >> 8] + 8;
    if (_word & 0xFF0000)
        return _lowest_bitmap[(_word & 0xFF0000) >> 16] + 16;
    return _lowest_bitmap[(_word & 0xFF000000) >> 24] + 24;
}

/* 从bitmap中获取最高优先级(数值越小，优先级越高) */
inline unsigned char __get_highest_ready_priority(void)
{
    unsigned char _tmp_num;
    _tmp_num = __ffb(os_ready_priority_group);
    return ((_tmp_num << 3L) + __ffb(os_ready_table[_tmp_num]));
}

inline void update_ready_queue_priority(void)
{
    _os_rq._highest_priority = __get_highest_ready_priority();
}

os_private void __os_rq_add_task_head(struct task_control_block *_task)
{
    unsigned char _task_prio = _task->_task_priority;
    if (list_empty(&_os_rq._queue[_task_prio])) {
        __insert_task_priority(_task_prio);
        if (_task_prio < _os_rq._highest_priority)
            _os_rq._highest_priority = _task_prio;
    }
    list_add(&_os_rq._queue[_task_prio], &_task->_slot_nd);
}

os_private void __os_rq_add_task_tail(struct task_control_block *_task)
{
    unsigned char _task_prio = _task->_task_priority;
    if (list_empty(&_os_rq._queue[_task_prio])) {
        __insert_task_priority(_task_prio);
        if (_task_prio < _os_rq._highest_priority)
            _os_rq._highest_priority = _task_prio;
    }
    list_add_tail(&_os_rq._queue[_task_prio], &_task->_slot_nd);
}

/* 往就绪队列中添加任务 */
void os_rq_add_task(struct task_control_block *_task)
{
    // 任务时间片加载
    // os_sched_timeslice_reload(_task);
    // 加入就绪队列的前提是任务当前不处于ready状态
    if (os_task_state_is_ready(_task))
        return;

    if (NULL == os_task_current || _task->_task_priority < os_task_current->_task_priority)
        __os_rq_add_task_head(_task);
    else
        __os_rq_add_task_tail(_task);
    os_task_state_set_ready(_task);
}

inline os_private void __os_sched_timeslice_task_rq_del(struct task_control_block *_task)
{
    /* 当任务被从就绪队列移除时，需要更新时间片指针信息 */
    if (_task->_task_priority == _os_sched_timeslice_pos._last_priority &&
        &_task->_slot_nd == _os_sched_timeslice_pos._last_task_node) {
        _os_sched_timeslice_pos._last_task_node = _task->_slot_nd.next;
    }
}

/* 从就绪队列中移除任务 */
void os_rq_del_task(struct task_control_block *_task)
{
    // 如果处于是时间片中的任务，需要更新时间片位置信息
    __os_sched_timeslice_task_rq_del(_task);
    unsigned char _task_prio = _task->_task_priority;
    list_del_init(&_task->_slot_nd);
    if (list_empty(&_os_rq._queue[_task_prio])) {
        __del_task_priority(_task_prio);
        if (_os_rq._highest_priority == _task_prio)
            update_ready_queue_priority(); // 更新就绪队列中的最高优先级
    }
}

/* 获取就绪队列中的任务最高优先级 */
struct task_control_block *os_rq_get_highest_prio_task(void)
{
    struct task_control_block *_ret = NULL;

    if (_os_sched_timeslice_pos._last_priority != _os_rq._highest_priority) {
        _os_sched_timeslice_pos._last_priority = _os_rq._highest_priority;
        _os_sched_timeslice_pos._last_task_node = _os_rq._queue[_os_rq._highest_priority].next;
    }
    // 一般存在锁、延时等其他需要将任务在_os_rq来回切换的情况下会出现下面的条件为true
    if (_os_sched_timeslice_pos._last_task_node == &_os_rq._queue[_os_rq._highest_priority])
        _os_sched_timeslice_pos._last_task_node = _os_sched_timeslice_pos._last_task_node->next;
    _ret = os_list_entry(_os_sched_timeslice_pos._last_task_node, struct task_control_block, _slot_nd);
    // 时间片轮盘旋转
    if (_ret->_task_timeslice != OS_SCHED_TIMESLICE_NULL &&
        _os_rq._queue[_os_rq._highest_priority].next != _os_rq._queue[_os_rq._highest_priority].prev) {
        if (_ret->_task_timeslice == 0) {
            // 时间片重新加载
            os_sched_timeslice_reload(_ret);
            _os_sched_timeslice_pos._last_task_node = _os_sched_timeslice_pos._last_task_node->next;
            if (_os_sched_timeslice_pos._last_task_node == &_os_rq._queue[_os_rq._highest_priority])
                _os_sched_timeslice_pos._last_task_node = _os_sched_timeslice_pos._last_task_node->next;
            _ret = os_list_entry(_os_sched_timeslice_pos._last_task_node, struct task_control_block, _slot_nd);
        }
    }
    return _ret;
}

/* 初始化就绪队列 */
void os_sys_ready_queue_init(void)
{
    _os_rq._highest_priority = OS_TASK_MAX_PRIORITY;
    for (unsigned int _i = 0; _i < OS_READY_LIST_SIZE; ++_i)
        list_head_init(&_os_rq._queue[_i]);
    for (unsigned int _i = 0; _i < 32; ++_i)
        os_ready_table[_i] = 0;
    os_ready_priority_group = 0;
    _os_sched_lock_nesting = 0;
}

/* 是否处在调度锁 */
inline bool os_sched_is_lock(void)
{
    return (_os_sched_lock_nesting > 0);
}

/* 调度锁 */
os_handle_state_t os_sched_lock(void)
{
    /* 递归调度锁层数不能超过200 */
    if (!os_sched_is_running() ||
        os_sys_is_in_irq() ||
        _os_sched_lock_nesting > 200)
        return OS_HANDLE_FAIL;
    __OS_OWNED_ENTER_CRITICAL
    _os_sched_lock_nesting++;
    __OS_OWNED_EXIT_CRITICAL
    return OS_HANDLE_SUCCESS;
}

/* 调度锁解锁 */
os_handle_state_t os_sched_unlock(void)
{
    /* 递归调度锁层数不能超过200 */
    if (!os_sched_is_running() ||
        !os_sched_is_lock() ||
        os_sys_is_in_irq())
        return OS_HANDLE_FAIL;
    __OS_OWNED_ENTER_CRITICAL
    _os_sched_lock_nesting--;
    __OS_OWNED_EXIT_CRITICAL
    // 再次执行调度
    __os_sched();
    return OS_HANDLE_SUCCESS;
}

/* 时间片初始化 */
inline void os_sched_timeslice_init(void)
{
    _os_sched_timeslice_pos._last_priority = OS_TASK_MAX_PRIORITY + 1;
    for (unsigned int _i = 0; _i < OS_READY_LIST_SIZE; ++_i)
        _sched_prio_timeslice[_i] = OS_TIMESLICE_STD;
}

/* 修改对应优先级的时间片 */
inline void os_sched_timeslice_set(unsigned int _prio, unsigned int _new_timeslice)
{
    _sched_prio_timeslice[_prio] = _new_timeslice;
}

/* 获取对应优先级的时间片 */
inline unsigned int os_sched_timeslice_get(unsigned int _prio)
{
    return _sched_prio_timeslice[_prio];
}

/* 任务时间片重新加载 */
inline void os_sched_timeslice_reload(struct task_control_block *_task_tcb)
{
    _task_tcb->_task_timeslice = os_sched_timeslice_get(_task_tcb->_task_priority);
}

/*********************************************************************
 * @fn      os_task_yield
 * @param   none
 * @brief   Yields the processor by voluntarily giving up
 *          the CPU execution time for the current thread.
 * @return  OS_HANDLE_SUCCESS: yield successfully
 */
os_handle_state_t os_task_yield(void)
{
    __OS_OWNED_ENTER_CRITICAL
    if (os_sched_timeslice_get(os_task_current->_task_priority) !=
            OS_SCHED_TIMESLICE_NULL) {
        os_task_current->_task_timeslice = 0;
        __OS_OWNED_EXIT_CRITICAL;
        __os_sched();
        return OS_HANDLE_SUCCESS;
    }
    __OS_OWNED_EXIT_CRITICAL
    return OS_HANDLE_FAIL;
}

/*********************************************************************
 * @fn      os_sched_timeslice_poll
 * @param   none
 * @brief   time-slice poll, ONLY called by os_systick_handler
 * @return  none
 */
static volatile unsigned int _in_crirical_poll_num;
void os_sched_timeslice_poll(void)
{
    if (os_sys_owned_critical_status()) {
        if (os_sched_timeslice_get(os_task_current->_task_priority) != OS_SCHED_TIMESLICE_NULL) {
            if (_in_crirical_poll_num >= os_task_current->_task_timeslice)
                os_task_current->_task_timeslice = 0;
            else {
                (os_task_current->_task_timeslice) -= _in_crirical_poll_num;
                (os_task_current->_task_timeslice)--;
            }
            _in_crirical_poll_num = 0;

            if (os_task_current->_task_timeslice > os_sched_timeslice_get(os_task_current->_task_priority))
                os_task_current->_task_timeslice = 0;
            if (os_task_current->_task_timeslice == 0) {
                __os_sched();
            }
        }
    } else {
        _in_crirical_poll_num++;
    }
}

static unsigned int __status_os_diable_sched;
/* 禁止任务调度 */
inline void os_diable_sched(void)
{
    // os_sys_enter_irq();
    __status_os_diable_sched = os_port_enter_critical();
}

/* 开启任务调度 */
inline void os_enable_sched(void)
{
    // os_sys_exit_irq();
    os_port_exit_critical(__status_os_diable_sched);
    __os_sched();
}

inline struct task_control_block *os_get_current_task_tcb(void)
{
    return os_task_current;
}

inline void os_set_current_task_tcb(struct task_control_block *tcb)
{
    os_task_current = tcb;
}

inline struct task_control_block *os_get_ready_task_tcb(void)
{
    return os_task_ready;
}

inline void os_sched_insert_ready(void)
{
    os_task_ready = os_rq_get_highest_prio_task();
}

inline bool __os_sched_status_called_by_sw(void)
{
    return os_task_current != os_task_ready;
}

int __os_sched_called_by_sw(void)
{
    if (os_task_current != os_task_ready)
        return 0;
    // 当线程处于调度锁并且存在延时时,将线程切换到系统空闲线程
    if (os_sched_is_lock()) {
        static struct task_control_block *_lock_tcb;
        if (os_task_current != os_get_idle_tcb())
            _lock_tcb = os_task_current;
        if (_lock_tcb->_task_block_state == OS_TASK_BLOCK_TICKING)
            os_task_ready = os_get_idle_tcb();
        else
            os_task_ready = _lock_tcb;
    } else
        os_task_ready = os_rq_get_highest_prio_task();

    // 如果没有就绪任务 | 当前任务即就绪任务则不调度
    if (NULL == os_task_ready ||
        os_task_current == os_task_ready) {
        return -1;
    }
    // 更改任务状态
    os_task_state_set_running(os_task_ready);
    if (os_task_state_is_running(os_task_current))
        os_task_state_set_ready(os_task_current);
    return 0;
}

int __os_sched(void)
{
    __OS_OWNED_ENTER_CRITICAL
    // 当线程处于调度锁并且存在延时时,将线程切换到系统空闲线程
    if (os_sched_is_lock()) {
        static struct task_control_block *_lock_tcb;
        if (os_task_current != os_get_idle_tcb())
            _lock_tcb = os_task_current;
        if (_lock_tcb->_task_block_state == OS_TASK_BLOCK_TICKING)
            os_task_ready = os_get_idle_tcb();
        else
            os_task_ready = _lock_tcb;
    } else
        os_task_ready = os_rq_get_highest_prio_task();

    if (NULL == os_task_ready ||
        os_task_current == os_task_ready) {
        __OS_OWNED_EXIT_CRITICAL
        return -1;
    }
    // 更改任务状态
    os_task_state_set_running(os_task_ready);
    if (os_task_state_is_running(os_task_current))
        os_task_state_set_ready(os_task_current);
    __OS_OWNED_EXIT_CRITICAL
    // 触发异常，以进行上下文切换
    os_ctx_sw();
    return 0;
}

inline void os_sched_halt(void)
{
    _os_sched_flag = 0;
}

inline bool os_sched_is_running(void)
{
    return (_os_sched_flag == 1);
}

inline void os_sched_set_running(void)
{
    _os_sched_flag = 1;
}

/*
static unsigned int _find_fist_bit(unsigned int _word)
{
        unsigned int _bit_index = 0;
        // 32位的单片机系统不需要考虑0xFFFFFFFF

        if (0 == (_word & 0xFFFF))
        {
                _bit_index += 16;
                _word >>= 16;
        }
        if (0 == (_word & 0xFF))
        {
                _bit_index += 8;
                _word >>= 8;
        }
        if (0 == (_word & 0xF))
        {
                _bit_index += 4;
                _word >>= 4;
        }
        if (0 == (_word & 0x03))
        {
                _bit_index += 2;
                _word >>= 2;
        }
        if (0 == (_word & 0x01))
                _bit_index += 1;
        return _bit_index;
}*/
