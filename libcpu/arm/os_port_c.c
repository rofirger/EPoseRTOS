/***********************
 * @file: os_port_c.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2023-10-29     Feijie Luo   Support cortex-M7 MCU platform
 * @note:
 ***********************/


#include "os_port_c.h"
#include "../../os_sys.h"
#include "../../os_core.h"
#include "../../os_config.h"

extern struct task_control_block* os_task_current;
extern struct task_control_block* os_task_ready;

struct exception_os_hw_stack_frame
{
    unsigned int r0;
    unsigned int r1;
    unsigned int r2;
    unsigned int r3;
    unsigned int r12;
    unsigned int lr;
    unsigned int pc;
    unsigned int psr;

#ifdef CONFIG_ARCH_FPU
    /* FPU register */
    unsigned int S0;
    unsigned int S1;
    unsigned int S2;
    unsigned int S3;
    unsigned int S4;
    unsigned int S5;
    unsigned int S6;
    unsigned int S7;
    unsigned int S8;
    unsigned int S9;
    unsigned int S10;
    unsigned int S11;
    unsigned int S12;
    unsigned int S13;
    unsigned int S14;
    unsigned int S15;
    unsigned int FPSCR;
    unsigned int NO_NAME;
#endif
};

struct os_hw_stack_frame
{
    unsigned int flag;

    /* r4 ~ r11 register */
    unsigned int r4;
    unsigned int r5;
    unsigned int r6;
    unsigned int r7;
    unsigned int r8;
    unsigned int r9;
    unsigned int r10;
    unsigned int r11;
/* float register */
#ifdef CONFIG_ARCH_FPU
     /* FPU register s16 ~ s31 */
    unsigned int s16;
    unsigned int s17;
    unsigned int s18;
    unsigned int s19;
    unsigned int s20;
    unsigned int s21;
    unsigned int s22;
    unsigned int s23;
    unsigned int s24;
    unsigned int s25;
    unsigned int s26;
    unsigned int s27;
    unsigned int s28;
    unsigned int s29;
    unsigned int s30;
    unsigned int s31;
#endif
    // 硬件自动保存 
	struct exception_os_hw_stack_frame exception_stack_frame;
};

unsigned int enter_critical_num;


unsigned int* os_process_stack_init(void* _fn_entry, 
									void* _arg, 
									void* _exit, 
									void* _stack_addr,
									unsigned int _stack_size)
{
    // 栈向下生长， uint32_t对应struct os_hw_stack_frame的成员大小
	// locate end of _stack_addr
    unsigned int* _tmp_stack_addr = (unsigned int*)((unsigned int)_stack_addr + _stack_size - 1);
	// note: 8-byte align		AAPCS  We must ensure that 8-byte alignment is maintained in C
	_tmp_stack_addr = (unsigned int*)((unsigned int)(_tmp_stack_addr) & 0xFFFFFFF8UL);
	// 储存上文
	_tmp_stack_addr = (unsigned int*)((unsigned int)_tmp_stack_addr - sizeof(struct os_hw_stack_frame));
	
	for (int i = 0; i < sizeof(struct os_hw_stack_frame) / sizeof(unsigned int); ++i)
	    _tmp_stack_addr[i] = 0xdeadbeef; // magic number

	struct os_hw_stack_frame* frame = (struct os_hw_stack_frame*)_tmp_stack_addr;

	frame->exception_stack_frame.psr = 0x01000000UL;
	frame->exception_stack_frame.pc  = (unsigned int)_fn_entry;
	frame->exception_stack_frame.lr  = (unsigned int)_exit;
	frame->exception_stack_frame.r12 = 0x00000000UL;
	frame->exception_stack_frame.r3  = 0x00000000UL;
	frame->exception_stack_frame.r2  = 0x00000000UL;
	frame->exception_stack_frame.r1  = 0x00000000UL;
	frame->exception_stack_frame.r0  = (unsigned int)_arg;
	
	frame->r4 = 0x00000004UL;
	frame->r5 = 0x00000005UL;
	frame->r6 = 0x00000006UL;

	frame->flag = 0;

	return _tmp_stack_addr;
}

void os_port_cpu_int_disable(void)
{
    __asm volatile ("CPSID I" : : : "memory");
}


void os_port_cpu_int_enable(void)
{
    __asm volatile ("CPSIE I" : : : "memory");
}

unsigned int os_port_cpu_primask_get(void)
{
	unsigned int _ret;

	__asm volatile ("MRS %0, PRIMASK" : "=r" (_ret));

	return(_ret);
}

void os_port_cpu_primask_set(unsigned int _primask)
{
	__asm volatile ("MSR PRIMASK, %0" : : "r" (_primask) : "memory");
}

/* 进入临界区 */
unsigned int os_port_enter_critical(void)
{
	unsigned int _ret = os_port_cpu_primask_get();
	
	os_port_cpu_int_disable();
	
	return _ret;
} 

/* 退出临界区 */
void os_port_exit_critical(unsigned int _state)
{
	os_port_cpu_primask_set(_state);
}


inline void os_clear_systick_flag(void)
{
}

inline void os_ready_to_current(void)
{
    os_task_current = os_task_ready;
}

/*
* init interrupt stack pointer
**/
inline void os_init_msp(void)
{
}
