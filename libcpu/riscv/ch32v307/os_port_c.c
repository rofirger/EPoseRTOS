/***********************
 * @file: os_port_c.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2023-10-11     Feijie Luo   Support RISC-V MCU platform
 * 2023-10-14     Feijie Luo   Delete os_start() function
 * @note:
 ***********************/


#include "../os_port_c.h"
#include "../../../os_sys.h"
#include "../../User/ch32v30x_it.h"
#include "../../../os_core.h"
#include "../../../os_config.h"

extern struct task_control_block* os_task_current;
extern struct task_control_block* os_task_ready;

struct os_hw_stack_frame
{
    uint32_t epc;        /* epc - epc    - program counter                     */
    uint32_t ra;         /* x1  - ra     - return address for jumps            */
    uint32_t mstatus;    /*              - machine status register             */
    uint32_t gp;         /* x3  - gp     - global pointer                      */
    uint32_t tp;         /* x4  - tp     - thread pointer                      */
    uint32_t t0;         /* x5  - t0     - temporary register 0                */
    uint32_t t1;         /* x6  - t1     - temporary register 1                */
    uint32_t t2;         /* x7  - t2     - temporary register 2                */
    uint32_t s0_fp;      /* x8  - s0/fp  - saved register 0 or frame pointer   */
    uint32_t s1;         /* x9  - s1     - saved register 1                    */
    uint32_t a0;         /* x10 - a0     - return value or function argument 0 */
    uint32_t a1;         /* x11 - a1     - return value or function argument 1 */
    uint32_t a2;         /* x12 - a2     - function argument 2                 */
    uint32_t a3;         /* x13 - a3     - function argument 3                 */
    uint32_t a4;         /* x14 - a4     - function argument 4                 */
    uint32_t a5;         /* x15 - a5     - function argument 5                 */
    uint32_t a6;         /* x16 - a6     - function argument 6                 */
    uint32_t a7;         /* x17 - s7     - function argument 7                 */
    uint32_t s2;         /* x18 - s2     - saved register 2                    */
    uint32_t s3;         /* x19 - s3     - saved register 3                    */
    uint32_t s4;         /* x20 - s4     - saved register 4                    */
    uint32_t s5;         /* x21 - s5     - saved register 5                    */
    uint32_t s6;         /* x22 - s6     - saved register 6                    */
    uint32_t s7;         /* x23 - s7     - saved register 7                    */
    uint32_t s8;         /* x24 - s8     - saved register 8                    */
    uint32_t s9;         /* x25 - s9     - saved register 9                    */
    uint32_t s10;        /* x26 - s10    - saved register 10                   */
    uint32_t s11;        /* x27 - s11    - saved register 11                   */
    uint32_t t3;         /* x28 - t3     - temporary register 3                */
    uint32_t t4;         /* x29 - t4     - temporary register 4                */
    uint32_t t5;         /* x30 - t5     - temporary register 5                */
    uint32_t t6;         /* x31 - t6     - temporary register 6                */

/* float register */
#ifdef CONFIG_ARCH_FPU
    int32_t f0;      /* f0  */
    int32_t f1;      /* f1  */
    int32_t f2;      /* f2  */
    int32_t f3;      /* f3  */
    int32_t f4;      /* f4  */
    int32_t f5;      /* f5  */
    int32_t f6;      /* f6  */
    int32_t f7;      /* f7  */
    int32_t f8;      /* f8  */
    int32_t f9;      /* f9  */
    int32_t f10;     /* f10 */
    int32_t f11;     /* f11 */
    int32_t f12;     /* f12 */
    int32_t f13;     /* f13 */
    int32_t f14;     /* f14 */
    int32_t f15;     /* f15 */
    int32_t f16;     /* f16 */
    int32_t f17;     /* f17 */
    int32_t f18;     /* f18 */
    int32_t f19;     /* f19 */
    int32_t f20;     /* f20 */
    int32_t f21;     /* f21 */
    int32_t f22;     /* f22 */
    int32_t f23;     /* f23 */
    int32_t f24;     /* f24 */
    int32_t f25;     /* f25 */
    int32_t f26;     /* f26 */
    int32_t f27;     /* f27 */
    int32_t f28;     /* f28 */
    int32_t f29;     /* f29 */
    int32_t f30;     /* f30 */
    int32_t f31;     /* f31 */
#endif
};

