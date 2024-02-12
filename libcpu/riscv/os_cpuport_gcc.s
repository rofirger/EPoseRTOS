/***********************
 * @file: os_port_gcc.S
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2023-10-11     Feijie Luo   Support RISC-V MCU platform
 * @note: 32bit risc-v mcu
 ***********************/
#include "../../os_config.h"
.global SW_Handler
.align 2
SW_Handler:
    // disable M-mode interrupt
    csrci mstatus, 0x8

    /** check whether it is a swi **/
    // save t0, t1 reg
    addi sp, sp, -8
    sw t0, 0(sp)
    sw t1, 4(sp)
    csrr t1, mcause
    srli t0, t1, 31
    beqz t0, otheri_entry
    //bgt t1, zero, otheri_entry
    // extract the the lower 12 bits
    andi t1, t1, 0x3FF
    li t0, 3
    bne t0, t1, otheri_entry
    
mswi_entry:
    la t0, os_task_current
    lw t1, 0(t0)
    bnez t1, NOT_NULL
    mv t0, sp
    addi t0, t0, 8
    csrw mscratch,t0
    // if os_task_current == NULL
    j no_current_task
NOT_NULL:
    la t0, os_task_current
    lw t0, 0(t0)

    // sp->The value when trapping into SW
    addi sp, sp, 8

    // save sp to the second member[void* sp] of os_task_current(struct task_control_block)
    sw sp, 1 * 4(t0)

    // the sp->The value after saving t0, t1 reg
    addi sp, sp, -8

    // t0->the first member[_stack_top] of 'struct task_control_block'
    lw t0, 0(t0)

    // push t1 to the stack of os_task_current
    lw t1, 1 * 4(sp)
    sw t1, 6 * 4(t0)

    // push t0 to the stack of os_task_current
    lw t1, 0(sp)
    sw t1, 5 * 4(t0)

    // we have saved t0, t1 to the stack of os_task_current
    // so, we can freely use the t0 and t1 registers 
    // sp->The value when trapping into SW
    addi sp, sp, 8

    // save sp to t1
    mv t1, sp

    // sp->the first member[_stack_top] of 'struct task_control_block'!!!!!
    mv sp, t0

    /* saved MPIE */
    li    t0,   0x80
    sw 	  t0,   2 * 4(sp)

    /** save the context **/
#ifdef CONFIG_ARCH_FPU
    addi    sp, sp, 32 * 4
    fsw  f0, 0 * 4(sp)
    fsw  f1, 1 * 4(sp)
    fsw  f2, 2 * 4(sp)
    fsw  f3, 3 * 4(sp)
    fsw  f4, 4 * 4(sp)
    fsw  f5, 5 * 4(sp)
    fsw  f6, 6 * 4(sp)
    fsw  f7, 7 * 4(sp)
    fsw  f8, 8 * 4(sp)
    fsw  f9, 9 * 4(sp)
    fsw  f10, 10 * 4(sp)
    fsw  f11, 11 * 4(sp)
    fsw  f12, 12 * 4(sp)
    fsw  f13, 13 * 4(sp)
    fsw  f14, 14 * 4(sp)
    fsw  f15, 15 * 4(sp)
    fsw  f16, 16 * 4(sp)
    fsw  f17, 17 * 4(sp)
    fsw  f18, 18 * 4(sp)
    fsw  f19, 19 * 4(sp)
    fsw  f20, 20 * 4(sp)
    fsw  f21, 21 * 4(sp)
    fsw  f22, 22 * 4(sp)
    fsw  f23, 23 * 4(sp)
    fsw  f24, 24 * 4(sp)
    fsw  f25, 25 * 4(sp)
    fsw  f26, 26 * 4(sp)
    fsw  f27, 27 * 4(sp)
    fsw  f28, 28 * 4(sp)
    fsw  f29, 29 * 4(sp)
    fsw  f30, 30 * 4(sp)
    fsw  f31, 31 * 4(sp)
    addi    sp, sp, -32 * 4
