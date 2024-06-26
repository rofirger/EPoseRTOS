/***********************
 * @file: os_sys.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2022-09-10     Feijie Luo   First version
 * 2023-10-11     Feijie Luo
 * 2023-10-12     Feijie Luo   modify os_sys_exit_irq function
 * @note:
 ***********************/

#include "board/libcpu_headfile.h"
#include "board/os_board.h"
#include "components/memory/os_malloc.h"
#include "os_sched.h"
#include "os_service.h"
#include "os_soft_timer.h"
#include "os_sys.h"
#include "os_tick.h"
#include "os_device.h"

#define IDLE_TASK_PRIO       OS_TASK_MAX_PRIORITY
#define IDLE_TASK_STACK_SIZE 1024
static struct task_control_block _idle_tcb;
static unsigned char idle_task_stack[IDLE_TASK_STACK_SIZE];

static unsigned char _os_iqr_nesting;

void os_idle_task(void *_arg)
{
    while (1) {
        OS_WFI;
    }
}

static void __idle_task_create(void)
{
    os_task_create(&_idle_tcb, (void *)idle_task_stack, IDLE_TASK_STACK_SIZE, IDLE_TASK_PRIO, os_idle_task, NULL, "KERNEL IDLE TASK");
}

inline struct task_control_block *os_get_idle_tcb(void)
{
    return (&_idle_tcb);
}

// 1.为什么 printf 会占用没有被 ld 分配的内存区域？？？
os_private void __os_welcome_kprint(void)
{
    os_printk(" ______ _____               _____ _______ ____   _____ \r\n");
    os_printk("|  ____|  __ \\             |  __ \\__   __/ __ \\ / ____|\r\n");
    os_printk("| |__  | |__) |__  ___  ___| |__) | | | | |  | | (___  \r\n");
    os_printk("|  __| |  ___/ _ \\/ __|/ _ \\  _  /  | | | |  | |\\___ \\ \r\n");
    os_printk("| |____| |  | (_) \\__ \\  __/ | \\ \\  | | | |__| |____) |\r\n");
    os_printk("|______|_|   \\___/|___/\\___|_|  \\_\\ |_|  \\____/|_____/ \r\n");
    os_printk("                                               --Kernel\r\n");
}

/**
 * @brief Initialize the operating system.
 *
 * This function initializes the operating system by performing the following steps:
 * 1. Halt the scheduler.
 * 2. Initialize the board.
 * 3. Print kernel information.
 * 4. Initialize memory.
 * 5. Initialize the software timer.
 * 6. Initialize the timeslice.
 * 7. Initialize the system ready queue.
 * 8. Initialize the device system.
 * 9. Initialize the service.
 * 10. Create the idle task.
 * 11. Reset the interrupt nesting level to 0.
 */
void os_sys_init(void)
{
    // halt the scheduler
    os_sched_halt();
    // board init
    os_board_init();
    // initialize device system
    os_sys_device_init();
    // print kernel info
    __os_welcome_kprint();
    // memory init
    os_memory_init();
    os_soft_timer_init();
    // initialize timeslice
    os_sched_timeslice_init();
    os_sys_owned_block_init();
    os_sys_ready_queue_init();
    // initialize service
    os_service_init();
    // create idle task
    __idle_task_create();
    _os_iqr_nesting = 0;
}

/**
 * @brief Start the kernel.
 *
 * @return None
 *
 * @note Prior to calling this function, hardware initialization and necessary system parameter
 *       setup should be completed. This function typically does NOT RETURN; once the kernel
 *       starts successfully, it enters the scheduling loop.
 */
void os_sys_start(void)
{
    os_sched_insert_ready();
    os_init_msp();
    os_sched_set_running();
    os_board_start_interrupt();
    os_ctx_sw();
    while (1) {
    };
}

/* 系统是否处在中断当中, 如果返回true也意味着实时系统此时不会发生调度 */
bool os_sys_is_in_irq(void)
{
    return (_os_iqr_nesting > 0);
}

/* 进入中断, 此函数用于当系统处在中断函数中时禁止实时系统调度 */
void os_sys_enter_irq(void)
{
    __OS_OWNED_ENTER_CRITICAL
    if (os_sched_is_running() &&
        _os_iqr_nesting < 255)
        _os_iqr_nesting++;
    __OS_OWNED_EXIT_CRITICAL
}

/* 退出中断 */
void os_sys_exit_irq(void)
{
    if (os_sched_is_running() &&
        os_sys_is_in_irq()) {
        __OS_OWNED_ENTER_CRITICAL
        _os_iqr_nesting--;
        __OS_OWNED_EXIT_CRITICAL
    }
}

inline void os_systick_handler(void)
{
    if (os_sched_is_running()) {
        os_sched_timeslice_poll();
        os_task_tick_poll();
    }
}
