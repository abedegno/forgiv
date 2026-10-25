; boot_main.asm: the boot loader's entry. The boot ROM jumps to codestart (0x3F7FF6,
; CodeStartBranch.asm), which branches here (0x3F5226).
;
; What it does: GPIO ports A and B become inputs with their pull-ups off. If the application's
; valid word at 0x3F3FFC (app_header.c) is 0xAA, it disables the watchdog, clears the RAM blocks
; M0, M1 and L0..L8, loads ST0 = 0 and ST1 = 0x0A0B and branches to the application's entry
; 0x3E8004 (start, os_main.c, which runs c_int00). Otherwise it disables the watchdog and
; interrupts and branches to boot_update (0x3F5000), the update path the ARM's DSPStore.bin
; update also uses (boot_flash_loader.c). No other check is made: there is no checksum or
; signature of the application, only the 0xAA word.
;
; Entry point: _boot_main. Data: none (one word of stack for the volatile copy of the valid word).
; Neighbours: CodeStartBranch.asm, boot_update.c, app_header.c, rtos/os_main.c (start).
; name: descriptive (map/filenames.tsv), name kept.
;
; Why this is assembly and not C (C with asm statements was tried, see docs/hard.md and
; docs/notes.md):
;  - the GpioCtrlRegs writes come before the frame is allocated (ADDB SP,#4 follows EDIS). A TI
;    C compiler always allocates the frame first.
;  - nothing follows the final LB to the application: no SUBB SP and no LRETR. Every TI C
;    compiler emits the epilogue of a function whose last statement is an asm statement.
;  - RPT || MOV block clears, POP ST1 / POP ST0 and LB to absolute addresses are boot ROM style
;    code that C cannot express.
; The middle part (the volatile local read back, the SBF/SB/SB pattern of an if/else) looks like
; compiler output that was pasted in and edited; it is kept exactly as the image has it.

        .def    _boot_main
        .ref    _boot_update            ; 0x3F5000, the update path
        .ref    _app_entry              ; 0x3E8004, the application

GPIO_CTRL_DP    .set    0x6F80 >> 6     ; GpioCtrlRegs data page
GPADIR          .set    0x0A            ; offsets in that page
GPAPUD          .set    0x0C
GPBDIR          .set    0x1A
GPBPUD          .set    0x1C
WDCR            .set    0x7029          ; watchdog control
WD_DISABLE_KEY  .set    0x0068          ; WDDIS = 1, WDCHK = 101b
APP_VALID       .set    0x3F3FFC        ; 0xAA when an application is loaded
APP_VALID_KEY   .set    0xAA

        .text

; the GPIO set-up, then the valid word test
_boot_main:
        EALLOW
        MOVB    ACC, #0
        MOVW    DP, #GPIO_CTRL_DP
        MOVL    @GPADIR, ACC            ; port A: all inputs
        SUBB    ACC, #1
        MOVL    @GPAPUD, ACC            ; pull-ups off
        MOVB    ACC, #0
        MOVL    @GPBDIR, ACC            ; port B: all inputs
        SUBB    ACC, #1
        MOVL    @GPBPUD, ACC            ; pull-ups off
        EDIS

        ADDB    SP, #4
        MOVL    XAR4, #APP_VALID
        MOV     AL, *+XAR4[0]
        MOV     *-SP[3], AL             ; volatile Uint16 valid = *APP_VALID
        MOV     AL, *-SP[3]
        CMPB    AL, #APP_VALID_KEY
        SBF     no_app, NEQ
        SB      start_app, UNC
no_app:
        SB      update, UNC
; no valid application: interrupts and watchdog off, then the update routine
update:
        SETC    OBJMODE
        SETC    INTM
        MOVL    XAR1, #WDCR
        EALLOW
        MOV     *+XAR1[0], #WD_DISABLE_KEY
        EDIS
        LB      _boot_update

; a valid application: watchdog off, RAM cleared, then the application
start_app:
        MOVL    XAR0, #WDCR
        EALLOW
        MOV     *+XAR0[0], #WD_DISABLE_KEY
        EDIS
        MOVB    AL, #0                  ; clear the RAM blocks
        MOVB    XAR5, #0x0              ; M0: 0x0000..0x03FF
        MOV     AR4, #0x03FF
        RPT     AR4
||      MOV     *XAR5++, AL
        MOVL    XAR5, #0x000400         ; M1: 0x0400..0x07FF
        MOV     AR4, #0x03FF
        RPT     AR4
||      MOV     *XAR5++, AL
        MOVL    XAR5, #0x008000         ; L0..L3: 0x8000..0x8FFF
        MOV     AR4, #0x0FFF
        RPT     AR4
||      MOV     *XAR5++, AL
        MOVL    XAR5, #0x009000         ; L4: 0x9000..0x9FFF
        MOV     AR4, #0x0FFF
        RPT     AR4
||      MOV     *XAR5++, AL
        MOVL    XAR5, #0x00A000         ; L5..L8, 8K words each
        MOV     AR4, #0x1FFF
        RPT     AR4
||      MOV     *XAR5++, AL
        MOVL    XAR5, #0x00C000
        MOV     AR4, #0x1FFF
        RPT     AR4
||      MOV     *XAR5++, AL
        MOVL    XAR5, #0x00E000
        MOV     AR4, #0x1FFF
        RPT     AR4
||      MOV     *XAR5++, AL
        MOVL    XAR5, #0x010000
        MOV     AR4, #0x1FFF
        RPT     AR4
||      MOV     *XAR5++, AL
        MOVL    XAR5, #0x012000
        MOV     AR4, #0x1FFF
        RPT     AR4
||      MOV     *XAR5++, AL

        MOVZ    AR0, AL                 ; the registers the application sees
        MOVZ    AR5, AL
        MOVZ    AR4, AL
        MOV     *SP++, #0               ; ST0 = 0x0000
        MOV     *SP++, #0x0A0B          ; ST1 = 0x0A0B (INTM, DBGM, VMAP, OBJMODE, M0M1MAP)
        POP     ST1
        POP     ST0
        LB      _app_entry

        .end
