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

#include "board/libcpu_headfile.h"
#include "board/os_board.h"
#include "components/memory/os_malloc.h"
#include "os_core.h"
#include "os_device.h"
#include "os_list.h"
#include "os_mqueue.h"
#include "os_sched.h"
#include "os_service.h"
#include "stdarg.h"
#include "string.h"
#include "os_soft_timer.h"
#include "os_config.h"
#include "components/lib/os_string.h"

#ifdef CONFIG_FISH

// History command list node.
struct os_fish_inp_nd {
    struct os_service_fish_input *data_pack;
    struct list_head list_nd;
};
// History command list
static LIST_HEAD(__os_service_tin_list);
static struct list_head* __os_service_tin_list_tail;
static unsigned char __os_service_tin_list_len;

// Every complete command(End with the 'CR (Carriage Return)' key) can be sent to this mq.
static struct os_mqueue __os_fish_inp_mq;
// Every typed key can be sent to this mq.
static struct os_mqueue __os_fish_step_inp_mq;

// Callback functions used to handle input keys.
static fish_irq_handle fish_irq_handle_vector[FISH_IRQ_HANDLE_SIZE];
static enum os_fish_irq_handle fish_irq_handle_vec_index;

// input buffer
static struct os_service_fish_input input_buffer;

// FISH thread configuration.
#define FISH_TASK_PRIO       (OS_KTHREAD_FISH_PRIO)
#define FISH_TASK_STACK_SIZE (3072)
static struct task_control_block _fish_tcb;
static unsigned char fish_task_stack[FISH_TASK_STACK_SIZE];

// (ld, sct, scf...)
#if defined(__CC_ARM) || defined(__CLANG_ARM)
extern const int oscmdtable$$Base;
extern const int oscmdtable$$Limit;
#elif defined(__GNUC__)
extern const int __os_fish_cmd_table_begin;
extern const int __os_fish_cmd_table_end;
#endif

struct os_fish_cmd_structure *cmd_addr_begin;
struct os_fish_cmd_structure *cmd_addr_end;

#endif

#define ZEROPAD     1  /* pad with zero */
#define SIGN        2  /* unsigned/signed long */
#define PLUS        4  /* show plus */
#define SPACE       8  /* space if plus */
#define LEFT        16 /* left justified */
#define SPECIAL     32 /* 0x */
#define SMALL       64 /* use 'abcdef' instead of 'ABCDEF' */
#define is_digit(c) ((c) >= '0' && (c) <= '9')

static int skip_atoi(const char **s)
{
    int i = 0;
    while (is_digit(**s)) {
        i = i * 10 + *((*s)++) - '0';
    }
    return i;
}

int do_div(unsigned long *n, int base)
{
    int res;
    res = (int)((*n) % base);
    *n = (long)((*n) / base);
    return res;
}

