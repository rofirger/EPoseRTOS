/***********************
 * @file: os_mutex.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2022-09-10     Feijie Luo   First version
 * 2023-10-13     Feijie Luo   Add os_mutex_lock time out. Fix priority bug in __mutex_owner_change
 * 2023-10-18     Feijie Luo   Change os_mutex_lock return value type
 * 2023-10-18     Feijie Luo   Fix `self bug in function os_mutex_try_lock
 * @note:
 ***********************/

#include "os_block.h"
#include "os_core.h"
#include "os_mutex.h"
#include "os_sched.h"
#include "os_tick.h"

#include "board/libcpu_headfile.h"

#include "stddef.h"

/*
 *@func: 将锁的拥有者(任务)的优先级提高，防止出现优先级反转
 */
__os_static __os_inline void __mutex_owner_prio_up(struct os_mutex *_mutex, unsigned int _prio)
{
    _mutex->_mutex_owner->_task_priority = _prio;
}

/*
 *@func: 更改锁的拥有者(任务)
 */
__os_static void __mutex_owner_change(struct os_mutex *_mutex, struct task_control_block *_task_tcb)
{
    // 恢复原来锁拥有者的优先级
    _mutex->_mutex_owner->_task_priority = _mutex->_owner_prio;

    _mutex->_lock_nesting = 1;
    _mutex->_owner_prio = _task_tcb->_task_priority;
    _mutex->_mutex_owner = _task_tcb;
}

/*
 *@func: 检测锁是否存在拥有者(任务)
 */
__os_inline bool os_mutex_is_owned(struct os_mutex *_mutex)
{
    return (NULL != _mutex->_mutex_owner);
}

/*
 *@func: 检测锁的拥有者是否是当前任务
 */
__os_inline bool os_mutex_is_self(struct os_mutex *_mutex)
{
    return (_mutex->_mutex_owner == os_get_current_task_tcb());
}

/*
 *@func: 检测锁是否允许递归
 */
__os_inline bool os_mutex_is_recursive(struct os_mutex *_mutex)
{
    return (_mutex->_mutex_type == OS_MUTEX_RECURSIVE);
}

/*
 *@func: 释放锁拥有者
 */
__os_static __os_inline void __os_mutex_owner_release(struct os_mutex *_mutex)
{
    // 恢复原来锁拥有者的优先级
    _mutex->_mutex_owner->_task_priority = _mutex->_owner_prio;

    _mutex->_mutex_owner = NULL;
}

/*
 *@func: 锁链表是否存在任务
 */
__os_inline bool os_mutex_block_is_empty(struct os_mutex *_mutex)
{
    return (list_empty(&_mutex->_block_obj._list));
}

/*
 *@func: 尝试将当前任务加锁, 如果该锁被其他任务所拥有则返回 OS_MUTEX_HANDLE_OTHER_OWNER
 */
os_mutex_handle_state_t os_mutex_try_lock(struct os_mutex *_mutex)
{
    if (NULL == _mutex)
        return OS_MUTEX_HANDLE_LOCK_FAIL;
    OS_ENTER_CRITICAL

    // 如果锁不存在拥有者，则将当前运行的任务置为该锁的拥有者
    if (!os_mutex_is_owned(_mutex)) {
        __mutex_owner_change(_mutex, os_get_current_task_tcb());
        OS_EXIT_CRITICAL
        return OS_MUTEX_HANDLE_GET_OWNER;
    }
    if (os_mutex_is_self(_mutex)) {
        if (os_mutex_is_recursive(_mutex)) {
            if (_mutex->_lock_nesting < OS_MUTEX_MAX_RECURSIVE) {
                _mutex->_lock_nesting++;
                OS_EXIT_CRITICAL
                return OS_MUTEX_HANDLE_RECURSIVE_SUECCESS;
            }
            OS_EXIT_CRITICAL
            return OS_MUTEX_HANDLE_RECURSIVE_FAIL;
        } else {
            OS_EXIT_CRITICAL
            return OS_MUTEX_HANDLE_GET_OWNER;
        }
    }

    OS_EXIT_CRITICAL
    return OS_MUTEX_HANDLE_OTHER_OWNER;
}

/*
 *@func: 将当前任务上锁.
 */
