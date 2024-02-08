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

extern unsigned int enter_critical_num;
void os_init_msp(void);
void os_ready_to_current(void);
void os_clear_systick_flag(void);
/*
 * _stack_size: byte
 * */
unsigned int* os_process_stack_init(void* _fn_entry,
									void* _arg,
									void* _exit,
									void* _stack_addr,
									unsigned int _stack_size);
unsigned int os_port_enter_critical(void);
void os_port_exit_critical(unsigned int _state);
void os_port_cpu_int_disable(void);
void os_port_cpu_int_enable(void);
void os_port_asm_init(void);
void os_ctx_sw(void);

/*
 * switch to interrupt stack
 * */
#define GET_INT_MSP()   //asm("csrrw sp,mscratch,sp")
#define FREE_INT_MSP()  //asm("csrrw sp,mscratch,sp")

#endif
