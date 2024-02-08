/***********************
 * @file: os_mqueue.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2023-10-20     Feijie Luo   First version
 * @note:
 ***********************/

#include "os_mqueue.h"
#include "stddef.h"
#include "os_config.h"
#include "os_sched.h"
#include "os_tick.h"
#include "board/libcpu_headfile.h"
#include "components/memory/os_malloc.h"
#include "string.h"

struct mqueue_msg_pack{
    void* _buffer;
    unsigned short _size;
    struct list_head _q_nd;
};

os_handle_state_t os_mqueue_init(struct os_mqueue* mq, unsigned short mnum, unsigned short msize)
{
    if (NULL == mq ||
        0 == mnum  ||
        0 == msize)
        return OS_HANDLE_FAIL;
    mq->_capacity = mnum;
    mq->_msg_size = msize;
    mq->_num_msgs = 0;
    os_block_init(&mq->_suspend, OS_BLOCK_MQUEUE);
    list_head_init(&mq->_msg_queue);
    return OS_HANDLE_SUCCESS;
}

__os_static __os_inline void __os_mqueue_send_cb(struct task_control_block* task)
{
    // 将该任务从挂载的tick上摘掉
    list_del_init(&(task->_bt_nd));
}

os_handle_state_t os_mqueue_send(struct os_mqueue* mq,
                                 void* buffer, unsigned short msize,
                                 unsigned int time_out)
{
    if (NULL == mq            ||
        NULL == buffer        ||
        0 == msize            ||
        msize > mq->_msg_size)
        return OS_HANDLE_FAIL;

    unsigned int _critical_state = os_port_enter_critical();

    if (mq->_num_msgs >= mq->_capacity){

        if (time_out == OS_MQUEUE_NO_WAIT) {
            os_port_exit_critical(_critical_state);
            return OS_HANDLE_FAIL;
        }

        struct task_control_block* _current_task_tcb = os_get_current_task_tcb();
        // 将当前任务挂起
        os_add_tick_task(_current_task_tcb, time_out, &mq->_suspend);
        os_port_exit_critical(_critical_state);
        __os_sched();
        // 该处是为了给软中断异常被触发前留足时间
        while(os_task_is_block(_current_task_tcb)){};

        _critical_state = os_port_enter_critical();

        // 超时
        if (_current_task_tcb->_task_block_state == OS_TASK_BLOCK_TIMEOUT) {
            _current_task_tcb->_task_block_state = OS_TASK_BLOCK_NONE;
            os_port_exit_critical(_critical_state);
            return OS_HANDLE_FAIL;
        }else {
            _current_task_tcb->_task_block_state = OS_TASK_BLOCK_NONE;
            os_port_exit_critical(_critical_state);
            // 添加消息的操作在函数后部
        }
    }

    struct mqueue_msg_pack* mq_pack =
            (struct mqueue_msg_pack*)os_malloc(sizeof(struct mqueue_msg_pack));
    mq_pack->_buffer = os_malloc(msize);
    memcpy(mq_pack->_buffer, buffer, msize);
    mq_pack->_size = msize;
    // 添加进 mq
    list_add_tail(&mq->_msg_queue, &mq_pack->_q_nd);
    mq->_num_msgs++;
    // 唤醒
    os_block_wakeup_first_task(&mq->_suspend, __os_mqueue_send_cb);

    os_port_exit_critical(_critical_state);
    return OS_HANDLE_SUCCESS;
}

__os_static __os_inline void __os_mqueue_receive_cb(struct task_control_block* task)
{
    // 将该任务从挂载的tick上摘掉
    list_del_init(&(task->_bt_nd));
}

os_handle_state_t os_mqueue_receive(struct os_mqueue* mq,
                                    void* buffer, unsigned short msize,
                                    unsigned int time_out)
{
    if (NULL == mq            ||
        NULL == buffer        ||
        0 == msize            ||
        msize > mq->_msg_size)
        return OS_HANDLE_FAIL;

    unsigned int _critical_state = os_port_enter_critical();

    if (mq->_num_msgs == 0) {

        if (time_out == OS_MQUEUE_NO_WAIT) {
            os_port_exit_critical(_critical_state);
            return OS_HANDLE_FAIL;
        }

        struct task_control_block* _current_task_tcb = os_get_current_task_tcb();
        // 将当前任务挂起
        os_add_tick_task(_current_task_tcb, time_out, &mq->_suspend);
        os_port_exit_critical(_critical_state);
        __os_sched();
        // 该处是为了给软中断异常被触发前留足时间
        while(os_task_is_block(_current_task_tcb)){};

        _critical_state = os_port_enter_critical();

        // 超时
        if (_current_task_tcb->_task_block_state == OS_TASK_BLOCK_TIMEOUT) {
            _current_task_tcb->_task_block_state = OS_TASK_BLOCK_NONE;
            os_port_exit_critical(_critical_state);
            return OS_HANDLE_FAIL;
        }else {
            _current_task_tcb->_task_block_state = OS_TASK_BLOCK_NONE;
            os_port_exit_critical(_critical_state);
            // 接收消息的操作在函数后部
        }
    }

    struct mqueue_msg_pack* _getmsg = NULL;
    _getmsg = os_list_first_entry(&mq->_msg_queue, struct mqueue_msg_pack, _q_nd);
    memcpy(buffer, _getmsg->_buffer, msize);
    list_del_init(&_getmsg->_q_nd);
    os_free(_getmsg->_buffer);
    os_free(_getmsg);

    mq->_num_msgs--;

    // 唤醒
    os_block_wakeup_first_task(&mq->_suspend, __os_mqueue_receive_cb);

    os_port_exit_critical(_critical_state);
    return OS_HANDLE_SUCCESS;
}

os_handle_state_t os_mqueue_clear(struct os_mqueue* mq)
{
    if (NULL == mq)
        return OS_HANDLE_FAIL;
    OS_ENTER_CRITICAL;
    if (mq->_num_msgs == 0) {
        OS_EXIT_CRITICAL;
        return OS_HANDLE_SUCCESS;
    }

    while (mq->_num_msgs != 0) {
        struct mqueue_msg_pack* _getmsg = NULL;
        _getmsg = os_list_first_entry(&mq->_msg_queue, struct mqueue_msg_pack, _q_nd);
        list_del_init(&_getmsg->_q_nd);
        os_free(_getmsg->_buffer);
        os_free(_getmsg);

        mq->_num_msgs--;

        // 唤醒
        os_block_wakeup_first_task(&mq->_suspend, __os_mqueue_receive_cb);
        // 请务必注意这里被唤醒的进程并不知道它被唤醒是否是可以插入消息或可以读取消息，还是极端的被clear了
    }

    OS_EXIT_CRITICAL;
    return OS_HANDLE_SUCCESS;
}
