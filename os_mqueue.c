/***********************
 * @file: os_mqueue.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2023-10-20     Feijie Luo   First version
 * @note:
 ***********************/

#include "board/libcpu_headfile.h"
#include "components/memory/os_malloc.h"
#include "os_config.h"
#include "os_mqueue.h"
#include "os_sched.h"
#include "os_tick.h"
#include "stddef.h"
#include "string.h"
#include "components/lib/os_string.h"

struct mqueue_msg_pack {
    void *_buffer;
    unsigned short _size;
    struct list_head _q_nd;
};

os_handle_state_t os_mqueue_init(struct os_mqueue *mq, unsigned short mnum, unsigned short msize)
{
    if (NULL == mq ||
        0 == mnum ||
        0 == msize)
        return OS_HANDLE_FAIL;
    mq->_capacity = mnum;
    mq->_msg_size = msize;
    mq->_num_msgs = 0;
    os_block_init(&mq->_suspend, OS_BLOCK_MQUEUE);
    list_head_init(&mq->_msg_queue);
    return OS_HANDLE_SUCCESS;
}

os_private inline void __os_mqueue_send_cb(struct task_control_block *task)
{
    // tick
    list_del_init(&(task->_bt_nd));
}

os_handle_state_t os_mqueue_send(struct os_mqueue *mq,
                                 const void *buffer, unsigned short msize,
                                 unsigned int time_out)
{
    if (NULL == mq ||
        NULL == buffer ||
        0 == msize ||
        msize > mq->_msg_size)
        return OS_HANDLE_FAIL;

    __OS_OWNED_ENTER_CRITICAL

    if (mq->_num_msgs >= mq->_capacity) {

        if (time_out == OS_MQUEUE_NO_WAIT) {
            __OS_OWNED_EXIT_CRITICAL
            return OS_HANDLE_FAIL;
        }

        struct task_control_block *_current_task_tcb = os_get_current_task_tcb();
        os_add_tick_task(_current_task_tcb, time_out, &mq->_suspend);
        __OS_OWNED_EXIT_CRITICAL
        __os_sched();
        while (os_task_is_block(_current_task_tcb)) {
        };

        __OS_OWNED_ENTER_CRITICAL

        if (_current_task_tcb->_task_block_state == OS_TASK_BLOCK_TIMEOUT) {
            _current_task_tcb->_task_block_state = OS_TASK_BLOCK_NONE;
            __OS_OWNED_EXIT_CRITICAL
            return OS_HANDLE_FAIL;
        } else {
            _current_task_tcb->_task_block_state = OS_TASK_BLOCK_NONE;
            __OS_OWNED_EXIT_CRITICAL
        }
    }

    struct mqueue_msg_pack *mq_pack =
        (struct mqueue_msg_pack *)os_kmalloc(sizeof(struct mqueue_msg_pack));
    OS_ASSERT(NULL != mq_pack);
    mq_pack->_buffer = os_kmalloc(msize);
    OS_ASSERT(NULL != mq_pack->_buffer);
    os_memcpy(mq_pack->_buffer, buffer, msize);
    mq_pack->_size = msize;
    list_add_tail(&mq->_msg_queue, &mq_pack->_q_nd);
    mq->_num_msgs++;
    os_block_wakeup_first_task(&mq->_suspend, __os_mqueue_send_cb);
    __OS_OWNED_EXIT_CRITICAL
    __os_sched();
    return OS_HANDLE_SUCCESS;
}

void __os_int_post_mqueue_send(struct os_mqueue *mq,
                               const void *buffer, unsigned short msize)
{
    if (NULL == mq ||
        NULL == buffer ||
        0 == msize ||
        mq->_num_msgs >= mq->_capacity ||
        msize > mq->_msg_size)
        return;

    struct mqueue_msg_pack *mq_pack =
        (struct mqueue_msg_pack *)os_kmalloc(sizeof(struct mqueue_msg_pack));
    OS_ASSERT(NULL != mq_pack);
    mq_pack->_buffer = os_kmalloc(msize);
    OS_ASSERT(NULL != mq_pack->_buffer);
    os_memcpy(mq_pack->_buffer, buffer, msize);
    mq_pack->_size = msize;
    list_add_tail(&mq->_msg_queue, &mq_pack->_q_nd);
    mq->_num_msgs++;
    os_block_wakeup_first_task(&mq->_suspend, __os_mqueue_send_cb);
}

os_private inline void __os_mqueue_receive_cb(struct task_control_block *task)
{
    // tick
    list_del_init(&(task->_bt_nd));
}

os_handle_state_t os_mqueue_receive(struct os_mqueue *mq,
                                    void *buffer, unsigned short msize,
                                    unsigned int time_out)
{
    if (NULL == mq ||
        NULL == buffer ||
        0 == msize ||
        msize > mq->_msg_size)
        return OS_HANDLE_FAIL;

    __OS_OWNED_ENTER_CRITICAL

    if (mq->_num_msgs == 0) {

        if (time_out == OS_MQUEUE_NO_WAIT) {
            __OS_OWNED_EXIT_CRITICAL
            return OS_HANDLE_FAIL;
        }

        struct task_control_block *_current_task_tcb = os_get_current_task_tcb();

        os_add_tick_task(_current_task_tcb, time_out, &mq->_suspend);
        __OS_OWNED_EXIT_CRITICAL
        __os_sched();

        while (os_task_is_block(_current_task_tcb)) {
        };

        __OS_OWNED_ENTER_CRITICAL

        if (_current_task_tcb->_task_block_state == OS_TASK_BLOCK_TIMEOUT) {
            _current_task_tcb->_task_block_state = OS_TASK_BLOCK_NONE;
            __OS_OWNED_EXIT_CRITICAL
            return OS_HANDLE_FAIL;
        } else {
            _current_task_tcb->_task_block_state = OS_TASK_BLOCK_NONE;
            __OS_OWNED_EXIT_CRITICAL
        }
    }

    struct mqueue_msg_pack *_getmsg = NULL;
    _getmsg = os_list_first_entry(&mq->_msg_queue, struct mqueue_msg_pack, _q_nd);
    os_memcpy(buffer, _getmsg->_buffer, msize);
    list_del_init(&_getmsg->_q_nd);
    os_kfree(_getmsg->_buffer);
    os_kfree(_getmsg);

    mq->_num_msgs--;

    os_block_wakeup_first_task(&mq->_suspend, __os_mqueue_receive_cb);
    __OS_OWNED_EXIT_CRITICAL
    __os_sched();
    return OS_HANDLE_SUCCESS;
}

os_handle_state_t os_mqueue_clear(struct os_mqueue *mq)
{
    if (NULL == mq)
        return OS_HANDLE_FAIL;
    __OS_OWNED_ENTER_CRITICAL;
    if (mq->_num_msgs == 0) {
        __OS_OWNED_EXIT_CRITICAL;
        return OS_HANDLE_SUCCESS;
    }

    while (mq->_num_msgs != 0) {
        struct mqueue_msg_pack *_getmsg = NULL;
        _getmsg = os_list_first_entry(&mq->_msg_queue, struct mqueue_msg_pack, _q_nd);
        list_del_init(&_getmsg->_q_nd);
        os_kfree(_getmsg->_buffer);
        os_kfree(_getmsg);

        mq->_num_msgs--;

        os_block_wakeup_first_task(&mq->_suspend, __os_mqueue_receive_cb);
        // clear
    }
    __OS_OWNED_EXIT_CRITICAL;
    __os_sched();
    return OS_HANDLE_SUCCESS;
}
