; at: OSIntExit 0x08030F06, OSPrioCur 0x200004D8, OSPrioHighRdy 0x200004D6, OSTCBCur 0x200004E0, OSTCBHighRdy 0x200004DC
; opts: --cpu Cortex-M3
;
; The task scheduler's context switch (0x08030CD0 to 0x080304E3): hand-written assembly in the
; shape of uC/OS-II's Cortex-M3 port (os_cpu_a.asm), cut down for the application's own small
; scheduler (src/include/rtos_types.h). The names are uC/OS-II's, mine: the C callers are
; OSStart (0x08030E7C, calls OSStartHighRdy), OS_Sched (0x08030B72, OSCtxSw) and OSIntExit
; (0x08030F06, OSIntCtxSw). Differences from Micrium's port: R4 and R5 as scratch (pushed in
; OSCtxSw and OSIntCtxSw), one word store of 0xFFFF0000 to SHPR3 (PendSV and SysTick both at the
; lowest priority) where Micrium stores a byte to 0xE000ED22, no OSRunning, no OSTaskSwHook, a
; one-byte OSPrioCur, interrupts enabled again between saving and restoring, and no
; OSStartHang loop: OSStartHighRdy runs on into OSCtxSw.
;
; match: the variables' addresses are DCD words after the code, loaded pc-relative, then the
; constants' literal pool. The first word, OSIntExit's address, is loaded by nothing; it is in
; the image, so it was in the source.
; Job in this firmware: OSStartHighRdy starts the first task (task_usb); a task gives up the CPU
; through OS_Sched and OSCtxSw when it waits in os_flags_pend; PendSV_Handler does the switch.
; OSIntCtxSw is reached only from OSIntExit, which has no caller, so a switch never happens on
; the way out of an interrupt.
; name: likely (map/filenames.tsv): the file and routine names are uC/OS-II's port names; the
; callers (OSStart, OS_Sched, OSIntExit) and the vector table's PendSV entry say which is which.

NVIC_INT_CTRL   EQU     0xE000ED04          ; interrupt control state register
NVIC_SYSPRI14   EQU     0xE000ED20          ; system handler priority register 3 (PendSV, SysTick)
NVIC_PENDSV_PRI EQU     0xFFFF0000          ; PendSV and SysTick priority: lowest
NVIC_PENDSVSET  EQU     0x10000000          ; value to trigger PendSV

        IMPORT  OSIntExit
        IMPORT  OSPrioCur
        IMPORT  OSPrioHighRdy
        IMPORT  OSTCBCur
        IMPORT  OSTCBHighRdy

        EXPORT  OSStartHighRdy
        EXPORT  OSCtxSw
        EXPORT  OSIntCtxSw
        EXPORT  PendSV_Handler

        PRESERVE8
        AREA    |.text|, CODE, READONLY
        THUMB

; start the highest priority task: PendSV switches to it
OSStartHighRdy
        LDR     R4, =NVIC_SYSPRI14
        LDR     R5, =NVIC_PENDSV_PRI
        STR     R5, [R4]
        MOV     R4, #0                      ; PSP 0: PendSV saves no context the first time
        MSR     PSP, R4
        LDR     R4, =NVIC_INT_CTRL
        LDR     R5, =NVIC_PENDSVSET
        STR     R5, [R4]
        CPSIE   I

; task level context switch
OSCtxSw
        PUSH    {R4, R5}
        LDR     R4, =NVIC_INT_CTRL
        LDR     R5, =NVIC_PENDSVSET
        STR     R5, [R4]
        POP     {R4, R5}
        BX      LR

; interrupt level context switch
OSIntCtxSw
        PUSH    {R4, R5}
        LDR     R4, =NVIC_INT_CTRL
        LDR     R5, =NVIC_PENDSVSET
        STR     R5, [R4]
        POP     {R4, R5}
        BX      LR
        NOP

; the context switch: save R4-R11 of the current task on its stack, switch, restore the new task's
PendSV_Handler
        CPSID   I
        MRS     R0, PSP
        CBZ     R0, PendSV_Handler_NoSave
        SUB     R0, R0, #0x20
        STM     R0, {R4-R11}
        LDR     R4, __OSTCBCur
        LDR     R4, [R4]
        STR     R0, [R4]                    ; OSTCBCur->StkPtr = SP
        CPSIE   I
PendSV_Handler_NoSave
        LDR     R4, __OSPrioCur
        LDR     R5, __OSPrioHighRdy
        LDRB    R6, [R5]
        STRB    R6, [R4]                    ; OSPrioCur = OSPrioHighRdy
        LDR     R4, __OSTCBCur
        LDR     R6, __OSTCBHighRdy
        LDR     R6, [R6]
        STR     R6, [R4]                    ; OSTCBCur = OSTCBHighRdy
        LDR     R0, [R6]                    ; SP = OSTCBHighRdy->StkPtr
        LDM     R0, {R4-R11}
        ADD     R0, R0, #0x20
        MSR     PSP, R0
        ORR     LR, LR, #0x04               ; return to thread mode on the process stack
        CPSIE   I
        BX      LR

        ALIGN
__OSIntExit     DCD     OSIntExit
__OSPrioCur     DCD     OSPrioCur
__OSPrioHighRdy DCD     OSPrioHighRdy
__OSTCBCur      DCD     OSTCBCur
__OSTCBHighRdy  DCD     OSTCBHighRdy

        END
