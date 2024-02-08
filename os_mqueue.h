/***********************
 * @file: os_mqueue.h
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2023-10-20     Feijie Luo   First version
 * @note:
 ***********************/

#ifndef _OS_MQUEUE_H_
#define _OS_MQUEUE_H_

#include "os_list.h"
#include "os_block.h"
#include "os_config.h"

#define OS_MQUEUE_NO_WAIT          (0)
#define OS_MQUEUE_NEVER_TIMEOUT    (OS_NEVER_TIME_OUT)

struct os_mqueue{
    // 消息的最大大小
    unsigned short _msg_size;
    // 消息队列的容量
    unsigned short _capacity;
    // 当前消息个数
    unsigned short _num_msgs;
    struct list_head _msg_queue;
    struct os_block_object _suspend;
};

os_handle_state_t os_mqueue_init(struct os_mqueue* mq, unsigned short mnum, unsigned short msize);
os_handle_state_t os_mqueue_send(struct os_mqueue* mq,
                                 void* buffer, unsigned short msize,
                                 unsigned int time_out);
os_handle_state_t os_mqueue_receive(struct os_mqueue* mq,
                                    void* buffer, unsigned short msize,
                                    unsigned int time_out);
os_handle_state_t os_mqueue_clear(struct os_mqueue* mq);
#endif