os_handle_state_t os_mutex_lock(struct os_mutex *_mutex, unsigned int time_out)
{
    if (NULL == _mutex)
        return OS_HANDLE_FAIL;

    OS_ENTER_CRITICAL

    os_mutex_handle_state_t _mutex_handle_state;
    _mutex_handle_state = os_mutex_try_lock(_mutex);
    if (_mutex_handle_state == OS_MUTEX_HANDLE_OTHER_OWNER) {
        struct task_control_block *_current_task_tcb = os_get_current_task_tcb();
        // 锁已被其他任务锁占用
        // 判断锁的任务优先级是否低于当前任务的优先级，
        // 如果小于则改变锁的优先级，以防止优先级反转
        if (_mutex->_owner_prio > _current_task_tcb->_task_priority)
            __mutex_owner_prio_up(_mutex, _current_task_tcb->_task_priority);
        // 将当前任务挂起
        os_add_tick_task(_current_task_tcb, time_out, &(_mutex->_block_obj));

        OS_EXIT_CRITICAL
        // 展开调度
        __os_sched();

        // 该处是为了给软中断异常被触发前留足时间
        while (os_task_is_block(_current_task_tcb)) {
        };

        OS_ENTER_CRITICAL

        // 超时
        if (_current_task_tcb->_task_block_state == OS_TASK_BLOCK_TIMEOUT) {
            _current_task_tcb->_task_block_state = OS_TASK_BLOCK_NONE;
            OS_EXIT_CRITICAL
            return OS_HANDLE_FAIL;
        } else {
            _current_task_tcb->_task_block_state = OS_TASK_BLOCK_NONE;
            __mutex_owner_change(_mutex, _current_task_tcb);
            OS_EXIT_CRITICAL
            return OS_HANDLE_SUCCESS;
        }
    }

    OS_EXIT_CRITICAL
    if (_mutex_handle_state == OS_MUTEX_HANDLE_GET_OWNER ||
        _mutex_handle_state == OS_MUTEX_HANDLE_RECURSIVE_SUECCESS)
        return OS_HANDLE_SUCCESS;
    else
        return OS_HANDLE_FAIL;
}

__os_static __os_inline void __os_mutex_wakeup_task_cb(struct task_control_block *tcb)
{
    // 将该任务从挂载的tick上摘掉
    list_del_init(&(tcb->_bt_nd));
}

/*
 *@func: 将当前任务解锁
 */
os_handle_state_t os_mutex_unlock(struct os_mutex *_mutex)
{
    if (NULL == _mutex)
        return OS_HANDLE_FAIL;

    OS_ENTER_CRITICAL

    // 如果锁不为自己所有, 则返回
    if (!os_mutex_is_self(_mutex)) {
        OS_EXIT_CRITICAL
        return OS_HANDLE_FAIL;
    }

    if (_mutex->_mutex_type == OS_MUTEX_RECURSIVE && --_mutex->_lock_nesting > 0) {
        OS_EXIT_CRITICAL
        return OS_HANDLE_SUCCESS;
    }

    __os_mutex_owner_release(_mutex);

    // 查看锁链表上是否存在任务
    if (os_mutex_block_is_empty(_mutex)) {
        OS_EXIT_CRITICAL
        return OS_HANDLE_SUCCESS;
    }
    // 唤醒锁阻塞队列中的第一个任务
    os_block_wakeup_first_task(&_mutex->_block_obj, __os_mutex_wakeup_task_cb);
    OS_EXIT_CRITICAL
    // 调度
    __os_sched();
    return OS_HANDLE_SUCCESS;
}

/*
 *@func: 销毁锁
 */
os_mutex_handle_state_t os_mutex_destory(struct os_mutex *_mutex)
{
    if (NULL == _mutex)
        return OS_MUTEX_HANDLE_DESTORY_FAIL;
    OS_ENTER_CRITICAL

    // 如果该锁被任务所拥有则释放锁主
    if (os_mutex_is_owned(_mutex))
        __os_mutex_owner_release(_mutex);

    // 释放锁阻塞队列的所有任务
    if (!os_mutex_block_is_empty(_mutex))
        os_block_wakeup_all_task(&_mutex->_block_obj);
    os_block_deinit(&_mutex->_block_obj);
    OS_EXIT_CRITICAL
    // 调度
    __os_sched();
    return OS_MUTEX_HANDLE_DESTORY_SUCCESS;
}

/*
 *@func: 初始化锁
 */
os_mutex_handle_state_t os_mutex_init(struct os_mutex *_mutex, os_mutex_type_t _type)
{
    if (NULL == _mutex)
        return OS_MUTEX_HANDLE_INIT_FAIL;
    _mutex->_mutex_owner = NULL;
    _mutex->_mutex_type = _type;
    _mutex->_lock_nesting = 0;
    _mutex->_owner_prio = OS_MUTEX_PRIO_LOWEST;
    os_block_init(&_mutex->_block_obj, OS_BLOCK_MUTEX);
    return OS_MUTEX_HANDLE_INIT_SUCCESS;
}
