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

__os_inline struct task_control_block *os_get_idle_tcb(void)
{
    return (&_idle_tcb);
}

// 1.为什么 printf 会占用没有被 ld 分配的内存区域？？？
__os_static void __os_welcome_kprint(void)
{
    os_printk("***********\r\n");
    os_printk("* WELCOME *\r\n");
    os_printk("***********\r\n");
}

/* 实时系统初始化 */
void os_sys_init(void)
{
    // board init
    os_board_init();
    // print kernel info
    __os_welcome_kprint();
    // memory init
    os_memory_init();
    // 定时器初始化
    os_soft_timer_init();
    // 时间片初始化
    os_sched_timeslice_init();
    // 优先级队列初始化
    os_ready_queue_init();
    os_cpu_running_flag = 0;
    // 创建空闲线程
    __idle_task_create();
    // 初始化service(如果开启Fish，则创建Fish线程)
    os_service_init();
    _os_iqr_nesting = 0;
}

/* 实时系统内核启动 */
/*** warning：该函数不返回 ***/
void os_sys_start(void)
{
    os_sched_init_ready();
    os_cpu_set_running();
    os_init_msp();
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
    OS_ENTER_CRITICAL
    if (os_cpu_is_running() &&
        _os_iqr_nesting < 255)
        _os_iqr_nesting++;
    OS_EXIT_CRITICAL
}

/* 退出中断 */
void os_sys_exit_irq(void)
{
    if (os_cpu_is_running() &&
        os_sys_is_in_irq()) {
        OS_ENTER_CRITICAL
        _os_iqr_nesting--;
        OS_EXIT_CRITICAL
    }
}

__os_inline void os_systick_handler(void)
{
    if (os_cpu_is_running()) {
        os_sched_timeslice_poll();
        os_task_tick_poll();
    }
}
