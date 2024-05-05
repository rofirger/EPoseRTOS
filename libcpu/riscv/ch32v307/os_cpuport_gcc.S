/***********************
 * @file: os_port_gcc.s
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2023-10-11     Feijie Luo   Support RISC-V MCU platform
 * 2023-10-19     Feijie Luo   *EXTREME ERROR* See code
 * @note: 32bit risc-v mcu
 ***********************/

.global SW_Handler
.align 2
SW_Handler:
	addi sp, sp, -8
	sw t0, 0(sp)
	sw t1, 4(sp)

	la t0, os_task_current
	lw t1, 0(t0)
	/* seqz t0, t1 */
	beqz t1, no_current_task

	la t0, os_task_current
	lw t0, 0(t0)

	addi sp, sp, 8
	sw sp, 1 * 4(t0)
	addi sp, sp, -8

	lw t0, 0(t0)

	/* save t0, t1 start*/
	lw t1, 1 * 4(sp)
	sw t1, 6 * 4(t0)

	lw t1, 0(sp)
	sw t1, 5 * 4(t0)
	/* save t0, t1 end*/

	addi sp, sp, 8

	mv t1, sp

	mv sp, t0

	/* saved MPIE */
 	li    t0,   0x80
    sw 	  t0,   2 * 4(sp)

	/* Temporarily disable HPE  */
	li   t0,    0x20
	csrs 0x804, t0

    /* save all from thread context */
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
    /*2023-10-19 change, a0->t0(t0 has been saved), *EXTREME ERROR* */
	csrr  t0, mepc
    sw t0, 	 0 * 4(sp)
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


	mv sp, t1
	j normal_stream
no_current_task:
	jal   os_ready_to_current
normal_stream:
    /* switch to interrupt stack */
	csrrw sp, mscratch, sp
    /* call  os_sys_enter_irq */
    /* clear interrupt */
    jal   os_ctx_sw_clear
    jal   __os_int_post_status_called_by_sw
	bnez  a0, no_int_post
    jal   __os_int_post_handle_called_by_sw
no_int_post:
	jal   __os_sched_status_called_by_sw
	bnez  a0, no_need_call_sched
    jal   __os_sched_called_by_sw
    bnez  a0, keep_current
no_need_call_sched:
    jal   os_ready_to_current
keep_current:
    /* call  os_sys_exit_irq */
    /* switch to from thread stack */
	csrrw sp, mscratch, sp

	/* switch context */
	la t0,  os_task_current
	lw t0, 0 * 4(t0)
	lw sp, 0 * 4(t0)

	/*2023-10-19 change, a0->t0(t0 can be used), *EXTREME ERROR* */
    lw  t0,  0 * 4(sp)
    csrw  mepc, t0

    lw  x1,   1 * 4(sp)

	li t0, 0x7800
	csrs mstatus, t0
	lw t0, 2 * 4(sp)
	csrs mstatus, t0

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
	la sp, os_task_current
	lw sp, 0(sp)
	lw sp, 1 * 4 (sp)
    mret
