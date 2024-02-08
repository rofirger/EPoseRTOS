/***********************
 * @file: os_service.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2023-10-19     Feijie Luo   First version
 * 2023-10-24     Feijie Luo   Add Fish(Friendly Interactive SHell) service
 * 2023-10-31     Feijie Luo   Fix cmd's size bug in
 *                                cmd_call_ptr cmd_call = __os_get_cmd_call(argv[0], strlen(cmd));
 * @note:
 ***********************/

#include "os_service.h"
#include "stdio.h"
#include "stdarg.h"
#include "os_sched.h"
#include "os_mqueue.h"
#include "os_list.h"
#include "string.h"
#include "os_core.h"
#include "board/os_board.h"
#include "board/libcpu_headfile.h"
#include "components/memory/os_malloc.h"
#include "os_device.h"

#ifdef CONFIG_FISH

struct os_fish_inp_nd {
    struct os_service_fish_input* data_pack;
    struct list_head list_nd;
};
static LIST_HEAD(__os_service_tin_list);
static unsigned char __os_service_tin_list_len;
static struct os_mqueue __os_fish_inp_mq;
static struct os_mqueue __os_fish_step_inp_mq;

static fish_irq_handle fish_irq_handle_vector[FISH_IRQ_HANDLE_SIZE];
static enum os_fish_irq_handle fish_irq_handle_vec_index;

#define FISH_TASK_PRIO            (OS_KTHREAD_FISH_PRIO)
#define FISH_TASK_STACK_SIZE      (3072)
static struct task_control_block _fish_tcb;
static unsigned char fish_task_stack[FISH_TASK_STACK_SIZE];

// 链接文件(ld, sct, scf...)提供的地址
#if defined(__CC_ARM) || defined(__CLANG_ARM)
extern const int oscmdtable$$Base;
extern const int oscmdtable$$Limit;
#elif defined (__GNUC__)
extern const int __os_fish_cmd_table_begin;
extern const int __os_fish_cmd_table_end;
#endif

struct os_fish_cmd_structure* cmd_addr_begin;
struct os_fish_cmd_structure* cmd_addr_end;

#endif

void os_printk(const char* fmt, ...)
{
#ifdef CONFIG_PRINTK
    va_list args;
    va_start(args, fmt);
    os_diable_sched();
    vprintf(fmt, args);
    os_enable_sched();
    va_end(args);
#endif
}

__os_inline void os_printk_flush()
{
    fflush(stdout);
}

#ifdef CONFIG_FISH

__os_static cmd_call_ptr __os_get_cmd_call(char* cmd, unsigned int size)
{
    struct os_fish_cmd_structure* cmd_structure;
    cmd_call_ptr cmd_call = NULL;

    for (cmd_structure = cmd_addr_begin;
         cmd_structure < cmd_addr_end;
         cmd_structure = (struct os_fish_cmd_structure*)((unsigned int)((char*)cmd_structure) +
                 sizeof(struct os_fish_cmd_structure))) {
        if (strncmp(cmd_structure->cmd_name, "__os_cmd_", 9) != 0)
            continue;
        if (strncmp(&(cmd_structure->cmd_name[9]), cmd, size) == 0 &&
                cmd_structure->cmd_name[9 + size] == '\0') {
            cmd_call = (cmd_call_ptr)cmd_structure->cmd_call;
            break;
        }
    }
    return cmd_call;
}

__os_static unsigned int __os_split_cmd(char* cmd,
                                        unsigned int size,
                                        char* argv[OS_SERVICE_FISH_ARG_MAX])
{
    unsigned short pos = 0;
    unsigned short arv_index = 0;
    while (pos < size) {
        while ((*cmd == ' ' || *cmd == '\t') &&
                pos < size) {
            *cmd = '\0';
            cmd++; pos++;
        }

        if (pos >= size)
            break;

        argv[arv_index++] = cmd;

        if (arv_index >= OS_SERVICE_FISH_ARG_MAX) {
            os_printk("Argument too much. Limit:%d\r\n", OS_SERVICE_FISH_ARG_MAX);
            return 0;
        }

        while (*cmd != ' ' && *cmd != '\t') {
            cmd++; pos++;
            if (pos >= size)
                break;
        }
    }
    return arv_index;
}

