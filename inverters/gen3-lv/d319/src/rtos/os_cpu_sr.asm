; os_cpu_sr.asm: the RTOS's critical-section helpers (C28x assembly).
;
; Entry points:
;   os_cpu_sr_save     returns ST1 (with the old INTM) in AL and disables interrupts;
;   os_cpu_sr_restore  writes AL back to ST1.
; Nothing in this image calls them: the C code uses DINT and EINT instead. They sit at the end
; of .text, apart from the context switch (os_cpu_a.asm), so they were a file of their own.
; Data: none. Neighbours: os_cpu_a.asm.
; name: descriptive (map/filenames.tsv), after the uC/OS-II OS_CPU_SR save and restore; name
; kept.

        .def    _os_cpu_sr_save
        .def    _os_cpu_sr_restore

        .text

; unused: AL = ST1, then INTM and DBGM set
_os_cpu_sr_save:
        PUSH    ST1
        SETC    INTM, DBGM
        MOV     AL, *--SP
        LRETR

; unused: ST1 = AL
_os_cpu_sr_restore:
        MOV     *SP++, AL
        POP     ST1
        LRETR

        .end
