/***********************
 * @file: os_block.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2022-09-10     Feijie Luo   First version
 * 2023-10-13     Feijie Luo   Decouple from os_task_state
 * 2023-10-19     Feijie Luo   Fix bug in function os_task_is_block
 * @note:
 ***********************/
#include "os_sched.h"
#include "os_tick.h"
#include "os_block.h"
#include "os_config.h"
#include "os_def.h"
#include "os_core.h"

/* 将链节挂载到阻塞对象 */
__os_static void __os_block_list_add(struct os_block_object * _block_obj, struct task_control_block* _task_tcb)
{
    struct list_head* _block_node = &_block_obj->_list;
    struct list_head* _current_node = NULL;
    struct task_control_block* _current_tcb = NULL;
	
    // 根据优先级先后挂载, 优先级高在表头
    list_for_each(_current_node, _block_node)
    {
        _current_tcb = os_list_entry(_current_node, struct task_control_block, _slot_nd);
	if (_current_tcb->_task_priority > _task_tcb->_task_priority)
            break;
    }
    list_add(_current_node, &_task_tcb->_slot_nd);
}

/* 将链节挂载到tick阻塞对象 */
__os_static void __os_tick_block_list_add(struct os_block_object * _block_obj, struct task_control_block* _task_tcb)
{
    struct list_head* _block_node = &_block_obj->_list;
    struct list_head* _current_node = NULL;
    struct task_control_block* _current_tcb = NULL;

    // 根据优先级先后挂载, 优先级高在表头
    list_for_each(_current_node, _block_node)
    {
        _current_tcb = os_list_entry(_current_node, struct task_control_block, _bt_nd);
        if (_current_tcb->_task_priority > _task_tcb->_task_priority)
            break;
    }
    list_add(_current_node, &_task_tcb->_bt_nd);
}

/* 将线程tcb从阻塞类对象链表中移除 */
__os_static void __os_block_list_del(struct task_control_block* _task_tcb)
{
    list_del_init(&_task_tcb->_slot_nd);
}

/* 检测线程tcb是否已被挂载在某一个阻塞类对象中 */
__os_inline bool os_task_is_block(struct task_control_block* _task_tcb)
{
    return (_task_tcb->_task_state == OS_TASK_BLOCKING);
}

/* 初始化阻塞类对象 */
void os_block_init(struct os_block_object * _block_obj, os_block_type_t _block_type)
{
    _block_obj->_type = _block_type;
    list_head_init(&_block_obj->_list);
}

/* 阻塞类对象去初始化 */
void os_block_deinit(struct os_block_object * _block_obj)
{
    _block_obj->_type = OS_BLOCK_NONE;
    list_head_init(&_block_obj->_list);
}

/* 检测阻塞类对象链表是否为空 */
__os_inline bool os_block_list_is_empty(struct os_block_object * _block_obj)
{
    return (list_empty(&_block_obj->_list));
}

/* 
 *@func: 将线程挂载进阻塞队列
 *@note: 包含线程的状态更新
 */
os_handle_state_t os_add_block_task(struct task_control_block* _task_tcb, 
									struct os_block_object* _block_obj)
{
    if (NULL == _task_tcb ||
	NULL == _block_obj)
	return OS_HANDLE_FAIL;

    // 先将线程从优先队列中移除
    os_rq_del_task(_task_tcb);
    __os_block_list_add(_block_obj, _task_tcb);

    // 状态更新
    os_task_state_set_blocking(_task_tcb);

    return OS_HANDLE_SUCCESS;
}

/*
 *@func: 将线程挂载进tick阻塞队列
 *@note: 包含线程的状态更新
 */
os_handle_state_t os_add_tick_block_task(struct task_control_block* _task_tcb,
                                         struct os_block_object* _block_obj)
{
    if (NULL == _task_tcb ||
        NULL == _block_obj)
        return OS_HANDLE_FAIL;

    // 先将线程从优先队列中移除
    os_rq_del_task(_task_tcb);
    __os_tick_block_list_add(_block_obj, _task_tcb);

    // 状态更新
    os_task_state_set_blocking(_task_tcb);

    return OS_HANDLE_SUCCESS;
}

/*
 *@func: 将线程从阻塞类对象队列中唤醒
 */
os_handle_state_t os_block_wakeup_task(struct task_control_block* _task_tcb)
{
    if (NULL == _task_tcb)
        return OS_HANDLE_FAIL;
    if (os_task_is_block(_task_tcb))
        __os_block_list_del(_task_tcb);
    // 将线程加入就绪队列
    os_rq_add_task(_task_tcb);
    return OS_HANDLE_SUCCESS;
}

/*
 *@func: 将阻塞类对象链表中的第一个线程唤醒
 */
__os_inline void os_block_wakeup_first_task(struct os_block_object* _block_obj,
                                            void (*callback)(struct task_control_block* task))
{
    if (_block_obj->_list.next != &_block_obj->_list)
    {
        struct task_control_block* _tcb = os_list_entry(_block_obj->_list.next, struct task_control_block, _slot_nd);
	os_block_wakeup_task(_tcb);
        if (NULL != callback)
            callback(_tcb);
    }
}

/*
 *@func: 将阻塞类对象链表中的所有线程唤醒
 */
void os_block_wakeup_all_task(struct os_block_object* _block_obj)
{
    struct task_control_block* _task_tcb = NULL;
    struct list_head* _current_node = NULL;
    struct list_head* _next_ndoe = NULL;
    list_for_each_safe(_current_node, _next_ndoe, &_block_obj->_list)
    {
        _task_tcb = os_list_entry(_current_node, struct task_control_block, _slot_nd);
        os_block_wakeup_task(_task_tcb);
    }
}
