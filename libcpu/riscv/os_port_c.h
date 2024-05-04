/***********************
 * @file: os_port_c.h
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2023-10-11     Feijie Luo   Support RISC-V MCU platform
 * @note:
 ***********************/

#ifndef _OS_PORT_H_
#define _OS_PORT_H_

#include "../../os_def.h"
void os_init_msp(void);
void os_ctx_sw(void);
void os_ctx_sw_clear(void);
void os_ready_to_current(void);
void os_clear_systick_flag(void);
os_base_t os_sys_owned_critical_status(void);
/*
 * _stack_size: byte
 * */
unsigned int *os_process_stack_init(void *_fn_entry,
                                    void *_arg,
                                    void *_exit,
                                    void *_stack_addr,
                                    unsigned int _stack_size);
unsigned int os_port_enter_critical(void);
void os_port_exit_critical(unsigned int _state);
unsigned int __os_enter_sys_owned_critical(void);
void __os_exit_sys_owned_critical(unsigned int _state);
void os_port_cpu_int_disable(void);
void os_port_cpu_int_enable(void);
void os_port_init(void);

/*
 * switch to interrupt stack
 * */
#define GET_INT_MSP()  asm volatile("csrrw sp,mscratch,sp")
#define FREE_INT_MSP() asm volatile("csrrw sp,mscratch,sp")

/*
 * Wait For Interrupt. 
 * Used to reduce the power consumption of the processor.
 * This "define" can be empty.
 */
#define OS_WFI                        \
        do{                           \
              asm volatile("wfi");  \
        } while (0)

#define OS_ENTER_CRITICAL unsigned int __critical_state__ = os_port_enter_critical();
#define OS_EXIT_CRITICAL  os_port_exit_critical(__critical_state__);

#define __OS_OWNED_ENTER_CRITICAL unsigned int __critical_state__ = __os_enter_sys_owned_critical();
#define __OS_OWNED_EXIT_CRITICAL __os_exit_sys_owned_critical(__critical_state__);

//#define __OS_OWNED_ENTER_CRITICAL unsigned int __critical_state__ = os_port_enter_critical();
//#define __OS_OWNED_EXIT_CRITICAL  os_port_exit_critical(__critical_state__);

#endif
