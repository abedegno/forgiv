; F2806x_usDelay.asm: TI's DSP28x_usDelay (F2806x_usDelay.asm), unchanged: a delay loop of
; 5 cycles per count, run from RAM (section ramfuncs; here it is the last ramfunc, loaded at
; 0x3F336B and run at 0x97B4).
;
; The delay macro DELAY_US(A) (TI's F2806x_Examples.h; device_init.c and op_states.c define it
; again) gives the loop count with CPU_RATE 11.111 ns (90 MHz):
;     DSP28x_usDelay(((((long double) A * 1000.0L) / (long double)CPU_RATE) - 9.0L) / 5.0L)
; meas_task.c calls it with the count 1798 directly (about 100 us).
; Entry point: DSP28x_usDelay. Data: none.
; Neighbours: drivers/device_init.c, app/state/op_states.c, app/meas/meas_task.c.
; name: original (map/filenames.tsv): TI's file and function name.

       .def _DSP28x_usDelay
       .sect "ramfuncs"

        .global  __DSP28x_usDelay
_DSP28x_usDelay:
        SUB    ACC,#1
        BF     _DSP28x_usDelay,GEQ    ;; Loop if ACC >= 0
        LRETR

;There is a 9/10 cycle overhead and each loop
;takes five cycles. The LoopCount is given by
;the following formula:
;  DELAY_CPU_CYCLES = 9 + 5*LoopCount
; LoopCount = (DELAY_CPU_CYCLES - 9) / 5
; The macro DELAY_US(A) performs this calculation for you
;==================================================

       .end
