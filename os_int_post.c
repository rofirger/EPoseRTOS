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
static volatile os_base_t _post_pos  __OS_ALIGNED__(4);
static volatile os_base_t _pull_pos  __OS_ALIGNED__(4);
static volatile os_base_t _lose_post_num __OS_ALIGNED__(4);
void os_int_post_init(void)
{
    os_memset(_int_post_objs, 0, CONFIG_OS_INT_POST_NUM * sizeof(struct os_int_post_pack));
    _post_pos = 0;
    _pull_pos = 0;
    _lose_post_num = 0;
}

void os_int_post(enum os_obj_type type, void* obj, void* msg, unsigned int msg_size)
{
    os_base_t reserve = os_atomic_add_bge_set_strong(&_pull_pos, 1, CONFIG_OS_INT_POST_NUM, 0);
    _int_post_objs[reserve].type = type;
    _int_post_objs[reserve].obj = obj;
    _int_post_objs[reserve].msg = msg;
    _int_post_objs[reserve].msg_size = msg_size;
}
