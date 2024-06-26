/***********************
 * @file: os_tick.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2022-09-10     Feijie Luo   First version
 * 2023-10-13     Feijie Luo   Fix __os_tick_add_node function bug:
 *                                    The _tick_count for the linked list is not updated.
 * 2023-10-15     Feijie Luo   Improve program structure. tick inheritance block
 * @note:
 ***********************/

#include "os_core.h"
#include "os_list.h"
#include "os_sched.h"
#include "os_sys.h"
#include "os_tick.h"

#include "board/libcpu_headfile.h"
#include "components/memory/os_malloc.h"

LIST_HEAD(_os_tick_list_head);

os_private void __os_tick_add_node(struct task_control_block *_task_tcb, unsigned int _tick)
{
    // 更新task的block状态
    _task_tcb->_task_block_state = OS_TASK_BLOCK_TICKING;
    // 先找一下是否存在相同的_tick对象
    struct list_head *_current_node = NULL;
    struct os_tick *_current_node_tick = NULL;
    unsigned int _prev_tick = 0;
    unsigned int _current_tick = 0;
    // 采取阶梯增量式tick计算方法
    list_for_each(_current_node, &_os_tick_list_head)
    {
        _current_node_tick = os_list_entry(_current_node, struct os_tick, _tick_list_nd);
        _current_tick = _prev_tick + _current_node_tick->_tick_count;
        if (_current_tick == _tick) {
            os_add_tick_block_task(_task_tcb, &(_current_node_tick->_block_head));
            return;
        }
        // 直至遇到第一个比_tick大的对象(注意是阶梯增量式tick计算)
        if (_current_tick > _tick) {
            break;
        }
        _prev_tick = _current_tick;
    }
    struct os_tick *new_tick = os_kmalloc(sizeof(struct os_tick));
    OS_ASSERT(NULL != new_tick);
    new_tick->_tick_state = OS_TICK_RUNNING;
    new_tick->_tick_count = _tick - _prev_tick;
    os_block_init(&(new_tick->_block_head), OS_BLOCK_TICK);
    list_add_tail(_current_node, &(new_tick->_tick_list_nd));
    // 添加任务进入block
    os_add_tick_block_task(_task_tcb, &(new_tick->_block_head));
    // 如果存在后一个任务，更新后面一个任务的_tick
    if (_current_node != &_os_tick_list_head)
        _current_node_tick->_tick_count -= new_tick->_tick_count;
}

os_private void __os_tick_del_node(struct os_tick *ptr_tick)
{
    list_del_init(&(ptr_tick->_tick_list_nd));
    os_kfree(ptr_tick);
}

os_handle_state_t os_add_tick_task(struct task_control_block *_task_tcb, unsigned int _tick,
                                   struct os_block_object *_block_obj)
{
    // 不可以处于 blocking 状态的任务再次 block
    if (NULL == _task_tcb || os_task_state_is_blocking(_task_tcb))
        return OS_HANDLE_FAIL;
    // 先__os_tick_add_node， 后os_add_block_task，不可以调换
    if (_tick != OS_NEVER_TIME_OUT)
        __os_tick_add_node(_task_tcb, _tick);
    if (_block_obj != NULL)
        os_add_block_task(_task_tcb, _block_obj);
    return OS_HANDLE_SUCCESS;
}

// void os_tick_del_task(struct task_control_block* _task_tcb)
//{
//     struct os_tick* task_mount_tick = os_list_entry(_task_tcb->_block_mount, struct os_tick, _block_head);
//	list_del_init(&(_task_tcb->_bt_nd));
//	// 检查这个block上是否无task
//	if (task_mount_tick->_block_head._list.next == &(task_mount_tick->_block_head._list)) {
//	    // 无task则free对应的tick对象
//	    __os_tick_del_node(task_mount_tick);
//	}
// }

os_private void __tick_tcb_time_out_cb(struct task_control_block *task)
{
    // task 目前处于 time out 状态
    task->_task_block_state = OS_TASK_BLOCK_TIMEOUT;
    // wake up from block list
    list_del_init(&(task->_slot_nd));
    // 加入就绪队列
    os_rq_add_task(task);
}

os_private void __os_tick_del_all_task(struct os_tick *ptr_tick, void (*callback)(struct task_control_block *task))
{
    struct list_head *_current_node = NULL;
    struct list_head *_next_node = NULL;
    list_for_each_safe(_current_node, _next_node, &(ptr_tick->_block_head._list))
    {
        struct task_control_block *tmp = os_list_entry(_current_node, struct task_control_block, _bt_nd);
        // 注意这里的两行代码不可以互换顺序，因为callback里可能包含将任务挂载在ready队列上
        list_del_init(_current_node);
        callback(tmp);
    }
    // 无task则free对应的tick对象
    __os_tick_del_node(ptr_tick);
}

