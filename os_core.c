/***********************
 * @file: os_core.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2022-09-10     Feijie Luo   First version
 * 2023-10-17     Feijie Luo   Add task status update function
 * 2023-11-07     Feijie Luo   Add ps cmd.
 * @note:
 ***********************/

#include "os_core.h"
#include "os_sched.h"
#include "os_service.h"
#include "os_sys.h"
#include "os_tick.h"
#include "components/lib/os_string.h"

#include "board/libcpu_headfile.h"

static struct task_control_block *_os_id_tcb_tab[OS_TASK_MAX_ID];

/***********************************************************************************/
/***********************************************************************************/
/*********************** new ***********************/
inline bool os_task_state_is_new(struct task_control_block *_task_tcb)
{
    return (_task_tcb->_task_state == OS_TASK_NEW);
}

inline void os_task_state_set_new(struct task_control_block *_task_tcb)
{
    _task_tcb->_task_state = OS_TASK_NEW;
}

/*********************** ready ***********************/
inline bool os_task_state_is_ready(struct task_control_block *_task_tcb)
{
    return (_task_tcb->_task_state == OS_TASK_READY);
}

inline void os_task_state_set_ready(struct task_control_block *_task_tcb)
{
    _task_tcb->_task_state = OS_TASK_READY;
}

/*********************** running ***********************/
inline bool os_task_state_is_running(struct task_control_block *_task_tcb)
{
    return (_task_tcb->_task_state == OS_TASK_RUNNING);
}

inline void os_task_state_set_running(struct task_control_block *_task_tcb)
{
    _task_tcb->_task_state = OS_TASK_RUNNING;
}

/*********************** stopped ***********************/
inline bool os_task_state_is_stopped(struct task_control_block *_task_tcb)
{
    return (_task_tcb->_task_state == OS_TASK_TERMINATED);
}

inline void os_task_state_set_stopped(struct task_control_block *_task_tcb)
{
    _task_tcb->_task_state = OS_TASK_TERMINATED;
}

/*********************** block ***********************/
inline bool os_task_state_is_blocking(struct task_control_block *_task_tcb)
{
    return (_task_tcb->_task_state == OS_TASK_BLOCKING);
}

inline void os_task_state_set_blocking(struct task_control_block *_task_tcb)
{
    _task_tcb->_task_state = OS_TASK_BLOCKING;
}

/***********************************************************************************/
/***********************************************************************************/

/* 线程退出 */
os_private void __os_task_exit_handle(void)
{
    __OS_OWNED_ENTER_CRITICAL

    struct task_control_block *_ct_tcb = os_get_current_task_tcb();
    // 清除id
    _os_id_tcb_tab[_ct_tcb->_task_id] = NULL;
    // 将线程从就绪队列中清除
    os_rq_del_task(_ct_tcb);
    __OS_OWNED_EXIT_CRITICAL
    // 执行调度
    __os_sched();
}

os_handle_state_t os_task_create(struct task_control_block *_task_tcb,
                                 unsigned int *_stack_addr,
                                 unsigned int _stack_size,
                                 unsigned char _prio,
                                 __task_fn_ _entry_fn,
                                 void *_entry_fn_arg,
                                 const char *_task_name)
{
    if (NULL == _task_tcb ||
        NULL == _stack_addr ||
        NULL == _entry_fn ||
        _prio > OS_TASK_MAX_PRIORITY)
        return OS_HANDLE_FAIL;

    __OS_OWNED_ENTER_CRITICAL

    unsigned int _tmp_id = OS_TASK_MAX_ID + 1;

    for (unsigned int _i = 0; _i <= OS_TASK_MAX_ID; ++_i) {
        if (NULL == _os_id_tcb_tab[_i]) {
            _tmp_id = _i;
            break;
        }
    }
    if (_tmp_id > OS_TASK_MAX_ID) {
        __OS_OWNED_EXIT_CRITICAL
        return OS_HANDLE_FAIL;
    }
    _os_id_tcb_tab[_tmp_id] = _task_tcb;
    _task_tcb->_task_id = _tmp_id;

    // 初始化线程堆栈
    _task_tcb->_stack_top = os_process_stack_init((void *)_entry_fn, _entry_fn_arg, (void *)__os_task_exit_handle, _stack_addr, _stack_size);

    _task_tcb->sp = _task_tcb->_stack_top;

    _task_tcb->_task_priority = _prio;
    _task_tcb->_task_name = _task_name;

    /* note: 在加入就绪队列中会将线程状态置为new状态 */
    os_task_state_set_new(_task_tcb);
    _task_tcb->_task_timeslice = 0;
    _task_tcb->_block_mount = NULL;

    // 链表初始化
    list_head_init(&_task_tcb->_bt_nd);
    list_head_init(&_task_tcb->_slot_nd);

    // 加入优先级队列
    os_rq_add_task(_task_tcb);
    __OS_OWNED_EXIT_CRITICAL

    // 	调度
    __os_sched();

    return OS_HANDLE_SUCCESS;
}

const char _os_state_str[5][11] = {
    "   NEW    ",
    "  READY   ",
    " RUNNING  ",
    "TERMINATED",
    " BLOCKING "};

const char _os_block_state_str[4][8] = {
    " NONE  ",
    "TICKING",
    "TIMEOUT",
    "WAKEUP ",
};

os_private OS_CMD_PROCESS_FN(ps_fn)
{
    unsigned int i;
    struct task_control_block *task;
    unsigned int task_name_max_len = 0;
    unsigned int tmp_len;
    for (i = 0; i < OS_TASK_MAX_ID; ++i) {
        task = _os_id_tcb_tab[i];
        tmp_len = os_strlen(task->_task_name);
        task_name_max_len =
                task_name_max_len >  tmp_len ?
                        task_name_max_len : tmp_len;
    }
    task_name_max_len = task_name_max_len > 9 ? task_name_max_len : 9;
    os_printk("+----+----------+--------+----------+----------+----------+--------+-----|");
    for (int j = 0; j < task_name_max_len; j++, os_printk("%c", '-'));
    os_printk("+\r\n");
    os_printk("| id |   state  | bstate |  sp top  |    sp    |stack used|priority|slice|task name");
    for (int j = 0; j < task_name_max_len - 9; j++, os_printk("%c", ' '));
    os_printk("|\r\n");
    os_printk("+----+----------+--------+----------+----------+----------+--------+-----|");
    for (int j = 0; j < task_name_max_len; j++, os_printk("%c", '-'));
    os_printk("+\r\n");
    for (i = 0; i < OS_TASK_MAX_ID; ++i) {
        task = _os_id_tcb_tab[i];
        if (NULL != task) {
            os_printk("|%4d|%s|%-8s|0x%8x|0x%8x|0x%08x|%8d|%5d|%-*s|\r\n",
                      task->_task_id,
                      _os_state_str[task->_task_state - 1],
                      _os_block_state_str[task->_task_block_state],
                      task->_stack_top,
                      task->sp,
                      (int)(task->_stack_top) - (int)(task->sp),
                      task->_task_priority,
                      task->_task_timeslice,
                      task_name_max_len,
                      task->_task_name);
        }
    }
    os_printk("+----+----------+--------+----------+----------+----------+--------+-----|");
    for (int j = 0; j < task_name_max_len; j++, os_printk("%c", '-'));
    os_printk("+\r\n");
    return 0;
}

OS_CMD_EXPORT(ps, ps_fn, "List the information of thread.");