unsigned int enter_critical_num;


unsigned int* os_process_stack_init(void* _fn_entry, 
									void* _arg, 
									void* _exit, 
									void* _stack_addr,
									unsigned int _stack_size)
{
    // stack downgrowing
	// locate end of _stack_addr
    unsigned int* _tmp_stack_addr = (unsigned int*)((unsigned int)_stack_addr + _stack_size - 1);
	// note: 8-byte align		AAPCS  We must ensure that 8-byte alignment is maintained in C
	_tmp_stack_addr = (unsigned int*)((unsigned int)(_tmp_stack_addr) & 0xFFFFFFF8UL);
	// save the context
	_tmp_stack_addr = (unsigned int*)((unsigned int)_tmp_stack_addr - sizeof(struct os_hw_stack_frame));
	
	for (int i = 0; i < sizeof(struct os_hw_stack_frame) / sizeof(uint32_t); ++i)
	    _tmp_stack_addr[i] = 0xdeadbeef; // magic number

	struct os_hw_stack_frame* frame = (struct os_hw_stack_frame*)_tmp_stack_addr;

	frame->ra = (uint32_t)_exit;
	frame->a0 = (uint32_t)_arg;
	frame->epc = (uint32_t)_fn_entry;

	/* force to machine mode(MPP=11) and set MPIE to 1 and FS=11 */
	// 使用机器模式，使用硬件浮点功能，进中断之前中断使能状态为使能，
	frame->mstatus = 0x00007880;

	return _tmp_stack_addr;
}

void os_port_cpu_int_disable(void)
{
    asm volatile("csrw mstatus, %0" :: "r" (0x7800));
}


void os_port_cpu_int_enable(void)
{
    asm volatile("csrw mstatus, %0" :: "r" (0x7880));
}

unsigned int os_port_cpu_primask_get(void)
{
	unsigned int _ret;
	asm volatile("csrr %0, mstatus" : "=r" (_ret));
	return(_ret);
}

void os_port_cpu_primask_set(unsigned int _primask)
{
	asm volatile("csrw mstatus, %0" :: "r" (_primask));
}

/*
 * Enter the critical section
 * Disable all interrupts
 * Return the interrupt state before entering the critical section
 */
unsigned int os_port_enter_critical(void)
{
    unsigned int _ret;
    asm("csrrw %0, mstatus, %1":"=r"(_ret):"r"(0x7800));
	return _ret;
} 

/*
 * Exit the critical section
 * Reset the interrupt enable state based on the given state
 * Parameter _state: The interrupt state before entering the critical section
 */
void os_port_exit_critical(unsigned int _state)
{
	os_port_cpu_primask_set(_state);
}

unsigned int __os_enter_sys_owned_critical(void)
{
    unsigned int old_status;
    OS_ENTER_CRITICAL;
    old_status = NVIC_GetStatusIRQ(Software_IRQn);
    old_status &= NVIC_GetStatusIRQ(SysTicK_IRQn);
    NVIC_DisableIRQ(Software_IRQn);
    NVIC_DisableIRQ(SysTicK_IRQn);
    OS_EXIT_CRITICAL;
    return old_status;
}

void __os_exit_sys_owned_critical(unsigned int _state)
{
    OS_ENTER_CRITICAL;
    if (_state) {
        NVIC_EnableIRQ(Software_IRQn);
        NVIC_EnableIRQ(SysTicK_IRQn);
    }
    OS_EXIT_CRITICAL;
}

/*
 * Trigger Soft Interrupt
 */
void os_ctx_sw(void)
{
    NVIC_SetPendingIRQ(Software_IRQn);
}

/*
 * Clear soft interrupt
 */
void os_ctx_sw_clear(void)
{
    NVIC_ClearPendingIRQ(Software_IRQn);
}

inline void os_clear_systick_flag(void)
{
    SysTick->SR  = 0;
}

inline void os_ready_to_current(void)
{
    os_task_current = os_task_ready;
}

/*
* Initialize interrupt stack pointer
**/
inline void os_init_msp(void)
{
    asm volatile("la t0, _eusrstack");
    asm volatile("addi t0, t0, -512");
    asm volatile("csrw mscratch, t0");
}
