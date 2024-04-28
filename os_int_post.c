/***********************
 * @file: os_int_post.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2024-04-27     Feijie Luo   First version
 * @note:
 ***********************/

#include "os_int_post.h"
#include "os_config.h"
#include "os_headfile.h"

static struct os_int_post_pack _int_post_objs[CONFIG_OS_INT_POST_NUM];
static volatile os_base_t _post_pos;
static volatile os_base_t _pull_pos;
static volatile os_base_t _lose_post_num;
static const os_base_t INT_POST_NUM = CONFIG_OS_INT_POST_NUM;
void os_int_post_init(void)
{
    os_memset(_int_post_objs, 0, CONFIG_OS_INT_POST_NUM * sizeof(struct os_int_post_pack));
    _post_pos = 0;
    _pull_pos = 0;
    _lose_post_num = 0;
}

void os_int_post(enum os_obj_type type, void* obj, void* msg, unsigned int msg_size)
{
    os_atomic_bge_set_strong(&_pull_pos, INT_POST_NUM, 0);
    os_base_t reserve = os_atomic_add(&_pull_pos, 1);
    _int_post_objs[reserve].type = type;
    _int_post_objs[reserve].obj = obj;
    _int_post_objs[reserve].msg = msg;
    _int_post_objs[reserve].msg_size = msg_size;
}
