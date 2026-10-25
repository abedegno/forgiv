; CodeStartBranch.asm: TI's CodeStartBranch.asm. For a boot to flash, the boot ROM jumps to
; the "codestart" section at 0x3F7FF6 (the two words before the CSM passwords), which branches
; to the start of the program.
;
; The one change from TI's text: the branch goes to the boot loader's entry _boot_main
; (0x3F5226, boot_main.asm), not to _c_int00. WD_DISABLE is 0 as in TI's file, so the watchdog
; is left alone here; boot_main turns it off on both of its paths.
; Entry point: code_start. Data: none. Neighbours: boot/boot_main.asm.
; name: original (map/filenames.tsv): TI's file name.

WD_DISABLE	.set	0		;set to 1 to disable WD, else set to 0

    .ref _boot_main
    .global code_start

;***********************************************************************
;* Function: codestart section
;*
;* Description: Branch to code starting point
;***********************************************************************

    .sect "codestart"

code_start:
    .if WD_DISABLE == 1
        LB wd_disable       ;Branch to watchdog disable code
    .else
        LB _boot_main       ;Branch to the start of the program
    .endif

;end codestart section

;***********************************************************************
;* Function: wd_disable
;*
;* Description: Disables the watchdog timer
;***********************************************************************
    .if WD_DISABLE == 1

    .text
wd_disable:
    SETC OBJMODE        ;Set OBJMODE for 28x object code
    EALLOW              ;Enable EALLOW protected register access
    MOVZ DP, #7029h>>6  ;Set data page for WDCR register
    MOV @7029h, #0068h  ;Set WDDIS bit in WDCR to disable WD
    EDIS                ;Disable EALLOW protected register access
    LB _boot_main       ;Branch to the start of the program

    .endif

;end wd_disable

	.end