static char *number(char *str,
                    int num,
                    int base,
                    int size,
                    int precision,
                    int type)
{
    char c, sign, tmp[36];
    const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int i;

    if (type & SMALL) { /* 小写字母集 */
        digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    }
    if (type & LEFT) { /* 左调整(靠左边界),则屏蔽类型中的填零标志 */
        type &= ~ZEROPAD;
    }
    if (base < 2 || base > 36) { /* 本程序只能处理基数在2-36之间的数 */
        return 0;
    }
    c = (type & ZEROPAD) ? '0' : ' ';
    if ((type & SIGN) && num < 0) {
        sign = '-';
        num = -num;
    } else {
        sign = (type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
    }
    if (sign) {
        size--;
    }
    if (type & SPECIAL) {
        if (base == 16) {
            size -= 2;
        } else if (base == 8) {
            size--;
        }
    }
    i = 0;
    if (num == 0) {
        tmp[i++] = '0';
    } else {
        while (num != 0) {
            tmp[i++] = digits[do_div((unsigned long *)&num, base)];
        }
    }
    if (i > precision) {
        precision = i;
    }
    size -= precision;
    if (!(type & (ZEROPAD + LEFT))) {
        while (size-- > 0) {
            *str++ = ' ';
        }
    }
    if (sign) {
        *str++ = sign;
    }
    if (type & SPECIAL) {
        if (base == 8) {
            *str++ = '0';
        } else if (base == 16) {
            *str++ = '0';
            *str++ = digits[33];
        }
    }
    if (!(type & LEFT)) {
        while (size-- > 0) {
            *str++ = c;
        }
    }
    while (i < precision--) {
        *str++ = '0';
    }
    while (i-- > 0) {
        *str++ = tmp[i];
    }
    while (size-- > 0) {
        *str++ = ' ';
    }
    return str;
}

int os_vsprintf(char *buf, const char *fmt, va_list args)
{
    int len;
    int i;
    char *str;
    char *s;
    int *ip;

    int flags; /* flags to number() */

    int field_width; /* width of output field */
    int precision;   /* min. # of digits for integers; max
                number of chars for from string */
    //int qualifier;   /* 'h', 'l', or 'L' for integer fields */

    for (str = buf; *fmt; ++fmt) {

        if (*fmt != '%') {
            *str++ = *fmt;
            continue;
        }

        /* process flags */
        flags = 0;
    repeat:
        ++fmt; /* this also skips first '%' */
        switch (*fmt) {
        case '-':
            flags |= LEFT;
            goto repeat;
        case '+':
            flags |= PLUS;
            goto repeat;
        case ' ':
            flags |= SPACE;
            goto repeat;
        case '#':
            flags |= SPECIAL;
            goto repeat;
        case '0':
            flags |= ZEROPAD;
            goto repeat;
        }

        /* get field width */
        field_width = -1;
        if (is_digit(*fmt))
            field_width = skip_atoi(&fmt);
        else if (*fmt == '*') {
            /* it's the next argument */
            field_width = va_arg(args, int);
            if (field_width < 0) {
                field_width = -field_width;
                flags |= LEFT;
            }
            ++fmt;
        }

        /* get the precision */
        precision = -1;
        if (*fmt == '.') {
            ++fmt;
            if (is_digit(*fmt))
                precision = skip_atoi(&fmt);
            else if (*fmt == '*') {
                /* it's the next argument */
                precision = va_arg(args, int);
            }
            if (precision < 0)
                precision = 0;
        }

        /* get the conversion qualifier */
        //qualifier = -1;
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
            //qualifier = *fmt;
            ++fmt;
        }

        switch (*fmt) {
        case 'c':
            if (!(flags & LEFT))
                while (--field_width > 0)
                    *str++ = ' ';
            *str++ = (unsigned char)va_arg(args, int);
            while (--field_width > 0)
                *str++ = ' ';
            break;

        case 's':
            s = va_arg(args, char *);
            len = strlen(s);
            if (precision < 0)
                precision = len;
            else if (len > precision)
                len = precision;

            if (!(flags & LEFT))
                while (len < field_width--)
                    *str++ = ' ';
            for (i = 0; i < len; ++i)
                *str++ = *s++;
            while (len < field_width--)
                *str++ = ' ';
            break;

        case 'o':
            str = number(str, va_arg(args, unsigned long), 8,
                         field_width, precision, flags);
            break;

        case 'p':
            if (field_width == -1) {
                field_width = 8;
                flags |= ZEROPAD;
            }
            str = number(str,
                         (unsigned long)va_arg(args, void *), 16,
                         field_width, precision, flags);
            break;

        case 'x':
            flags |= SMALL; // @suppress("No break at end of case")
        case 'X':
            str = number(str, va_arg(args, unsigned long), 16,
                         field_width, precision, flags);
            break;

        case 'd':
        case 'i':
            flags |= SIGN; // @suppress("No break at end of case")
        case 'u':
            str = number(str, va_arg(args, unsigned long), 10,
                         field_width, precision, flags);
            break;

        case 'n':
            ip = va_arg(args, int *);
            *ip = (str - buf);
            break;

        default:
            if (*fmt != '%') {
                *str++ = '%';
            }
            if (*fmt) {
                *str++ = *fmt;
            } else {
                --fmt;
            }
            break;
        }
    }
    *str = '\0';
    return str - buf;
}

static char __os_printk_buf[CONFIG_OS_PRINTK_BUF_SIZE];
void os_printk(const char *fmt, ...)
{
#ifdef CONFIG_PRINTK
    va_list args;
    va_start(args, fmt);
    os_diable_sched();
    os_vsprintf(__os_printk_buf, fmt, args);
    os_enable_sched();
    va_end(args);
    os_device_write(os_get_sys_uart_device_handle(), 0, __os_printk_buf, strlen(__os_printk_buf));
#endif
}