os_handle_state_t os_exec_cmd(char* cmd, unsigned int size, int* cmd_ret_val)
{
    while (*cmd == ' ' || *cmd == '\t') {
        cmd++;
        size--;
    }

    if (size == 0)
        return OS_HANDLE_FAIL;

    // 提取command [arg1] [arg2] ...
    char *argv[OS_SERVICE_FISH_ARG_MAX];
    memset(argv, 0x00, sizeof(argv));
    unsigned int argc = __os_split_cmd(cmd, size, argv);
    if (0 == argc)
        return OS_HANDLE_FAIL;

    cmd_call_ptr cmd_call = __os_get_cmd_call(argv[0], strlen(cmd));

    if (cmd_call == NULL)
        return OS_HANDLE_FAIL;

    *cmd_ret_val = cmd_call(argc - 1, &argv[1]);

    return OS_HANDLE_SUCCESS;
}

__os_static void __tin_list_add_tail(struct os_service_fish_input* inp)
{
    // 先判断当前链表中的信息个数是否超过最大值
    if (__os_service_tin_list_len >= OS_SERVICE_FISH_MSG_LIST_CAP) {
        // 删除队列头的第一个节点
        struct os_fish_inp_nd* tmp =
                os_list_entry(__os_service_tin_list.next, struct os_fish_inp_nd, list_nd);
        os_free(tmp->data_pack);
        list_del(&tmp->list_nd);
        os_free(tmp);
        __os_service_tin_list_len--;
    }

    struct os_fish_inp_nd* nd =
            (struct os_fish_inp_nd*)os_malloc(sizeof(struct os_fish_inp_nd));
    nd->data_pack = inp;
    list_add_tail(&__os_service_tin_list, &nd->list_nd);
    __os_service_tin_list_len++;
}

__os_static void __tin_list_clear(void)
{
    struct list_head* current_pos = NULL;
    struct list_head* next_pos = NULL;
    list_for_each_safe(current_pos, next_pos, &__os_service_tin_list) {
        struct os_fish_inp_nd* tmp =
                os_list_entry(current_pos, struct os_fish_inp_nd, list_nd);
        list_del_init(&tmp->list_nd);
        os_free(tmp->data_pack);
        os_free(tmp);
        __os_service_tin_list_len--;
    }
}

static void __kthread_terminal(void* _arg)
{
    struct os_service_fish_input* _inp;
    while (1)
    {
        _inp = (struct os_service_fish_input*)os_malloc(sizeof(struct os_service_fish_input));
        if (OS_HANDLE_SUCCESS ==
                os_mqueue_receive(os_get_fish_input_mq(), (void*)_inp,
                        sizeof(struct os_service_fish_input), OS_MQUEUE_NEVER_TIMEOUT)) {
            os_printk("\r\n");
            int ret_val;
            if (os_exec_cmd(_inp->data, _inp->size, &ret_val) == OS_HANDLE_SUCCESS) {
                __tin_list_add_tail(_inp);
                // os_printk(":Command return value:%d\r\n", ret_val);
            }
            os_printk(">"); os_printk_flush();
        }
    }
}

extern os_size_t sys_uart_write(struct os_device* dev, os_off_t pos, const void* buffer, os_size_t size);
__os_static os_handle_state_t os_fish_irq_handle_default_fn(unsigned int rec)
{
    static struct os_service_fish_input _uart_irq_get;
    switch (rec) {
    case 13:   // 回车
        os_mqueue_send(os_get_fish_input_mq(),(void*)(&_uart_irq_get),
                sizeof(struct os_service_fish_input), OS_MQUEUE_NO_WAIT);
        memset(_uart_irq_get.data, 0, OS_SERVICE_FISH_MSG_MAX_SIZE);
        _uart_irq_get.size = 0;
        // 回车换行打印在消息处理
        break;
    case 127:   // 回退
        if (_uart_irq_get.size == 0)
            break;
        _uart_irq_get.data[_uart_irq_get.size] = 0;
        _uart_irq_get.size--;
        sys_uart_write(os_get_sys_uart_device_handle(), 0, (void*)&rec, 2);
        break;
    case 38: // 方位键：上
        break;
    case 40: // 方位键：下
        break;
    case 37: // 方位键：左
        break;
    case 39: // 方位键：右
        break;
    default:
        _uart_irq_get.data[_uart_irq_get.size] = rec;
        _uart_irq_get.size++;
        sys_uart_write(os_get_sys_uart_device_handle(), 0, (void*)&rec, 2);
        break;
    }
    return OS_HANDLE_SUCCESS;
}

