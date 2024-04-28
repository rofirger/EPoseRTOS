/***********************
 * @file: os_int_post.h
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2024-04-27     Feijie Luo   First version
 * @note:
 ***********************/

#ifndef _OS_INT_POST_H_
#define _OS_INT_POST_H_

#include "os_list.h"
enum os_obj_type {
    OS_OBJ_NONE = 0,
    OS_OBJ_MQUEUE,
    OS_OBJ_SEM,
};

struct os_int_post_pack {
    enum os_obj_type type;
    void *obj;
    void *msg;
    unsigned int msg_size;
};

void os_int_post_init(void);
void os_int_post(enum os_obj_type type, void* obj, void* msg, unsigned int msg_size);

#endif /* _OS_INT_POST_H_ */