os_private os_handle_state_t __os_wakeup_tick_task(struct task_control_block *task,
                                                   void (*callback)(struct task_control_block *task))
{
    /*
     * There is no need to worry if the TICK node wakes up "task"
     * without any other task nodes attached,
     * as the TICK subsystem will automatically reclaim it.
     * */
    list_del_init(&(task->_bt_nd));
    callback(task);

    return OS_HANDLE_SUCCESS;
}

os_private void __tick_tcb_early_wakeup_cb(struct task_control_block *task)
{
    // task 目前处于 time out 状态
    task->_task_block_state = OS_TASK_BLOCK_EARLY_WAKEUP;
    // wake up from block list
    list_del_init(&(task->_slot_nd));
    // 加入就绪队列
    os_rq_add_task(task);
}

os_handle_state_t os_wakeup_tick_task(struct task_control_block *task)
{
    __OS_OWNED_ENTER_CRITICAL
    if (NULL == task ||
        list_empty(&(task->_bt_nd))) {
        __OS_OWNED_EXIT_CRITICAL
        return OS_HANDLE_FAIL;
    }
    os_handle_state_t ret_status = __os_wakeup_tick_task(task, __tick_tcb_early_wakeup_cb);
    __OS_OWNED_EXIT_CRITICAL
    return ret_status;
}

/* 任务 tick 轮询 */
static volatile unsigned int _in_crirical_poll_num;
void os_task_tick_poll(void)
{
    if (os_sys_owned_critical_status()) {
        if (list_empty(&_os_tick_list_head))
            return;

        struct os_tick *_current_tick_node = NULL;
        _current_tick_node = os_list_first_entry(&_os_tick_list_head, struct os_tick, _tick_list_nd);
        --(_current_tick_node->_tick_count);

        if (_in_crirical_poll_num > _current_tick_node->_tick_count) {
            _current_tick_node->_tick_count = 0;
            _in_crirical_poll_num -= _current_tick_node->_tick_count;
        } else {
            _current_tick_node->_tick_count -= _in_crirical_poll_num;
            _in_crirical_poll_num = 0;
        }

        if (_current_tick_node->_tick_count > 0) {
            return;
        }
        // 从 _os_tick_list_head 队列中移除
        __os_tick_del_all_task(_current_tick_node, __tick_tcb_time_out_cb);

        // Is there a ZERO after that
        struct list_head *_current_node;
        struct list_head *_next_node;
        /* 由于需要将_tick_count == 0的节点移除，
                                 这里需要使用list_for_each_safe以免指针丢失 */
        list_for_each_safe(_current_node, _next_node, &_os_tick_list_head)
        {
            _current_tick_node = os_list_entry(_current_node, struct os_tick, _tick_list_nd);

            if (_in_crirical_poll_num > _current_tick_node->_tick_count) {
                _current_tick_node->_tick_count = 0;
                _in_crirical_poll_num -= _current_tick_node->_tick_count;
            } else {
                _current_tick_node->_tick_count -= _in_crirical_poll_num;
                _in_crirical_poll_num = 0;
            }

            if (_current_tick_node->_tick_count > 0)
                break;

            // 从 _os_tick_list_head 队列中移除
            __os_tick_del_all_task(_current_tick_node, __tick_tcb_time_out_cb);
        }
        // Don't compromise the next roll of the task
        _in_crirical_poll_num = 0;
        // 调度
        __os_sched();
    } else {
        _in_crirical_poll_num++;
    }
}

os_handle_state_t os_task_delay_ms(unsigned int _tick_ms)
{
    if (_tick_ms == 0)
        return OS_HANDLE_SUCCESS;
    __OS_OWNED_ENTER_CRITICAL
    struct task_control_block *_current_task_tcb = os_get_current_task_tcb();
    // 加入延时队列
    os_add_tick_task(_current_task_tcb, _tick_ms, NULL);
    __OS_OWNED_EXIT_CRITICAL
    // 调度开启
    __os_sched();
    if (_current_task_tcb->_task_block_state ==
            OS_TASK_BLOCK_EARLY_WAKEUP) {
        _current_task_tcb->_task_block_state = OS_TASK_BLOCK_NONE;
        return OS_HANDLE_FAIL;
    }
    return OS_HANDLE_SUCCESS;
}