os_handle_state_t os_fish_irq_handle_callback(unsigned int rec)
{
#ifdef CONFIG_FISH
    if (fish_irq_handle_vector[os_fish_get_now_irq_handle()] != NULL)
        return (fish_irq_handle_vector[os_fish_get_now_irq_handle()])(rec);
    else
        return OS_HANDLE_FAIL;
#else
    return OS_HANDLE_FAIL;
#endif
}

__os_static os_handle_state_t os_fish_irq_handle_step_fn(unsigned int rec)
{
    unsigned char get_rec = (unsigned char)rec;
    os_mqueue_send(os_get_fish_step_input_mq(), (void*)&get_rec,
            sizeof(unsigned char), OS_MQUEUE_NO_WAIT);
    return OS_HANDLE_SUCCESS;
}

bool os_fish_change_irq_handle(enum os_fish_irq_handle new_handle)
{
#ifdef CONFIG_FISH
    if (new_handle >= 0 &&
            new_handle < FISH_IRQ_HANDLE_SIZE) {
        fish_irq_handle_vec_index = new_handle;
        return true;
    }
#endif
    return false;
}

__os_inline enum os_fish_irq_handle os_fish_get_now_irq_handle(void)
{
#ifdef CONFIG_FISH
    return fish_irq_handle_vec_index;
#else
    return OS_FISH_IRQ_HANDLE_NONE;
#endif
}

__os_static OS_CMD_PROCESS_FN(hello_world)
{
    os_printk("Hello world!\r\n");
    return 0;
}

__os_static OS_CMD_PROCESS_FN(read_cmd_cap)
{
    return __os_service_tin_list_len;
}

__os_static OS_CMD_PROCESS_FN(clear_cmd_history)
{
    __tin_list_clear();
    return __os_service_tin_list_len;
}

__os_static OS_CMD_PROCESS_FN(list_cmd)
{
    struct os_fish_cmd_structure* cmd_structure;
    const char* str;
    int num = 0;
    for (cmd_structure = cmd_addr_begin;
         cmd_structure < cmd_addr_end;
         cmd_structure = (struct os_fish_cmd_structure*)((unsigned int)((char*)cmd_structure) +
                 sizeof(struct os_fish_cmd_structure))) {
            str = &(cmd_structure->cmd_name[9]);
            os_printk("%s  ", str);
            num++;
    }
    os_printk("\r\n");
    return num;
}

OS_CMD_EXPORT(hello, hello_world);
OS_CMD_EXPORT(readcmdcap, read_cmd_cap);
OS_CMD_EXPORT(clearcmd, clear_cmd_history);
OS_CMD_EXPORT(listcmd, list_cmd);
#endif

__os_inline struct os_mqueue* os_get_fish_input_mq(void)
{
#ifdef CONFIG_FISH
    return &__os_fish_inp_mq;
#else
    return NULL;
#endif
}

__os_inline struct os_mqueue* os_get_fish_step_input_mq(void)
{
#ifdef CONFIG_FISH
    return &__os_fish_step_inp_mq;
#else
    return NULL;
#endif
}

void os_service_init(void)
{
#ifdef CONFIG_FISH
#if defined(__CC_ARM) || defined(__CLANG_ARM)
    cmd_addr_begin = (struct os_fish_cmd_structure*)&oscmdtable$$Base;
    cmd_addr_end = (struct os_fish_cmd_structure*)&oscmdtable$$Limit;
#elif defined (__GNUC__)
    cmd_addr_begin = (struct os_fish_cmd_structure*)&__os_fish_cmd_table_begin;
    cmd_addr_end = (struct os_fish_cmd_structure*)&__os_fish_cmd_table_end;
#endif
    os_mqueue_init(&__os_fish_inp_mq, 5, sizeof(struct os_service_fish_input));
    os_mqueue_init(&__os_fish_step_inp_mq, 5, sizeof(struct os_service_fish_input));
    os_task_create(&_fish_tcb, (void*)fish_task_stack,
            FISH_TASK_STACK_SIZE, FISH_TASK_PRIO,
            __kthread_terminal, NULL, "KERNEL FISH TASK");
    fish_irq_handle_vector[OS_FISH_IRQ_HANDLE_NONE] = NULL;
    fish_irq_handle_vector[OS_FISH_IRQ_HANDLE_DEFAULT] = os_fish_irq_handle_default_fn;
    fish_irq_handle_vector[OS_FISH_IRQ_HANDLE_STEP] = os_fish_irq_handle_step_fn;
    os_fish_change_irq_handle(OS_FISH_IRQ_HANDLE_DEFAULT);
#endif
}