#endif
    csrr  t0, mepc
    sw t0,   0 * 4(sp)
    // we don't need to save gp register.
    sw x1,   1 * 4(sp)
    sw x4,   4 * 4(sp)
    sw x7,   7 * 4(sp)
    sw x8,   8 * 4(sp)
    sw x9,   9 * 4(sp)
    sw x10, 10 * 4(sp)
    sw x11, 11 * 4(sp)
    sw x12, 12 * 4(sp)
    sw x13, 13 * 4(sp)
    sw x14, 14 * 4(sp)
    sw x15, 15 * 4(sp)
    sw x16, 16 * 4(sp)
    sw x17, 17 * 4(sp)
    sw x18, 18 * 4(sp)
    sw x19, 19 * 4(sp)
    sw x20, 20 * 4(sp)
    sw x21, 21 * 4(sp)
    sw x22, 22 * 4(sp)
    sw x23, 23 * 4(sp)
    sw x24, 24 * 4(sp)
    sw x25, 25 * 4(sp)
    sw x26, 26 * 4(sp)
    sw x27, 27 * 4(sp)
    sw x28, 28 * 4(sp)
    sw x29, 29 * 4(sp)
    sw x30, 30 * 4(sp)
    sw x31, 31 * 4(sp)

    // restore sp
    mv sp, t1

no_current_task:
    /* switch to interrupt stack */
    csrrw sp, mscratch, sp
    /* call  os_sys_enter_irq */
    /* clear interrupt */
    jal   os_ctx_sw_clear
    /* call  os_sys_exit_irq */
    /* switch to from thread stack */
    csrrw sp, mscratch, sp

    /* switch context */
    jal os_ready_to_current
    la t0,  os_task_ready
    lw t0, 0(t0)
    
    /** restore the context **/
    // sp->the first member[_stack_top] of 'struct task_control_block'!!!!!
    lw sp, 0 * 4(t0)

    lw  t0,  0 * 4(sp)
    csrw  mepc, t0

    lw  x1,   1 * 4(sp)

#ifdef CONFIG_ARCH_FPU
    li t0, 0x7800
#else
    li t0, 0x1800
#endif
    csrs mstatus, t0
    lw t0, 2 * 4(sp)
    csrs mstatus, t0
    // we don't need to restore gp register.
    lw  x4,   4 * 4(sp)
    lw  x5,   5 * 4(sp)
    lw  x6,   6 * 4(sp)
    lw  x7,   7 * 4(sp)
    lw  x8,   8 * 4(sp)
    lw  x9,   9 * 4(sp)
    lw  x10, 10 * 4(sp)
    lw  x11, 11 * 4(sp)
    lw  x12, 12 * 4(sp)
    lw  x13, 13 * 4(sp)
    lw  x14, 14 * 4(sp)
    lw  x15, 15 * 4(sp)
    lw  x16, 16 * 4(sp)
    lw  x17, 17 * 4(sp)
    lw  x18, 18 * 4(sp)
    lw  x19, 19 * 4(sp)
    lw  x20, 20 * 4(sp)
    lw  x21, 21 * 4(sp)
    lw  x22, 22 * 4(sp)
    lw  x23, 23 * 4(sp)
    lw  x24, 24 * 4(sp)
    lw  x25, 25 * 4(sp)
    lw  x26, 26 * 4(sp)
    lw  x27, 27 * 4(sp)
    lw  x28, 28 * 4(sp)
    lw  x29, 29 * 4(sp)
    lw  x30, 30 * 4(sp)
    lw  x31, 31 * 4(sp)

    /* load float reg */
