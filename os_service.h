
#ifndef _OS_SERVICE_H_
#define _OS_SERVICE_H_

#include "os_config.h"
#include "os_def.h"

#define OS_SERVICE_FISH_MSG_MAX_SIZE (32)
//  OS_SERVICE_FISH_MSG_LIST_CAP
#define OS_SERVICE_FISH_MSG_LIST_CAP (10)
// fish
#define OS_SERVICE_FISH_ARG_MAX (8)

struct os_service_fish_input {
    char data[OS_SERVICE_FISH_MSG_MAX_SIZE];
    unsigned int size;
};

typedef int (*cmd_call_ptr)(int argc, char **argv);

struct os_fish_cmd_structure {
    const char *cmd_name;
    cmd_call_ptr cmd_call;
#ifdef CONFIG_FISH_CMD_DESC
    const char *cmd_desc;
#endif
};

typedef os_handle_state_t (*fish_irq_handle)(unsigned int rec);

#define FISH_IRQ_HANDLE_SIZE (3)
enum os_fish_irq_handle {
    OS_FISH_IRQ_HANDLE_NONE,
    OS_FISH_IRQ_HANDLE_DEFAULT,
    OS_FISH_IRQ_HANDLE_STEP,
};

os_handle_state_t os_fish_irq_handle_callback(unsigned int rec);
bool os_fish_change_irq_handle(enum os_fish_irq_handle new_handle);
enum os_fish_irq_handle os_fish_get_now_irq_handle(void);
struct os_mqueue *os_get_fish_input_mq(void);
struct os_mqueue *os_get_fish_step_input_mq(void);
void os_service_init(void);

#ifdef CONFIG_FISH

#ifdef CONFIG_FISH_CMD_DESC
#define __OS_CMD_EXPORT_BASE(cmd_name, typical_name, cmd_function_call, cmd_desc) \
    const char __os_cmd_##cmd_name##_name[]                             \
        __attribute__((section(".rodata.name"))) = #typical_name;       \
    __attribute__((used))                                               \
    const char __os_cmd_##cmd_name##_desc[]                             \
        __attribute__((section(".rodata.name"))) = cmd_desc;            \
    __attribute__((used))                                               \
    const struct os_fish_cmd_structure __os_cmd_##cmd_name##_structure  \
        __attribute__((section("oscmdtable"))) =                        \
        {__os_cmd_##cmd_name##_name, (cmd_call_ptr)cmd_function_call, __os_cmd_##cmd_name##_desc};

#else
#define __OS_CMD_EXPORT_BASE(cmd_name, typical_name, cmd_function_call, cmd_desc) \
    const char __os_cmd_##cmd_name##_name[]                             \
        __attribute__((section(".rodata.name"))) = #typical_name;       \
    __attribute__((used))                                               \
    const struct os_fish_cmd_structure __os_cmd_##cmd_name##_structure  \
        __attribute__((section("oscmdtable"))) =                        \
        {__os_cmd_##cmd_name##_name, (cmd_call_ptr)cmd_function_call, __os_cmd_##cmd_name##_desc};
#endif

#define OS_CMD_EXPORT(cmd_name, cmd_function_call, cmd_desc) \
    __OS_CMD_EXPORT_BASE(cmd_name, __os_cmd_##cmd_name, cmd_function_call, cmd_desc)

#else
#define OS_CMD_EXPORT(cmd_name, cmd_function_call)
#endif

#define OS_CMD_PROCESS_FN(funcion_name) \
    int funcion_name(int argc, char **argv)

void os_printk_flush();
void os_printk(const char *fmt, ...);
void os_fish_clear_input_buffer(bool is_clear_screen);
#endif
