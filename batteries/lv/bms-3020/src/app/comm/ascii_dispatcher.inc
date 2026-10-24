; Interim exact assembler dispatcher; native C draft remains DF70 108.
; Composed with nine C functions into one .text area by tools/ascii_compose.py.
; opts: --cpu Cortex-M3
; at: sub_0801D424 0x0801D424
; at: sub_0801D606 0x0801D606
; at: sub_0801D6E4 0x0801D6E4
; at: sub_0801D8C0 0x0801D8C0
; at: sub_0801DB3A 0x0801DB3A
; at: sub_0801DF70 0x0801DF70
; at: sub_08020D52 0x08020D52
; at: sub_08020E40 0x08020E40
; at: sub_08020FA4 0x08020FA4
; at: sub_0802126C 0x0802126C
; at: pace_buf 0x200035EA
; at: modbus_tx 0x20003694
; at: modbus_resp 0x2000372B
; at: modbus_regs 0x200037C2
; at: rx_ready 0x2000050E
; at: tx_cid2 0x2000050F
; at: mb_addr 0x20000510
; at: rx_count 0x20000511
; at: rx_idx 0x20000512
; at: ascii_rx_state 0x20000513
; at: tx_pack 0x20000514
; at: mb_len 0x20000515
; at: mb_wait 0x20000516
; at: tx_len 0x20000518
; at: tx_idx 0x2000051A
; at: rx_info_len 0x2000051C
; at: tx_sum 0x2000051E
; at: mb_crc 0x20000520
; at: pace_mode 0x200000DE
; at: hexbuf 0x20000109
; at: packs 0x20003D52
; at: cfgs 0x20004149
; at: measure 0x200014D2
; at: limits 0x20001522
; at: alarm_mask 0x20000275
; at: charging 0x2000009F
; at: pack_count 0x2000009E
; at: own_addr 0x20000163
; at: g_20000180 0x20000180
; at: g_2000017C 0x2000017C
; at: g_20000182 0x20000182
; at: g_20000164 0x20000164
; at: g_2000016C 0x2000016C
; at: g_20000162 0x20000162
; at: g_20000077 0x20000077
; at: g_20000062 0x20000062
; at: g_200000A2 0x200000A2
; at: g_20000124 0x20000124
; at: g_20000110 0x20000110
; at: g_20000148 0x20000148
; at: g_20000184 0x20000184
; at: g_20000185 0x20000185
; at: g_20000271 0x20000271
; at: g_20000272 0x20000272
; at: g_20000273 0x20000273
; at: g_2000050D 0x2000050D
; at: g_20000274 0x20000274
; at: g_20000276 0x20000276
; at: g_20000277 0x20000277
; at: g_20000278 0x20000278
; at: g_2000010C 0x2000010C
; at: g_2000010E 0x2000010E
; at: g_20001094 0x20001094
; at: g_2000012A 0x2000012A
; at: g_2000012C 0x2000012C
; at: g_2000119E 0x2000119E
; at: g_20000051 0x20000051
; at: g_20000130 0x20000130
; at: g_20000134 0x20000134
; at: g_200000A0 0x200000A0
; at: g_20000138 0x20000138
; at: g_20000139 0x20000139
; at: g_20000098 0x20000098
; at: g_200000AD 0x200000AD
; at: prot 0x20001552
; at: recov 0x2000153C
; at: delays 0x200002F8
; at: alarm_mask2 0x20000276
; at: g_200000FA 0x200000FA
; at: g_200000F8 0x200000F8
; at: g_20000168 0x20000168
; at: g_200000A9 0x200000A9
; at: g_20000186 0x20000186
; at: g_20000096 0x20000096
; at: g_20000094 0x20000094
; at: g_20000154 0x20000154
; at: g_2000014C 0x2000014C
; at: g_20000150 0x20000150
; at: log_index 0x20000188
; at: g_2000018A 0x2000018A
; at: date_now 0x2000018C
; at: date_start 0x200000B6
; at: date_end 0x200000BC
; at: log_interval 0x200000B4
; at: g_200000CE 0x200000CE
; at: os_task_dly_idx 0x20000606
    THUMB
    AREA ascii_dispatch, CODE, READONLY, ALIGN=2
    EXPORT sub_0801DF70
sub_0801DF70 PROC

    PRESERVE8
    IMPORT USART_ITConfig
    IMPORT __aeabi_f2uiz
    IMPORT __aeabi_fdiv
    IMPORT __aeabi_fmul
    IMPORT __aeabi_ui2f
    IMPORT alarm_mask
    IMPORT date_end
    IMPORT date_now
    IMPORT date_start
    IMPORT delays
    IMPORT g_20000062
    IMPORT g_20000077
    IMPORT g_20000094
    IMPORT g_20000096
    IMPORT g_200000A2
    IMPORT g_200000A9
    IMPORT g_200000CE
    IMPORT g_200000F8
    IMPORT g_200000FA
    IMPORT g_20000110
    IMPORT g_20000148
    IMPORT g_2000014C
    IMPORT g_20000150
    IMPORT g_20000154
    IMPORT g_20000162
    IMPORT g_20000164
    IMPORT g_20000168
    IMPORT g_2000016C
    IMPORT g_20000182
    IMPORT g_20000186
    IMPORT g_2000018A
    IMPORT g_20000271
    IMPORT g_20000272
    IMPORT g_20000273
    IMPORT g_20000274
    IMPORT g_20000276
    IMPORT g_2000050D
    IMPORT hexbuf
    IMPORT limits
    IMPORT log_index
    IMPORT log_interval
    IMPORT os_task_delay
    IMPORT os_task_dly_idx
    IMPORT own_addr
    IMPORT pace_buf
    IMPORT pack_count
    IMPORT prot
    IMPORT recov
    IMPORT rx_ready
    IMPORT sub_08014FD6
    IMPORT sub_08015034
    IMPORT sub_080150EE
    IMPORT sub_0801512E
    IMPORT sub_080151D8
    IMPORT sub_080151EA
    IMPORT sub_0801535E
    IMPORT sub_08016F1E
    IMPORT sub_080172B2
    IMPORT sub_080179EA
    IMPORT sub_0801D8C0
    IMPORT sub_0801DB3A
    IMPORT sub_080223BC
    IMPORT sub_08022442
    IMPORT sub_08022488
    IMPORT tx_len
