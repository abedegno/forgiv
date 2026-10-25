; Interim exact assembler dispatcher; native C draft remains DF70 108.
; Composed with nine C functions into one .text area by tools/ascii_compose.py.
; opts: --cpu Cortex-M3
; at: sub_0801D424 0x0801D57C
; at: sub_0801D606 0x0801D75E
; at: sub_0801D6E4 0x0801D83C
; at: sub_0801D8C0 0x0801DA18
; at: sub_0801DB3A 0x0801DC92
; at: sub_0801DF70 0x0801E0C8
; at: sub_08020D52 0x08020EAA
; at: sub_08020E40 0x08020F98
; at: sub_08020FA4 0x080210FC
; at: sub_0802126C 0x080213C4
; at: pace_buf 0x20003602
; at: modbus_tx 0x200036AC
; at: modbus_resp 0x20003743
; at: modbus_regs 0x200037DA
; at: rx_ready 0x2000051A
; at: tx_cid2 0x2000051B
; at: mb_addr 0x2000051C
; at: rx_count 0x2000051D
; at: rx_idx 0x2000051E
; at: ascii_rx_state 0x2000051F
; at: tx_pack 0x20000520
; at: mb_len 0x20000521
; at: mb_wait 0x20000522
; at: tx_len 0x20000524
; at: tx_idx 0x20000526
; at: rx_info_len 0x20000528
; at: tx_sum 0x2000052A
; at: mb_crc 0x2000052C
; at: pace_mode 0x200000DE
; at: hexbuf 0x2000010D
; at: packs 0x20003D6A
; at: cfgs 0x20004161
; at: measure 0x200014EA
; at: limits 0x2000153A
; at: alarm_mask 0x2000027D
; at: charging 0x2000009F
; at: pack_count 0x2000009E
; at: own_addr 0x20000167
; at: g_20000180 0x20000184
; at: g_2000017C 0x20000180
; at: g_20000182 0x20000186
; at: g_20000164 0x20000168
; at: g_2000016C 0x20000170
; at: g_20000162 0x20000166
; at: g_20000077 0x20000077
; at: g_20000062 0x20000062
; at: g_200000A2 0x200000A2
; at: g_20000124 0x20000128
; at: g_20000110 0x20000114
; at: g_20000148 0x2000014C
; at: g_20000184 0x20000188
; at: g_20000185 0x20000189
; at: g_20000271 0x20000279
; at: g_20000272 0x2000027A
; at: g_20000273 0x2000027B
; at: g_2000050D 0x20000519
; at: g_20000274 0x2000027C
; at: g_20000276 0x2000027E
; at: g_20000277 0x2000027F
; at: g_20000278 0x20000280
; at: g_2000010C 0x20000110
; at: g_2000010E 0x20000112
; at: g_20001094 0x200010AC
; at: g_2000012A 0x2000012E
; at: g_2000012C 0x20000130
; at: g_2000119E 0x200011B6
; at: g_20000051 0x20000051
; at: g_20000130 0x20000134
; at: g_20000134 0x20000138
; at: g_200000A0 0x200000A0
; at: g_20000138 0x2000013C
; at: g_20000139 0x2000013D
; at: g_20000098 0x20000098
; at: g_200000AD 0x200000AD
; at: prot 0x2000156A
; at: recov 0x20001554
; at: delays 0x20000304
; at: alarm_mask2 0x2000027E
; at: g_200000FA 0x200000FC
; at: g_200000F8 0x200000FA
; at: g_20000168 0x2000016C
; at: g_200000A9 0x200000A9
; at: g_20000186 0x2000018A
; at: g_20000096 0x20000096
; at: g_20000094 0x20000094
; at: g_20000154 0x20000158
; at: g_2000014C 0x20000150
; at: g_20000150 0x20000154
; at: log_index 0x2000018C
; at: g_2000018A 0x2000018E
; at: date_now 0x20000190
; at: date_start 0x200000B6
; at: date_end 0x200000BC
; at: log_interval 0x200000B4
; at: g_200000CE 0x200000CE
; at: os_task_dly_idx 0x2000061E
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
ascii_L_080211C8 EQU sub_0801DF70 + 0x3100
ascii_L_080211CC EQU sub_0801DF70 + 0x3104
ascii_L_080211D0 EQU sub_0801DF70 + 0x3108
ascii_L_080211D4 EQU sub_0801DF70 + 0x310C
ascii_L_080211D8 EQU sub_0801DF70 + 0x3110
ascii_L_080211DC EQU sub_0801DF70 + 0x3114
    PUSH.W {r0, r4, r5, r6, r7, r8, r9, r10, r11, lr}
    SUB.N sp, #0x10
    MOVS.N r5, #0
    MOVS.N r0, #0
    STR.N r0, [sp, #8]
    MOVW.W r0, #0xffff
    STR.N r0, [sp, #4]
    LDR.N r6, ascii_L_0801E3C8
    MOVS.N r0, #0
    STRH.N r0, [r6, #0xa]
    LDR.N r4, ascii_L_0801E3CC
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
    LDR.N r0, ascii_L_0801E3D0
    LDR.N r1, ascii_L_0801E3C4
    LDRB.N r0, [r0]
    BL sub_08015034
    LDR.W r9, ascii_L_0801E3C4
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
    LDR.W r8, ascii_L_0801E3D4
    LDR.N r7, ascii_L_0801E3D8
    LDR.W r10, ascii_L_0801E3DC
    CMP.N r1, #0xba
    BEQ.N ascii_L_0801E264
    BGT.N ascii_L_0801E208
    CMP.N r1, #0xa8
    BEQ.N ascii_L_0801E266
    BGT.N ascii_L_0801E1D0
    CMP.N r1, #0x9c
    BEQ.N ascii_L_0801E266
    BGT.N ascii_L_0801E1B6
    CMP.N r1, #0x90
    BEQ.N ascii_L_0801E268
    BGT.N ascii_L_0801E1A0
    CMP.N r1, #2
    BEQ.N ascii_L_0801E26A
    CMP.N r1, #3
    BEQ.N ascii_L_0801E26C
    CMP.N r1, #0x42
    BEQ.N ascii_L_0801E26E
    CMP.N r1, #0x44
    BNE.N ascii_L_0801E270
    B.N ascii_L_0801E68A
ascii_L_0801E1A0
    LDR.N r7, ascii_L_0801E3E0
    CMP.N r1, #0x99
    BEQ.N ascii_L_0801E272
    LDR.N r3, ascii_L_0801E3E4
    LDR.N r2, ascii_L_0801E3E8
    LDR.N r0, ascii_L_0801E3EC
    CMP.N r1, #0x9a
    BEQ.N ascii_L_0801E274
    CMP.N r1, #0x9b
    BNE.N ascii_L_0801E270
    B.N ascii_L_0801E90E
ascii_L_0801E1B6
    SUBS.N r1, #0xa0
    CMP.N r1, #8
    BHS.N ascii_L_0801E2B6
    TBH [pc, r1, lsl #1]
ascii_L_0801E1C0
    DCW (ascii_L_0801EE3A - ascii_L_0801E1C0) / 2
    DCW (ascii_L_0801F1B8 - ascii_L_0801E1C0) / 2
    DCW (ascii_L_0801EFFA - ascii_L_0801E1C0) / 2
    DCW (ascii_L_0801E2B6 - ascii_L_0801E1C0) / 2
    DCW (ascii_L_08020CEC - ascii_L_0801E1C0) / 2
    DCW (ascii_L_0801E9FA - ascii_L_0801E1C0) / 2
    DCW (ascii_L_0801EA1E - ascii_L_0801E1C0) / 2
    DCW (ascii_L_0801EB02 - ascii_L_0801E1C0) / 2
ascii_L_0801E1D0
    SUBS.N r1, #0xa9
    LDR.W r8, ascii_L_0801E3F0
    LDR.W r10, ascii_L_0801E3F4
    MOV.W r7, #0x3f800000
    CMP.N r1, #0x11
    BHS.N ascii_L_0801E2B6
    TBH [pc, r1, lsl #1]
ascii_L_0801E1E6
    DCW (ascii_L_0801EFC6 - ascii_L_0801E1E6) / 2
    DCW (ascii_L_0801E2B6 - ascii_L_0801E1E6) / 2
    DCW (ascii_L_0801F0AE - ascii_L_0801E1E6) / 2
    DCW (ascii_L_0801F134 - ascii_L_0801E1E6) / 2
    DCW (ascii_L_0801F02E - ascii_L_0801E1E6) / 2
    DCW (ascii_L_0801E2B8 - ascii_L_0801E1E6) / 2
    DCW (ascii_L_0801EEB4 - ascii_L_0801E1E6) / 2
    DCW (ascii_L_0801E2B6 - ascii_L_0801E1E6) / 2
    DCW (ascii_L_0801F2D0 - ascii_L_0801E1E6) / 2
    DCW (ascii_L_0801F26C - ascii_L_0801E1E6) / 2
    DCW (ascii_L_0801F24E - ascii_L_0801E1E6) / 2
    DCW (ascii_L_08020D1A - ascii_L_0801E1E6) / 2
    DCW (ascii_L_0801E2B8 - ascii_L_0801E1E6) / 2
    DCW (ascii_L_0801EF4C - ascii_L_0801E1E6) / 2
    DCW (ascii_L_0801F652 - ascii_L_0801E1E6) / 2
    DCW (ascii_L_0801F66A - ascii_L_0801E1E6) / 2
    DCW (ascii_L_0801F3C2 - ascii_L_0801E1E6) / 2
ascii_L_0801E208
    CMP.N r1, #0xd9
    BEQ.W ascii_L_0801FE9C
    BGT.N ascii_L_0801E252
    CMP.N r1, #0xd1
    BEQ.N ascii_L_0801E310
    BGT.N ascii_L_0801E23A
    CMP.N r1, #0xc1
    BEQ.N ascii_L_0801E312
    BGT.N ascii_L_0801E22C
    LDR.N r7, ascii_L_0801E3F8
    CMP.N r1, #0xbb
    BEQ.N ascii_L_0801E314
    CMP.N r1, #0xbc
    BEQ.N ascii_L_0801E316
    CMP.N r1, #0xbd
    BNE.N ascii_L_0801E2B6
    B.N ascii_L_0801E9A8
ascii_L_0801E22C
    CMP.N r1, #0xc2
    BEQ.N ascii_L_0801E318
    CMP.N r1, #0xc3
    BEQ.N ascii_L_0801E31A
    CMP.N r1, #0xd0
    BNE.N ascii_L_0801E2B6
    B.N ascii_L_0801EA1C
ascii_L_0801E23A
    SUBS.N r1, #0xd2
    CMP.N r1, #7
    BHS.N ascii_L_0801E2B6
    TBH [pc, r1, lsl #1]
ascii_L_0801E244
    DCW (ascii_L_0801F85E - ascii_L_0801E244) / 2
    DCW (ascii_L_0801F93C - ascii_L_0801E244) / 2
    DCW (ascii_L_0801FA40 - ascii_L_0801E244) / 2
    DCW (ascii_L_0801FB0E - ascii_L_0801E244) / 2
    DCW (ascii_L_0801FC16 - ascii_L_0801E244) / 2
    DCW (ascii_L_0801FCF2 - ascii_L_0801E244) / 2
    DCW (ascii_L_0801FDF6 - ascii_L_0801E244) / 2
ascii_L_0801E252
    CMP.N r1, #0xe1
    BEQ.W ascii_L_08020790
    BGT.N ascii_L_0801E288
    SUB.W r1, r1, #0xda
    CMP.N r1, #7
    BHS.N ascii_L_0801E2B6
    B.N ascii_L_0801E276
ascii_L_0801E264
    B.N ascii_L_0801EA62
ascii_L_0801E266
    B.N ascii_L_0801E2B8
ascii_L_0801E268
    B.N ascii_L_0801E2DA
ascii_L_0801E26A
    B.N ascii_L_0801E2D2
ascii_L_0801E26C
    B.N ascii_L_0801E2D6
ascii_L_0801E26E
    B.N ascii_L_0801E6FE
ascii_L_0801E270
    B.N ascii_L_0801E2B6
ascii_L_0801E272
    B.N ascii_L_0801E772
ascii_L_0801E274
    B.N ascii_L_0801E87A
ascii_L_0801E276
    TBH [pc, r1, lsl #1]
ascii_L_0801E27A
    DCW (ascii_L_0801FF62 - ascii_L_0801E27A) / 2
    DCW (ascii_L_08020012 - ascii_L_0801E27A) / 2
    DCW (ascii_L_080200DA - ascii_L_0801E27A) / 2
    DCW (ascii_L_0802023E - ascii_L_0801E27A) / 2
    DCW (ascii_L_080203EA - ascii_L_0801E27A) / 2
    DCW (ascii_L_0802054A - ascii_L_0801E27A) / 2
    DCW (ascii_L_080206F8 - ascii_L_0801E27A) / 2
ascii_L_0801E288
    CMP.N r1, #0xe5
    BEQ.W ascii_L_08020C7E
    BGT.N ascii_L_0801E2A4
    CMP.N r1, #0xe2
    BEQ.W ascii_L_08020B58
    CMP.N r1, #0xe3
    BEQ.W ascii_L_08020B98
    CMP.N r1, #0xe4
    BNE.N ascii_L_0801E2B6
    B.W ascii_L_08020BF8
ascii_L_0801E2A4
    CMP.N r1, #0xe6
    BEQ.W ascii_L_08020874
    CMP.N r1, #0xe7
    BEQ.W ascii_L_080209AC
    CMP.N r1, #0xff
    BEQ.W ascii_L_08020D3E
ascii_L_0801E2B6
    MOVS.N r5, #4
ascii_L_0801E2B8
    MOV.N r1, r5
    LDR.N r0, ascii_L_0801E3CC
    BL sub_0801512E
    LDR.N r0, [sp, #0x10]
    CMP.N r0, #0x42
ascii_L_0801E2C4
    BEQ.W ascii_L_08020DE2
    LDR.N r0, [sp, #0x10]
    CMP.N r0, #0x44
    BEQ.N ascii_L_0801E2C4
    B.W ascii_L_08020E20
ascii_L_0801E2D2
    MOVS.N r5, #2
    B.N ascii_L_0801E2B8
ascii_L_0801E2D6
    MOVS.N r5, #3
    B.N ascii_L_0801E2B8
ascii_L_0801E2DA
    LDR.N r1, ascii_L_0801E3D0
    LDRB.N r1, [r1]
    CMP.N r1, #1
    BEQ.N ascii_L_0801E2F0
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
    B.N ascii_L_0801E2B8
ascii_L_0801E2F0
    LDR.N r0, ascii_L_0801E3FC
    LDR.N r1, ascii_L_0801E3C4
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
    B.N ascii_L_0801E31C
ascii_L_0801E310
    B.N ascii_L_0801EAFC
ascii_L_0801E312
    B.N ascii_L_0801E320
ascii_L_0801E314
    B.N ascii_L_0801EAFE
ascii_L_0801E316
    B.N ascii_L_0801EB00
ascii_L_0801E318
    B.N ascii_L_0801E350
ascii_L_0801E31A
    B.N ascii_L_0801E380
ascii_L_0801E31C
    STRB.N r2, [r4, r0]
ascii_L_0801E31E
    B.N ascii_L_0801E2B8
ascii_L_0801E320
    MOVS.N r7, #0
    LDR.W r8, ascii_L_0801E400
ascii_L_0801E326
    LDR.N r1, ascii_L_0801E3C4
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
    BLO.N ascii_L_0801E326
    B.N ascii_L_0801E2B8
ascii_L_0801E350
    MOVS.N r7, #0
    LDR.W r8, ascii_L_0801E404
ascii_L_0801E356
    LDR.N r1, ascii_L_0801E3C4
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
    BLO.N ascii_L_0801E356
    B.N ascii_L_0801E2B8
ascii_L_0801E380
    MOV.W r10, #0
ascii_L_0801E384
    ADD.W r0, r4, r10, lsl #1
    LDRB.N r1, [r0, #0xe]
    LDRB.N r0, [r0, #0xd]
    BL sub_08014FD6
    MOV.N r1, r0
    LDR.N r0, ascii_L_0801E404
    STRB.W r1, [r0, r10]
    ADD.W r10, r10, #1
    UXTH.W r10, r10
    CMP.W r10, #0x14
    BLO.N ascii_L_0801E384
    MOV.W r10, #0
ascii_L_0801E3AA
    LDR.N r0, ascii_L_0801E404
    ADD.W r1, r0, r10, lsl #1
    LDRB.W r0, [r0, r10, lsl #1]
    LDRB.N r1, [r1, #1]
    ADD.W r0, r1, r0, lsl #8
    UXTH.N r1, r0
    MOVS.N r0, #0x3c
    ADD.W r0, r0, r10, lsl #1
    B.N ascii_L_0801E408
ascii_L_0801E3C4
    DCD hexbuf
ascii_L_0801E3C8
    DCD rx_ready
ascii_L_0801E3CC
    DCD pace_buf
ascii_L_0801E3D0
    DCD own_addr
ascii_L_0801E3D4
    DCD limits
ascii_L_0801E3D8
    DCD prot
ascii_L_0801E3DC
    DCD alarm_mask
ascii_L_0801E3E0
    DCD g_20000273
ascii_L_0801E3E4
    DCD g_20000274
ascii_L_0801E3E8
    DCD g_20000271
ascii_L_0801E3EC
    DCD g_20000272
ascii_L_0801E3F0
    DCD 0xC0B33333
ascii_L_0801E3F4
    DCD date_now
ascii_L_0801E3F8
    DCD date_end
ascii_L_0801E3FC
    DCD pack_count
ascii_L_0801E400
    DCD g_20000062
ascii_L_0801E404
    DCD g_20000077
ascii_L_0801E408
    UXTH.N r0, r0
    BL sub_08022442
    ADD.W r10, r10, #1
    UXTH.W r10, r10
    CMP.W r10, #0xa
    BLO.N ascii_L_0801E3AA
    LDR.N r1, ascii_L_0801E7FC
    LDRB.N r0, [r1]
    CMP.N r0, #0x42
    BEQ.N ascii_L_0801E45A
    CMP.N r0, #0x44
    BEQ.N ascii_L_0801E524
    LDR.N r1, ascii_L_0801E800
    MOVS.N r0, #0
    STRH.N r0, [r1]
    LDR.N r0, ascii_L_0801E804
    MOVS.N r1, #2
    STRH.N r1, [r0]
    LDR.N r0, ascii_L_0801E808
    MOVW.W r1, #0x1770
    LDRH.N r0, [r0]
    CMP.N r0, r1
    BHI.N ascii_L_0801E526
    MVN.W r0, #0x3b
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x3c
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x40
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x41
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801E664
ascii_L_0801E45A
    LDR.N r2, ascii_L_0801E800
    MOVS.N r0, #0
    STRH.N r0, [r2]
    LDRB.N r0, [r1, #1]
    CMP.N r0, #0x47
    BNE.N ascii_L_0801E470
    LDR.N r1, ascii_L_0801E808
    LDRH.N r1, [r1]
    CMP.W r1, #0x3e80
    BEQ.N ascii_L_0801E476
ascii_L_0801E470
    CMP.N r0, #0x59
    BEQ.N ascii_L_0801E496
    B.N ascii_L_0801E4A0
ascii_L_0801E476
    LDR.N r1, ascii_L_0801E804
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
    B.N ascii_L_0801E504
ascii_L_0801E496
    LDR.N r0, ascii_L_0801E808
    LDRH.N r0, [r0]
    CMP.W r0, #0x3e80
    BEQ.N ascii_L_0801E4CC
ascii_L_0801E4A0
    LDR.N r1, ascii_L_0801E804
    MOVS.N r0, #2
    STRH.N r0, [r1]
    LDR.N r0, ascii_L_0801E808
    MOVW.W r1, #0x1770
    LDRH.N r0, [r0]
    CMP.N r0, r1
    BHI.N ascii_L_0801E4EC
    MVN.W r0, #0x3b
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x3c
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x40
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x41
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801E504
ascii_L_0801E4CC
    LDR.N r0, ascii_L_0801E804
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
    B.N ascii_L_0801E504
ascii_L_0801E4EC
    MVN.W r0, #0x45
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x46
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x4a
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x4b
    STRH.N r0, [r7, #8]
ascii_L_0801E504
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
    B.N ascii_L_0801E528
ascii_L_0801E524
    B.N ascii_L_0801E530
ascii_L_0801E526
    B.N ascii_L_0801E64C
ascii_L_0801E528
    MOVS.N r0, #0xa4
    BL sub_08022442
ascii_L_0801E52E
    B.N ascii_L_0801E2B8
ascii_L_0801E530
    LDR.N r0, ascii_L_0801E800
    MOVS.N r2, #1
    STRH.N r2, [r0]
    LDRB.N r0, [r1, #1]
    CMP.N r0, #0x47
    BNE.N ascii_L_0801E546
    LDR.N r1, ascii_L_0801E808
    LDRH.N r1, [r1]
    CMP.W r1, #0x3e80
    BEQ.N ascii_L_0801E54C
ascii_L_0801E546
    CMP.N r0, #0x59
    BEQ.N ascii_L_0801E56A
    B.N ascii_L_0801E574
ascii_L_0801E54C
    LDR.N r0, ascii_L_0801E804
    STRH.N r2, [r0]
    MVN.W r0, #0x59
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x55
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x5e
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x5a
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801E626
ascii_L_0801E56A
    LDR.N r1, ascii_L_0801E808
    LDRH.N r1, [r1]
    CMP.W r1, #0x3e80
    BEQ.N ascii_L_0801E586
ascii_L_0801E574
    CMP.N r0, #0x57
    BEQ.N ascii_L_0801E5A6
    CMP.N r0, #0x58
    BEQ.N ascii_L_0801E5A6
    CMP.N r0, #0x5a
    BEQ.N ascii_L_0801E5A6
    CMP.N r0, #0x46
    BEQ.N ascii_L_0801E5A6
    B.N ascii_L_0801E5B2
ascii_L_0801E586
    LDR.N r1, ascii_L_0801E804
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
    B.N ascii_L_0801E626
ascii_L_0801E5A6
    LDR.N r0, ascii_L_0801E808
    LDRH.N r0, [r0]
    SUB.W r1, r0, #0x4800
    SUBS.N r1, #0xa8
    BEQ.N ascii_L_0801E5DC
ascii_L_0801E5B2
    LDR.N r0, ascii_L_0801E804
    MOVS.N r1, #2
    STRH.N r1, [r0]
    LDR.N r0, ascii_L_0801E808
    LDRH.N r0, [r0]
    CMP.W r0, #0x3e80
    BLO.N ascii_L_0801E5FC
    MVN.W r0, #0x59
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x55
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x5e
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x5a
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801E626
ascii_L_0801E5DC
    LDR.N r1, ascii_L_0801E804
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
    B.N ascii_L_0801E626
ascii_L_0801E5FC
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
ascii_L_0801E626
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
ascii_L_0801E64A
    B.N ascii_L_0801E2B8
ascii_L_0801E64C
    MVN.W r0, #0x45
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x46
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x4a
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x4b
    STRH.N r0, [r7, #8]
ascii_L_0801E664
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
    B.N ascii_L_0801E2B8
ascii_L_0801E68A
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    CMP.W r11, #0xff
    BEQ.N ascii_L_0801E6D4
    LDR.N r1, ascii_L_0801E80C
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
    BEQ.N ascii_L_0801E64A
    CMP.W r11, #1
    BHI.N ascii_L_0801E64A
    SUB.W r0, r11, #1
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801E810
    BL sub_0801DB3A
ascii_L_0801E6D2
    B.N ascii_L_0801E2B8
ascii_L_0801E6D4
    LDR.N r0, ascii_L_0801E814
    LDR.N r1, ascii_L_0801E80C
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
    LDR.N r1, ascii_L_0801E810
    MOVS.N r0, #0
    BL sub_0801DB3A
    B.N ascii_L_0801E2B8
ascii_L_0801E6FE
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    CMP.W r11, #0xff
    BEQ.N ascii_L_0801E748
    LDR.N r1, ascii_L_0801E80C
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
    BEQ.N ascii_L_0801E64A
    CMP.W r11, #1
    BHI.N ascii_L_0801E64A
    SUB.W r0, r11, #1
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801E810
    BL sub_0801D8C0
    B.N ascii_L_0801E2B8
ascii_L_0801E748
    LDR.N r0, ascii_L_0801E814
    LDR.N r1, ascii_L_0801E80C
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
    LDR.N r1, ascii_L_0801E810
    MOVS.N r0, #0
    BL sub_0801D8C0
ascii_L_0801E770
    B.N ascii_L_0801E2B8
ascii_L_0801E772
    SUB.W r0, r11, #6
    CMP.N r0, #8
    BHS.N ascii_L_0801E82C
    TBB [pc, r0]
ascii_L_0801E77E
    DCB (ascii_L_0801E786 - ascii_L_0801E77E) / 2
    DCB (ascii_L_0801E792 - ascii_L_0801E77E) / 2
    DCB (ascii_L_0801E79E - ascii_L_0801E77E) / 2
    DCB (ascii_L_0801E7AA - ascii_L_0801E77E) / 2
    DCB (ascii_L_0801E7B6 - ascii_L_0801E77E) / 2
    DCB (ascii_L_0801E7C2 - ascii_L_0801E77E) / 2
    DCB (ascii_L_0801E7F0 - ascii_L_0801E77E) / 2
    DCB (ascii_L_0801E820 - ascii_L_0801E77E) / 2
ascii_L_0801E786
    LDR.N r0, ascii_L_0801E818
    LDRB.N r1, [r0]
    AND.W r1, r1, #0xdf
    STRB.N r1, [r0]
    B.N ascii_L_0801E82E
ascii_L_0801E792
    LDR.N r0, ascii_L_0801E818
    LDRB.N r1, [r0]
    ORR.W r1, r1, #0x20
    STRB.N r1, [r0]
    B.N ascii_L_0801E82E
ascii_L_0801E79E
    LDR.N r0, ascii_L_0801E818
    LDRB.N r1, [r0]
    AND.W r1, r1, #0xf7
    STRB.N r1, [r0]
    B.N ascii_L_0801E82E
ascii_L_0801E7AA
    LDR.N r0, ascii_L_0801E818
    LDRB.N r1, [r0]
    ORR.W r1, r1, #8
    STRB.N r1, [r0]
    B.N ascii_L_0801E82E
ascii_L_0801E7B6
    LDR.N r0, ascii_L_0801E818
    LDRB.N r1, [r0]
    AND.W r1, r1, #0xef
    STRB.N r1, [r0]
    B.N ascii_L_0801E82E
ascii_L_0801E7C2
    LDR.N r0, ascii_L_0801E818
    LDRB.N r1, [r0]
    ORR.W r1, r1, #0x10
    STRB.N r1, [r0]
    LDRB.N r0, [r7]
    LSLS.N r0, r0, #0x1f
    BEQ.N ascii_L_0801E7E6
    LDRB.N r0, [r7]
    ORR.W r0, r0, #2
    STRB.N r0, [r7]
    LDR.N r0, ascii_L_0801E81C
    MOVS.N r2, #0xa
    MOV.N r1, r2
    LDRB.N r0, [r0]
    BL os_task_delay
ascii_L_0801E7E6
    LDRB.N r0, [r7]
    AND.W r0, r0, #0xfe
    STRB.N r0, [r7]
    B.N ascii_L_0801E82E
ascii_L_0801E7F0
    LDR.N r0, ascii_L_0801E818
    LDRB.N r1, [r0]
    AND.W r1, r1, #0xfe
    STRB.N r1, [r0]
    B.N ascii_L_0801E82E
ascii_L_0801E7FC
    DCD g_20000077
ascii_L_0801E800
    DCD g_200000FA
ascii_L_0801E804
    DCD g_200000F8
ascii_L_0801E808
    DCD g_20000182
ascii_L_0801E80C
    DCD hexbuf
ascii_L_0801E810
    DCD tx_len
ascii_L_0801E814
    DCD pack_count
ascii_L_0801E818
    DCD g_2000050D
ascii_L_0801E81C
    DCD os_task_dly_idx
ascii_L_0801E820
    LDR.N r0, ascii_L_0801EC14
    LDRB.N r1, [r0]
    ORR.W r1, r1, #1
    STRB.N r1, [r0]
    B.N ascii_L_0801E82E
ascii_L_0801E82C
    MOVS.N r5, #9
ascii_L_0801E82E
    LDR.N r7, ascii_L_0801EC14
    LDRB.N r0, [r7]
    AND.W r1, r0, #0x7f
    MOVS.N r0, #0x38
    BL sub_08022442
    LDR.N r1, ascii_L_0801EC18
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
    LDR.N r1, ascii_L_0801EC18
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
ascii_L_0801E878
    B.N ascii_L_0801E2B8
ascii_L_0801E87A
    CMP.W r11, #0
    BEQ.N ascii_L_0801E8A8
    CMP.W r11, #1
    BEQ.N ascii_L_0801E8E6
    MOVS.N r5, #9
ascii_L_0801E888
    LDR.N r1, ascii_L_0801EC18
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
    B.N ascii_L_0801E2B8
ascii_L_0801E8A8
    LDRB.N r1, [r0]
    TST.W r1, #0x70
    BNE.N ascii_L_0801E8B8
    LDRB.N r1, [r3]
    TST.W r1, #0x2c
    BEQ.N ascii_L_0801E8BC
ascii_L_0801E8B8
    MOVS.N r5, #9
    B.N ascii_L_0801E888
ascii_L_0801E8BC
    LDRB.N r1, [r7]
    LSLS.N r1, r1, #0x1e
    BMI.N ascii_L_0801E888
    LDR.N r1, ascii_L_0801EC14
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
    B.N ascii_L_0801E888
ascii_L_0801E8E6
    LDR.N r0, ascii_L_0801EC1C
    LDR.N r0, [r0]
    CMN.W r0, #0x7f800000
    BHS.N ascii_L_0801E90A
    LDR.N r0, ascii_L_0801EC14
    LDRB.N r1, [r0]
    LSLS.N r1, r1, #0x1d
    BMI.N ascii_L_0801E90A
    LDRB.N r1, [r0]
    ORR.W r1, r1, #2
    STRB.N r1, [r0]
    LDRB.N r0, [r7]
    AND.W r0, r0, #0xfc
    STRB.N r0, [r7]
    B.N ascii_L_0801E888
ascii_L_0801E90A
    MOVS.N r5, #9
    B.N ascii_L_0801E888
ascii_L_0801E90E
    CMP.W r11, #0
    BEQ.N ascii_L_0801E93C
    CMP.W r11, #1
    BEQ.N ascii_L_0801E980
    MOVS.N r5, #9
ascii_L_0801E91C
    LDR.N r1, ascii_L_0801EC18
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
ascii_L_0801E93A
    B.N ascii_L_0801E2B8
ascii_L_0801E93C
    LDRB.N r1, [r2]
    LSLS.N r1, r1, #0x19
    BMI.N ascii_L_0801E952
    LDRB.N r1, [r0]
    TST.W r1, #0x70
    BNE.N ascii_L_0801E952
    LDRB.N r1, [r3]
    TST.W r1, #0x2c
    BEQ.N ascii_L_0801E956
ascii_L_0801E952
    MOVS.N r5, #9
    B.N ascii_L_0801E91C
ascii_L_0801E956
    LDRB.N r1, [r7]
    LSLS.N r1, r1, #0x1d
    BMI.N ascii_L_0801E91C
    LDR.N r1, ascii_L_0801EC14
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
    B.N ascii_L_0801E91C
ascii_L_0801E980
    LDR.N r0, ascii_L_0801EC1C
    LDR.N r0, [r0]
    CMP.W r0, #0x800000
    BGE.N ascii_L_0801E9A4
    LDR.N r0, ascii_L_0801EC14
    LDRB.N r1, [r0]
    LSLS.N r1, r1, #0x1e
    BMI.N ascii_L_0801E9A4
    LDRB.N r1, [r0]
    ORR.W r1, r1, #4
    STRB.N r1, [r0]
    LDRB.N r0, [r7]
    AND.W r0, r0, #0xfb
    STRB.N r0, [r7]
    B.N ascii_L_0801E91C
ascii_L_0801E9A4
    MOVS.N r5, #9
    B.N ascii_L_0801E91C
ascii_L_0801E9A8
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    LDR.N r7, ascii_L_0801EC20
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
    LDR.N r7, ascii_L_0801EC24
    MOVS.N r0, #0
    STR.N r0, [r7]
    LDR.W r8, ascii_L_0801EC28
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
ascii_L_0801E9F8
    B.N ascii_L_0801E2B8
ascii_L_0801E9FA
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    SUB.W r1, r0, #0x4b
    CMP.N r1, #0x16
    BHS.N ascii_L_0801EA18
    UXTB.N r1, r0
    LDR.N r0, ascii_L_0801EC2C
    STRB.N r1, [r0]
    MOVS.N r0, #0x18
    BL sub_08022442
    B.N ascii_L_0801E2B8
ascii_L_0801EA18
    MOVS.N r5, #9
    B.N ascii_L_0801E2B8
ascii_L_0801EA1C
    B.N ascii_L_0801F1FE
ascii_L_0801EA1E
    LDR.N r7, ascii_L_0801EC30
    LDR.N r0, [r7]
    BL __aeabi_f2uiz
    UBFX.W r0, r0, #8, #8
    LDR.N r1, ascii_L_0801EC18
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
    LDR.N r1, ascii_L_0801EC18
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    B.N ascii_L_0801EA64
ascii_L_0801EA62
    B.N ascii_L_0801F24C
ascii_L_0801EA64
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    STRB.N r1, [r4, r0]
    LDR.N r7, ascii_L_0801EC34
    LDR.N r0, [r7]
    BL __aeabi_f2uiz
    UBFX.W r0, r0, #8, #8
    LDR.N r1, ascii_L_0801EC18
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
    LDR.N r1, ascii_L_0801EC18
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
    LDR.N r7, ascii_L_0801EC38
    LDR.N r1, ascii_L_0801EC18
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
    LDR.N r1, ascii_L_0801EC18
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
ascii_L_0801EAFA
    B.N ascii_L_0801E31E
ascii_L_0801EAFC
    B.N ascii_L_0801F2FC
ascii_L_0801EAFE
    B.N ascii_L_0801F2FE
ascii_L_0801EB00
    B.N ascii_L_0801F300
ascii_L_0801EB02
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
    LDR.N r1, ascii_L_0801EC30
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
    LDR.N r1, ascii_L_0801EC34
    STR.N r0, [r1]
    LDRB.N r1, [r4, #0x16]
    LDRB.N r0, [r4, #0x15]
    BL sub_08014FD6
    LDR.W r10, ascii_L_0801EC38
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
    LDR.N r2, ascii_L_0801EC3C
    LDRB.N r1, [r2]
    CMP.N r1, #0x42
    BEQ.N ascii_L_0801EBD8
    CMP.N r1, #0x44
    BEQ.N ascii_L_0801EC9E
    LDR.N r1, ascii_L_0801EC40
    MOVS.N r0, #0
    STRH.N r0, [r1]
    LDR.N r1, ascii_L_0801EC44
    MOVS.N r0, #2
    STRH.N r0, [r1]
    LDRH.W r0, [r10]
    MOVW.W r1, #0x1770
    CMP.N r0, r1
    BHI.N ascii_L_0801ECA0
    MVN.W r0, #0x3b
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x3c
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x40
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x41
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801EE14
ascii_L_0801EBD8
    LDR.N r1, ascii_L_0801EC40
    MOVS.N r0, #0
    STRH.N r0, [r1]
    LDRB.N r0, [r2, #1]
    CMP.N r0, #0x47
    BNE.N ascii_L_0801EBEE
    LDRH.W r1, [r10]
    CMP.W r1, #0x3e80
    BEQ.N ascii_L_0801EBF4
ascii_L_0801EBEE
    CMP.N r0, #0x59
    BEQ.N ascii_L_0801EC48
    B.N ascii_L_0801EC52
ascii_L_0801EBF4
    LDR.N r1, ascii_L_0801EC44
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
    B.N ascii_L_0801ECBA
ascii_L_0801EC14
    DCD g_2000050D
ascii_L_0801EC18
    DCD hexbuf
ascii_L_0801EC1C
    DCD g_20000148
ascii_L_0801EC20
    DCD g_200000A2
ascii_L_0801EC24
    DCD g_20000168
ascii_L_0801EC28
    DCD g_200000A9
ascii_L_0801EC2C
    DCD g_20000186
ascii_L_0801EC30
    DCD g_2000016C
ascii_L_0801EC34
    DCD g_20000164
ascii_L_0801EC38
    DCD g_20000182
ascii_L_0801EC3C
    DCD g_20000077
ascii_L_0801EC40
    DCD g_200000FA
ascii_L_0801EC44
    DCD g_200000F8
ascii_L_0801EC48
    LDRH.W r0, [r10]
    CMP.W r0, #0x3e80
    BEQ.N ascii_L_0801EC7E
ascii_L_0801EC52
    LDR.N r1, ascii_L_0801F050
    MOVS.N r0, #2
    STRH.N r0, [r1]
    LDRH.W r0, [r10]
    MOVW.W r1, #0x1770
    CMP.N r0, r1
    BHI.N ascii_L_0801ECA2
    MVN.W r0, #0x3b
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x3c
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x40
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x41
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801ECBA
ascii_L_0801EC7E
    LDR.N r1, ascii_L_0801F050
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
    B.N ascii_L_0801ECBA
ascii_L_0801EC9E
    B.N ascii_L_0801ECE0
ascii_L_0801ECA0
    B.N ascii_L_0801EDFC
ascii_L_0801ECA2
    MVN.W r0, #0x45
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x46
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x4a
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x4b
    STRH.N r0, [r7, #8]
ascii_L_0801ECBA
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
ascii_L_0801ECDE
    B.N ascii_L_0801E52E
ascii_L_0801ECE0
    LDR.N r0, ascii_L_0801F054
    MOVS.N r1, #1
    STRH.N r1, [r0]
    LDRB.N r0, [r2, #1]
    CMP.N r0, #0x47
    BNE.N ascii_L_0801ECF6
    LDRH.W r2, [r10]
    CMP.W r2, #0x3e80
    BEQ.N ascii_L_0801ECFC
ascii_L_0801ECF6
    CMP.N r0, #0x59
    BEQ.N ascii_L_0801ED1A
    B.N ascii_L_0801ED24
ascii_L_0801ECFC
    LDR.N r0, ascii_L_0801F050
    STRH.N r1, [r0]
    MVN.W r0, #0x59
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x55
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x5e
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x5a
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801EDD6
ascii_L_0801ED1A
    LDRH.W r1, [r10]
    CMP.W r1, #0x3e80
    BEQ.N ascii_L_0801ED36
ascii_L_0801ED24
    CMP.N r0, #0x57
    BEQ.N ascii_L_0801ED56
    CMP.N r0, #0x58
    BEQ.N ascii_L_0801ED56
    CMP.N r0, #0x5a
    BEQ.N ascii_L_0801ED56
    CMP.N r0, #0x46
    BEQ.N ascii_L_0801ED56
    B.N ascii_L_0801ED62
ascii_L_0801ED36
    LDR.N r0, ascii_L_0801F050
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
    B.N ascii_L_0801EDD6
ascii_L_0801ED56
    LDRH.W r0, [r10]
    SUB.W r1, r0, #0x4800
    SUBS.N r1, #0xa8
    BEQ.N ascii_L_0801ED8C
ascii_L_0801ED62
    LDR.N r1, ascii_L_0801F050
    MOVS.N r0, #2
    STRH.N r0, [r1]
    LDRH.W r0, [r10]
    CMP.W r0, #0x3e80
    BLO.N ascii_L_0801EDAC
    MVN.W r0, #0x59
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x55
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x5e
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x5a
    STRH.N r0, [r7, #8]
    B.N ascii_L_0801EDD6
ascii_L_0801ED8C
    LDR.N r1, ascii_L_0801F050
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
    B.N ascii_L_0801EDD6
ascii_L_0801EDAC
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
ascii_L_0801EDD6
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
ascii_L_0801EDFA
    B.N ascii_L_0801E64A
ascii_L_0801EDFC
    MVN.W r0, #0x45
    STRH.W r0, [r8, #0xa]
    MOVS.N r0, #0x46
    STRH.W r0, [r8, #8]
    MVN.W r0, #0x4a
    STRH.N r0, [r7, #0xa]
    MOVS.N r0, #0x4b
    STRH.N r0, [r7, #8]
ascii_L_0801EE14
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
    B.N ascii_L_0801E64A
ascii_L_0801EE3A
    LDR.N r1, ascii_L_0801F058
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
    LDR.N r1, ascii_L_0801F058
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
    LDR.N r1, ascii_L_0801F058
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
    LDR.N r1, ascii_L_0801F058
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
ascii_L_0801EEB2
    B.N ascii_L_0801E6D2
ascii_L_0801EEB4
    LDR.N r1, ascii_L_0801F058
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
    LDR.N r1, ascii_L_0801F058
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
    LDR.N r1, ascii_L_0801F058
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
    LDR.N r1, ascii_L_0801F058
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
    LDR.N r1, ascii_L_0801F058
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
    B.N ascii_L_0801E770
ascii_L_0801EF4C
    LDR.N r1, ascii_L_0801F058
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
    LDR.N r1, ascii_L_0801F058
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
    LDR.N r1, ascii_L_0801F058
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
    LDR.N r1, ascii_L_0801F058
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
ascii_L_0801EFC4
    B.N ascii_L_0801E878
ascii_L_0801EFC6
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
    BHS.N ascii_L_0801EFF6
    LDR.N r0, ascii_L_0801F05C
    STRH.N r1, [r0]
    MOVS.N r0, #0x1e
    BL sub_08022442
    B.N ascii_L_0801E878
ascii_L_0801EFF6
    MOVS.N r5, #9
    B.N ascii_L_0801E878
ascii_L_0801EFFA
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
    BHS.N ascii_L_0801F02A
    LDR.N r0, ascii_L_0801F060
    STRH.N r1, [r0]
    MOVS.N r0, #0x1c
    BL sub_08022442
    B.N ascii_L_0801E878
ascii_L_0801F02A
    MOVS.N r5, #9
    B.N ascii_L_0801E878
ascii_L_0801F02E
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
    LDR.N r0, ascii_L_0801F064
    B.N ascii_L_0801F068
ascii_L_0801F050
    DCD g_200000F8
ascii_L_0801F054
    DCD g_200000FA
ascii_L_0801F058
    DCD hexbuf
ascii_L_0801F05C
    DCD g_20000096
ascii_L_0801F060
    DCD g_20000094
ascii_L_0801F064
    DCD g_20000110
ascii_L_0801F068
    LDRH.N r0, [r0]
    CBZ.N r0, ascii_L_0801F0AA
    BL __aeabi_ui2f
    STR.N r0, [sp]
    MOV.N r0, r10
    BL __aeabi_ui2f
    LDR.N r1, [sp]
    BL __aeabi_fdiv
    LDR.W r10, ascii_L_0801F488
    LDR.W r1, [r10]
    BL __aeabi_fmul
    STR.W r0, [r10]
    ADD.W r1, r0, r8
    LDR.N r0, ascii_L_0801F48C
    CMP.N r1, r0
    BLO.N ascii_L_0801F09E
    STR.W r7, [r10]
    MOVS.N r5, #9
ascii_L_0801F09E
    MOVS.N r0, #0x2c
    LDR.W r1, [r10]
    BL sub_08022488
ascii_L_0801F0A8
    B.N ascii_L_0801E93A
ascii_L_0801F0AA
    MOVS.N r5, #9
    B.N ascii_L_0801E93A
ascii_L_0801F0AE
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
    CBZ.N r0, ascii_L_0801F122
    SUB.W r1, r0, #0x33
    MOVW.W r2, #0x1f0d
    CMP.N r1, r2
    BHS.N ascii_L_0801F130
    LDR.N r1, ascii_L_0801F490
    LDR.N r1, [r1]
    CMN.W r1, #0x7f800000
    BLO.N ascii_L_0801F130
    EOR.W r10, r1, #0x80000000
    BL __aeabi_ui2f
    LDR.N r1, ascii_L_0801F494
    BL __aeabi_fmul
    MOV.N r1, r10
    BL __aeabi_fdiv
    LDR.W r10, ascii_L_0801F498
    LDR.W r1, [r10]
    BL __aeabi_fmul
    STR.W r0, [r10]
    ADD.W r1, r0, r8
    LDR.N r0, ascii_L_0801F48C
    CMP.N r1, r0
    BLO.N ascii_L_0801F116
    STR.W r7, [r10]
    MOVS.N r5, #9
ascii_L_0801F116
    MOVS.N r0, #0x24
    LDR.W r1, [r10]
    BL sub_08022488
ascii_L_0801F120
    B.N ascii_L_0801E93A
ascii_L_0801F122
    LDR.N r0, ascii_L_0801F498
    MOV.N r1, r7
    STR.N r7, [r0]
    MOVS.N r0, #0x24
    BL sub_08022488
    B.N ascii_L_0801E93A
ascii_L_0801F130
    MOVS.N r5, #9
    B.N ascii_L_0801E9F8
ascii_L_0801F134
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
    CBZ.N r0, ascii_L_0801F1A6
    SUB.W r1, r0, #0x33
    MOVW.W r2, #0x1f0d
    CMP.N r1, r2
    BHS.N ascii_L_0801F1B4
    LDR.N r1, ascii_L_0801F490
    LDR.W r10, [r1]
    CMP.W r10, #0x800000
    BLT.N ascii_L_0801F1B4
    BL __aeabi_ui2f
    LDR.N r1, ascii_L_0801F494
    BL __aeabi_fmul
    MOV.N r1, r10
    BL __aeabi_fdiv
    LDR.W r10, ascii_L_0801F49C
    LDR.W r1, [r10]
    BL __aeabi_fmul
    STR.W r0, [r10]
    ADD.W r1, r0, r8
    LDR.N r0, ascii_L_0801F48C
    CMP.N r1, r0
    BLO.N ascii_L_0801F19A
    STR.W r7, [r10]
    MOVS.N r5, #9
ascii_L_0801F19A
    MOVS.N r0, #0x28
    LDR.W r1, [r10]
    BL sub_08022488
ascii_L_0801F1A4
    B.N ascii_L_0801E9F8
ascii_L_0801F1A6
    LDR.N r0, ascii_L_0801F49C
    MOV.N r1, r7
    STR.N r7, [r0]
    MOVS.N r0, #0x28
    BL sub_08022488
    B.N ascii_L_0801E9F8
ascii_L_0801F1B4
    MOVS.N r5, #9
    B.N ascii_L_0801E9F8
ascii_L_0801F1B8
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
    BHS.N ascii_L_0801F20C
    LDR.N r1, ascii_L_0801F4A0
    LDRH.N r1, [r1]
    LSLS.N r3, r1, #0x10
    BPL.N ascii_L_0801F200
    UBFX.W r1, r1, #0, #0xf
    RSB.W r0, r0, #0x190
    ADD.N r0, r1
    SUBS.N r0, r0, #1
    SDIV.W r1, r0, r2
    MLS.W r0, r2, r1, r0
    UXTH.N r0, r0
    STR.N r0, [sp, #4]
    B.N ascii_L_0801F20C
ascii_L_0801F1FE
    B.N ascii_L_0801F68C
ascii_L_0801F200
    CMP.N r0, r1
    BHS.N ascii_L_0801F20C
    SUBS.N r0, r1, r0
    SUBS.N r0, r0, #1
    UXTH.N r0, r0
    STR.N r0, [sp, #4]
ascii_L_0801F20C
    LDR.N r0, [sp, #4]
    CMP.N r0, r7
    BEQ.N ascii_L_0801F120
    MOVS.N r7, #0
    LDR.N r0, [sp, #4]
    ADD.W r8, r0, r0, lsl #6
ascii_L_0801F21A
    ADD.W r0, r8, r7
    BL sub_080223BC
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F4A4
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
    BLO.N ascii_L_0801F21A
ascii_L_0801F24A
    B.N ascii_L_0801EAFA
ascii_L_0801F24C
    B.N ascii_L_0801F41A
ascii_L_0801F24E
    LDR.N r0, ascii_L_0801F4A0
    MOVS.N r1, #0
    STRH.N r1, [r0]
    LDR.N r7, ascii_L_0801F4A8
    STRH.N r1, [r7]
    MOVS.N r0, #0x14
    BL sub_08022442
    LDRH.N r1, [r7]
    MOVS.N r0, #0x16
    BL sub_08022442
    BL sub_080172B2
    B.N ascii_L_0801EAFA
ascii_L_0801F26C
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
    B.N ascii_L_0801EAFA
ascii_L_0801F2D0
    BL sub_0801535E
    LDRB.W r0, [r10]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F4A4
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
    B.N ascii_L_0801F302
ascii_L_0801F2FC
    B.N ascii_L_0801F75A
ascii_L_0801F2FE
    B.N ascii_L_0801F522
ascii_L_0801F300
    B.N ascii_L_0801F578
ascii_L_0801F302
    LDRB.W r0, [r10, #1]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F4A4
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
    LDR.N r1, ascii_L_0801F4A4
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
    LDR.N r1, ascii_L_0801F4A4
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
    LDR.N r1, ascii_L_0801F4A4
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
    LDR.N r1, ascii_L_0801F4A4
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
ascii_L_0801F3C0
    B.N ascii_L_0801ECDE
ascii_L_0801F3C2
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    BL sub_080151EA
    LDR.N r7, ascii_L_0801F4AC
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
    B.N ascii_L_0801ECDE
ascii_L_0801F41A
    LDR.N r7, ascii_L_0801F4AC
    LDRB.N r0, [r7]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F4A4
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
    LDR.N r1, ascii_L_0801F4A4
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
    LDR.N r1, ascii_L_0801F4A4
    BL sub_08015034
    LDRH.N r1, [r6, #0xa]
    LDRB.W r3, [r9]
    ADDS.N r0, r1, #1
    UXTH.N r0, r0
    STRB.N r3, [r4, r1]
    LDRB.W r1, [r9, #1]
    ADDS.N r2, r0, #1
    STRH.N r2, [r6, #0xa]
    B.N ascii_L_0801F4B0
ascii_L_0801F488
    DCD g_20000154
ascii_L_0801F48C
    DCD 0x00733334
ascii_L_0801F490
    DCD g_20000148
ascii_L_0801F494
    DCD 0x41200000
ascii_L_0801F498
    DCD g_2000014C
ascii_L_0801F49C
    DCD g_20000150
ascii_L_0801F4A0
    DCD log_index
ascii_L_0801F4A4
    DCD hexbuf
ascii_L_0801F4A8
    DCD g_2000018A
ascii_L_0801F4AC
    DCD date_start
ascii_L_0801F4B0
    STRB.N r1, [r4, r0]
    LDRB.N r0, [r7, #3]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F8B4
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
    LDR.N r1, ascii_L_0801F8B4
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
    LDR.N r1, ascii_L_0801F8B4
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
ascii_L_0801F520
    B.N ascii_L_0801EDFA
ascii_L_0801F522
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
    B.N ascii_L_0801EDFA
ascii_L_0801F578
    LDRB.N r0, [r7]
    BL sub_080151D8
    UXTB.N r0, r0
    LDR.N r1, ascii_L_0801F8B4
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
    LDR.N r1, ascii_L_0801F8B4
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
    LDR.N r1, ascii_L_0801F8B4
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
    LDR.N r1, ascii_L_0801F8B4
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
    LDR.N r1, ascii_L_0801F8B4
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
    LDR.N r1, ascii_L_0801F8B4
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
ascii_L_0801F650
    B.N ascii_L_0801EEB2
ascii_L_0801F652
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    SUBS.N r1, r0, #1
    CMP.N r1, #0x77
    BHS.N ascii_L_0801F666
    LDR.N r1, ascii_L_0801F8B8
    STRB.N r0, [r1]
    B.N ascii_L_0801EEB2
ascii_L_0801F666
    MOVS.N r5, #9
    B.N ascii_L_0801EEB2
ascii_L_0801F66A
    LDR.N r0, ascii_L_0801F8B8
    LDR.N r1, ascii_L_0801F8B4
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
    B.N ascii_L_0801EEB2
ascii_L_0801F68C
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_0801F714
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
    LDR.N r7, ascii_L_0801F8BC
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.N r0, [r7]
    B.N ascii_L_0801F716
ascii_L_0801F714
    B.N ascii_L_0801F74C
ascii_L_0801F716
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
    LDR.N r0, ascii_L_0801F8C0
    STRB.N r1, [r0]
    MOVS.N r0, #0x88
    BL sub_08022442
ascii_L_0801F740
    LDRB.W r1, [r10]
    MOVS.N r0, #0x30
    BL sub_08022442
ascii_L_0801F74A
    B.N ascii_L_0801EFC4
ascii_L_0801F74C
    LDRB.W r0, [r10]
    ORR.W r0, r0, #1
    STRB.W r0, [r10]
    B.N ascii_L_0801F740
ascii_L_0801F75A
    LDRB.W r1, [r10]
    LSLS.N r1, r1, #0x1f
    BEQ.N ascii_L_0801F770
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_0801F77C
ascii_L_0801F770
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_0801F77C
    LDRH.W r0, [r8]
    LDR.N r1, ascii_L_0801F8B4
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
    LDR.N r1, ascii_L_0801F8B4
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
    LDR.N r1, ascii_L_0801F8B4
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
    LDR.N r1, ascii_L_0801F8B4
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
    LDR.N r7, ascii_L_0801F8BC
    LDR.N r1, ascii_L_0801F8B4
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
    LDR.N r1, ascii_L_0801F8B4
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
    LDR.N r0, ascii_L_0801F8C0
    LDR.N r1, ascii_L_0801F8B4
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
ascii_L_0801F85C
    B.N ascii_L_0801F0A8
ascii_L_0801F85E
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_0801F8C4
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
    B.N ascii_L_0801F8C6
ascii_L_0801F8B4
    DCD hexbuf
ascii_L_0801F8B8
    DCD log_interval
ascii_L_0801F8BC
    DCD recov
ascii_L_0801F8C0
    DCD delays
ascii_L_0801F8C4
    B.N ascii_L_0801F92E
ascii_L_0801F8C6
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
    LDR.N r7, ascii_L_0801FCD8
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
    LDR.N r0, ascii_L_0801FCDC
    STRB.N r1, [r0, #1]
    MOVS.N r0, #0x90
    BL sub_08022442
ascii_L_0801F922
    LDRB.W r1, [r10]
    MOVS.N r0, #0x30
    BL sub_08022442
ascii_L_0801F92C
    B.N ascii_L_0801F1A4
ascii_L_0801F92E
    LDRB.W r0, [r10]
    ORR.W r0, r0, #2
    STRB.W r0, [r10]
    B.N ascii_L_0801F922
ascii_L_0801F93C
    LDRB.W r1, [r10]
    LSLS.N r1, r1, #0x1e
    BPL.N ascii_L_0801F952
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_0801F95E
ascii_L_0801F952
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_0801F95E
    LDRH.W r0, [r8, #2]
    LDR.N r1, ascii_L_0801FCE0
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
    LDR.N r1, ascii_L_0801FCE0
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
    LDR.N r1, ascii_L_0801FCE0
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
    LDR.N r1, ascii_L_0801FCE0
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
    LDR.N r7, ascii_L_0801FCD8
    LDR.N r1, ascii_L_0801FCE0
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
    LDR.N r1, ascii_L_0801FCE0
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
    LDR.N r0, ascii_L_0801FCDC
    LDR.N r1, ascii_L_0801FCE0
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
ascii_L_0801FA3E
    B.N ascii_L_0801F24A
ascii_L_0801FA40
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_0801FAC8
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
    LDR.N r7, ascii_L_0801FCD8
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    STRH.N r0, [r7, #4]
    B.N ascii_L_0801FACA
ascii_L_0801FAC8
    B.N ascii_L_0801FB00
ascii_L_0801FACA
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
    LDR.N r0, ascii_L_0801FCDC
    STRB.N r1, [r0, #2]
    MOVS.N r0, #0x98
    BL sub_08022442
ascii_L_0801FAF4
    LDRB.W r1, [r10]
    MOVS.N r0, #0x30
    BL sub_08022442
    B.N ascii_L_0801F3C0
ascii_L_0801FB00
    LDRB.W r0, [r10]
    ORR.W r0, r0, #4
    STRB.W r0, [r10]
    B.N ascii_L_0801FAF4
ascii_L_0801FB0E
    LDRB.W r1, [r10]
    LSLS.N r1, r1, #0x1d
    BPL.N ascii_L_0801FB24
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_0801FB30
ascii_L_0801FB24
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_0801FB30
    LDRH.W r0, [r8, #4]
    LDR.N r1, ascii_L_0801FCE0
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
    LDR.N r1, ascii_L_0801FCE0
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
    LDR.N r1, ascii_L_0801FCE0
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
    LDR.N r1, ascii_L_0801FCE0
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
    LDR.N r7, ascii_L_0801FCD8
    LDR.N r1, ascii_L_0801FCE0
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
    LDR.N r1, ascii_L_0801FCE0
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
    LDR.N r0, ascii_L_0801FCDC
    LDR.N r1, ascii_L_0801FCE0
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
ascii_L_0801FC14
    B.N ascii_L_0801F520
ascii_L_0801FC16
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_0801FC9C
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
    LDR.N r7, ascii_L_0801FCD8
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    AND.W r0, r0, r1, lsl #8
    B.N ascii_L_0801FC9E
ascii_L_0801FC9C
    B.N ascii_L_0801FCE4
ascii_L_0801FC9E
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
    LDR.N r0, ascii_L_0801FCDC
    STRB.N r1, [r0, #3]
    MOVS.N r0, #0xa0
    BL sub_08022442
ascii_L_0801FCCA
    LDRB.W r1, [r10]
    MOVS.N r0, #0x30
    BL sub_08022442
    B.N ascii_L_0801F520
    DCB 0 ; native pool/table alignment
    DCB 0 ; native pool/table alignment
ascii_L_0801FCD8
    DCD recov
ascii_L_0801FCDC
    DCD delays
ascii_L_0801FCE0
    DCD hexbuf
ascii_L_0801FCE4
    LDRB.W r0, [r10]
    ORR.W r0, r0, #8
    STRB.W r0, [r10]
    B.N ascii_L_0801FCCA
ascii_L_0801FCF2
    LDRB.W r1, [r10]
    LSLS.N r1, r1, #0x1c
    BPL.N ascii_L_0801FD08
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_0801FD14
ascii_L_0801FD08
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_0801FD14
    LDRH.W r0, [r8, #6]
    LDR.N r1, ascii_L_08020114
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
    LDR.N r1, ascii_L_08020114
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
    LDR.N r1, ascii_L_08020114
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
    LDR.N r1, ascii_L_08020114
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
    LDR.N r7, ascii_L_08020118
    LDR.N r1, ascii_L_08020114
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
    LDR.N r1, ascii_L_08020114
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
    LDR.N r0, ascii_L_0802011C
    LDR.N r1, ascii_L_08020114
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
ascii_L_0801FDF4
    B.N ascii_L_0801F650
ascii_L_0801FDF6
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_0801FE7C
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
    BLS.N ascii_L_0801FE34
    MOVS.N r1, #0x78
ascii_L_0801FE34
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
    BLS.N ascii_L_0801FE62
    MOVS.N r1, #0x78
ascii_L_0801FE62
    STRH.N r1, [r7, #8]
    MOVS.N r0, #0xa4
    BL sub_08022442
    LDRB.N r1, [r4, #0x18]
    LDRB.N r0, [r4, #0x17]
    BL sub_08014FD6
    LDR.N r1, ascii_L_0802011C
    STRB.N r0, [r1, #4]
    MOV.N r1, r0
    MOVS.N r0, #0xa6
    B.N ascii_L_0801FE7E
ascii_L_0801FE7C
    B.N ascii_L_0801FE8E
ascii_L_0801FE7E
    BL sub_08022442
ascii_L_0801FE82
    LDRB.W r1, [r10]
    MOVS.N r0, #0x30
    BL sub_08022442
    B.N ascii_L_0801F74A
ascii_L_0801FE8E
    LDRB.W r0, [r10]
    ORR.W r0, r0, #0x10
    STRB.W r0, [r10]
    B.N ascii_L_0801FE82
ascii_L_0801FE9C
    LDRB.W r1, [r10]
    LSLS.N r1, r1, #0x1b
    BPL.N ascii_L_0801FEB2
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_0801FEBE
ascii_L_0801FEB2
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_0801FEBE
    LDRH.W r0, [r8, #8]
    LDR.N r1, ascii_L_08020114
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
    LDR.N r1, ascii_L_08020114
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
    LDR.N r1, ascii_L_08020114
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
    LDR.N r1, ascii_L_08020114
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
    LDR.N r0, ascii_L_0802011C
    LDR.N r1, ascii_L_08020114
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
ascii_L_0801FF60
    B.N ascii_L_0801F85C
ascii_L_0801FF62
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_0801FFE6
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
    BLS.N ascii_L_0801FFA0
    MOVS.N r0, #0x64
ascii_L_0801FFA0
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
    BLS.N ascii_L_0801FFD4
    MOVS.N r1, #0x78
ascii_L_0801FFD4
    RSBS.N r0, r1, #0
    STRH.N r0, [r7, #0xa]
    LDRH.N r1, [r7, #0xa]
    MOVS.N r0, #0xaa
    BL sub_08022442
    LDRB.N r1, [r4, #0x18]
    LDRB.N r0, [r4, #0x17]
    B.N ascii_L_0801FFE8
ascii_L_0801FFE6
    B.N ascii_L_08020004
ascii_L_0801FFE8
    BL sub_08014FD6
    LDR.N r1, ascii_L_0802011C
    STRB.N r0, [r1, #5]
    MOV.N r1, r0
    MOVS.N r0, #0xac
    BL sub_08022442
ascii_L_0801FFF8
    LDRB.W r1, [r10]
    MOVS.N r0, #0x30
    BL sub_08022442
ascii_L_08020002
    B.N ascii_L_0801F85C
ascii_L_08020004
    LDRB.W r0, [r10]
    ORR.W r0, r0, #0x20
    STRB.W r0, [r10]
    B.N ascii_L_0801FFF8
ascii_L_08020012
    LDRB.W r1, [r10]
    LSLS.N r1, r1, #0x1a
    BPL.N ascii_L_08020028
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_08020034
ascii_L_08020028
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_08020034
    LDRH.W r0, [r8, #0xa]
    LDR.N r1, ascii_L_08020114
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
    LDR.N r1, ascii_L_08020114
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
    LDR.N r1, ascii_L_08020114
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
    LDR.N r1, ascii_L_08020114
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
    LDR.N r0, ascii_L_0802011C
    LDR.N r1, ascii_L_08020114
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
ascii_L_080200D8
    B.N ascii_L_0801F92C
ascii_L_080200DA
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_0802015E
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
    LDR.N r0, ascii_L_08020120
    STRH.N r1, [r0, #0xc]
    LDRB.N r1, [r4, #3]
    LDRB.N r0, [r4, #2]
    BL sub_08014FD6
    MOV.N r1, r0
    B.N ascii_L_08020124
ascii_L_08020114
    DCD hexbuf
ascii_L_08020118
    DCD recov
ascii_L_0802011C
    DCD delays
ascii_L_08020120
    DCD limits
ascii_L_08020124
    LDR.N r0, ascii_L_08020520
    LDRH.N r0, [r0, #0xc]
    ADD.N r0, r1
    UXTH.N r1, r0
    LDR.N r0, ascii_L_08020520
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
    B.N ascii_L_08020160
ascii_L_0802015E
    B.N ascii_L_08020230
ascii_L_08020160
    BL sub_08022442
    LDRB.N r1, [r4, #9]
    LDRB.N r0, [r4, #8]
    BL sub_08014FD6
    LDR.W r8, ascii_L_08020524
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
    LDR.N r0, ascii_L_08020520
    STRH.N r1, [r0, #0xe]
    LDRB.N r1, [r4, #0xf]
    LDRB.N r0, [r4, #0xe]
    BL sub_08014FD6
    LDR.N r1, ascii_L_08020520
    LDRH.N r1, [r1, #0xe]
    ADD.N r0, r1
    UXTH.N r1, r0
    LDR.N r0, ascii_L_08020520
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
ascii_L_08020224
    LDRB.W r1, [r10]
    MOVS.N r0, #0x30
    BL sub_08022442
ascii_L_0802022E
    B.N ascii_L_0801FA3E
ascii_L_08020230
    LDRB.W r0, [r10]
    ORR.W r0, r0, #0x40
    STRB.W r0, [r10]
    B.N ascii_L_08020224
ascii_L_0802023E
    LDRB.W r1, [r10]
    LSLS.N r1, r1, #0x19
    BPL.N ascii_L_08020254
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_08020260
ascii_L_08020254
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_08020260
    LDRH.W r0, [r8, #0xc]
    LDR.N r1, ascii_L_08020528
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
    LDR.N r1, ascii_L_08020528
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
    LDR.N r1, ascii_L_08020528
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
    LDR.N r1, ascii_L_08020528
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
    LDR.W r10, ascii_L_08020524
    LDR.N r1, ascii_L_08020528
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
    LDR.N r1, ascii_L_08020528
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
    LDR.N r1, ascii_L_08020528
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
    LDR.N r1, ascii_L_08020528
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
    LDR.N r1, ascii_L_08020528
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
    LDR.N r1, ascii_L_08020528
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
    LDR.N r1, ascii_L_08020528
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
    LDR.N r1, ascii_L_08020528
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
ascii_L_080203E8
    B.N ascii_L_0801FC14
ascii_L_080203EA
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_08020470
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
    LDR.N r0, ascii_L_08020520
    STRH.N r1, [r0, #0x10]
    LDRB.N r1, [r4, #3]
    LDRB.N r0, [r4, #2]
    BL sub_08014FD6
    LDR.N r1, ascii_L_08020520
    LDRH.N r1, [r1, #0x10]
    ADD.N r0, r1
    UXTH.N r1, r0
    LDR.N r0, ascii_L_08020520
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
    LDR.W r8, ascii_L_08020524
    MOV.N r1, r0
    MOVW.W r0, #0xffff
    B.N ascii_L_08020472
ascii_L_08020470
    B.N ascii_L_0802053C
ascii_L_08020472
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
    LDR.N r0, ascii_L_08020520
    STRH.N r1, [r0, #0x12]
    LDRB.N r1, [r4, #0xf]
    LDRB.N r0, [r4, #0xe]
    BL sub_08014FD6
    LDR.N r1, ascii_L_08020520
    LDRH.N r1, [r1, #0x12]
    ADD.N r0, r1
    UXTH.N r1, r0
    LDR.N r0, ascii_L_08020520
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
    B.N ascii_L_0802052C
    DCB 0 ; native pool/table alignment
    DCB 0 ; native pool/table alignment
ascii_L_08020520
    DCD limits
ascii_L_08020524
    DCD recov
ascii_L_08020528
    DCD hexbuf
ascii_L_0802052C
    BL sub_08022442
ascii_L_08020530
    LDRB.W r1, [r10]
    MOVS.N r0, #0x30
    BL sub_08022442
ascii_L_0802053A
    B.N ascii_L_0801FDF4
ascii_L_0802053C
    LDRB.W r0, [r10]
    ORR.W r0, r0, #0x80
    STRB.W r0, [r10]
    B.N ascii_L_08020530
ascii_L_0802054A
    LDRB.W r1, [r10]
    LSLS.N r1, r1, #0x18
    BPL.N ascii_L_08020560
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_0802056E
ascii_L_08020560
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    MOV.N r0, r1
    ADDS.N r1, r1, #1
    STRH.N r1, [r6, #0xa]
    STRB.N r3, [r4, r0]
ascii_L_0802056E
    LDRH.W r0, [r8, #0x10]
    LDR.N r1, ascii_L_08020970
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
    LDR.N r1, ascii_L_08020970
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
    LDR.N r1, ascii_L_08020970
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
    LDR.N r1, ascii_L_08020970
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
    LDR.W r10, ascii_L_08020974
    LDR.N r1, ascii_L_08020970
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
    LDR.N r1, ascii_L_08020970
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
    LDR.N r1, ascii_L_08020970
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
    LDR.N r1, ascii_L_08020970
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
    LDR.N r1, ascii_L_08020970
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
    LDR.N r1, ascii_L_08020970
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
    LDR.N r1, ascii_L_08020970
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
    LDR.N r1, ascii_L_08020970
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
ascii_L_080206F6
    B.N ascii_L_0801FF60
ascii_L_080206F8
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_0802077E
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
    LDR.N r7, ascii_L_08020974
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
    B.N ascii_L_08020780
ascii_L_0802077E
    B.N ascii_L_08020784
ascii_L_08020780
    BL sub_08022442
ascii_L_08020784
    LDR.N r0, ascii_L_08020978
    LDRB.N r1, [r0]
    MOVS.N r0, #0x3a
    BL sub_08022442
    B.N ascii_L_08020002
ascii_L_08020790
    LDR.N r1, ascii_L_08020978
    LDRB.N r1, [r1]
    LSLS.N r1, r1, #0x1f
    BEQ.N ascii_L_080207A6
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_080207B2
ascii_L_080207A6
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_080207B2
    LDRH.W r0, [r8, #0x14]
    LDR.N r1, ascii_L_08020970
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
    LDR.N r1, ascii_L_08020970
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
    LDR.N r1, ascii_L_08020970
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
    LDR.N r1, ascii_L_08020970
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
    LDR.N r7, ascii_L_08020974
    LDR.N r1, ascii_L_08020970
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
    LDR.N r1, ascii_L_08020970
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
    B.N ascii_L_080200D8
ascii_L_08020874
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CBZ.N r0, ascii_L_080208FC
    ADDS.N r4, #0xf
    LDRB.N r1, [r4, #1]
    LDRB.N r0, [r4]
    BL sub_08014FD6
    MOV.N r1, r0
    LDR.N r0, ascii_L_0802097C
    MOVW.W r10, #0xffff
    AND.W r1, r10, r1, lsl #8
    STRH.N r1, [r0, #0x18]
    LDRB.N r1, [r4, #3]
    LDRB.N r0, [r4, #2]
    BL sub_08014FD6
    MOV.N r1, r0
    LDR.N r0, ascii_L_0802097C
    LDRH.N r0, [r0, #0x18]
    ADD.N r0, r1
    UXTH.N r1, r0
    LDR.N r0, ascii_L_0802097C
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
    LDR.W r8, ascii_L_08020974
    MOV.N r1, r0
    AND.W r0, r10, r1, lsl #8
    STRH.W r0, [r8, #0x14]
    LDRB.N r1, [r4, #0xb]
    LDRB.N r0, [r4, #0xa]
    BL sub_08014FD6
    LDRH.W r1, [r8, #0x14]
    B.N ascii_L_080208FE
ascii_L_080208FC
    B.N ascii_L_080209A0
ascii_L_080208FE
    ADD.N r0, r1
    UXTH.N r1, r0
    STRH.W r1, [r8, #0x14]
    MOVS.N r0, #0xf6
    BL sub_08022442
    LDRB.N r1, [r4, #0xd]
    LDRB.N r0, [r4, #0xc]
    BL sub_08014FD6
    MOV.N r1, r0
    LDR.N r0, ascii_L_0802097C
    AND.W r1, r10, r1, lsl #8
    STRH.N r1, [r0, #0x16]
    LDRB.N r1, [r4, #0xf]
    LDRB.N r0, [r4, #0xe]
    BL sub_08014FD6
    LDR.N r1, ascii_L_0802097C
    LDRH.N r1, [r1, #0x16]
    ADD.N r0, r1
    UXTH.N r1, r0
    LDR.N r0, ascii_L_0802097C
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
    B.N ascii_L_08020980
    DCB 0 ; native pool/table alignment
    DCB 0 ; native pool/table alignment
ascii_L_08020970
    DCD hexbuf
ascii_L_08020974
    DCD recov
ascii_L_08020978
    DCD g_20000276
ascii_L_0802097C
    DCD limits
ascii_L_08020980
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
ascii_L_080209A0
    LDR.N r0, ascii_L_08020D94
    LDRB.N r1, [r0]
    MOVS.N r0, #0x3a
    BL sub_08022442
    B.N ascii_L_0802022E
ascii_L_080209AC
    LDR.N r1, ascii_L_08020D94
    LDRB.N r1, [r1]
    LSLS.N r1, r1, #0x1e
    BPL.N ascii_L_080209C2
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r2, [r4, r1]
    B.N ascii_L_080209CE
ascii_L_080209C2
    ADDS.N r1, r0, #1
    UXTH.N r1, r1
    STRB.N r2, [r4, r0]
    ADDS.N r0, r1, #1
    STRH.N r0, [r6, #0xa]
    STRB.N r3, [r4, r1]
ascii_L_080209CE
    LDRH.W r0, [r8, #0x18]
    LDR.N r1, ascii_L_08020D98
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
    LDR.N r1, ascii_L_08020D98
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
    LDR.N r1, ascii_L_08020D98
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
    LDR.N r1, ascii_L_08020D98
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
    LDR.W r10, ascii_L_08020D9C
    LDR.N r1, ascii_L_08020D98
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
    LDR.N r1, ascii_L_08020D98
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
    LDR.N r1, ascii_L_08020D98
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
    LDR.N r1, ascii_L_08020D98
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
    LDR.N r1, ascii_L_08020D98
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
    LDR.N r1, ascii_L_08020D98
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
    LDR.N r1, ascii_L_08020D98
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
    LDR.N r1, ascii_L_08020D98
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
    B.N ascii_L_080203E8
ascii_L_08020B58
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
    BLS.N ascii_L_08020B7A
    MOVS.N r1, #0x78
ascii_L_08020B7A
    STRH.N r1, [r7, #0xc]
    MOVS.N r0, #0xd0
    BL sub_08022442
    LDRB.N r1, [r4, #0x12]
    LDRB.N r0, [r4, #0x11]
    BL sub_08014FD6
    LDR.N r1, ascii_L_08020DA0
    STRB.N r0, [r1, #6]
    MOV.N r1, r0
    MOVS.N r0, #0xd2
    BL sub_08022442
    B.N ascii_L_080203E8
ascii_L_08020B98
    LDRH.N r0, [r7, #0xc]
    LDR.N r1, ascii_L_08020D98
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
    LDR.N r1, ascii_L_08020D98
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
    LDR.N r0, ascii_L_08020DA0
    LDR.N r1, ascii_L_08020D98
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
    B.N ascii_L_0802053A
ascii_L_08020BF8
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    CMP.N r0, #0x13
    BLS.N ascii_L_08020C0C
    LDR.N r0, ascii_L_08020DA0
    MOVS.N r1, #0xa
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020C72
ascii_L_08020C0C
    CMP.N r0, #0x11
    BLS.N ascii_L_08020C18
    LDR.N r0, ascii_L_08020DA0
    MOVS.N r1, #9
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020C72
ascii_L_08020C18
    CMP.N r0, #0xf
    BLS.N ascii_L_08020C24
    LDR.N r0, ascii_L_08020DA0
    MOVS.N r1, #8
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020C72
ascii_L_08020C24
    CMP.N r0, #0xd
    BLS.N ascii_L_08020C30
    LDR.N r0, ascii_L_08020DA0
    MOVS.N r1, #7
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020C72
ascii_L_08020C30
    CMP.N r0, #0xb
    BLS.N ascii_L_08020C3C
    LDR.N r0, ascii_L_08020DA0
    MOVS.N r1, #6
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020C72
ascii_L_08020C3C
    CMP.N r0, #9
    BLS.N ascii_L_08020C48
    LDR.N r0, ascii_L_08020DA0
    MOVS.N r1, #5
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020C72
ascii_L_08020C48
    CMP.N r0, #7
    BLS.N ascii_L_08020C54
    LDR.N r0, ascii_L_08020DA0
    MOVS.N r1, #4
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020C72
ascii_L_08020C54
    CMP.N r0, #5
    BLS.N ascii_L_08020C60
    LDR.N r0, ascii_L_08020DA0
    MOVS.N r1, #3
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020C72
ascii_L_08020C60
    CMP.N r0, #3
    BLS.N ascii_L_08020C6C
    LDR.N r0, ascii_L_08020DA0
    MOVS.N r1, #2
    STRB.N r1, [r0, #7]
    B.N ascii_L_08020C72
ascii_L_08020C6C
    LDR.N r0, ascii_L_08020DA0
    MOVS.N r1, #1
    STRB.N r1, [r0, #7]
ascii_L_08020C72
    LDR.N r0, ascii_L_08020DA0
    LDRB.N r1, [r0, #7]
    MOVS.N r0, #0xd4
    BL sub_08022442
ascii_L_08020C7C
    B.N ascii_L_0802053A
ascii_L_08020C7E
    LDR.N r0, ascii_L_08020DA0
    LDRB.N r0, [r0, #7]
    CMP.N r0, #9
    BLS.N ascii_L_08020C8A
    MOVS.N r0, #0x14
    B.N ascii_L_08020CCC
ascii_L_08020C8A
    CMP.N r0, #8
    BLS.N ascii_L_08020C92
    MOVS.N r0, #0x12
    B.N ascii_L_08020CCC
ascii_L_08020C92
    CMP.N r0, #7
    BLS.N ascii_L_08020C9A
    MOVS.N r0, #0x10
    B.N ascii_L_08020CCC
ascii_L_08020C9A
    CMP.N r0, #6
    BLS.N ascii_L_08020CA2
    MOVS.N r0, #0xe
    B.N ascii_L_08020CCC
ascii_L_08020CA2
    CMP.N r0, #5
    BLS.N ascii_L_08020CAA
    MOVS.N r0, #0xc
    B.N ascii_L_08020CCC
ascii_L_08020CAA
    CMP.N r0, #4
    BLS.N ascii_L_08020CB2
    MOVS.N r0, #0xa
    B.N ascii_L_08020CCC
ascii_L_08020CB2
    CMP.N r0, #3
    BLS.N ascii_L_08020CBA
    MOVS.N r0, #8
    B.N ascii_L_08020CCC
ascii_L_08020CBA
    CMP.N r0, #2
    BLS.N ascii_L_08020CC2
    MOVS.N r0, #6
    B.N ascii_L_08020CCC
ascii_L_08020CC2
    CMP.N r0, #1
    BLS.N ascii_L_08020CCA
    MOVS.N r0, #4
    B.N ascii_L_08020CCC
ascii_L_08020CCA
    MOVS.N r0, #2
ascii_L_08020CCC
    UXTB.N r0, r0
    LDR.N r1, ascii_L_08020D98
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
    B.N ascii_L_0802053A
ascii_L_08020CEC
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
    LDR.N r1, ascii_L_08020D94
    STRB.N r0, [r1]
    BL sub_08016F1E
    MOVS.N r0, #0
    BL sub_080179EA
    B.N ascii_L_0802053A
ascii_L_08020D1A
    LDRB.N r1, [r4, #0xe]
    LDRB.N r0, [r4, #0xd]
    BL sub_08014FD6
    MOV.N r1, r0
    SUB.W r0, r1, #8
    CMP.N r0, #8
    BHI.N ascii_L_08020D3A
    MOVS.N r0, #0x32
    BL sub_08022442
    CMP.N r0, #0
    BNE.N ascii_L_08020C7C
    MOVS.N r5, #9
    B.N ascii_L_080206F6
ascii_L_08020D3A
    MOVS.N r5, #9
    B.N ascii_L_080206F6
ascii_L_08020D3E
    CMP.W r11, #1
    BEQ.N ascii_L_08020D54
    CMP.W r11, #2
    BEQ.N ascii_L_08020DB0
    CMP.W r11, #0x50
    BEQ.N ascii_L_08020DC2
    MOVS.N r5, #9
    B.N ascii_L_080206F6
ascii_L_08020D54
    LDR.N r0, ascii_L_08020DA4
    LDRB.N r1, [r0]
    ORR.W r1, r1, #0x90
    STRB.N r1, [r0]
    LDRH.N r1, [r7]
    LDR.N r0, ascii_L_08020D9C
    SUBS.N r1, #0x32
    STRH.N r1, [r0]
    LDRH.N r1, [r7, #2]
    ADDS.N r1, #0x32
    STRH.N r1, [r0, #2]
    LDR.N r1, ascii_L_08020DA8
    MOVS.N r0, #0
    STRH.N r0, [r1]
    LDR.N r2, ascii_L_08020DAC
    MOVS.N r7, #1
    LDRB.N r3, [r2]
    B.N ascii_L_08020D8C
ascii_L_08020D7A
    LDRH.W r12, [r1]
    LSL.W r2, r7, r0
    ORR.W r2, r2, r12
    STRH.N r2, [r1]
    ADDS.N r0, r0, #1
    UXTH.N r0, r0
ascii_L_08020D8C
    CMP.N r0, r3
    BLO.N ascii_L_08020D7A
    B.N ascii_L_080206F6
    DCB 0 ; native pool/table alignment
    DCB 0 ; native pool/table alignment
ascii_L_08020D94
    DCD g_20000276
ascii_L_08020D98
    DCD hexbuf
ascii_L_08020D9C
    DCD recov
ascii_L_08020DA0
    DCD delays
ascii_L_08020DA4
    DCD g_2000050D
ascii_L_08020DA8
    DCD g_200000CE
ascii_L_08020DAC
    DCD g_20000162
ascii_L_08020DB0
    MOVS.N r1, #0
    MOV.W r0, #0x200
    BL sub_08022442
    MOVS.N r0, #1
    BL sub_080179EA
    B.N ascii_L_080206F6
ascii_L_08020DC2
    MOVW.W r1, #0xaaaa
    MOVW.W r0, #0x7ff5
    BL sub_08022442
    LDR.N r0, ascii_L_080211C8
    MOVS.N r2, #0xa
    MOV.N r1, r2
    LDRB.N r0, [r0]
    BL os_task_delay
    LDR.N r1, ascii_L_080211CC
    MOVS.N r0, #0x80
    STR.N r0, [r1]
    B.N ascii_L_080206F6
ascii_L_08020DE2
    CMP.W r11, #0xff
    BNE.N ascii_L_08020E20
    LDR.N r0, ascii_L_080211D0
    LDRB.N r0, [r0]
    CMP.N r0, #1
    BLS.N ascii_L_08020E20
    LDRH.N r1, [r6, #0xa]
    SUBS.N r1, #0x11
    MULS.N r1, r0, r1
    ADDS.N r1, r1, #4
    UXTH.N r1, r1
    LDR.N r0, ascii_L_080211D4
    BL sub_080150EE
    MOVS.N r0, #0
    STRH.N r0, [r6, #0x10]
    LDR.N r0, [sp, #0x10]
    STRB.N r0, [r6, #1]
    MOVS.N r0, #1
    LDRH.N r1, [r6, #0xa]
    B.N ascii_L_08020E1A
ascii_L_08020E0E
    LDRB.N r2, [r4, r0]
    LDRH.N r3, [r6, #0x10]
    ADD.N r2, r3
    STRH.N r2, [r6, #0x10]
    ADDS.N r0, r0, #1
    UXTH.N r0, r0
ascii_L_08020E1A
    CMP.N r0, r1
    BLO.N ascii_L_08020E0E
    B.N ascii_L_08020E94
ascii_L_08020E20
    LDRH.N r0, [r6, #0xa]
    CMP.N r0, #0xd
    BLO.N ascii_L_08020E30
    SUBS.N r0, #0xd
    UXTH.N r1, r0
    LDR.N r0, ascii_L_080211D4
    BL sub_080150EE
ascii_L_08020E30
    MOVS.N r0, #1
    LDRH.N r2, [r6, #0xa]
    B.N ascii_L_08020E44
ascii_L_08020E36
    LDRB.N r3, [r4, r0]
    LDR.N r1, [sp, #8]
    ADD.N r1, r3
    UXTH.N r1, r1
    STR.N r1, [sp, #8]
    ADDS.N r0, r0, #1
    UXTH.N r0, r0
ascii_L_08020E44
    CMP.N r0, r2
    BLO.N ascii_L_08020E36
    LDR.N r0, [sp, #8]
    MVNS.N r0, r0
    ADDS.N r0, r0, #1
    UXTH.N r5, r0
    LSRS.N r0, r5, #8
    LDR.N r1, ascii_L_080211D8
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
    LDR.N r1, ascii_L_080211D8
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
ascii_L_08020E94
    MOVS.N r0, #0
    STRH.N r0, [r6, #0xc]
    ADD.N sp, #0x14
    MOVS.N r2, #1
    POP.W {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    MOVW.W r1, #0x727
    LDR.N r0, [pc, #0x334] ; native shared UART4 pool at 0x080211DC
    B.W USART_ITConfig
    ENDP

; Following literal pool shared with the C suffix.
; at: ascii_following_pool 0x080211C8
; at: os_task_dly_idx 0x2000061E
; at: ascii_watchdog_base 0x40002C00
; at: pack_count 0x2000009E
; at: pace_buf 0x20003602
; at: hexbuf 0x2000010D
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