#ifdef CONFIG_ARCH_FPU
    addi  sp, sp, 32 * 4
    flw   f0, 0 * 4(sp)
    flw   f1, 1 * 4(sp)
    flw   f2, 2 * 4(sp)
    flw   f3, 3 * 4(sp)
    flw   f4, 4 * 4(sp)
    flw   f5, 5 * 4(sp)
    flw   f6, 6 * 4(sp)
    flw   f7, 7 * 4(sp)
    flw   f8, 8 * 4(sp)
    flw   f9, 9 * 4(sp)
    flw   f10, 10 * 4(sp)
    flw   f11, 11 * 4(sp)
    flw   f12, 12 * 4(sp)
    flw   f13, 13 * 4(sp)
    flw   f14, 14 * 4(sp)
    flw   f15, 15 * 4(sp)
    flw   f16, 16 * 4(sp)
    flw   f17, 17 * 4(sp)
    flw   f18, 18 * 4(sp)
    flw   f19, 19 * 4(sp)
    flw   f20, 20 * 4(sp)
    flw   f21, 21 * 4(sp)
    flw   f22, 22 * 4(sp)
    flw   f23, 23 * 4(sp)
    flw   f24, 24 * 4(sp)
    flw   f25, 25 * 4(sp)
    flw   f26, 26 * 4(sp)
    flw   f27, 27 * 4(sp)
    flw   f28, 28 * 4(sp)
    flw   f29, 29 * 4(sp)
    flw   f30, 30 * 4(sp)
    flw   f31, 31 * 4(sp)
#endif
    // restore sp from the second member[void* sp] of os_task_current(struct task_control_block)
    la sp, os_task_ready
    lw sp, 0(sp)
    lw sp, 1 * 4 (sp)
    mret

otheri_entry:
    // restore t0, t1 reg
    lw t0, 0(sp)
    lw t1, 4(sp)
    addi sp, sp, 8
#ifdef CONFIG_ARCH_FPU
    addi sp, sp, -32 * 4
    fsw  f0, 0 * 4(sp)
    fsw  f1, 1 * 4(sp)
    fsw  f2, 2 * 4(sp)
    fsw  f3, 3 * 4(sp)
    fsw  f4, 4 * 4(sp)
    fsw  f5, 5 * 4(sp)
    fsw  f6, 6 * 4(sp)
    fsw  f7, 7 * 4(sp)
    fsw  f8, 8 * 4(sp)
    fsw  f9, 9 * 4(sp)
    fsw  f10, 10 * 4(sp)
    fsw  f11, 11 * 4(sp)
    fsw  f12, 12 * 4(sp)
    fsw  f13, 13 * 4(sp)
    fsw  f14, 14 * 4(sp)
    fsw  f15, 15 * 4(sp)
    fsw  f16, 16 * 4(sp)
    fsw  f17, 17 * 4(sp)
    fsw  f18, 18 * 4(sp)
    fsw  f19, 19 * 4(sp)
    fsw  f20, 20 * 4(sp)
    fsw  f21, 21 * 4(sp)
    fsw  f22, 22 * 4(sp)
    fsw  f23, 23 * 4(sp)
    fsw  f24, 24 * 4(sp)
    fsw  f25, 25 * 4(sp)
    fsw  f26, 26 * 4(sp)
    fsw  f27, 27 * 4(sp)
    fsw  f28, 28 * 4(sp)
    fsw  f29, 29 * 4(sp)
    fsw  f30, 30 * 4(sp)
    fsw  f31, 31 * 4(sp)
