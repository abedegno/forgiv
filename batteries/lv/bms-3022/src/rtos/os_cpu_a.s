; opts: --cpu Cortex-M3
; at: OSCtxSw 0x0802A4FC
; at: OSIntCtxSw 0x0802A50A
; at: OSIntExit 0x0802A6AC
; at: OSPrioCur 0x2000061E
; at: OSPrioHighRdy 0x2000061C
; at: OSStartHighRdy 0x0802A4E4
; at: OSTCBCur 0x20000624
; at: OSTCBHighRdy 0x20000620
; at: PendSV_Handler 0x0802A51A
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
