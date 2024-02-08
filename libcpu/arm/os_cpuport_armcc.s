; ***********************
; *@file: os_cpuport_armcc.s
; *@author: Feijie Luo
; *@data: 2023-10-29
; *@compiling envir.: armcc 
; ***********************


SCB_SHPR3_PRI13		EQU	0xE000ED20		; focus on priority14 reg.
SCB_PENDSV_PRIORITY	EQU 0x00FF0000		; pendsv priority
SCB_ICSR			EQU 0xE000ED04		; interrupt control and state reg adr
SCB_ICSR_PENDSV_SET	EQU	0x10000000 		; trigger pendsv interrupt
	
    AREA |.text|, CODE, READONLY, ALIGN=2
    THUMB
    REQUIRE8
    PRESERVE8
		
	IMPORT enter_critical_num
	IMPORT os_task_current
	IMPORT os_task_ready
	

os_port_asm_init PROC
	EXPORT os_port_asm_init
	CPSID I								; disable global interrupt
	PUSH {R4, R5}						; note: arm:FD(full decrease), R5 in higher address
	LDR R4, =SCB_SHPR3_PRI13
	LDR R5, =SCB_PENDSV_PRIORITY
	STR R5, [R4]						; set pendsv priority
	
	MOVS R4, #0
	MSR	PSP, R4							; reset psp
	POP {R4, R5}						; load R4 from lower address
	CPSIE I								; enable global interrupt
	BX LR
	ENDP

os_ctx_sw PROC
	EXPORT os_ctx_sw
	PUSH {R4, R5}
	LDR R4, =SCB_ICSR
	LDR R5, =SCB_ICSR_PENDSV_SET
	STR R5, [R4]						; trigger pendsv exception
	POP {R4, R5}
	BX LR
	ENDP
	
PendSV_Handler PROC
	EXPORT PendSV_Handler
    ; disable interrupt to protect context switch
    MRS     R3, PRIMASK
    CPSID   I
	
	;MRS R0, PSP
    ;;;;;; CBZ R0, switch_process				; regard psp = 0x000 as a flag of switching process
	;CMP R0, #0
	;BEQ switch_process
	
	;;; struct exception_os_hw_stack_frame will be pushed by system automatically.
	;;; for  -mfpu=fpv5-d16 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	LDR  R0, =os_task_current
	LDR  R1, [R0]
	CBZ  R1, switch_process
	
	MRS  R1, psp
	
	;;; save float-unit regs
	IF      {FPU} != "SoftVFP"
    TST       LR, #0x10             ; if(!EXC_RETURN[4])
	;;; ARM采用满降栈
    VSTMFDEQ  R1!, {d8 - d15}       ; push FPU register s16~s31
	SUBNE     R1, #0x40
    ENDIF
		
	;;; ARM采用满降栈
	STMFD   R1!, {R4 - R11}         ; push r4 - r11 register
	
	;;; for flag
    MOV     R4, #0x00               ; flag = 0
	IF      {FPU} != "SoftVFP"
    TST     LR, #0x10               ; if(!EXC_RETURN[4])
    MOVEQ   R4, #0x01               ; flag = 1
    ENDIF
    ;;; ARM采用满降栈
	STMFD   R1!, {R4}               ; push flag	
	
	LDR R0, [R0]
	STR R1, [R0]							; update _stack_top of os_task_current
											; and then execute switch_process
	
switch_process
	LDR R0, =os_task_current
	LDR R1, =os_task_ready
	LDR R2, [R1]						; R2 = os_task_ready
	STR R2, [R0]						; os_task_current = os_task_ready
	
	LDR R0, [R2]						; see [struct task_control_block], R0 = _stack_top
	
	IF    {FPU} != "SoftVFP"
    LDMFD R0!, {R1}               ; pop flag
    ENDIF
	
	LDMFD R0!, {R4-R11}					; restore R4-R11 from new process stack, note: _stack_top must point to [full]
										; the rest reg value will be poped by system automatically
	
	IF    {FPU} != "SoftVFP"
    CMP   R1,  #0                 ; if(flag_r3 != 0)
    VLDMFDNE  R0!, {d8 - d15}     ; pop FPU register s16~s31
	ADDEQ R0, #0x40               ; 4 * 16
    ENDIF
	
	MSR PSP, R0							; update PSP
	
	IF      {FPU} != "SoftVFP"
    ORR     LR, LR, #0x10           ; lr |=  (1 << 4), clean FPCA.
    CMP     R1,  #0                 ; if(flag_r1 != 0)
    BICNE   LR, LR, #0x10           ; lr &= ~(1 << 4), set FPCA.
    ENDIF
	
pendsv_exit
	MSR PRIMASK, R3
	ORR LR, LR, #0x04					; Return to Thread mode, exception return uses non-floating-point state from
										; the PSP and execution uses PSP after return.
	CPSIE I
	BX LR
	
	ENDP
	
	;;; memory model
	ALIGN   4
	END