ascii_L_08021070 EQU sub_0801DF70 + 0x3100
ascii_L_08021074 EQU sub_0801DF70 + 0x3104
ascii_L_08021078 EQU sub_0801DF70 + 0x3108
ascii_L_0802107C EQU sub_0801DF70 + 0x310C
ascii_L_08021080 EQU sub_0801DF70 + 0x3110
ascii_L_08021084 EQU sub_0801DF70 + 0x3114
    PUSH.W {r0, r4, r5, r6, r7, r8, r9, r10, r11, lr}
    SUB.N sp, #0x10
    MOVS.N r5, #0
    MOVS.N r0, #0
    STR.N r0, [sp, #8]
    MOVW.W r0, #0xffff
    STR.N r0, [sp, #4]
    LDR.N r6, ascii_L_0801E270
    MOVS.N r0, #0
    STRH.N r0, [r6, #0xa]
    LDR.N r4, ascii_L_0801E274
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    MOV.N r11, r0
    LDRH.N r0, [r6, #0xa]
    MOVS.N r2, #0x7e
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    MOVS.N r2, #0x32
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    MOVS.N r2, #0x35
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
    LDR.N r0, ascii_L_0801E278
    LDR.N r1, ascii_L_0801E26C
    LDRB.N r0, [r0]
    BL sub_08015034
    LDR.W r9, ascii_L_0801E26C
    LDRH.N r0, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    MOV.N r0, r9
    LDRB.N r2, [r0, #1]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    MOVS.N r1, #0x34
    ADDS.N r2, r0, #1
    UXTH.N r2, r2
    STRB.N r1, [r4, r0]
    MOVS.N r0, #0x36
    ADDS.N r1, r2, #1
    UXTH.N r1, r1
    STRB.N r0, [r4, r2]
    MOVS.N r2, #0x30
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    MOVS.N r7, #0x30
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r7, [r4, r0]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r7, [r4, r1]
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r7, [r4, r0]
    MOVS.N r0, #0x30
    ADDS.N r3, r1, #1
    UXTH.N r7, r3
    STRB.N r0, [r4, r1]
    MOVS.N r1, #0x30
    ADDS.N r0, r7, #1
    UXTH.N r0, r0
    STRH.N r0, [r6, #0xa]
    STRB.N r1, [r4, r7]
    LDR.N r1, [sp, #0x10]
    MOVS.N r3, #0x31
    LDR.W r8, ascii_L_0801E27C
    LDR.N r7, ascii_L_0801E280
    LDR.W r10, ascii_L_0801E284
    CMP.N r1, #0xba
    BEQ.N ascii_L_0801E10C
    BGT.N ascii_L_0801E0B0
    CMP.N r1, #0xa8
    BEQ.N ascii_L_0801E10E
    BGT.N ascii_L_0801E078
    CMP.N r1, #0x9c
    BEQ.N ascii_L_0801E10E
    BGT.N ascii_L_0801E05E
    CMP.N r1, #0x90
    BEQ.N ascii_L_0801E110
    BGT.N ascii_L_0801E048
    CMP.N r1, #2
    BEQ.N ascii_L_0801E112
    CMP.N r1, #3
    BEQ.N ascii_L_0801E114
    CMP.N r1, #0x42
    BEQ.N ascii_L_0801E116
    CMP.N r1, #0x44
    BNE.N ascii_L_0801E118
    B.N ascii_L_0801E532
ascii_L_0801E048
    LDR.N r7, ascii_L_0801E288
    CMP.N r1, #0x99
    BEQ.N ascii_L_0801E11A
    LDR.N r3, ascii_L_0801E28C
    LDR.N r2, ascii_L_0801E290
    LDR.N r0, ascii_L_0801E294
    CMP.N r1, #0x9a
    BEQ.N ascii_L_0801E11C
    CMP.N r1, #0x9b
    BNE.N ascii_L_0801E118
    B.N ascii_L_0801E7B6
ascii_L_0801E05E
    SUBS.N r1, #0xa0
    CMP.N r1, #8
    BHS.N ascii_L_0801E15E
    TBH [pc, r1, lsl #1]
ascii_L_0801E068
    DCW (ascii_L_0801ECE2 - ascii_L_0801E068) / 2
    DCW (ascii_L_0801F060 - ascii_L_0801E068) / 2
    DCW (ascii_L_0801EEA2 - ascii_L_0801E068) / 2
    DCW (ascii_L_0801E15E - ascii_L_0801E068) / 2
    DCW (ascii_L_08020B94 - ascii_L_0801E068) / 2
    DCW (ascii_L_0801E8A2 - ascii_L_0801E068) / 2
    DCW (ascii_L_0801E8C6 - ascii_L_0801E068) / 2
    DCW (ascii_L_0801E9AA - ascii_L_0801E068) / 2
ascii_L_0801E078
    SUBS.N r1, #0xa9
    LDR.W r8, ascii_L_0801E298
    LDR.W r10, ascii_L_0801E29C
    MOV.W r7, #0x3f800000
    CMP.N r1, #0x11
    BHS.N ascii_L_0801E15E
    TBH [pc, r1, lsl #1]
ascii_L_0801E08E
    DCW (ascii_L_0801EE6E - ascii_L_0801E08E) / 2
    DCW (ascii_L_0801E15E - ascii_L_0801E08E) / 2
    DCW (ascii_L_0801EF56 - ascii_L_0801E08E) / 2
    DCW (ascii_L_0801EFDC - ascii_L_0801E08E) / 2
    DCW (ascii_L_0801EED6 - ascii_L_0801E08E) / 2
    DCW (ascii_L_0801E160 - ascii_L_0801E08E) / 2
    DCW (ascii_L_0801ED5C - ascii_L_0801E08E) / 2
    DCW (ascii_L_0801E15E - ascii_L_0801E08E) / 2
    DCW (ascii_L_0801F178 - ascii_L_0801E08E) / 2
    DCW (ascii_L_0801F114 - ascii_L_0801E08E) / 2
    DCW (ascii_L_0801F0F6 - ascii_L_0801E08E) / 2
    DCW (ascii_L_08020BC2 - ascii_L_0801E08E) / 2
    DCW (ascii_L_0801E160 - ascii_L_0801E08E) / 2
    DCW (ascii_L_0801EDF4 - ascii_L_0801E08E) / 2
    DCW (ascii_L_0801F4FA - ascii_L_0801E08E) / 2
    DCW (ascii_L_0801F512 - ascii_L_0801E08E) / 2
    DCW (ascii_L_0801F26A - ascii_L_0801E08E) / 2
ascii_L_0801E0B0
    CMP.N r1, #0xd9
    BEQ.W ascii_L_0801FD44
    BGT.N ascii_L_0801E0FA
    CMP.N r1, #0xd1
    BEQ.N ascii_L_0801E1B8
    BGT.N ascii_L_0801E0E2
    CMP.N r1, #0xc1
    BEQ.N ascii_L_0801E1BA
    BGT.N ascii_L_0801E0D4
    LDR.N r7, ascii_L_0801E2A0
    CMP.N r1, #0xbb
    BEQ.N ascii_L_0801E1BC
    CMP.N r1, #0xbc
    BEQ.N ascii_L_0801E1BE
    CMP.N r1, #0xbd
    BNE.N ascii_L_0801E15E
    B.N ascii_L_0801E850
ascii_L_0801E0D4
    CMP.N r1, #0xc2
    BEQ.N ascii_L_0801E1C0
    CMP.N r1, #0xc3
    BEQ.N ascii_L_0801E1C2
    CMP.N r1, #0xd0
    BNE.N ascii_L_0801E15E
    B.N ascii_L_0801E8C4
ascii_L_0801E0E2
    SUBS.N r1, #0xd2
    CMP.N r1, #7
    BHS.N ascii_L_0801E15E
    TBH [pc, r1, lsl #1]
ascii_L_0801E0EC
    DCW (ascii_L_0801F706 - ascii_L_0801E0EC) / 2
    DCW (ascii_L_0801F7E4 - ascii_L_0801E0EC) / 2
    DCW (ascii_L_0801F8E8 - ascii_L_0801E0EC) / 2
    DCW (ascii_L_0801F9B6 - ascii_L_0801E0EC) / 2
    DCW (ascii_L_0801FABE - ascii_L_0801E0EC) / 2
    DCW (ascii_L_0801FB9A - ascii_L_0801E0EC) / 2
    DCW (ascii_L_0801FC9E - ascii_L_0801E0EC) / 2
ascii_L_0801E0FA
    CMP.N r1, #0xe1
    BEQ.W ascii_L_08020638
    BGT.N ascii_L_0801E130
    SUB.W r1, r1, #0xda
    CMP.N r1, #7
    BHS.N ascii_L_0801E15E
    B.N ascii_L_0801E11E
ascii_L_0801E10C
    B.N ascii_L_0801E90A
ascii_L_0801E10E
    B.N ascii_L_0801E160
ascii_L_0801E110
    B.N ascii_L_0801E182
ascii_L_0801E112
    B.N ascii_L_0801E17A
ascii_L_0801E114
    B.N ascii_L_0801E17E
ascii_L_0801E116
    B.N ascii_L_0801E5A6
ascii_L_0801E118
    B.N ascii_L_0801E15E
ascii_L_0801E11A
    B.N ascii_L_0801E61A
ascii_L_0801E11C
    B.N ascii_L_0801E722
ascii_L_0801E11E
    TBH [pc, r1, lsl #1]
ascii_L_0801E122
    DCW (ascii_L_0801FE0A - ascii_L_0801E122) / 2
    DCW (ascii_L_0801FEBA - ascii_L_0801E122) / 2
    DCW (ascii_L_0801FF82 - ascii_L_0801E122) / 2
    DCW (ascii_L_080200E6 - ascii_L_0801E122) / 2
    DCW (ascii_L_08020292 - ascii_L_0801E122) / 2
    DCW (ascii_L_080203F2 - ascii_L_0801E122) / 2
    DCW (ascii_L_080205A0 - ascii_L_0801E122) / 2
ascii_L_0801E130
    CMP.N r1, #0xe5
    BEQ.W ascii_L_08020B26
    BGT.N ascii_L_0801E14C
    CMP.N r1, #0xe2
    BEQ.W ascii_L_08020A00
    CMP.N r1, #0xe3
    BEQ.W ascii_L_08020A40
    CMP.N r1, #0xe4
    BNE.N ascii_L_0801E15E
    B.W ascii_L_08020AA0
ascii_L_0801E14C
    CMP.N r1, #0xe6
    BEQ.W ascii_L_0802071C
    CMP.N r1, #0xe7
    BEQ.W ascii_L_08020854
    CMP.N r1, #0xff
    BEQ.W ascii_L_08020BE6
ascii_L_0801E15E
    MOVS.N r5, #4
ascii_L_0801E160
    MOV.N r1, r5
    LDR.N r0, ascii_L_0801E274
    BL sub_0801512E
    LDR.N r0, [sp, #0x10]
    CMP.N r0, #0x42
ascii_L_0801E16C
    BEQ.W ascii_L_08020C8A
    LDR.N r0, [sp, #0x10]
    CMP.N r0, #0x44
    BEQ.N ascii_L_0801E16C
    B.W ascii_L_08020CC8
ascii_L_0801E17A
    MOVS.N r5, #2
    B.N ascii_L_0801E160
ascii_L_0801E17E
    MOVS.N r5, #3
    B.N ascii_L_0801E160
ascii_L_0801E182
    LDR.N r1, ascii_L_0801E278
    LDRB.N r1, [r1]
    CMP.N r1, #1
    BEQ.N ascii_L_0801E198
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
    B.N ascii_L_0801E160
ascii_L_0801E198
    LDR.N r0, ascii_L_0801E2A4
    LDR.N r1, ascii_L_0801E26C
    LDRB.N r0, [r0]
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    B.N ascii_L_0801E1C4
ascii_L_0801E1B8
    B.N ascii_L_0801E9A4
ascii_L_0801E1BA
    B.N ascii_L_0801E1C8
ascii_L_0801E1BC
    B.N ascii_L_0801E9A6
ascii_L_0801E1BE
    B.N ascii_L_0801E9A8
ascii_L_0801E1C0
    B.N ascii_L_0801E1F8
ascii_L_0801E1C2
    B.N ascii_L_0801E228
ascii_L_0801E1C4
    STRB.N r2, [r4, r0]
ascii_L_0801E1C6
    B.N ascii_L_0801E160
ascii_L_0801E1C8
    MOVS.N r7, #0
    LDR.W r8, ascii_L_0801E2A8
ascii_L_0801E1CE
    LDR.N r1, ascii_L_0801E26C
    LDRB.W r0, [r8, r7]
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    ADDS.N r7, r7, #1
    UXTH.N r7, r7
    CMP.N r7, #0x14
    BLO.N ascii_L_0801E1CE
    B.N ascii_L_0801E160
ascii_L_0801E1F8
    MOVS.N r7, #0
    LDR.W r8, ascii_L_0801E2AC
ascii_L_0801E1FE
    LDR.N r1, ascii_L_0801E26C
    LDRB.W r0, [r8, r7]
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    ADDS.N r7, r7, #1
    UXTH.N r7, r7
    CMP.N r7, #0x14
    BLO.N ascii_L_0801E1FE
    B.N ascii_L_0801E160
ascii_L_0801E228
    MOV.W r10, #0
ascii_L_0801E22C
    ADD.W r0, r4, r10, lsl #1
    LDRB.N r1, [r0, #0xe]
    LDRB.N r0, [r0, #0xd]
    BL sub_08014FD6
    MOV.N r1, r0
    LDR.N r0, ascii_L_0801E2AC
    STRB.W r1, [r0, r10]
    ADD.W r10, r10, #1
    UXTH.W r10, r10
    CMP.W r10, #0x14
    BLO.N ascii_L_0801E22C
    MOV.W r10, #0
ascii_L_0801E252
    LDR.N r0, ascii_L_0801E2AC
    ADD.W r1, r0, r10, lsl #1
    LDRB.W r0, [r0, r10, lsl #1]
    LDRB.N r1, [r1, #1]
    ADD.W r0, r1, r0, lsl #8
    UXTH.N r1, r0
    MOVS.N r0, #0x3c
    ADD.W r0, r0, r10, lsl #1
    B.N ascii_L_0801E2B0
ascii_L_0801E26C
    DCD hexbuf
ascii_L_0801E270
    DCD rx_ready
ascii_L_0801E274
    DCD pace_buf
ascii_L_0801E278
    DCD own_addr
ascii_L_0801E27C
    DCD limits
ascii_L_0801E280
    DCD prot
ascii_L_0801E284
    DCD alarm_mask
ascii_L_0801E288
    DCD g_20000273
ascii_L_0801E28C
    DCD g_20000274
ascii_L_0801E290
    DCD g_20000271
ascii_L_0801E294
    DCD g_20000272
ascii_L_0801E298
    DCD 0xC0B33333
ascii_L_0801E29C
    DCD date_now
ascii_L_0801E2A0
    DCD date_end
ascii_L_0801E2A4
    DCD pack_count
ascii_L_0801E2A8
    DCD g_20000062
ascii_L_0801E2AC
    DCD g_20000077
ascii_L_0801E2B0
    UXTH.N r0, r0
    BL sub_08022442
    ADD.W r10, r10, #1
    UXTH.W r10, r10
    CMP.W r10, #0xa
    BLO.N ascii_L_0801E252
    LDR.N r1, ascii_L_0801E6A4
    LDRB.N r0, [r1]
    CMP.N r0, #0x42
    BEQ.N ascii_L_0801E302
    CMP.N r0, #0x44
    BEQ.N ascii_L_0801E3CC
    LDR.N r1, ascii_L_0801E6A8
    MOVS.N r0, #0
    STRH.N r0, [r1]
    LDR.N r0, ascii_L_0801E6AC
    MOVS.N r1, #2
    STRH.N r1, [r0]
    LDR.N r0, ascii_L_0801E6B0
    MOVW.W r1, #0x1770
    LDRH.N r0, [r0]
    CMP.N r0, r1
    BHI.N ascii_L_0801E3CE
    MVN.W r0, #0x3b
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x3c
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x40
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x41
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801E50C
ascii_L_0801E302
    LDR.N r2, ascii_L_0801E6A8
    MOVS.N r0, #0
    STRH.N r0, [r2]
    LDRB.N r0, [r1, #1]
    CMP.N r0, #0x47
    BNE.N ascii_L_0801E318
    LDR.N r1, ascii_L_0801E6B0
    LDRH.N r1, [r1]
    CMP.W r1, #0x3e80
    BEQ.N ascii_L_0801E31E
ascii_L_0801E318
    CMP.N r0, #0x59
    BEQ.N ascii_L_0801E33E
    B.N ascii_L_0801E348
ascii_L_0801E31E
    LDR.N r1, ascii_L_0801E6AC
    MOVS.N r0, #1
    STRH.N r0, [r1]
    MVN.W r0, #0x45
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x46
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x4a
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x4b
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801E3AC
ascii_L_0801E33E
    LDR.N r0, ascii_L_0801E6B0
    LDRH.N r0, [r0]
    CMP.W r0, #0x3e80
    BEQ.N ascii_L_0801E374
ascii_L_0801E348
    LDR.N r1, ascii_L_0801E6AC
    MOVS.N r0, #2
    STRH.N r0, [r1]
    LDR.N r0, ascii_L_0801E6B0
    MOVW.W r1, #0x1770
    LDRH.N r0, [r0]
    CMP.N r0, r1
    BHI.N ascii_L_0801E394
    MVN.W r0, #0x3b
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x3c
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x40
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x41
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801E3AC
ascii_L_0801E374
    LDR.N r0, ascii_L_0801E6AC
    MOVS.N r1, #4
    STRH.N r1, [r0]
    MVN.W r0, #0x59
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x5a
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x5e
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x5f
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801E3AC
ascii_L_0801E394
    MVN.W r0, #0x45
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x46
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x4a
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x4b
    STRH.N r0, [r7, #8]
ascii_L_0801E3AC
    LDRH.W r1, [r8, #0xa]
    MOVS.N r0, #0xa8
    BL sub_08022442
    LDRH.N r1, [r7, #0xa]
    MOVS.N r0, #0xaa
    BL sub_08022442
    LDRH.W r1, [r8, #8]
    MOVS.N r0, #0xa2
    BL sub_08022442
    LDRH.N r1, [r7, #8]
    B.N ascii_L_0801E3D0
ascii_L_0801E3CC
    B.N ascii_L_0801E3D8
ascii_L_0801E3CE
    B.N ascii_L_0801E4F4
ascii_L_0801E3D0
    MOVS.N r0, #0xa4
    BL sub_08022442
ascii_L_0801E3D6
    B.N ascii_L_0801E160
ascii_L_0801E3D8
    LDR.N r0, ascii_L_0801E6A8
    MOVS.N r2, #1
    STRH.N r2, [r0]
    LDRB.N r0, [r1, #1]
    CMP.N r0, #0x47
    BNE.N ascii_L_0801E3EE
    LDR.N r1, ascii_L_0801E6B0
    LDRH.N r1, [r1]
    CMP.W r1, #0x3e80
    BEQ.N ascii_L_0801E3F4
ascii_L_0801E3EE
    CMP.N r0, #0x59
    BEQ.N ascii_L_0801E412
    B.N ascii_L_0801E41C
ascii_L_0801E3F4
    LDR.N r0, ascii_L_0801E6AC
    STRH.N r2, [r0]
    MVN.W r0, #0x59
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x55
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x5e
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x5a
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801E4CE
ascii_L_0801E412
    LDR.N r1, ascii_L_0801E6B0
    LDRH.N r1, [r1]
    CMP.W r1, #0x3e80
    BEQ.N ascii_L_0801E42E
ascii_L_0801E41C
    CMP.N r0, #0x57
    BEQ.N ascii_L_0801E44E
    CMP.N r0, #0x58
    BEQ.N ascii_L_0801E44E
    CMP.N r0, #0x5a
    BEQ.N ascii_L_0801E44E
    CMP.N r0, #0x46
    BEQ.N ascii_L_0801E44E
    B.N ascii_L_0801E45A
ascii_L_0801E42E
    LDR.N r1, ascii_L_0801E6AC
    MOVS.N r0, #4
    STRH.N r0, [r1]
    MVN.W r0, #0x59
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x5a
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x5e
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x5f
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801E4CE
ascii_L_0801E44E
    LDR.N r0, ascii_L_0801E6B0
    LDRH.N r0, [r0]
    SUB.W r1, r0, #0x4800
    SUBS.N r1, #0xa8
    BEQ.N ascii_L_0801E484
ascii_L_0801E45A
    LDR.N r0, ascii_L_0801E6AC
    MOVS.N r1, #2
    STRH.N r1, [r0]
    LDR.N r0, ascii_L_0801E6B0
    LDRH.N r0, [r0]
    CMP.W r0, #0x3e80
    BLO.N ascii_L_0801E4A4
    MVN.W r0, #0x59
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x55
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x5e
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x5a
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801E4CE
ascii_L_0801E484
    LDR.N r1, ascii_L_0801E6AC
    MOVS.N r0, #4
    STRH.N r0, [r1]
    MVN.W r0, #0x59
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x5a
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x5e
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x5f
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801E4CE
ascii_L_0801E4A4
    MOVS.N r1, #0xc8
    UDIV.W r2, r0, r1
    ADDS.N r2, #0xa
    RSBS.N r2, r2, #0
    STRH.W r2, [r8, #0xa]
    UDIV.W r2, r0, r1
    ADDS.N r2, #0xa
    STRH.W r2, [r8, #8]
    UDIV.W r2, r0, r1
    ADDS.N r2, #0xf
    RSBS.N r2, r2, #0
    STRH.N r2, [r7, #0xa]
    UDIV.W r0, r0, r1
    ADDS.N r0, #0xf
    STRH.N r0, [r7, #8]
ascii_L_0801E4CE
    LDRH.W r1, [r8, #0xa]
    MOVS.N r0, #0xa8
    BL sub_08022442
    LDRH.N r1, [r7, #0xa]
    MOVS.N r0, #0xaa
    BL sub_08022442
    LDRH.W r1, [r8, #8]
    MOVS.N r0, #0xa2
    BL sub_08022442
    LDRH.N r1, [r7, #8]
    MOVS.N r0, #0xa4
    BL sub_08022442
ascii_L_0801E4F2
    B.N ascii_L_0801E160
ascii_L_0801E4F4
    MVN.W r0, #0x45
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x46
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x4a
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x4b
    STRH.N r0, [r7, #8]
ascii_L_0801E50C
    LDRH.W r1, [r8, #0xa]
    MOVS.N r0, #0xa8
    BL sub_08022442
    LDRH.N r1, [r7, #0xa]
    MOVS.N r0, #0xaa
    BL sub_08022442
    LDRH.W r1, [r8, #8]
    MOVS.N r0, #0xa2
    BL sub_08022442
    LDRH.N r1, [r7, #8]
    MOVS.N r0, #0xa4
    BL sub_08022442
    B.N ascii_L_0801E160
ascii_L_0801E532
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    CMP.W r11, #0xff
    BEQ.N ascii_L_0801E57C
    LDR.N r1, ascii_L_0801E6B4
    MOV.N r0, r11
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
    CMP.W r11, #0
    BEQ.N ascii_L_0801E4F2
    CMP.W r11, #1
    BHI.N ascii_L_0801E4F2
    SUB.W r0, r11, #1
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801E6B8
    BL sub_0801DB3A
ascii_L_0801E57A
    B.N ascii_L_0801E160
ascii_L_0801E57C
    LDR.N r0, ascii_L_0801E6BC
    LDR.N r1, ascii_L_0801E6B4
    LDRB.N r0, [r0]
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
    LDR.N r1, ascii_L_0801E6B8
    MOVS.N r0, #0
    BL sub_0801DB3A
    B.N ascii_L_0801E160
ascii_L_0801E5A6
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    CMP.W r11, #0xff
    BEQ.N ascii_L_0801E5F0
    LDR.N r1, ascii_L_0801E6B4
    MOV.N r0, r11
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
    CMP.W r11, #0
    BEQ.N ascii_L_0801E4F2
    CMP.W r11, #1
    BHI.N ascii_L_0801E4F2
    SUB.W r0, r11, #1
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801E6B8
    BL sub_0801D8C0
    B.N ascii_L_0801E160
ascii_L_0801E5F0
    LDR.N r0, ascii_L_0801E6BC
    LDR.N r1, ascii_L_0801E6B4
    LDRB.N r0, [r0]
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
    LDR.N r1, ascii_L_0801E6B8
    MOVS.N r0, #0
    BL sub_0801D8C0
ascii_L_0801E618
    B.N ascii_L_0801E160
ascii_L_0801E61A
    SUB.W r0, r11, #6
    CMP.N r0, #8
    BHS.N ascii_L_0801E6D4
    TBB [pc, r0]
ascii_L_0801E626
    DCB (ascii_L_0801E62E - ascii_L_0801E626) / 2
    DCB (ascii_L_0801E63A - ascii_L_0801E626) / 2
    DCB (ascii_L_0801E646 - ascii_L_0801E626) / 2
    DCB (ascii_L_0801E652 - ascii_L_0801E626) / 2
    DCB (ascii_L_0801E65E - ascii_L_0801E626) / 2
    DCB (ascii_L_0801E66A - ascii_L_0801E626) / 2
    DCB (ascii_L_0801E698 - ascii_L_0801E626) / 2
    DCB (ascii_L_0801E6C8 - ascii_L_0801E626) / 2
ascii_L_0801E62E
    LDR.N r0, ascii_L_0801E6C0
    LDRB.N r1, [r0]
    AND.W r1, r1, #0xdf
    STRB.N r1, [r0]
    B.N ascii_L_0801E6D6
ascii_L_0801E63A
    LDR.N r0, ascii_L_0801E6C0
    LDRB.N r1, [r0]
    ORR.W r1, r1, #0x20
    STRB.N r1, [r0]
    B.N ascii_L_0801E6D6
ascii_L_0801E646
    LDR.N r0, ascii_L_0801E6C0
    LDRB.N r1, [r0]
    AND.W r1, r1, #0xf7
    STRB.N r1, [r0]
    B.N ascii_L_0801E6D6
ascii_L_0801E652
    LDR.N r0, ascii_L_0801E6C0
    LDRB.N r1, [r0]
    ORR.W r1, r1, #8
    STRB.N r1, [r0]
    B.N ascii_L_0801E6D6
ascii_L_0801E65E
    LDR.N r0, ascii_L_0801E6C0
    LDRB.N r1, [r0]
    AND.W r1, r1, #0xef
    STRB.N r1, [r0]
    B.N ascii_L_0801E6D6
ascii_L_0801E66A
    LDR.N r0, ascii_L_0801E6C0
    LDRB.N r1, [r0]
    ORR.W r1, r1, #0x10
    STRB.N r1, [r0]
    LDRB.N r0, [r7]
    LSLS.N r0, r0, #0x1f
    BEQ.N ascii_L_0801E68E
    LDRB.N r0, [r7]
    ORR.W r0, r0, #2
    STRB.N r0, [r7]
    LDR.N r0, ascii_L_0801E6C4
    MOVS.N r2, #0xa
    MOV.N r1, r2
    LDRB.N r0, [r0]
    BL os_task_delay
ascii_L_0801E68E
    LDRB.N r0, [r7]
    AND.W r0, r0, #0xfe
    STRB.N r0, [r7]
    B.N ascii_L_0801E6D6
ascii_L_0801E698
    LDR.N r0, ascii_L_0801E6C0
    LDRB.N r1, [r0]
    AND.W r1, r1, #0xfe
    STRB.N r1, [r0]
    B.N ascii_L_0801E6D6
ascii_L_0801E6A4
    DCD g_20000077
ascii_L_0801E6A8
    DCD g_200000FA
ascii_L_0801E6AC
    DCD g_200000F8
ascii_L_0801E6B0
    DCD g_20000182
ascii_L_0801E6B4
    DCD hexbuf
ascii_L_0801E6B8
    DCD tx_len
ascii_L_0801E6BC
    DCD pack_count
ascii_L_0801E6C0
    DCD g_2000050D
ascii_L_0801E6C4
    DCD os_task_dly_idx
ascii_L_0801E6C8
    LDR.N r0, ascii_L_0801EABC
    LDRB.N r1, [r0]
    ORR.W r1, r1, #1
    STRB.N r1, [r0]
    B.N ascii_L_0801E6D6
ascii_L_0801E6D4
    MOVS.N r5, #9
ascii_L_0801E6D6
    LDR.N r7, ascii_L_0801EABC
    LDRB.N r0, [r7]
    AND.W r1, r0, #0x7f
    MOVS.N r0, #0x38
    BL sub_08022442
    LDR.N r1, ascii_L_0801EAC0
    MOV.N r0, r11
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r1, ascii_L_0801EAC0
    LDRB.N r0, [r7]
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
ascii_L_0801E720
    B.N ascii_L_0801E160
ascii_L_0801E722
    CMP.W r11, #0
    BEQ.N ascii_L_0801E750
    CMP.W r11, #1
    BEQ.N ascii_L_0801E78E
    MOVS.N r5, #9
ascii_L_0801E730
    LDR.N r1, ascii_L_0801EAC0
    LDRB.N r0, [r7]
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
    B.N ascii_L_0801E160
ascii_L_0801E750
    LDRB.N r1, [r0]
    TST.W r1, #0x70
    BNE.N ascii_L_0801E760
    LDRB.N r1, [r3]
    TST.W r1, #0x2c
    BEQ.N ascii_L_0801E764
ascii_L_0801E760
    MOVS.N r5, #9
    B.N ascii_L_0801E730
ascii_L_0801E764
    LDRB.N r1, [r7]
    LSLS.N r1, r1, #0x1e
    BMI.N ascii_L_0801E730
    LDR.N r1, ascii_L_0801EABC
    LDRB.N r3, [r1]
    BIC.W r3, r3, #2
    STRB.N r3, [r1]
    LDRB.N r1, [r2]
    AND.W r1, r1, #0x6a
    STRB.N r1, [r2]
    LDRB.N r1, [r0]
    AND.W r1, r1, #0xfa
    STRB.N r1, [r0]
    LDRB.N r0, [r7]
    ORR.W r0, r0, #2
    STRB.N r0, [r7]
    B.N ascii_L_0801E730
ascii_L_0801E78E
    LDR.N r0, ascii_L_0801EAC4
    LDR.N r0, [r0]
    CMN.W r0, #0x7f800000
    BHS.N ascii_L_0801E7B2
    LDR.N r0, ascii_L_0801EABC
    LDRB.N r1, [r0]
    LSLS.N r1, r1, #0x1d
    BMI.N ascii_L_0801E7B2
    LDRB.N r1, [r0]
    ORR.W r1, r1, #2
    STRB.N r1, [r0]
    LDRB.N r0, [r7]
    AND.W r0, r0, #0xfc
    STRB.N r0, [r7]
    B.N ascii_L_0801E730
ascii_L_0801E7B2
    MOVS.N r5, #9
    B.N ascii_L_0801E730
ascii_L_0801E7B6
    CMP.W r11, #0
    BEQ.N ascii_L_0801E7E4
    CMP.W r11, #1
    BEQ.N ascii_L_0801E828
    MOVS.N r5, #9
ascii_L_0801E7C4
    LDR.N r1, ascii_L_0801EAC0
    LDRB.N r0, [r7]
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
ascii_L_0801E7E2
    B.N ascii_L_0801E160
ascii_L_0801E7E4
    LDRB.N r1, [r2]
    LSLS.N r1, r1, #0x19
    BMI.N ascii_L_0801E7FA
    LDRB.N r1, [r0]
    TST.W r1, #0x70
    BNE.N ascii_L_0801E7FA
    LDRB.N r1, [r3]
    TST.W r1, #0x2c
    BEQ.N ascii_L_0801E7FE
ascii_L_0801E7FA
    MOVS.N r5, #9
    B.N ascii_L_0801E7C4
ascii_L_0801E7FE
    LDRB.N r1, [r7]
    LSLS.N r1, r1, #0x1d
    BMI.N ascii_L_0801E7C4
    LDR.N r1, ascii_L_0801EABC
    LDRB.N r3, [r1]
    BIC.W r3, r3, #4
    STRB.N r3, [r1]
    LDRB.N r1, [r2]
    AND.W r1, r1, #0xd5
    STRB.N r1, [r2]
    LDRB.N r1, [r0]
    AND.W r1, r1, #0xf5
    STRB.N r1, [r0]
    LDRB.N r0, [r7]
    ORR.W r0, r0, #4
    STRB.N r0, [r7]
    B.N ascii_L_0801E7C4
ascii_L_0801E828
    LDR.N r0, ascii_L_0801EAC4
    LDR.N r0, [r0]
    CMP.W r0, #0x800000
    BGE.N ascii_L_0801E84C
    LDR.N r0, ascii_L_0801EABC
    LDRB.N r1, [r0]
    LSLS.N r1, r1, #0x1e
    BMI.N ascii_L_0801E84C
    LDRB.N r1, [r0]
    ORR.W r1, r1, #4
    STRB.N r1, [r0]
    LDRB.N r0, [r7]
    AND.W r0, r0, #0xfb
    STRB.N r0, [r7]
    B.N ascii_L_0801E7C4
ascii_L_0801E84C
    MOVS.N r5, #9
    B.N ascii_L_0801E7C4
ascii_L_0801E850
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    LDR.N r7, ascii_L_0801EAC8
    MOVW.W r1, #0xffff
    AND.W r0, r1, r0, lsl #8
    STRH.N r0, [r7]
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    LDRH.N r1, [r7]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7]
    LDR.N r7, ascii_L_0801EACC
    MOVS.N r0, #0
    STR.N r0, [r7]
    LDR.W r8, ascii_L_0801EAD0
    STRB.W r0, [r8]
    MOVS.N r0, #0xa
    BL sub_08022442
    LDR.N r0, [r7]
    BL __aeabi_f2uiz
    UXTH.N r1, r0
    MOVS.N r0, #0xe
    BL sub_08022442
    LDRB.W r1, [r8]
    MOVS.N r0, #0x10
    BL sub_08022442
ascii_L_0801E8A0
    B.N ascii_L_0801E160
ascii_L_0801E8A2
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    SUB.W r1, r0, #0x4b
    CMP.N r1, #0x16
    BHS.N ascii_L_0801E8C0
    UXTB.N r1, r0
    LDR.N r0, ascii_L_0801EAD4
    STRB.N r1, [r0]
    MOVS.N r0, #0x18
    BL sub_08022442
    B.N ascii_L_0801E160
ascii_L_0801E8C0
    MOVS.N r5, #9
    B.N ascii_L_0801E160
ascii_L_0801E8C4
    B.N ascii_L_0801F0A6
ascii_L_0801E8C6
    LDR.N r7, ascii_L_0801EAD8
    LDR.N r0, [r7]
    BL __aeabi_f2uiz
    UBFX.W r0, r0, #8, #8
    LDR.N r1, ascii_L_0801EAC0
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r0, [r7]
    BL __aeabi_f2uiz
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801EAC0
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    B.N ascii_L_0801E90C
ascii_L_0801E90A
    B.N ascii_L_0801F0F4
ascii_L_0801E90C
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r7, ascii_L_0801EADC
    LDR.N r0, [r7]
    BL __aeabi_f2uiz
    UBFX.W r0, r0, #8, #8
    LDR.N r1, ascii_L_0801EAC0
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r0, [r7]
    BL __aeabi_f2uiz
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801EAC0
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r7, ascii_L_0801EAE0
    LDR.N r1, ascii_L_0801EAC0
    LDRH.N r0, [r7]
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7]
    LDR.N r1, ascii_L_0801EAC0
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r3, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r3, [r4, r0]
ascii_L_0801E9A2
    B.N ascii_L_0801E1C6
ascii_L_0801E9A4
    B.N ascii_L_0801F1A4
ascii_L_0801E9A6
    B.N ascii_L_0801F1A6
ascii_L_0801E9A8
    B.N ascii_L_0801F1A8
ascii_L_0801E9AA
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r10, r0, r1, lsl #8
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    ADD.N r0, r10
    UXTH.W r10, r0
    MOV.N r1, r10
    MOVS.N r0, #6
    BL sub_08022442
    MOV.N r0, r10
    BL __aeabi_ui2f
    LDR.N r1, ascii_L_0801EAD8
    STR.N r0, [r1]
    LDRB.N r1, [r4, #0x12]
    LDRB.N r0, [r4, #0x11]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r10, r0, r1, lsl #8
    LDRB.N r1, [r4, #0x14]
    LDRB.N r0, [r4, #0x13]
    BL sub_08014FD6
    ADD.N r0, r10
    UXTH.W r10, r0
    MOV.N r1, r10
    MOVS.N r0, #8
    BL sub_08022442
    MOV.N r0, r10
    BL __aeabi_ui2f
    LDR.N r1, ascii_L_0801EADC
    STR.N r0, [r1]
    LDRB.N r1, [r4, #0x16]
    LDRB.N r0, [r4, #0x15]
    BL sub_08014FD6
    LDR.W r10, ascii_L_0801EAE0
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.W r0, [r10]
    LDRB.N r1, [r4, #0x18]
    LDRB.N r0, [r4, #0x17]
    BL sub_08014FD6
    LDRH.W r1, [r10]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.W r1, [r10]
    MOVS.N r0, #0xc
    BL sub_08022442
    LDR.N r2, ascii_L_0801EAE4
    LDRB.N r1, [r2]
    CMP.N r1, #0x42
    BEQ.N ascii_L_0801EA80
    CMP.N r1, #0x44
    BEQ.N ascii_L_0801EB46
    LDR.N r1, ascii_L_0801EAE8
    MOVS.N r0, #0
    STRH.N r0, [r1]
    LDR.N r1, ascii_L_0801EAEC
    MOVS.N r0, #2
    STRH.N r0, [r1]
    LDRH.W r0, [r10]
    MOVW.W r1, #0x1770
    CMP.N r0, r1
    BHI.N ascii_L_0801EB48
    MVN.W r0, #0x3b
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x3c
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x40
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x41
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801ECBC
ascii_L_0801EA80
    LDR.N r1, ascii_L_0801EAE8
    MOVS.N r0, #0
    STRH.N r0, [r1]
    LDRB.N r0, [r2, #1]
    CMP.N r0, #0x47
    BNE.N ascii_L_0801EA96
    LDRH.W r1, [r10]
    CMP.W r1, #0x3e80
    BEQ.N ascii_L_0801EA9C
ascii_L_0801EA96
    CMP.N r0, #0x59
    BEQ.N ascii_L_0801EAF0
    B.N ascii_L_0801EAFA
ascii_L_0801EA9C
    LDR.N r1, ascii_L_0801EAEC
    MOVS.N r0, #1
    STRH.N r0, [r1]
    MVN.W r0, #0x45
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x46
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x4a
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x4b
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801EB62
ascii_L_0801EABC
    DCD g_2000050D
ascii_L_0801EAC0
    DCD hexbuf
ascii_L_0801EAC4
    DCD g_20000148
ascii_L_0801EAC8
    DCD g_200000A2
ascii_L_0801EACC
    DCD g_20000168
ascii_L_0801EAD0
    DCD g_200000A9
ascii_L_0801EAD4
    DCD g_20000186
ascii_L_0801EAD8
    DCD g_2000016C
ascii_L_0801EADC
    DCD g_20000164
ascii_L_0801EAE0
    DCD g_20000182
ascii_L_0801EAE4
    DCD g_20000077
ascii_L_0801EAE8
    DCD g_200000FA
ascii_L_0801EAEC
    DCD g_200000F8
ascii_L_0801EAF0
    LDRH.W r0, [r10]
    CMP.W r0, #0x3e80
    BEQ.N ascii_L_0801EB26
ascii_L_0801EAFA
    LDR.N r1, ascii_L_0801EEF8
    MOVS.N r0, #2
    STRH.N r0, [r1]
    LDRH.W r0, [r10]
    MOVW.W r1, #0x1770
    CMP.N r0, r1
    BHI.N ascii_L_0801EB4A
    MVN.W r0, #0x3b
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x3c
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x40
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x41
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801EB62
ascii_L_0801EB26
    LDR.N r1, ascii_L_0801EEF8
    MOVS.N r0, #4
    STRH.N r0, [r1]
    MVN.W r0, #0x59
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x5a
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x5e
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x5f
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801EB62
ascii_L_0801EB46
    B.N ascii_L_0801EB88
ascii_L_0801EB48
    B.N ascii_L_0801ECA4
ascii_L_0801EB4A
    MVN.W r0, #0x45
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x46
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x4a
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x4b
    STRH.N r0, [r7, #8]
ascii_L_0801EB62
    LDRH.W r1, [r8, #0xa]
    MOVS.N r0, #0xa8
    BL sub_08022442
    LDRH.N r1, [r7, #0xa]
    MOVS.N r0, #0xaa
    BL sub_08022442
    LDRH.W r1, [r8, #8]
    MOVS.N r0, #0xa2
    BL sub_08022442
    LDRH.N r1, [r7, #8]
    MOVS.N r0, #0xa4
    BL sub_08022442
ascii_L_0801EB86
    B.N ascii_L_0801E3D6
ascii_L_0801EB88
    LDR.N r0, ascii_L_0801EEFC
    MOVS.N r1, #1
    STRH.N r1, [r0]
    LDRB.N r0, [r2, #1]
    CMP.N r0, #0x47
    BNE.N ascii_L_0801EB9E
    LDRH.W r2, [r10]
    CMP.W r2, #0x3e80
    BEQ.N ascii_L_0801EBA4
ascii_L_0801EB9E
    CMP.N r0, #0x59
    BEQ.N ascii_L_0801EBC2
    B.N ascii_L_0801EBCC
ascii_L_0801EBA4
    LDR.N r0, ascii_L_0801EEF8
    STRH.N r1, [r0]
    MVN.W r0, #0x59
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x55
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x5e
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x5a
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801EC7E
ascii_L_0801EBC2
    LDRH.W r1, [r10]
    CMP.W r1, #0x3e80
    BEQ.N ascii_L_0801EBDE
ascii_L_0801EBCC
    CMP.N r0, #0x57
    BEQ.N ascii_L_0801EBFE
    CMP.N r0, #0x58
    BEQ.N ascii_L_0801EBFE
    CMP.N r0, #0x5a
    BEQ.N ascii_L_0801EBFE
    CMP.N r0, #0x46
    BEQ.N ascii_L_0801EBFE
    B.N ascii_L_0801EC0A
ascii_L_0801EBDE
    LDR.N r0, ascii_L_0801EEF8
    MOVS.N r1, #4
    STRH.N r1, [r0]
    MVN.W r0, #0x59
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x5a
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x5e
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x5f
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801EC7E
ascii_L_0801EBFE
    LDRH.W r0, [r10]
    SUB.W r1, r0, #0x4800
    SUBS.N r1, #0xa8
    BEQ.N ascii_L_0801EC34
ascii_L_0801EC0A
    LDR.N r1, ascii_L_0801EEF8
    MOVS.N r0, #2
    STRH.N r0, [r1]
    LDRH.W r0, [r10]
    CMP.W r0, #0x3e80
    BLO.N ascii_L_0801EC54
    MVN.W r0, #0x59
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x55
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x5e
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x5a
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801EC7E
ascii_L_0801EC34
    LDR.N r1, ascii_L_0801EEF8
    MOVS.N r0, #4
    STRH.N r0, [r1]
    MVN.W r0, #0x59
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x5a
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x5e
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x5f
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801EC7E
ascii_L_0801EC54
    MOVS.N r1, #0xc8
    UDIV.W r2, r0, r1
    ADDS.N r2, #0xa
    RSBS.N r2, r2, #0
    STRH.W r2, [r8, #0xa]
    UDIV.W r2, r0, r1
    ADDS.N r2, #0xa
    STRH.W r2, [r8, #8]
    UDIV.W r2, r0, r1
    ADDS.N r2, #0xf
    RSBS.N r2, r2, #0
    STRH.N r2, [r7, #0xa]
    UDIV.W r0, r0, r1
    ADDS.N r0, #0xf
    STRH.N r0, [r7, #8]
ascii_L_0801EC7E
    LDRH.W r1, [r8, #0xa]
    MOVS.N r0, #0xa8
    BL sub_08022442
    LDRH.N r1, [r7, #0xa]
    MOVS.N r0, #0xaa
    BL sub_08022442
    LDRH.W r1, [r8, #8]
    MOVS.N r0, #0xa2
    BL sub_08022442
    LDRH.N r1, [r7, #8]
    MOVS.N r0, #0xa4
    BL sub_08022442
ascii_L_0801ECA2
    B.N ascii_L_0801E4F2
ascii_L_0801ECA4
    MVN.W r0, #0x45
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x46
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x4a
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x4b
    STRH.N r0, [r7, #8]
ascii_L_0801ECBC
    LDRH.W r1, [r8, #0xa]
    MOVS.N r0, #0xa8
    BL sub_08022442
    LDRH.N r1, [r7, #0xa]
    MOVS.N r0, #0xaa
    BL sub_08022442
    LDRH.W r1, [r8, #8]
    MOVS.N r0, #0xa2
    BL sub_08022442
    LDRH.N r1, [r7, #8]
    MOVS.N r0, #0xa4
    BL sub_08022442
    B.N ascii_L_0801E4F2
ascii_L_0801ECE2
    LDR.N r1, ascii_L_0801EF00
    MOVS.N r0, #0xc
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r1, ascii_L_0801EF00
    MOVS.N r0, #0x1c
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r1, ascii_L_0801EF00
    MOVS.N r0, #0
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r1, ascii_L_0801EF00
    MOVS.N r0, #1
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
ascii_L_0801ED5A
    B.N ascii_L_0801E57A
ascii_L_0801ED5C
    LDR.N r1, ascii_L_0801EF00
    MOVS.N r0, #0xda
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r1, ascii_L_0801EF00
    MOVS.N r0, #0xc0
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r1, ascii_L_0801EF00
    MOVS.N r0, #3
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r1, ascii_L_0801EF00
    MOVS.N r0, #0xe8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r1, ascii_L_0801EF00
    MOVS.N r0, #0x1e
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
    B.N ascii_L_0801E618
ascii_L_0801EDF4
    LDR.N r1, ascii_L_0801EF00
    MOVS.N r0, #0xd
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r1, ascii_L_0801EF00
    MOVS.N r0, #0x48
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r1, ascii_L_0801EF00
    MOVS.N r0, #0
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r1, ascii_L_0801EF00
    MOVS.N r0, #0x1e
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
ascii_L_0801EE6C
    B.N ascii_L_0801E720
ascii_L_0801EE6E
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    MOVW.W r1, #0xffff
    AND.W r7, r1, r0, lsl #8
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    ADD.N r0, r7
    UXTH.N r1, r0
    SUBW.W r0, r1, #0xcb3
    CMP.N r0, #0x63
    BHS.N ascii_L_0801EE9E
    LDR.N r0, ascii_L_0801EF04
    STRH.N r1, [r0]
    MOVS.N r0, #0x1e
    BL sub_08022442
    B.N ascii_L_0801E720
ascii_L_0801EE9E
    MOVS.N r5, #9
    B.N ascii_L_0801E720
ascii_L_0801EEA2
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    MOVW.W r1, #0xffff
    AND.W r7, r1, r0, lsl #8
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    ADD.N r0, r7
    UXTH.N r1, r0
    SUBW.W r0, r1, #0xcb3
    CMP.N r0, #0x63
    BHS.N ascii_L_0801EED2
    LDR.N r0, ascii_L_0801EF08
    STRH.N r1, [r0]
    MOVS.N r0, #0x1c
    BL sub_08022442
    B.N ascii_L_0801E720
ascii_L_0801EED2
    MOVS.N r5, #9
    B.N ascii_L_0801E720
ascii_L_0801EED6
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    MOVW.W r1, #0xffff
    AND.W r10, r1, r0, lsl #8
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    ADD.N r0, r10
    UXTH.W r10, r0
    LDR.N r0, ascii_L_0801EF0C
    B.N ascii_L_0801EF10
ascii_L_0801EEF8
    DCD g_200000F8
ascii_L_0801EEFC
    DCD g_200000FA
ascii_L_0801EF00
    DCD hexbuf
ascii_L_0801EF04
    DCD g_20000096
ascii_L_0801EF08
    DCD g_20000094
ascii_L_0801EF0C
    DCD g_20000110
ascii_L_0801EF10
    LDRH.N r0, [r0]
    CBZ.N r0, ascii_L_0801EF52
    BL __aeabi_ui2f
    STR.N r0, [sp]
    MOV.N r0, r10
    BL __aeabi_ui2f
    LDR.N r1, [sp]
    BL __aeabi_fdiv
    LDR.W r10, ascii_L_0801F330
    LDR.W r1, [r10]
    BL __aeabi_fmul
    STR.W r0, [r10]
    ADD.W r1, r0, r8
    LDR.N r0, ascii_L_0801F334
    CMP.N r1, r0
    BLO.N ascii_L_0801EF46
    STR.W r7, [r10]
    MOVS.N r5, #9
ascii_L_0801EF46
    MOVS.N r0, #0x2c
    LDR.W r1, [r10]
    BL sub_08022488
ascii_L_0801EF50
    B.N ascii_L_0801E7E2
ascii_L_0801EF52
    MOVS.N r5, #9
    B.N ascii_L_0801E7E2
ascii_L_0801EF56
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    MOVW.W r1, #0xffff
    AND.W r10, r1, r0, lsl #8
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    ADD.N r0, r10
    UXTH.N r0, r0
    CBZ.N r0, ascii_L_0801EFCA
    SUB.W r1, r0, #0x33
    MOVW.W r2, #0x1f0d
    CMP.N r1, r2
    BHS.N ascii_L_0801EFD8
    LDR.N r1, ascii_L_0801F338
    LDR.N r1, [r1]
    CMN.W r1, #0x7f800000
    BLO.N ascii_L_0801EFD8
    EOR.W r10, r1, #0x80000000
    BL __aeabi_ui2f
    LDR.N r1, ascii_L_0801F33C
    BL __aeabi_fmul
    MOV.N r1, r10
    BL __aeabi_fdiv
    LDR.W r10, ascii_L_0801F340
    LDR.W r1, [r10]
    BL __aeabi_fmul
    STR.W r0, [r10]
    ADD.W r1, r0, r8
    LDR.N r0, ascii_L_0801F334
    CMP.N r1, r0
    BLO.N ascii_L_0801EFBE
    STR.W r7, [r10]
    MOVS.N r5, #9
ascii_L_0801EFBE
    MOVS.N r0, #0x24
    LDR.W r1, [r10]
    BL sub_08022488
ascii_L_0801EFC8
    B.N ascii_L_0801E7E2
ascii_L_0801EFCA
    LDR.N r0, ascii_L_0801F340
    MOV.N r1, r7
    STR.N r7, [r0]
    MOVS.N r0, #0x24
    BL sub_08022488
    B.N ascii_L_0801E7E2
ascii_L_0801EFD8
    MOVS.N r5, #9
    B.N ascii_L_0801E8A0
ascii_L_0801EFDC
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    MOVW.W r1, #0xffff
    AND.W r10, r1, r0, lsl #8
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    ADD.N r0, r10
    UXTH.N r0, r0
    CBZ.N r0, ascii_L_0801F04E
    SUB.W r1, r0, #0x33
    MOVW.W r2, #0x1f0d
    CMP.N r1, r2
    BHS.N ascii_L_0801F05C
    LDR.N r1, ascii_L_0801F338
    LDR.W r10, [r1]
    CMP.W r10, #0x800000
    BLT.N ascii_L_0801F05C
    BL __aeabi_ui2f
    LDR.N r1, ascii_L_0801F33C
    BL __aeabi_fmul
    MOV.N r1, r10
    BL __aeabi_fdiv
    LDR.W r10, ascii_L_0801F344
    LDR.W r1, [r10]
    BL __aeabi_fmul
    STR.W r0, [r10]
    ADD.W r1, r0, r8
    LDR.N r0, ascii_L_0801F334
    CMP.N r1, r0
    BLO.N ascii_L_0801F042
    STR.W r7, [r10]
    MOVS.N r5, #9
ascii_L_0801F042
    MOVS.N r0, #0x28
    LDR.W r1, [r10]
    BL sub_08022488
ascii_L_0801F04C
    B.N ascii_L_0801E8A0
ascii_L_0801F04E
    LDR.N r0, ascii_L_0801F344
    MOV.N r1, r7
    STR.N r7, [r0]
    MOVS.N r0, #0x28
    BL sub_08022488
    B.N ascii_L_0801E8A0
ascii_L_0801F05C
    MOVS.N r5, #9
    B.N ascii_L_0801E8A0
ascii_L_0801F060
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    MOVW.W r7, #0xffff
    AND.W r8, r7, r0, lsl #8
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    ADD.N r0, r8
    UXTH.N r0, r0
    MOV.W r2, #0x190
    CMP.N r0, r2
    BHS.N ascii_L_0801F0B4
    LDR.N r1, ascii_L_0801F348
    LDRH.N r1, [r1]
    LSLS.N r3, r1, #0x10
    BPL.N ascii_L_0801F0A8
    UBFX.W r1, r1, #0, #0xf
    RSB.W r0, r0, #0x190
    ADD.N r0, r1
    SUBS.N r0, r0, #1
    SDIV.W r1, r0, r2
    MLS.W r0, r2, r1, r0
    UXTH.N r0, r0
    STR.N r0, [sp, #4]
    B.N ascii_L_0801F0B4
ascii_L_0801F0A6
    B.N ascii_L_0801F534
ascii_L_0801F0A8
    CMP.N r0, r1
    BHS.N ascii_L_0801F0B4
    SUBS.N r0, r1, r0
    SUBS.N r0, r0, #1
    UXTH.N r0, r0
    STR.N r0, [sp, #4]
ascii_L_0801F0B4
    LDR.N r0, [sp, #4]
    CMP.N r0, r7
    BEQ.N ascii_L_0801EFC8
    MOVS.N r7, #0
    LDR.N r0, [sp, #4]
    ADD.W r8, r0, r0, lsl #6
ascii_L_0801F0C2
    ADD.W r0, r8, r7
    BL sub_080223BC
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F34C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    MOV.N r2, r0
    LDRB.W r1, [r9, #1]
    ADDS.N r0, r0, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r1, [r4, r2]
    ADDS.N r7, r7, #1
    UXTH.N r7, r7
    CMP.N r7, #0x41
    BLO.N ascii_L_0801F0C2
ascii_L_0801F0F2
    B.N ascii_L_0801E9A2
ascii_L_0801F0F4
    B.N ascii_L_0801F2C2
ascii_L_0801F0F6
    LDR.N r0, ascii_L_0801F348
    MOVS.N r1, #0
    STRH.N r1, [r0]
    LDR.N r7, ascii_L_0801F350
    STRH.N r1, [r7]
    MOVS.N r0, #0x14
    BL sub_08022442
    LDRH.N r1, [r7]
    MOVS.N r0, #0x16
    BL sub_08022442
    BL sub_080172B2
    B.N ascii_L_0801E9A2
ascii_L_0801F114
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.W r0, [r10]
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.W r0, [r10, #1]
    LDRB.N r1, [r4, #0x12]
    LDRB.N r0, [r4, #0x11]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.W r0, [r10, #2]
    LDRB.N r1, [r4, #0x14]
    LDRB.N r0, [r4, #0x13]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.W r0, [r10, #3]
    LDRB.N r1, [r4, #0x16]
    LDRB.N r0, [r4, #0x15]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.W r0, [r10, #4]
    LDRB.N r1, [r4, #0x18]
    LDRB.N r0, [r4, #0x17]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.W r0, [r10, #5]
    MOVS.N r5, #9
    B.N ascii_L_0801E9A2
ascii_L_0801F178
    BL sub_0801535E
    LDRB.W r0, [r10]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F34C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    B.N ascii_L_0801F1AA
ascii_L_0801F1A4
    B.N ascii_L_0801F602
ascii_L_0801F1A6
    B.N ascii_L_0801F3CA
ascii_L_0801F1A8
    B.N ascii_L_0801F420
ascii_L_0801F1AA
    LDRB.W r0, [r10, #1]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F34C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.W r0, [r10, #2]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F34C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.W r0, [r10, #3]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F34C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.W r0, [r10, #4]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F34C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.W r0, [r10, #5]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F34C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r3, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r3, [r4, r0]
ascii_L_0801F268
    B.N ascii_L_0801EB86
ascii_L_0801F26A
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    BL sub_080151EA
    LDR.N r7, ascii_L_0801F354
    STRB.N r0, [r7]
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.N r0, [r7, #1]
    LDRB.N r1, [r4, #0x12]
    LDRB.N r0, [r4, #0x11]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.N r0, [r7, #2]
    LDRB.N r1, [r4, #0x14]
    LDRB.N r0, [r4, #0x13]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.N r0, [r7, #3]
    LDRB.N r1, [r4, #0x16]
    LDRB.N r0, [r4, #0x15]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.N r0, [r7, #4]
    LDRB.N r1, [r4, #0x18]
    LDRB.N r0, [r4, #0x17]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.N r0, [r7, #5]
    B.N ascii_L_0801EB86
ascii_L_0801F2C2
    LDR.N r7, ascii_L_0801F354
    LDRB.N r0, [r7]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F34C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #1]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F34C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #2]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F34C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    B.N ascii_L_0801F358
ascii_L_0801F330
    DCD g_20000154
ascii_L_0801F334
    DCD 0x00733334
ascii_L_0801F338
    DCD g_20000148
ascii_L_0801F33C
    DCD 0x41200000
ascii_L_0801F340
    DCD g_2000014C
ascii_L_0801F344
    DCD g_20000150
ascii_L_0801F348
    DCD log_index
ascii_L_0801F34C
    DCD hexbuf
ascii_L_0801F350
    DCD g_2000018A
ascii_L_0801F354
    DCD date_start
ascii_L_0801F358
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #3]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F75C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #4]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F75C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    MOV.N r3, r0
    LDRB.W r1, [r9, #1]
    ADDS.N r0, r0, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r1, [r4, r3]
    LDRB.N r0, [r7, #5]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F75C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r3, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r3, [r4, r0]
ascii_L_0801F3C8
    B.N ascii_L_0801ECA2
ascii_L_0801F3CA
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.N r0, [r7]
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.N r0, [r7, #1]
    LDRB.N r1, [r4, #0x12]
    LDRB.N r0, [r4, #0x11]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.N r0, [r7, #2]
    LDRB.N r1, [r4, #0x14]
    LDRB.N r0, [r4, #0x13]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.N r0, [r7, #3]
    LDRB.N r1, [r4, #0x16]
    LDRB.N r0, [r4, #0x15]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.N r0, [r7, #4]
    LDRB.N r1, [r4, #0x18]
    LDRB.N r0, [r4, #0x17]
    BL sub_08014FD6
    BL sub_080151EA
    STRB.N r0, [r7, #5]
    B.N ascii_L_0801ECA2
ascii_L_0801F420
    LDRB.N r0, [r7]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F75C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #1]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F75C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #2]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F75C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #3]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F75C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #4]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F75C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #5]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F75C
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r3, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r3, [r4, r0]
ascii_L_0801F4F8
    B.N ascii_L_0801ED5A
ascii_L_0801F4FA
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    SUBS.N r1, r0, #1
    CMP.N r1, #0x77
    BHS.N ascii_L_0801F50E
    LDR.N r1, ascii_L_0801F760
    STRB.N r0, [r1]
    B.N ascii_L_0801ED5A
ascii_L_0801F50E
    MOVS.N r5, #9
    B.N ascii_L_0801ED5A
ascii_L_0801F512
    LDR.N r0, ascii_L_0801F760
    LDR.N r1, ascii_L_0801F75C
    LDRB.N r0, [r0]
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
    B.N ascii_L_0801ED5A
ascii_L_0801F534
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_0801F5BC
    LDRB.W r0, [r10]
    AND.W r0, r0, #0xfe
    STRB.W r0, [r10]
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.W r0, [r8]
    LDRB.N r1, [r4, #0x12]
    LDRB.N r0, [r4, #0x11]
    BL sub_08014FD6
    LDRH.W r1, [r8]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.W r1, [r8]
    MOVS.N r0, #0x82
    BL sub_08022442
    LDRB.N r1, [r4, #0x14]
    LDRB.N r0, [r4, #0x13]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.N r0, [r7]
    LDRB.N r1, [r4, #0x16]
    LDRB.N r0, [r4, #0x15]
    BL sub_08014FD6
    LDRH.N r1, [r7]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7]
    MOVS.N r0, #0x84
    BL sub_08022442
    LDRB.N r1, [r4, #0x18]
    LDRB.N r0, [r4, #0x17]
    BL sub_08014FD6
    LDR.N r7, ascii_L_0801F764
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.N r0, [r7]
    B.N ascii_L_0801F5BE
ascii_L_0801F5BC
    B.N ascii_L_0801F5F4
ascii_L_0801F5BE
    LDRB.N r1, [r4, #0x1a]
    LDRB.N r0, [r4, #0x19]
    BL sub_08014FD6
    LDRH.N r1, [r7]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7]
    MOVS.N r0, #0x86
    BL sub_08022442
    LDRB.N r1, [r4, #0x1c]
    LDRB.N r0, [r4, #0x1b]
    BL sub_08014FD6
    MOV.N r1, r0
    LDR.N r0, ascii_L_0801F768
    STRB.N r1, [r0]
    MOVS.N r0, #0x88
    BL sub_08022442
ascii_L_0801F5E8
    LDRB.W r1, [r10]
    MOVS.N r0, #0x30
    BL sub_08022442
ascii_L_0801F5F2
    B.N ascii_L_0801EE6C
ascii_L_0801F5F4
    LDRB.W r0, [r10]
    ORR.W r0, r0, #1
    STRB.W r0, [r10]
    B.N ascii_L_0801F5E8
ascii_L_0801F602
    LDRB.W r1, [r10]
    LSLS.N r1, r1, #0x1f
    BEQ.N ascii_L_0801F618
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_0801F624
ascii_L_0801F618
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_0801F624
    LDRH.W r0, [r8]
    LDR.N r1, ascii_L_0801F75C
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDRB.W r0, [r8]
    LDR.N r1, ascii_L_0801F75C
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDRH.N r0, [r7]
    LDR.N r1, ascii_L_0801F75C
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDRB.N r0, [r7]
    LDR.N r1, ascii_L_0801F75C
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDR.N r7, ascii_L_0801F764
    LDR.N r1, ascii_L_0801F75C
    LDRH.N r0, [r7]
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDRB.N r0, [r7]
    LDR.N r1, ascii_L_0801F75C
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDR.N r0, ascii_L_0801F768
    LDR.N r1, ascii_L_0801F75C
    LDRB.N r0, [r0]
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
ascii_L_0801F704
    B.N ascii_L_0801EF50
ascii_L_0801F706
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_0801F76C
    LDRB.W r0, [r10]
    AND.W r0, r0, #0xfd
    STRB.W r0, [r10]
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.W r0, [r8, #2]
    LDRB.N r1, [r4, #0x12]
    LDRB.N r0, [r4, #0x11]
    BL sub_08014FD6
    LDRH.W r1, [r8, #2]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.W r1, [r8, #2]
    MOVS.N r0, #0x8a
    BL sub_08022442
    LDRB.N r1, [r4, #0x14]
    LDRB.N r0, [r4, #0x13]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    B.N ascii_L_0801F76E
ascii_L_0801F75C
    DCD hexbuf
ascii_L_0801F760
    DCD log_interval
ascii_L_0801F764
    DCD recov
ascii_L_0801F768
    DCD delays
ascii_L_0801F76C
    B.N ascii_L_0801F7D6
ascii_L_0801F76E
    AND.W r0, r0, r1, lsl #8
    STRH.N r0, [r7, #2]
    LDRB.N r1, [r4, #0x16]
    LDRB.N r0, [r4, #0x15]
    BL sub_08014FD6
    LDRH.N r1, [r7, #2]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7, #2]
    MOVS.N r0, #0x8c
    BL sub_08022442
    LDRB.N r1, [r4, #0x18]
    LDRB.N r0, [r4, #0x17]
    BL sub_08014FD6
    LDR.N r7, ascii_L_0801FB80
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.N r0, [r7, #2]
    LDRB.N r1, [r4, #0x1a]
    LDRB.N r0, [r4, #0x19]
    BL sub_08014FD6
    LDRH.N r1, [r7, #2]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7, #2]
    MOVS.N r0, #0x8e
    BL sub_08022442
    LDRB.N r1, [r4, #0x1c]
    LDRB.N r0, [r4, #0x1b]
    BL sub_08014FD6
    MOV.N r1, r0
    LDR.N r0, ascii_L_0801FB84
    STRB.N r1, [r0, #1]
    MOVS.N r0, #0x90
    BL sub_08022442
ascii_L_0801F7CA
    LDRB.W r1, [r10]
    MOVS.N r0, #0x30
    BL sub_08022442
ascii_L_0801F7D4
    B.N ascii_L_0801F04C
ascii_L_0801F7D6
    LDRB.W r0, [r10]
    ORR.W r0, r0, #2
    STRB.W r0, [r10]
    B.N ascii_L_0801F7CA
ascii_L_0801F7E4
    LDRB.W r1, [r10]
    LSLS.N r1, r1, #0x1e
    BPL.N ascii_L_0801F7FA
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_0801F806
ascii_L_0801F7FA
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_0801F806
    LDRH.W r0, [r8, #2]
    LDR.N r1, ascii_L_0801FB88
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDRB.W r0, [r8, #2]
    LDR.N r1, ascii_L_0801FB88
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDRH.N r0, [r7, #2]
    LDR.N r1, ascii_L_0801FB88
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDRB.N r0, [r7, #2]
    LDR.N r1, ascii_L_0801FB88
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDR.N r7, ascii_L_0801FB80
    LDR.N r1, ascii_L_0801FB88
    LDRH.N r0, [r7, #2]
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDRB.N r0, [r7, #2]
    LDR.N r1, ascii_L_0801FB88
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDR.N r0, ascii_L_0801FB84
    LDR.N r1, ascii_L_0801FB88
    LDRB.N r0, [r0, #1]
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
ascii_L_0801F8E6
    B.N ascii_L_0801F0F2
ascii_L_0801F8E8
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_0801F970
    LDRB.W r0, [r10]
    AND.W r0, r0, #0xfb
    STRB.W r0, [r10]
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.W r0, [r8, #4]
    LDRB.N r1, [r4, #0x12]
    LDRB.N r0, [r4, #0x11]
    BL sub_08014FD6
    LDRH.W r1, [r8, #4]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.W r1, [r8, #4]
    MOVS.N r0, #0x92
    BL sub_08022442
    LDRB.N r1, [r4, #0x14]
    LDRB.N r0, [r4, #0x13]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.N r0, [r7, #4]
    LDRB.N r1, [r4, #0x16]
    LDRB.N r0, [r4, #0x15]
    BL sub_08014FD6
    LDRH.N r1, [r7, #4]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7, #4]
    MOVS.N r0, #0x94
    BL sub_08022442
    LDRB.N r1, [r4, #0x18]
    LDRB.N r0, [r4, #0x17]
    BL sub_08014FD6
    LDR.N r7, ascii_L_0801FB80
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.N r0, [r7, #4]
    B.N ascii_L_0801F972
ascii_L_0801F970
    B.N ascii_L_0801F9A8
ascii_L_0801F972
    LDRB.N r1, [r4, #0x1a]
    LDRB.N r0, [r4, #0x19]
    BL sub_08014FD6
    LDRH.N r1, [r7, #4]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7, #4]
    MOVS.N r0, #0x96
    BL sub_08022442
    LDRB.N r1, [r4, #0x1c]
    LDRB.N r0, [r4, #0x1b]
    BL sub_08014FD6
    MOV.N r1, r0
    LDR.N r0, ascii_L_0801FB84
    STRB.N r1, [r0, #2]
    MOVS.N r0, #0x98
    BL sub_08022442
ascii_L_0801F99C
    LDRB.W r1, [r10]
    MOVS.N r0, #0x30
    BL sub_08022442
    B.N ascii_L_0801F268
ascii_L_0801F9A8
    LDRB.W r0, [r10]
    ORR.W r0, r0, #4
    STRB.W r0, [r10]
    B.N ascii_L_0801F99C
ascii_L_0801F9B6
    LDRB.W r1, [r10]
    LSLS.N r1, r1, #0x1d
    BPL.N ascii_L_0801F9CC
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_0801F9D8
ascii_L_0801F9CC
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_0801F9D8
    LDRH.W r0, [r8, #4]
    LDR.N r1, ascii_L_0801FB88
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDRB.W r0, [r8, #4]
    LDR.N r1, ascii_L_0801FB88
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDRH.N r0, [r7, #4]
    LDR.N r1, ascii_L_0801FB88
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    MOV.N r12, r1
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r1, #1
    STRH.N r1, [r6, #0xa]
    STRB.W r2, [r4, r12]
    LDRB.N r0, [r7, #4]
    LDR.N r1, ascii_L_0801FB88
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDR.N r7, ascii_L_0801FB80
    LDR.N r1, ascii_L_0801FB88
    LDRH.N r0, [r7, #4]
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDRB.N r0, [r7, #4]
    LDR.N r1, ascii_L_0801FB88
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDR.N r0, ascii_L_0801FB84
    LDR.N r1, ascii_L_0801FB88
    LDRB.N r0, [r0, #2]
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
ascii_L_0801FABC
    B.N ascii_L_0801F3C8
ascii_L_0801FABE
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_0801FB44
    LDRB.W r0, [r10]
    AND.W r0, r0, #0xf7
    STRB.W r0, [r10]
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.W r0, [r8, #6]
    LDRB.N r1, [r4, #0x12]
    LDRB.N r0, [r4, #0x11]
    BL sub_08014FD6
    LDRH.W r1, [r8, #6]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.W r1, [r8, #6]
    MOVS.N r0, #0x9a
    BL sub_08022442
    LDRB.N r1, [r4, #0x14]
    LDRB.N r0, [r4, #0x13]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.N r0, [r7, #6]
    LDRB.N r1, [r4, #0x16]
    LDRB.N r0, [r4, #0x15]
    BL sub_08014FD6
    LDRH.N r1, [r7, #6]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7, #6]
    MOVS.N r0, #0x9c
    BL sub_08022442
    LDRB.N r1, [r4, #0x18]
    LDRB.N r0, [r4, #0x17]
    BL sub_08014FD6
    LDR.N r7, ascii_L_0801FB80
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    B.N ascii_L_0801FB46
ascii_L_0801FB44
    B.N ascii_L_0801FB8C
ascii_L_0801FB46
    STRH.N r0, [r7, #6]
    LDRB.N r1, [r4, #0x1a]
    LDRB.N r0, [r4, #0x19]
    BL sub_08014FD6
    LDRH.N r1, [r7, #6]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7, #6]
    MOVS.N r0, #0x9e
    BL sub_08022442
    LDRB.N r1, [r4, #0x1c]
    LDRB.N r0, [r4, #0x1b]
    BL sub_08014FD6
    MOV.N r1, r0
    LDR.N r0, ascii_L_0801FB84
    STRB.N r1, [r0, #3]
    MOVS.N r0, #0xa0
    BL sub_08022442
ascii_L_0801FB72
    LDRB.W r1, [r10]
    MOVS.N r0, #0x30
    BL sub_08022442
    B.N ascii_L_0801F3C8
    DCB 0 ; native pool/table alignment
    DCB 0 ; native pool/table alignment
ascii_L_0801FB80
    DCD recov
ascii_L_0801FB84
    DCD delays
ascii_L_0801FB88
    DCD hexbuf
ascii_L_0801FB8C
    LDRB.W r0, [r10]
    ORR.W r0, r0, #8
    STRB.W r0, [r10]
    B.N ascii_L_0801FB72
ascii_L_0801FB9A
    LDRB.W r1, [r10]
    LSLS.N r1, r1, #0x1c
    BPL.N ascii_L_0801FBB0
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_0801FBBC
ascii_L_0801FBB0
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_0801FBBC
    LDRH.W r0, [r8, #6]
    LDR.N r1, ascii_L_0801FFBC
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.W r0, [r8, #6]
    LDR.N r1, ascii_L_0801FFBC
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRH.N r0, [r7, #6]
    LDR.N r1, ascii_L_0801FFBC
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #6]
    LDR.N r1, ascii_L_0801FFBC
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r7, ascii_L_0801FFC0
    LDR.N r1, ascii_L_0801FFBC
    LDRH.N r0, [r7, #6]
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #6]
    LDR.N r1, ascii_L_0801FFBC
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r0, ascii_L_0801FFC4
    LDR.N r1, ascii_L_0801FFBC
    LDRB.N r0, [r0, #3]
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
ascii_L_0801FC9C
    B.N ascii_L_0801F4F8
ascii_L_0801FC9E
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_0801FD24
    LDRB.W r0, [r10]
    AND.W r0, r0, #0xef
    STRB.W r0, [r10]
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    MOVW.W r1, #0xffff
    AND.W r0, r1, r0, lsl #8
    STR.N r0, [sp, #0xc]
    LDRB.N r1, [r4, #0x12]
    LDRB.N r0, [r4, #0x11]
    BL sub_08014FD6
    MOV.N r1, r0
    LDR.N r0, [sp, #0xc]
    ADD.N r0, r1
    UXTH.N r1, r0
    CMP.N r1, #0x78
    BLS.N ascii_L_0801FCDC
    MOVS.N r1, #0x78
ascii_L_0801FCDC
    STRH.W r1, [r8, #8]
    MOVS.N r0, #0xa2
    BL sub_08022442
    LDRB.N r1, [r4, #0x14]
    LDRB.N r0, [r4, #0x13]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r8, r0, r1, lsl #8
    LDRB.N r1, [r4, #0x16]
    LDRB.N r0, [r4, #0x15]
    BL sub_08014FD6
    ADD.N r0, r8
    UXTH.N r1, r0
    CMP.N r1, #0x78
    BLS.N ascii_L_0801FD0A
    MOVS.N r1, #0x78
ascii_L_0801FD0A
    STRH.N r1, [r7, #8]
    MOVS.N r0, #0xa4
    BL sub_08022442
    LDRB.N r1, [r4, #0x18]
    LDRB.N r0, [r4, #0x17]
    BL sub_08014FD6
    LDR.N r1, ascii_L_0801FFC4
    STRB.N r0, [r1, #4]
    MOV.N r1, r0
    MOVS.N r0, #0xa6
    B.N ascii_L_0801FD26
ascii_L_0801FD24
    B.N ascii_L_0801FD36
ascii_L_0801FD26
    BL sub_08022442
ascii_L_0801FD2A
    LDRB.W r1, [r10]
    MOVS.N r0, #0x30
    BL sub_08022442
    B.N ascii_L_0801F5F2
ascii_L_0801FD36
    LDRB.W r0, [r10]
    ORR.W r0, r0, #0x10
    STRB.W r0, [r10]
    B.N ascii_L_0801FD2A
ascii_L_0801FD44
    LDRB.W r1, [r10]
    LSLS.N r1, r1, #0x1b
    BPL.N ascii_L_0801FD5A
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_0801FD66
ascii_L_0801FD5A
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_0801FD66
    LDRH.W r0, [r8, #8]
    LDR.N r1, ascii_L_0801FFBC
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.W r0, [r8, #8]
    LDR.N r1, ascii_L_0801FFBC
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRH.N r0, [r7, #8]
    LDR.N r1, ascii_L_0801FFBC
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #8]
    LDR.N r1, ascii_L_0801FFBC
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r7, r0, #1
    STRH.N r7, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r0, ascii_L_0801FFC4
    LDR.N r1, ascii_L_0801FFBC
    LDRB.N r0, [r0, #4]
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    MOV.N r1, r0
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r0, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
ascii_L_0801FE08
    B.N ascii_L_0801F704
ascii_L_0801FE0A
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_0801FE8E
    LDRB.W r0, [r10]
    AND.W r0, r0, #0xdf
    STRB.W r0, [r10]
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    MOVW.W r1, #0xffff
    AND.W r0, r1, r0, lsl #8
    STR.N r0, [sp, #0xc]
    LDRB.N r1, [r4, #0x12]
    LDRB.N r0, [r4, #0x11]
    BL sub_08014FD6
    MOV.N r1, r0
    LDR.N r0, [sp, #0xc]
    ADD.N r0, r1
    UXTH.N r0, r0
    CMP.N r0, #0x64
    BLS.N ascii_L_0801FE48
    MOVS.N r0, #0x64
ascii_L_0801FE48
    RSBS.N r0, r0, #0
    STRH.W r0, [r8, #0xa]
    LDRH.W r1, [r8, #0xa]
    MOVS.N r0, #0xa8
    BL sub_08022442
    LDRB.N r1, [r4, #0x14]
    LDRB.N r0, [r4, #0x13]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r8, r0, r1, lsl #8
    LDRB.N r1, [r4, #0x16]
    LDRB.N r0, [r4, #0x15]
    BL sub_08014FD6
    ADD.N r0, r8
    UXTH.N r1, r0
    CMP.N r1, #0x78
    BLS.N ascii_L_0801FE7C
    MOVS.N r1, #0x78
ascii_L_0801FE7C
    RSBS.N r0, r1, #0
    STRH.N r0, [r7, #0xa]
    LDRH.N r1, [r7, #0xa]
    MOVS.N r0, #0xaa
    BL sub_08022442
    LDRB.N r1, [r4, #0x18]
    LDRB.N r0, [r4, #0x17]
    B.N ascii_L_0801FE90
ascii_L_0801FE8E
    B.N ascii_L_0801FEAC
ascii_L_0801FE90
    BL sub_08014FD6
    LDR.N r1, ascii_L_0801FFC4
    STRB.N r0, [r1, #5]
    MOV.N r1, r0
    MOVS.N r0, #0xac
    BL sub_08022442
ascii_L_0801FEA0
    LDRB.W r1, [r10]
    MOVS.N r0, #0x30
    BL sub_08022442
ascii_L_0801FEAA
    B.N ascii_L_0801F704
ascii_L_0801FEAC
    LDRB.W r0, [r10]
    ORR.W r0, r0, #0x20
    STRB.W r0, [r10]
    B.N ascii_L_0801FEA0
ascii_L_0801FEBA
    LDRB.W r1, [r10]
    LSLS.N r1, r1, #0x1a
    BPL.N ascii_L_0801FED0
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_0801FEDC
ascii_L_0801FED0
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_0801FEDC
    LDRH.W r0, [r8, #0xa]
    LDR.N r1, ascii_L_0801FFBC
    UBFX.W r0, r0, #8, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.W r0, [r8, #0xa]
    LDR.N r1, ascii_L_0801FFBC
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRH.N r0, [r7, #0xa]
    LDR.N r1, ascii_L_0801FFBC
    UBFX.W r0, r0, #8, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #0xa]
    LDR.N r1, ascii_L_0801FFBC
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r0, ascii_L_0801FFC4
    LDR.N r1, ascii_L_0801FFBC
    LDRB.N r0, [r0, #5]
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
ascii_L_0801FF80
    B.N ascii_L_0801F7D4
ascii_L_0801FF82
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_08020006
    LDRB.W r0, [r10]
    AND.W r0, r0, #0xbf
    STRB.W r0, [r10]
    ADDS.N r4, #0xf
    LDRB.N r1, [r4, #1]
    LDRB.N r0, [r4]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r1, r0, r1, lsl #8
    LDR.N r0, ascii_L_0801FFC8
    STRH.N r1, [r0, #0xc]
    LDRB.N r1, [r4, #3]
    LDRB.N r0, [r4, #2]
    BL sub_08014FD6
    MOV.N r1, r0
    B.N ascii_L_0801FFCC
ascii_L_0801FFBC
    DCD hexbuf
ascii_L_0801FFC0
    DCD recov
ascii_L_0801FFC4
    DCD delays
ascii_L_0801FFC8
    DCD limits
ascii_L_0801FFCC
    LDR.N r0, ascii_L_080203C8
    LDRH.N r0, [r0, #0xc]
    ADD.N r0, r1
    UXTH.N r1, r0
    LDR.N r0, ascii_L_080203C8
    STRH.N r1, [r0, #0xc]
    MOVS.N r0, #0xae
    BL sub_08022442
    LDRB.N r1, [r4, #5]
    LDRB.N r0, [r4, #4]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.N r0, [r7, #0xe]
    LDRB.N r1, [r4, #7]
    LDRB.N r0, [r4, #6]
    BL sub_08014FD6
    LDRH.N r1, [r7, #0xe]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7, #0xe]
    MOVS.N r0, #0xb0
    B.N ascii_L_08020008
ascii_L_08020006
    B.N ascii_L_080200D8
ascii_L_08020008
    BL sub_08022442
    LDRB.N r1, [r4, #9]
    LDRB.N r0, [r4, #8]
    BL sub_08014FD6
    LDR.W r8, ascii_L_080203CC
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.W r0, [r8, #8]
    LDRB.N r1, [r4, #0xb]
    LDRB.N r0, [r4, #0xa]
    BL sub_08014FD6
    LDRH.W r1, [r8, #8]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.W r1, [r8, #8]
    MOVS.N r0, #0xb2
    BL sub_08022442
    LDRB.N r1, [r4, #0xd]
    LDRB.N r0, [r4, #0xc]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r1, r0, r1, lsl #8
    LDR.N r0, ascii_L_080203C8
    STRH.N r1, [r0, #0xe]
    LDRB.N r1, [r4, #0xf]
    LDRB.N r0, [r4, #0xe]
    BL sub_08014FD6
    LDR.N r1, ascii_L_080203C8
    LDRH.N r1, [r1, #0xe]
    ADD.N r0, r1
    UXTH.N r1, r0
    LDR.N r0, ascii_L_080203C8
    STRH.N r1, [r0, #0xe]
    MOVS.N r0, #0xb4
    BL sub_08022442
    LDRB.N r1, [r4, #0x11]
    LDRB.N r0, [r4, #0x10]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.N r0, [r7, #0x10]
    LDRB.N r1, [r4, #0x13]
    LDRB.N r0, [r4, #0x12]
    BL sub_08014FD6
    LDRH.N r1, [r7, #0x10]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7, #0x10]
    MOVS.N r0, #0xb6
    BL sub_08022442
    LDRB.N r1, [r4, #0x15]
    LDRB.N r0, [r4, #0x14]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.W r0, [r8, #0xa]
    LDRB.N r1, [r4, #0x17]
    LDRB.N r0, [r4, #0x16]
    SUBS.N r4, #0xf
    BL sub_08014FD6
    LDRH.W r1, [r8, #0xa]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.W r1, [r8, #0xa]
    MOVS.N r0, #0xb8
    BL sub_08022442
ascii_L_080200CC
    LDRB.W r1, [r10]
    MOVS.N r0, #0x30
    BL sub_08022442
ascii_L_080200D6
    B.N ascii_L_0801F8E6
ascii_L_080200D8
    LDRB.W r0, [r10]
    ORR.W r0, r0, #0x40
    STRB.W r0, [r10]
    B.N ascii_L_080200CC
ascii_L_080200E6
    LDRB.W r1, [r10]
    LSLS.N r1, r1, #0x19
    BPL.N ascii_L_080200FC
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_08020108
ascii_L_080200FC
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_08020108
    LDRH.W r0, [r8, #0xc]
    LDR.N r1, ascii_L_080203D0
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.W r0, [r8, #0xc]
    LDR.N r1, ascii_L_080203D0
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRH.N r0, [r7, #0xe]
    LDR.N r1, ascii_L_080203D0
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #0xe]
    LDR.N r1, ascii_L_080203D0
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.W r10, ascii_L_080203CC
    LDR.N r1, ascii_L_080203D0
    LDRH.W r0, [r10, #8]
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r0, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    LDRB.W r0, [r9, #1]
    ADDS.N r2, r1, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r0, [r4, r1]
    LDRB.W r0, [r10, #8]
    LDR.N r1, ascii_L_080203D0
    BL sub_08015034
    LDRH.N r0, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    LDRB.W r0, [r9, #1]
    ADDS.N r2, r1, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r0, [r4, r1]
    LDRH.W r0, [r8, #0xe]
    LDR.N r1, ascii_L_080203D0
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r0, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    LDRB.W r0, [r9, #1]
    ADDS.N r2, r1, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r0, [r4, r1]
    LDRB.W r0, [r8, #0xe]
    LDR.N r1, ascii_L_080203D0
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRH.N r0, [r7, #0x10]
    LDR.N r1, ascii_L_080203D0
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #0x10]
    LDR.N r1, ascii_L_080203D0
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRH.W r0, [r10, #0xa]
    LDR.N r1, ascii_L_080203D0
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.W r0, [r10, #0xa]
    LDR.N r1, ascii_L_080203D0
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
ascii_L_08020290
    B.N ascii_L_0801FABC
ascii_L_08020292
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_08020318
    LDRB.W r0, [r10]
    AND.W r0, r0, #0x7f
    STRB.W r0, [r10]
    ADDS.N r4, #0xf
    LDRB.N r1, [r4, #1]
    LDRB.N r0, [r4]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r1, r0, r1, lsl #8
    LDR.N r0, ascii_L_080203C8
    STRH.N r1, [r0, #0x10]
    LDRB.N r1, [r4, #3]
    LDRB.N r0, [r4, #2]
    BL sub_08014FD6
    LDR.N r1, ascii_L_080203C8
    LDRH.N r1, [r1, #0x10]
    ADD.N r0, r1
    UXTH.N r1, r0
    LDR.N r0, ascii_L_080203C8
    STRH.N r1, [r0, #0x10]
    MOVS.N r0, #0xba
    BL sub_08022442
    LDRB.N r1, [r4, #5]
    LDRB.N r0, [r4, #4]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.N r0, [r7, #0x12]
    LDRB.N r1, [r4, #7]
    LDRB.N r0, [r4, #6]
    BL sub_08014FD6
    LDRH.N r1, [r7, #0x12]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7, #0x12]
    MOVS.N r0, #0xbc
    BL sub_08022442
    LDRB.N r1, [r4, #9]
    LDRB.N r0, [r4, #8]
    BL sub_08014FD6
    LDR.W r8, ascii_L_080203CC
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    B.N ascii_L_0802031A
ascii_L_08020318
    B.N ascii_L_080203E4
ascii_L_0802031A
    AND.W r0, r0, r1, lsl #8
    STRH.W r0, [r8, #0xc]
    LDRB.N r1, [r4, #0xb]
    LDRB.N r0, [r4, #0xa]
    BL sub_08014FD6
    LDRH.W r1, [r8, #0xc]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.W r1, [r8, #0xc]
    MOVS.N r0, #0xbe
    BL sub_08022442
    LDRB.N r1, [r4, #0xd]
    LDRB.N r0, [r4, #0xc]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r1, r0, r1, lsl #8
    LDR.N r0, ascii_L_080203C8
    STRH.N r1, [r0, #0x12]
    LDRB.N r1, [r4, #0xf]
    LDRB.N r0, [r4, #0xe]
    BL sub_08014FD6
    LDR.N r1, ascii_L_080203C8
    LDRH.N r1, [r1, #0x12]
    ADD.N r0, r1
    UXTH.N r1, r0
    LDR.N r0, ascii_L_080203C8
    STRH.N r1, [r0, #0x12]
    MOVS.N r0, #0xc0
    BL sub_08022442
    LDRB.N r1, [r4, #0x11]
    LDRB.N r0, [r4, #0x10]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.N r0, [r7, #0x14]
    LDRB.N r1, [r4, #0x13]
    LDRB.N r0, [r4, #0x12]
    BL sub_08014FD6
    LDRH.N r1, [r7, #0x14]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7, #0x14]
    MOVS.N r0, #0xc2
    BL sub_08022442
    LDRB.N r1, [r4, #0x15]
    LDRB.N r0, [r4, #0x14]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.W r0, [r8, #0xe]
    LDRB.N r1, [r4, #0x17]
    LDRB.N r0, [r4, #0x16]
    SUBS.N r4, #0xf
    BL sub_08014FD6
    LDRH.W r1, [r8, #0xe]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.W r1, [r8, #0xe]
    MOVS.N r0, #0xc4
    B.N ascii_L_080203D4
    DCB 0 ; native pool/table alignment
    DCB 0 ; native pool/table alignment
ascii_L_080203C8
    DCD limits
ascii_L_080203CC
    DCD recov
ascii_L_080203D0
    DCD hexbuf
ascii_L_080203D4
    BL sub_08022442
ascii_L_080203D8
    LDRB.W r1, [r10]
    MOVS.N r0, #0x30
    BL sub_08022442
ascii_L_080203E2
    B.N ascii_L_0801FC9C
ascii_L_080203E4
    LDRB.W r0, [r10]
    ORR.W r0, r0, #0x80
    STRB.W r0, [r10]
    B.N ascii_L_080203D8
ascii_L_080203F2
    LDRB.W r1, [r10]
    LSLS.N r1, r1, #0x18
    BPL.N ascii_L_08020408
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_08020416
ascii_L_08020408
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    MOV.N r0, r1
    ADDS.N r1, r1, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r3, [r4, r0]
ascii_L_08020416
    LDRH.W r0, [r8, #0x10]
    LDR.N r1, ascii_L_08020818
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.W r0, [r8, #0x10]
    LDR.N r1, ascii_L_08020818
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRH.N r0, [r7, #0x12]
    LDR.N r1, ascii_L_08020818
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #0x12]
    LDR.N r1, ascii_L_08020818
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.W r10, ascii_L_0802081C
    LDR.N r1, ascii_L_08020818
    LDRH.W r0, [r10, #0xc]
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r0, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r3, [r4, r0]
    LDRB.W r0, [r9, #1]
    ADDS.N r2, r1, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r0, [r4, r1]
    LDRB.W r0, [r10, #0xc]
    LDR.N r1, ascii_L_08020818
    BL sub_08015034
    LDRH.N r0, [r6, #0xa]
    LDRB.W r1, [r9]
    ADDS.N r2, r0, #1
    UXTH.N r3, r2
    STRB.N r1, [r4, r0]
    LDRB.W r0, [r9, #1]
    ADDS.N r1, r3, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r0, [r4, r3]
    LDRH.W r0, [r8, #0x12]
    LDR.N r1, ascii_L_08020818
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r0, [r6, #0xa]
    LDRB.W r1, [r9]
    ADDS.N r2, r0, #1
    UXTH.N r3, r2
    STRB.N r1, [r4, r0]
    LDRB.W r0, [r9, #1]
    ADDS.N r1, r3, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r0, [r4, r3]
    LDRB.W r0, [r8, #0x12]
    LDR.N r1, ascii_L_08020818
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRH.N r0, [r7, #0x14]
    LDR.N r1, ascii_L_08020818
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #0x14]
    LDR.N r1, ascii_L_08020818
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRH.W r0, [r10, #0xe]
    LDR.N r1, ascii_L_08020818
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.W r0, [r10, #0xe]
    LDR.N r1, ascii_L_08020818
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r3, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r3, [r4, r0]
ascii_L_0802059E
    B.N ascii_L_0801FE08
ascii_L_080205A0
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_08020626
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    MOV.N r1, r0
    MOVW.W r10, #0xffff
    AND.W r0, r10, r1, lsl #8
    STRH.W r0, [r8, #0x14]
    LDRB.N r1, [r4, #0x12]
    LDRB.N r0, [r4, #0x11]
    BL sub_08014FD6
    LDRH.W r1, [r8, #0x14]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.W r1, [r8, #0x14]
    MOVS.N r0, #0xc6
    BL sub_08022442
    LDRB.N r1, [r4, #0x14]
    LDRB.N r0, [r4, #0x13]
    BL sub_08014FD6
    MOV.N r1, r0
    AND.W r0, r10, r1, lsl #8
    STRH.N r0, [r7, #0x16]
    LDRB.N r1, [r4, #0x16]
    LDRB.N r0, [r4, #0x15]
    BL sub_08014FD6
    LDRH.N r1, [r7, #0x16]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7, #0x16]
    MOVS.N r0, #0xc8
    BL sub_08022442
    LDRB.N r1, [r4, #0x18]
    LDRB.N r0, [r4, #0x17]
    BL sub_08014FD6
    LDR.N r7, ascii_L_0802081C
    MOV.N r1, r0
    AND.W r0, r10, r1, lsl #8
    STRH.N r0, [r7, #0x10]
    LDRB.N r1, [r4, #0x1a]
    LDRB.N r0, [r4, #0x19]
    BL sub_08014FD6
    LDRH.N r1, [r7, #0x10]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7, #0x10]
    MOVS.N r0, #0xca
    B.N ascii_L_08020628
ascii_L_08020626
    B.N ascii_L_0802062C
ascii_L_08020628
    BL sub_08022442
ascii_L_0802062C
    LDR.N r0, ascii_L_08020820
    LDRB.N r1, [r0]
    MOVS.N r0, #0x3a
    BL sub_08022442
    B.N ascii_L_0801FEAA
ascii_L_08020638
    LDR.N r1, ascii_L_08020820
    LDRB.N r1, [r1]
    LSLS.N r1, r1, #0x1f
    BEQ.N ascii_L_0802064E
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_0802065A
ascii_L_0802064E
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_0802065A
    LDRH.W r0, [r8, #0x14]
    LDR.N r1, ascii_L_08020818
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDRB.W r0, [r8, #0x14]
    LDR.N r1, ascii_L_08020818
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r2, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r0, r2, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r1, [r4, r2]
    LDRH.N r0, [r7, #0x16]
    LDR.N r1, ascii_L_08020818
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDRB.N r0, [r7, #0x16]
    LDR.N r1, ascii_L_08020818
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDR.N r7, ascii_L_0802081C
    LDR.N r1, ascii_L_08020818
    LDRH.N r0, [r7, #0x10]
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    LDRB.N r0, [r7, #0x10]
    LDR.N r1, ascii_L_08020818
    BL sub_08015034
    LDRH.N r2, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r2, #1
    UXTH.N r1, r0
    STRB.N r3, [r4, r2]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_0801FF80
ascii_L_0802071C
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_080207A4
    ADDS.N r4, #0xf
    LDRB.N r1, [r4, #1]
    LDRB.N r0, [r4]
    BL sub_08014FD6
    MOV.N r1, r0
    LDR.N r0, ascii_L_08020824
    MOVW.W r10, #0xffff
    AND.W r1, r10, r1, lsl #8
    STRH.N r1, [r0, #0x18]
    LDRB.N r1, [r4, #3]
    LDRB.N r0, [r4, #2]
    BL sub_08014FD6
    MOV.N r1, r0
    LDR.N r0, ascii_L_08020824
    LDRH.N r0, [r0, #0x18]
    ADD.N r0, r1
    UXTH.N r1, r0
    LDR.N r0, ascii_L_08020824
    STRH.N r1, [r0, #0x18]
    MOVS.N r0, #0xce
    BL sub_08022442
    LDRB.N r1, [r4, #5]
    LDRB.N r0, [r4, #4]
    BL sub_08014FD6
    MOV.N r1, r0
    AND.W r0, r10, r1, lsl #8
    STRH.N r0, [r7, #0x1a]
    LDRB.N r1, [r4, #7]
    LDRB.N r0, [r4, #6]
    BL sub_08014FD6
    LDRH.N r1, [r7, #0x1a]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7, #0x1a]
    MOVS.N r0, #0xf4
    BL sub_08022442
    LDRB.N r1, [r4, #9]
    LDRB.N r0, [r4, #8]
    BL sub_08014FD6
    LDR.W r8, ascii_L_0802081C
    MOV.N r1, r0
    AND.W r0, r10, r1, lsl #8
    STRH.W r0, [r8, #0x14]
    LDRB.N r1, [r4, #0xb]
    LDRB.N r0, [r4, #0xa]
    BL sub_08014FD6
    LDRH.W r1, [r8, #0x14]
    B.N ascii_L_080207A6
ascii_L_080207A4
    B.N ascii_L_08020848
ascii_L_080207A6
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.W r1, [r8, #0x14]
    MOVS.N r0, #0xf6
    BL sub_08022442
    LDRB.N r1, [r4, #0xd]
    LDRB.N r0, [r4, #0xc]
    BL sub_08014FD6
    MOV.N r1, r0
    LDR.N r0, ascii_L_08020824
    AND.W r1, r10, r1, lsl #8
    STRH.N r1, [r0, #0x16]
    LDRB.N r1, [r4, #0xf]
    LDRB.N r0, [r4, #0xe]
    BL sub_08014FD6
    LDR.N r1, ascii_L_08020824
    LDRH.N r1, [r1, #0x16]
    ADD.N r0, r1
    UXTH.N r1, r0
    LDR.N r0, ascii_L_08020824
    STRH.N r1, [r0, #0x16]
    MOVS.N r0, #0xcc
    BL sub_08022442
    LDRB.N r1, [r4, #0x11]
    LDRB.N r0, [r4, #0x10]
    BL sub_08014FD6
    MOV.N r1, r0
    AND.W r0, r10, r1, lsl #8
    STRH.N r0, [r7, #0x18]
    LDRB.N r1, [r4, #0x13]
    LDRB.N r0, [r4, #0x12]
    BL sub_08014FD6
    LDRH.N r1, [r7, #0x18]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.N r1, [r7, #0x18]
    MOVS.N r0, #0xf0
    BL sub_08022442
    LDRB.N r1, [r4, #0x15]
    LDRB.N r0, [r4, #0x14]
    BL sub_08014FD6
    MOV.N r1, r0
    AND.W r0, r10, r1, lsl #8
    B.N ascii_L_08020828
    DCB 0 ; native pool/table alignment
    DCB 0 ; native pool/table alignment
ascii_L_08020818
    DCD hexbuf
ascii_L_0802081C
    DCD recov
ascii_L_08020820
    DCD g_20000276
ascii_L_08020824
    DCD limits
ascii_L_08020828
    STRH.W r0, [r8, #0x12]
    LDRB.N r1, [r4, #0x17]
    LDRB.N r0, [r4, #0x16]
    SUBS.N r4, #0xf
    BL sub_08014FD6
    LDRH.W r1, [r8, #0x12]
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.W r1, [r8, #0x12]
    MOVS.N r0, #0xf2
    BL sub_08022442
ascii_L_08020848
    LDR.N r0, ascii_L_08020C3C
    LDRB.N r1, [r0]
    MOVS.N r0, #0x3a
    BL sub_08022442
    B.N ascii_L_080200D6
ascii_L_08020854
    LDR.N r1, ascii_L_08020C3C
    LDRB.N r1, [r1]
    LSLS.N r1, r1, #0x1e
    BPL.N ascii_L_0802086A
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_08020876
ascii_L_0802086A
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_08020876
    LDRH.W r0, [r8, #0x18]
    LDR.N r1, ascii_L_08020C40
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.W r0, [r8, #0x18]
    LDR.N r1, ascii_L_08020C40
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRH.N r0, [r7, #0x1a]
    LDR.N r1, ascii_L_08020C40
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #0x1a]
    LDR.N r1, ascii_L_08020C40
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.W r10, ascii_L_08020C44
    LDR.N r1, ascii_L_08020C40
    LDRH.W r0, [r10, #0x14]
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r0, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    LDRB.W r0, [r9, #1]
    ADDS.N r2, r1, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r0, [r4, r1]
    LDRB.W r0, [r10, #0x14]
    LDR.N r1, ascii_L_08020C40
    BL sub_08015034
    LDRH.N r0, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    LDRB.W r0, [r9, #1]
    ADDS.N r2, r1, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r0, [r4, r1]
    LDRH.W r0, [r8, #0x16]
    LDR.N r1, ascii_L_08020C40
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r0, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    LDRB.W r0, [r9, #1]
    ADDS.N r2, r1, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r0, [r4, r1]
    LDRB.W r0, [r8, #0x16]
    LDR.N r1, ascii_L_08020C40
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRH.N r0, [r7, #0x18]
    LDR.N r1, ascii_L_08020C40
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r0, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    LDRB.W r0, [r9, #1]
    ADDS.N r2, r1, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r0, [r4, r1]
    LDRB.N r0, [r7, #0x18]
    LDR.N r1, ascii_L_08020C40
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRH.W r0, [r10, #0x12]
    LDR.N r1, ascii_L_08020C40
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.W r0, [r10, #0x12]
    LDR.N r1, ascii_L_08020C40
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
    B.N ascii_L_08020290
ascii_L_08020A00
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    MOVW.W r1, #0xffff
    AND.W r8, r1, r0, lsl #8
    LDRB.N r1, [r4, #0x10]
    LDRB.N r0, [r4, #0xf]
    BL sub_08014FD6
    ADD.N r0, r8
    UXTH.N r1, r0
    CMP.N r1, #0x78
    BLS.N ascii_L_08020A22
    MOVS.N r1, #0x78
ascii_L_08020A22
    STRH.N r1, [r7, #0xc]
    MOVS.N r0, #0xd0
    BL sub_08022442
    LDRB.N r1, [r4, #0x12]
    LDRB.N r0, [r4, #0x11]
    BL sub_08014FD6
    LDR.N r1, ascii_L_08020C48
    STRB.N r0, [r1, #6]
    MOV.N r1, r0
    MOVS.N r0, #0xd2
    BL sub_08022442
    B.N ascii_L_08020290
ascii_L_08020A40
    LDRH.N r0, [r7, #0xc]
    LDR.N r1, ascii_L_08020C40
    LSRS.N r0, r0, #8
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #0xc]
    LDR.N r1, ascii_L_08020C40
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r0, ascii_L_08020C48
    LDR.N r1, ascii_L_08020C40
    LDRB.N r0, [r0, #6]
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    B.N ascii_L_080203E2
ascii_L_08020AA0
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CMP.N r0, #0x13
    BLS.N ascii_L_08020AB4
    LDR.N r0, ascii_L_08020C48
    MOVS.N r1, #0xa
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020B1A
ascii_L_08020AB4
    CMP.N r0, #0x11
    BLS.N ascii_L_08020AC0
    LDR.N r0, ascii_L_08020C48
    MOVS.N r1, #9
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020B1A
ascii_L_08020AC0
    CMP.N r0, #0xf
    BLS.N ascii_L_08020ACC
    LDR.N r0, ascii_L_08020C48
    MOVS.N r1, #8
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020B1A
ascii_L_08020ACC
    CMP.N r0, #0xd
    BLS.N ascii_L_08020AD8
    LDR.N r0, ascii_L_08020C48
    MOVS.N r1, #7
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020B1A
ascii_L_08020AD8
    CMP.N r0, #0xb
    BLS.N ascii_L_08020AE4
    LDR.N r0, ascii_L_08020C48
    MOVS.N r1, #6
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020B1A
ascii_L_08020AE4
    CMP.N r0, #9
    BLS.N ascii_L_08020AF0
    LDR.N r0, ascii_L_08020C48
    MOVS.N r1, #5
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020B1A
ascii_L_08020AF0
    CMP.N r0, #7
    BLS.N ascii_L_08020AFC
    LDR.N r0, ascii_L_08020C48
    MOVS.N r1, #4
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020B1A
ascii_L_08020AFC
    CMP.N r0, #5
    BLS.N ascii_L_08020B08
    LDR.N r0, ascii_L_08020C48
    MOVS.N r1, #3
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020B1A
ascii_L_08020B08
    CMP.N r0, #3
    BLS.N ascii_L_08020B14
    LDR.N r0, ascii_L_08020C48
    MOVS.N r1, #2
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020B1A
ascii_L_08020B14
    LDR.N r0, ascii_L_08020C48
    MOVS.N r1, #1
    STRB.N r1, [r0, #7]
ascii_L_08020B1A
    LDR.N r0, ascii_L_08020C48
    LDRB.N r1, [r0, #7]
    MOVS.N r0, #0xd4
    BL sub_08022442
ascii_L_08020B24
    B.N ascii_L_080203E2
ascii_L_08020B26
    LDR.N r0, ascii_L_08020C48
    LDRB.N r0, [r0, #7]
    CMP.N r0, #9
    BLS.N ascii_L_08020B32
    MOVS.N r0, #0x14
    B.N ascii_L_08020B74
ascii_L_08020B32
    CMP.N r0, #8
    BLS.N ascii_L_08020B3A
    MOVS.N r0, #0x12
    B.N ascii_L_08020B74
ascii_L_08020B3A
    CMP.N r0, #7
    BLS.N ascii_L_08020B42
    MOVS.N r0, #0x10
    B.N ascii_L_08020B74
ascii_L_08020B42
    CMP.N r0, #6
    BLS.N ascii_L_08020B4A
    MOVS.N r0, #0xe
    B.N ascii_L_08020B74
ascii_L_08020B4A
    CMP.N r0, #5
    BLS.N ascii_L_08020B52
    MOVS.N r0, #0xc
    B.N ascii_L_08020B74
ascii_L_08020B52
    CMP.N r0, #4
    BLS.N ascii_L_08020B5A
    MOVS.N r0, #0xa
    B.N ascii_L_08020B74
ascii_L_08020B5A
    CMP.N r0, #3
    BLS.N ascii_L_08020B62
    MOVS.N r0, #8
    B.N ascii_L_08020B74
ascii_L_08020B62
    CMP.N r0, #2
    BLS.N ascii_L_08020B6A
    MOVS.N r0, #6
    B.N ascii_L_08020B74
ascii_L_08020B6A
    CMP.N r0, #1
    BLS.N ascii_L_08020B72
    MOVS.N r0, #4
    B.N ascii_L_08020B74
ascii_L_08020B72
    MOVS.N r0, #2
ascii_L_08020B74
    UXTB.N r0, r0
    LDR.N r1, ascii_L_08020C40
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
    B.N ascii_L_080203E2
ascii_L_08020B94
    MOVS.N r1, #0
    MOVS.N r0, #0x80
    BL sub_08022442
    MOVS.N r1, #0
    MOVS.N r0, #0x30
    BL sub_08022442
    MOVS.N r1, #0
    MOVS.N r0, #0x3a
    BL sub_08022442
    MOVS.N r0, #0
    STRB.W r0, [r10]
    LDR.N r1, ascii_L_08020C3C
    STRB.N r0, [r1]
    BL sub_08016F1E
    MOVS.N r0, #0
    BL sub_080179EA
    B.N ascii_L_080203E2
ascii_L_08020BC2
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    MOV.N r1, r0
    SUB.W r0, r1, #8
    CMP.N r0, #8
    BHI.N ascii_L_08020BE2
    MOVS.N r0, #0x32
    BL sub_08022442
    CMP.N r0, #0
    BNE.N ascii_L_08020B24
    MOVS.N r5, #9
    B.N ascii_L_0802059E
ascii_L_08020BE2
    MOVS.N r5, #9
    B.N ascii_L_0802059E
ascii_L_08020BE6
    CMP.W r11, #1
    BEQ.N ascii_L_08020BFC
    CMP.W r11, #2
    BEQ.N ascii_L_08020C58
    CMP.W r11, #0x50
    BEQ.N ascii_L_08020C6A
    MOVS.N r5, #9
    B.N ascii_L_0802059E
ascii_L_08020BFC
    LDR.N r0, ascii_L_08020C4C
    LDRB.N r1, [r0]
    ORR.W r1, r1, #0x90
    STRB.N r1, [r0]
    LDRH.N r1, [r7]
    LDR.N r0, ascii_L_08020C44
    SUBS.N r1, #0x32
    STRH.N r1, [r0]
    LDRH.N r1, [r7, #2]
    ADDS.N r1, #0x32
    STRH.N r1, [r0, #2]
    LDR.N r1, ascii_L_08020C50
    MOVS.N r0, #0
    STRH.N r0, [r1]
    LDR.N r2, ascii_L_08020C54
    MOVS.N r7, #1
    LDRB.N r3, [r2]
    B.N ascii_L_08020C34
ascii_L_08020C22
    LDRH.W r12, [r1]
    LSL.W r2, r7, r0
    ORR.W r2, r2, r12
    STRH.N r2, [r1]
    ADDS.N r0, r0, #1
    UXTH.N r0, r0
ascii_L_08020C34
    CMP.N r0, r3
    BLO.N ascii_L_08020C22
    B.N ascii_L_0802059E
    DCB 0 ; native pool/table alignment
    DCB 0 ; native pool/table alignment
ascii_L_08020C3C
    DCD g_20000276
ascii_L_08020C40
    DCD hexbuf
ascii_L_08020C44
    DCD recov
ascii_L_08020C48
    DCD delays
ascii_L_08020C4C
    DCD g_2000050D
ascii_L_08020C50
    DCD g_200000CE
ascii_L_08020C54
    DCD g_20000162
ascii_L_08020C58
    MOVS.N r1, #0
    MOV.W r0, #0x200
    BL sub_08022442
    MOVS.N r0, #1
    BL sub_080179EA
    B.N ascii_L_0802059E
ascii_L_08020C6A
    MOVW.W r1, #0xaaaa
    MOVW.W r0, #0x7ff5
    BL sub_08022442
    LDR.N r0, ascii_L_08021070
    MOVS.N r2, #0xa
    MOV.N r1, r2
    LDRB.N r0, [r0]
    BL os_task_delay
    LDR.N r1, ascii_L_08021074
    MOVS.N r0, #0x80
    STR.N r0, [r1]
    B.N ascii_L_0802059E
ascii_L_08020C8A
    CMP.W r11, #0xff
    BNE.N ascii_L_08020CC8
    LDR.N r0, ascii_L_08021078
    LDRB.N r0, [r0]
    CMP.N r0, #1
    BLS.N ascii_L_08020CC8
    LDRH.N r1, [r6, #0xa]
    SUBS.N r1, #0x11
    MULS.N r1, r0, r1
    ADDS.N r1, r1, #4
    UXTH.N r1, r1
    LDR.N r0, ascii_L_0802107C
    BL sub_080150EE
    MOVS.N r0, #0
    STRH.N r0, [r6, #0x10]
    LDR.N r0, [sp, #0x10]
    STRB.N r0, [r6, #1]
    MOVS.N r0, #1
    LDRH.N r1, [r6, #0xa]
    B.N ascii_L_08020CC2
ascii_L_08020CB6
    LDRB.N r2, [r4, r0]
    LDRH.N r3, [r6, #0x10]
    ADD.N r2, r3
    STRH.N r2, [r6, #0x10]
    ADDS.N r0, r0, #1
    UXTH.N r0, r0
ascii_L_08020CC2
    CMP.N r0, r1
    BLO.N ascii_L_08020CB6
    B.N ascii_L_08020D3C
ascii_L_08020CC8
    LDRH.N r0, [r6, #0xa]
    CMP.N r0, #0xd
    BLO.N ascii_L_08020CD8
    SUBS.N r0, #0xd
    UXTH.N r1, r0
    LDR.N r0, ascii_L_0802107C
    BL sub_080150EE
ascii_L_08020CD8
    MOVS.N r0, #1
    LDRH.N r2, [r6, #0xa]
    B.N ascii_L_08020CEC
ascii_L_08020CDE
    LDRB.N r3, [r4, r0]
    LDR.N r1, [sp, #8]
    ADD.N r1, r3
    UXTH.N r1, r1
    STR.N r1, [sp, #8]
    ADDS.N r0, r0, #1
    UXTH.N r0, r0
ascii_L_08020CEC
    CMP.N r0, r2
    BLO.N ascii_L_08020CDE
    LDR.N r0, [sp, #8]
    MVNS.N r0, r0
    ADDS.N r0, r0, #1
    UXTH.N r5, r0
    LSRS.N r0, r5, #8
    LDR.N r1, ascii_L_08021080
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    LDRB.W r2, [r9, #1]
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
    UXTB.N r0, r5
    LDR.N r1, ascii_L_08021080
    BL sub_08015034
    LDRH.N r0, [r6, #0xa]
    LDRB.W r2, [r9]
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    LDRB.W r2, [r9, #1]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r2, [r4, r1]
    MOVS.N r2, #0xd
    ADDS.N r1, r0, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r2, [r4, r0]
ascii_L_08020D3C
    MOVS.N r0, #0
    STRH.N r0, [r6, #0xc]
    ADD.N sp, #0x14
    MOVS.N r2, #1
    POP.W {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    MOVW.W r1, #0x727
    LDR.N r0, [pc, #0x334] ; native shared UART4 pool at 0x08021084
    B.W USART_ITConfig
    ENDP

; Following literal pool shared with the C suffix.
; at: ascii_following_pool 0x08021070
; at: os_task_dly_idx 0x20000606
; at: ascii_watchdog_base 0x40002C00
; at: pack_count 0x2000009E
; at: pace_buf 0x200035EA
; at: hexbuf 0x20000109
; at: ascii_uart4_base 0x40004C00
    AREA ascii_pool_ro, DATA, READONLY, ALIGN=2

    IMPORT ascii_watchdog_base

    IMPORT ascii_uart4_base
    EXPORT ascii_following_pool
ascii_following_pool
    DCD os_task_dly_idx
    DCD ascii_watchdog_base
    DCD pack_count
    DCD pace_buf
    DCD hexbuf
    DCD ascii_uart4_base
    END