#endif
    addi sp, sp, -32 * 4
    // we don't need to save sp & gp
    // so we can use the position of sp to save mstatus
    sw x5,   5 * 4(sp)
    li x5,   0x80
    sw x5,   2 * 4(sp)
    sw x1,   1 * 4(sp)
    sw x4,   4 * 4(sp)
    sw x6,   6 * 4(sp)
    sw x7,   7 * 4(sp)
    sw x8,   8 * 4(sp)
    sw x9,   9 * 4(sp)
    sw x10, 10 * 4(sp)
    sw x11, 11 * 4(sp)
    sw x12, 12 * 4(sp)
    sw x13, 13 * 4(sp)
    sw x14, 14 * 4(sp)
    sw x15, 15 * 4(sp)
    sw x16, 16 * 4(sp)
    sw x17, 17 * 4(sp)
    sw x18, 18 * 4(sp)
    sw x19, 19 * 4(sp)
    sw x20, 20 * 4(sp)
    sw x21, 21 * 4(sp)
    sw x22, 22 * 4(sp)
    sw x23, 23 * 4(sp)
    sw x24, 24 * 4(sp)
    sw x25, 25 * 4(sp)
    sw x26, 26 * 4(sp)
    sw x27, 27 * 4(sp)
    sw x28, 28 * 4(sp)
    sw x29, 29 * 4(sp)
    sw x30, 30 * 4(sp)
    sw x31, 31 * 4(sp)
    
    la t0, irq_handler_trap
    jalr t0

#ifdef CONFIG_ARCH_FPU
    li t0, 0x7800
#else
    li t0, 0x1800
#endif
    csrs mstatus, t0
    lw t0, 2 * 4(sp)
    csrs mstatus, t0
    lw  x1,   1 * 4(sp)
    lw  x4,   4 * 4(sp)
    lw  x5,   5 * 4(sp)
    lw  x6,   6 * 4(sp)
    lw  x7,   7 * 4(sp)
    lw  x8,   8 * 4(sp)
    lw  x9,   9 * 4(sp)
    lw  x10, 10 * 4(sp)
    lw  x11, 11 * 4(sp)
    lw  x12, 12 * 4(sp)
    lw  x13, 13 * 4(sp)
    lw  x14, 14 * 4(sp)
    lw  x15, 15 * 4(sp)
    lw  x16, 16 * 4(sp)
    lw  x17, 17 * 4(sp)
    lw  x18, 18 * 4(sp)
    lw  x19, 19 * 4(sp)
    lw  x20, 20 * 4(sp)
    lw  x21, 21 * 4(sp)
    lw  x22, 22 * 4(sp)
    lw  x23, 23 * 4(sp)
    lw  x24, 24 * 4(sp)
    lw  x25, 25 * 4(sp)
    lw  x26, 26 * 4(sp)
    lw  x27, 27 * 4(sp)
    lw  x28, 28 * 4(sp)
    lw  x29, 29 * 4(sp)
    lw  x30, 30 * 4(sp)
    lw  x31, 31 * 4(sp)

    addi sp, sp, 32 * 4
#ifdef CONFIG_ARCH_FPU
    flw   f0, 0 * 4(sp)
    flw   f1, 1 * 4(sp)
    flw   f2, 2 * 4(sp)
    flw   f3, 3 * 4(sp)
    flw   f4, 4 * 4(sp)
    flw   f5, 5 * 4(sp)
    flw   f6, 6 * 4(sp)
    flw   f7, 7 * 4(sp)
    flw   f8, 8 * 4(sp)
    flw   f9, 9 * 4(sp)
    flw   f10, 10 * 4(sp)
    flw   f11, 11 * 4(sp)
    flw   f12, 12 * 4(sp)
    flw   f13, 13 * 4(sp)
    flw   f14, 14 * 4(sp)
    flw   f15, 15 * 4(sp)
    flw   f16, 16 * 4(sp)
    flw   f17, 17 * 4(sp)
    flw   f18, 18 * 4(sp)
    flw   f19, 19 * 4(sp)
    flw   f20, 20 * 4(sp)
    flw   f21, 21 * 4(sp)
    flw   f22, 22 * 4(sp)
    flw   f23, 23 * 4(sp)
    flw   f24, 24 * 4(sp)
    flw   f25, 25 * 4(sp)
    flw   f26, 26 * 4(sp)
    flw   f27, 27 * 4(sp)
    flw   f28, 28 * 4(sp)
    flw   f29, 29 * 4(sp)
    flw   f30, 30 * 4(sp)
    flw   f31, 31 * 4(sp)
    addi  sp,  sp, 32 * 4
#endif
    mret
