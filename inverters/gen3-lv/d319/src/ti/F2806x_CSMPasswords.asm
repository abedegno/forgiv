; F2806x_CSMPasswords.asm: TI's F2806x_CSMPasswords.asm with this build's password: the
; section csmpasswds (the eight password words PWL0..PWL7 at 0x3F7FF8) and csm_rsvd
; (0x3F7F80 .. 0x3F7FF5, which must be 0 when a password is set).
;
; The change from TI's text: TI's file has 0xFFFF in all eight words (no password). This build
; has 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xF5F4, 0xF0F3, 0xF5F2, 0xF2F1, so the device is secure
; after reset. CsmUnlock (boot/boot_csm.c) writes the same words as keys before the update
; routine runs from RAM. The words sit in sector A, which the update never erases or writes
; (boot_flash_loader.c). They are in the clear in DSPStore.bin, which is the whole flash
; image 0x3E8000..0x3F7FFF (D316_DSPStore.bin).
; Entry points: none (data). Neighbours: boot/boot_csm.c.
; name: original (map/filenames.tsv): TI's file name.

; The "csmpasswds" section contains the actual CSM passwords that will be
; linked and programmed into to the CSM password locations (PWL) in flash.
; These passwords must be known in order to unlock the CSM module.

      .sect "csmpasswds"

      .int	0xFFFF		;PWL0 (LSW of 128-bit password)
      .int	0xFFFF		;PWL1
      .int	0xFFFF		;PWL2
      .int	0xFFFF		;PWL3
      .int	0xF5F4		;PWL4
      .int	0xF0F3		;PWL5
      .int	0xF5F2		;PWL6
      .int	0xF2F1		;PWL7 (MSW of 128-bit password)

;----------------------------------------------------------------------

; For code security operation, all addresses between 0x3F7F80 and
; 0x3F7FF5 cannot be used as program code or data.  These locations
; must be programmed to 0x0000 when the code security password locations
; (PWL) are programmed.  If security is not a concern, then these addresses
; can be used for code or data.

; The section "csm_rsvd" can be used as a program section to
; initialize these locations to 0x0000.

        .sect "csm_rsvd"
        .loop (3F7FF5h - 3F7F80h + 1)
              .int 0x0000
        .endloop

;----------------------------------------------------------------------

      .end