inline void os_printk_flush()
{
    sys_uart_write_flush();
}

#ifdef CONFIG_FISH

void os_fish_clear_input_buffer(bool is_clear_screen)
{
    if (input_buffer.size == 0)
        return;
    static unsigned char del_char = 127;
    static struct os_device *handle;
    handle = os_get_sys_uart_device_handle();
    if (is_clear_screen) {
        while(--input_buffer.size > 0)
            os_device_write(handle, 0, (void *)&del_char, 1);
        os_device_write(handle, 0, (void *)&del_char, 1);
    }
    else
      input_buffer.size = 0;
}

os_private cmd_call_ptr __os_get_cmd_call(char *cmd, unsigned int size)
{
    struct os_fish_cmd_structure *cmd_structure;
    cmd_call_ptr cmd_call = NULL;

    for (cmd_structure = cmd_addr_begin;
         cmd_structure < cmd_addr_end;
         cmd_structure = (struct os_fish_cmd_structure *)((unsigned int)((char *)cmd_structure) +
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

os_private unsigned int __os_split_cmd(char *cmd,
                                        unsigned int size,
                                        char *argv[OS_SERVICE_FISH_ARG_MAX])
{
    unsigned short pos = 0;
    unsigned short arv_index = 0;
    while (pos < size) {
        while ((*cmd == ' ' || *cmd == '\t') &&
               pos < size) {
            *cmd = '\0';
            cmd++;
            pos++;
        }

        if (pos >= size)
            break;

        argv[arv_index++] = cmd;

        if (arv_index >= OS_SERVICE_FISH_ARG_MAX) {
            os_printk("Argument too much. Limit:%d\r\n", OS_SERVICE_FISH_ARG_MAX);
            return 0;
        }

        while (*cmd != ' ' && *cmd != '\t') {
            cmd++;
            pos++;
            if (pos >= size)
                break;
        }
    }
    return arv_index;
}

os_handle_state_t os_exec_cmd(char *cmd, unsigned int size, int *cmd_ret_val)
{
    while (*cmd == ' ' || *cmd == '\t') {
        cmd++;
        size--;
    }

    if (size == 0)
        return OS_HANDLE_FAIL;

    // command [arg1] [arg2] ...
    char *argv[OS_SERVICE_FISH_ARG_MAX];
    memset(argv, 0x00, sizeof(argv));
    unsigned int argc = __os_split_cmd(cmd, size, argv);
    if (0 == argc)
        return OS_HANDLE_FAIL;

    cmd_call_ptr cmd_call = __os_get_cmd_call(argv[0], strlen(cmd));

    if (cmd_call == NULL)
        return OS_HANDLE_FAIL;

    *cmd_ret_val = cmd_call(argc - 1, &argv[1]);

    // Restore cmd to its original data-set
    for (unsigned int i = 0; i < size; ++i)
        cmd[i] = (cmd[i] == '\0' ? ' ' : cmd[i]);

    return OS_HANDLE_SUCCESS;
}

os_private void __tin_list_add_tail(struct os_service_fish_input *inp)
{
    // Buffer full
    if (__os_service_tin_list_len >= OS_SERVICE_FISH_MSG_LIST_CAP) {
        // Free the 'first' one, the head of the queue.
        struct os_fish_inp_nd *tmp =
            os_list_entry(__os_service_tin_list.next, struct os_fish_inp_nd, list_nd);
        os_kfree(tmp->data_pack);
        list_del(&tmp->list_nd);
        os_kfree(tmp);
        __os_service_tin_list_len--;
    }

    // Add a new one to the tail of queue.
    struct os_fish_inp_nd *nd =
        (struct os_fish_inp_nd *)os_kmalloc(sizeof(struct os_fish_inp_nd));
    OS_ASSERT(NULL != nd);
    nd->data_pack = inp;
    list_add_tail(&__os_service_tin_list, &nd->list_nd);
    __os_service_tin_list_len++;
    __os_service_tin_list_tail = __os_service_tin_list.prev;
}

os_private void __tin_list_clear(void)
{
    struct list_head *current_pos = NULL;
    struct list_head *next_pos = NULL;
    list_for_each_safe(current_pos, next_pos, &__os_service_tin_list)
    {
        struct os_fish_inp_nd *tmp =
            os_list_entry(current_pos, struct os_fish_inp_nd, list_nd);
        list_del_init(&tmp->list_nd);
        os_kfree(tmp->data_pack);
        os_kfree(tmp);
        __os_service_tin_list_len--;
    }
}

static void __kthread_terminal(void *_arg)
{
    struct os_service_fish_input *_inp;
    while (1) {
        _inp = (struct os_service_fish_input *)os_kmalloc(sizeof(struct os_service_fish_input));
        OS_ASSERT(NULL != _inp);
        if (OS_HANDLE_SUCCESS ==
            os_mqueue_receive(os_get_fish_input_mq(), (void *)_inp,
                              sizeof(struct os_service_fish_input), OS_MQUEUE_NEVER_TIMEOUT)) {
            os_printk("\r\n");
            int ret_val;
            if (os_exec_cmd(_inp->data, _inp->size, &ret_val) == OS_HANDLE_SUCCESS) {
                __tin_list_add_tail(_inp);
                // os_printk(":Command return value:%d\r\n", ret_val);
            }
            os_printk("esh>");
            os_printk_flush();
        }
    }
}

os_private os_handle_state_t __os_fish_irq_handle_default_fn(unsigned int rec)
{
    
    static unsigned char combination_keys[5];
    static unsigned char combination_keys_index;
    static struct jiffies_structure last_jiffies;
    
    if (27 == rec) {
        combination_keys_index = 0;
        combination_keys[combination_keys_index++] = (unsigned char)rec;
        last_jiffies = os_get_timestamp();
        return OS_HANDLE_SUCCESS;
    }
    if (0 != combination_keys_index &&
        os_get_timestamp().c - last_jiffies.c < 100) {
        combination_keys[combination_keys_index++] = (unsigned char)rec;
        if (combination_keys_index == 3) {
            os_fish_clear_input_buffer(true);
            if (combination_keys[0] == 27 && 
                combination_keys[1] == 91) {
                switch(combination_keys[2]) {
                case 'A': // up
                    if (__os_service_tin_list_tail != (&__os_service_tin_list)) {
                        struct os_fish_inp_nd *tmp =
                            os_list_entry(__os_service_tin_list_tail, struct os_fish_inp_nd, list_nd);
                        os_memcpy(&input_buffer, tmp->data_pack, sizeof(struct os_service_fish_input));
                        os_device_write(os_get_sys_uart_device_handle(), 0,
                                (void *)input_buffer.data, input_buffer.size);
                        __os_service_tin_list_tail = __os_service_tin_list_tail->prev;
                    }
                    break;
                case 'B': // buttom
                    if (__os_service_tin_list_tail->next != (&__os_service_tin_list)) {
                        struct os_fish_inp_nd *tmp =
                            os_list_entry(__os_service_tin_list_tail->next, struct os_fish_inp_nd, list_nd);
                        os_memcpy(&input_buffer, tmp->data_pack, sizeof(struct os_service_fish_input));
                        os_device_write(os_get_sys_uart_device_handle(), 0,
                                (void *)input_buffer.data, input_buffer.size);
                        __os_service_tin_list_tail = __os_service_tin_list_tail->next;
                    }
                    break;
                case 'C': // left
                    break;
                case 'D': // right
                    break;
                default:
                    break;
                }
            }
            combination_keys_index = 0;
        }
        last_jiffies = os_get_timestamp();
        return OS_HANDLE_SUCCESS;
    } else {
        combination_keys_index = 0;
    }

    switch (rec) {
    case 13:  // CR (Carriage Return)
        os_mqueue_send(os_get_fish_input_mq(), (void *)(&input_buffer),
                       sizeof(struct os_service_fish_input), OS_MQUEUE_NO_WAIT);
        memset(input_buffer.data, 0, OS_SERVICE_FISH_MSG_MAX_SIZE);
        input_buffer.size = 0;
        break;
    case 127: // DEL (Delete)
    case 8: // backspace
        if (input_buffer.size == 0)
            break;
        input_buffer.data[--input_buffer.size] = 0;
        os_device_write(os_get_sys_uart_device_handle(), 0, (void *)&rec, 2);
        break;
    case 9:  // tab
        break;
    default:
        input_buffer.data[input_buffer.size] = rec;
        input_buffer.size++;
        os_device_write(os_get_sys_uart_device_handle(), 0, (void *)&rec, 2);
        break;
    }

    last_jiffies = os_get_timestamp();
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

os_private os_handle_state_t __os_fish_irq_handle_step_fn(unsigned int rec)
{
    unsigned char get_rec = (unsigned char)rec;
    os_mqueue_send(os_get_fish_step_input_mq(), (void *)&get_rec,
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

inline enum os_fish_irq_handle os_fish_get_now_irq_handle(void)
{
#ifdef CONFIG_FISH
    return fish_irq_handle_vec_index;
#else
    return OS_FISH_IRQ_HANDLE_NONE;
#endif
}

os_private OS_CMD_PROCESS_FN(hello_world)
{
    os_printk("Hello world!\r\n");
    return 0;
}

os_private OS_CMD_PROCESS_FN(read_cmd_cap)
{
    return __os_service_tin_list_len;
}

os_private OS_CMD_PROCESS_FN(clear_cmd_history)
{
    __tin_list_clear();
    return __os_service_tin_list_len;
}

os_private OS_CMD_PROCESS_FN(list_cmd)
{
    struct os_fish_cmd_structure *cmd_structure;
    const char *str;
    int num = 0;

    for (cmd_structure = cmd_addr_begin;
         cmd_structure < cmd_addr_end;
         cmd_structure = (struct os_fish_cmd_structure *)((unsigned int)((char *)cmd_structure) +
                                                          sizeof(struct os_fish_cmd_structure))) {
        // skip "__os_cmd"
        str = &(cmd_structure->cmd_name[9]);
        os_printk("%s    ", str);
        os_printk("- %s\r\n", cmd_structure->cmd_desc);
        num++;
    }
    os_printk("\r\n");
    return num;
}

OS_CMD_EXPORT(hello, hello_world, "Hello wolrd.");
OS_CMD_EXPORT(readcmdcap, read_cmd_cap, "Read the buffer size of the commands that executed.");
OS_CMD_EXPORT(clearcmd, clear_cmd_history, "Clear the buffer for the commands that executed.");
OS_CMD_EXPORT(help, list_cmd, "Help?");
#endif

inline struct os_mqueue *os_get_fish_input_mq(void)
{
#ifdef CONFIG_FISH
    return &__os_fish_inp_mq;
#else
    return NULL;
#endif
}

inline struct os_mqueue *os_get_fish_step_input_mq(void)
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
    cmd_addr_begin = (struct os_fish_cmd_structure *)&oscmdtable$$Base;
    cmd_addr_end = (struct os_fish_cmd_structure *)&oscmdtable$$Limit;
#elif defined(__GNUC__)
    cmd_addr_begin = (struct os_fish_cmd_structure *)&__os_fish_cmd_table_begin;
    cmd_addr_end = (struct os_fish_cmd_structure *)&__os_fish_cmd_table_end;
#endif
    os_mqueue_init(&__os_fish_inp_mq, 5, sizeof(struct os_service_fish_input));
    os_mqueue_init(&__os_fish_step_inp_mq, 10, sizeof(struct os_service_fish_input));
    fish_irq_handle_vector[OS_FISH_IRQ_HANDLE_NONE] = NULL;
    fish_irq_handle_vector[OS_FISH_IRQ_HANDLE_DEFAULT] = __os_fish_irq_handle_default_fn;
    fish_irq_handle_vector[OS_FISH_IRQ_HANDLE_STEP] = __os_fish_irq_handle_step_fn;
    os_fish_change_irq_handle(OS_FISH_IRQ_HANDLE_DEFAULT);
    __os_service_tin_list_tail = __os_service_tin_list.prev;
    os_task_create(&_fish_tcb, (void *)fish_task_stack,
                   FISH_TASK_STACK_SIZE, FISH_TASK_PRIO,
                   __kthread_terminal, NULL, "KERNEL FISH TASK");
#endif
}
