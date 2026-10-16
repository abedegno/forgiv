; os_cpu_a.asm: the RTOS's context switch (C28x assembly).
;
; A task's saved context is the interrupt frame the CPU pushes (ST0, T, AL, AH, PL, PH, AR0, AR1,
; ST1, DP, IER, DBGSTAT, the return address), then AR1H:AR0H, XAR2 .. XAR7, XT and RPC. The
; stack pointer goes in the first word of the task's control block (OS_TCB.sp, os.h). os_stack_init
; (os_tick.c) builds the same frame for a new task.
;
; Entry points:
;   os_start_high  starts the first task: restores os_cur_tcb's context (from os_start).
;   os_ctx_sw_isr  the TRAP #31 handler (USER12 in the PIE table, set by init_system), entered
;                  from os_sched: saves the context in os_cur_tcb, makes os_next_tcb current and
;                  restores it. ASP/NASP keep the stack even-aligned for the 32-bit pushes.
;   os_int_ctx_sw  the same from an interrupt (os_sched_isr): drops two return addresses and
;                  joins os_ctx_sw_isr after its save. os_sched_isr has no callers, so this is
;                  never run.
; Data: none of its own; os_cur_tcb and os_next_tcb (os_task.c).
; Neighbours: os_main.c (os_sched), os_task.c, os_tick.c, drivers/device_init.c (the vector).
; name: descriptive (map/filenames.tsv); the name follows the uC/OS-II port convention (the RTOS
; is a small custom one with uC/OS-like names); name kept.

        .ref    _os_cur_tcb
        .ref    _os_next_tcb
        .def    _os_start_high
        .def    _os_ctx_sw_isr
        .def    _os_int_ctx_sw

        .text

; restore the first task (os_start)
_os_start_high:
        MOVW    DP, #_os_cur_tcb
        MOVL    XAR6, @_os_cur_tcb      ; SP = os_cur_tcb->sp
        MOVL    ACC, *+XAR6[0]
        MOV     SP, AL
        POP     RPC                     ; restore the task's context
        MOVL    XT, *--SP
        MOVL    XAR7, *--SP
        MOVL    XAR6, *--SP
        MOVL    XAR5, *--SP
        MOVL    XAR4, *--SP
        MOVL    XAR3, *--SP
        MOVL    XAR2, *--SP
        POP     AR1H:AR0H
        POP     DP:ST1
        IRET

; TRAP #31 from os_sched: save the running task, restore os_next_tcb
_os_ctx_sw_isr:
        ASP
        PUSH    DP:ST1                  ; save the task's context
        PUSH    AR1H:AR0H
        MOVL    *SP++, XAR2
        MOVL    *SP++, XAR3
        MOVL    *SP++, XAR4
        MOVL    *SP++, XAR5
        MOVL    *SP++, XAR6
        MOVL    *SP++, XAR7
        MOVL    *SP++, XT
        PUSH    RPC
os_ctx_sw_save:
        MOVW    DP, #_os_cur_tcb
        MOVL    XAR6, @_os_cur_tcb      ; os_cur_tcb->sp = SP
        MOV     AL, SP
        MOVL    *+XAR6[0], ACC
        MOVW    DP, #_os_next_tcb
        MOVL    XAR7, @_os_next_tcb     ; SP = os_next_tcb->sp
        MOVL    ACC, *+XAR7[0]
        MOV     SP, AL
        MOVL    ACC, @_os_next_tcb      ; os_cur_tcb = os_next_tcb
        MOVL    @_os_cur_tcb, ACC
        POP     RPC                     ; restore the new task's context
        MOVL    XT, *--SP
        MOVL    XAR7, *--SP
        MOVL    XAR6, *--SP
        MOVL    XAR5, *--SP
        MOVL    XAR4, *--SP
        MOVL    XAR3, *--SP
        MOVL    XAR2, *--SP
        POP     AR1H:AR0H
        POP     DP:ST1
        NASP
        IRET

; the switch from an interrupt (os_sched_isr, which has no callers)
_os_int_ctx_sw:
        POP     RPC                     ; drop the return addresses of os_sched_isr
        POP     RPC                     ; and of its caller
        SB      os_ctx_sw_save, UNC

        .end
