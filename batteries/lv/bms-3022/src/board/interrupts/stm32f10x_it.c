/* stm32f10x_it.c: the Cortex-M3 fault and peripheral interrupt handlers.
   One source file from 0x080142F0 to 0x080148F8: armcc places an intermediate literal pool
   after RTCAlarm_IRQHandler, and EXTI4_IRQHandler reaches adc_ring_clear from the base of
   this file's .data, as it does for a global defined in the same file. */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I follow the interrupt entry points here; their handlers hand work to the peripheral and application tasks.
   Entry examples: NMI_Handler, SVC_Handler, board_delay_poll_loops.
   name: I use this descriptive subsystem filename; I have no original filename evidence. */
/* at: NMI_Handler 0x080142F0, HardFault_Handler 0x080142F2, MemManage_Handler 0x080142F4, BusFault_Handler 0x080142F6, UsageFault_Handler 0x080142F8, SVC_Handler 0x080142FA */
/* at: DebugMon_Handler 0x080142FC, TIM3_IRQHandler 0x080142FE, TIM4_IRQHandler 0x08014314, USART2_IRQHandler 0x08014344, USART1_IRQHandler 0x0801437C, USART3_IRQHandler 0x080143EE, UART4_IRQHandler 0x08014446, ADC1_2_IRQHandler 0x080144AE, EXTI1_IRQHandler 0x08014570, SysTick_Handler 0x08014592, board_delay_poll_loops 0x080145CA, board_check_pa0_high 0x080145E4, board_check_disabled_output_voltage 0x08014612, board_poll_startup_voltage 0x0801463E, EXTI0_IRQHandler 0x080146C0, RTCAlarm_IRQHandler 0x080146D8 */
/* at: EXTI4_IRQHandler 0x080147AC, TIM2_IRQHandler 0x08014892, OTG_FS_IRQHandler 0x08014896 */
/* at: g_2000005A 0x2000005A, g_2000005B 0x2000005B, g_2000005C 0x2000005C, g_2000005E 0x2000005E, g_20000077 0x20000077, g_20000090 0x20000090, g_20000092 0x20000092, g_200000C2 0x200000C2, g_200000C4 0x200000C4, g_20000144 0x20000148, g_20000148 0x2000014C, g_20000271 0x20000279, g_20000273 0x2000027B, g_2000056C 0x20000578, g_20000F9C 0x20000FB4, g_200011B2 0x200011CA, g_20001342 0x2000135A, g_20002D1C 0x20002D34 */
/* at: g_2000004D 0x2000004D, g_2000004E 0x2000004E, g_20000060 0x20000060, g_20000061 0x20000061, g_2000008D 0x2000008D, g_2000008E 0x2000008E, g_200000CC 0x200000CC, g_200000D0 0x200000D0, g_200000D4 0x200000D4, g_200000D5 0x200000D5, g_200000DE 0x200000DE, g_200000F1 0x200000F3, g_20000110 0x20000114, g_20000120 0x20000124, g_20000124 0x20000128, g_20000128 0x2000012C, g_20000158 0x2000015C, g_2000015A 0x2000015E, g_20000160 0x20000164, g_20000162 0x20000166, g_20000273 0x2000027B, g_2000110C 0x20001124, g_200011A8 0x200011C0, g_200011B2 0x200011CA, g_20001342 0x2000135A, g_200032CC 0x200032E4, g_20003BBA 0x20003BD2 */
#include "../../include/ghidra_types.h"
#include "../../include/bms_measurements.h"
#include "../../include/stdperiph_types.h"
extern uchar g_2000005A;
extern uchar g_2000005B;
extern uchar g_2000005C;
extern ushort g_2000005E;
extern uchar g_20000077;
extern ushort g_20000090;
extern uchar g_20000092;
extern ushort g_200000C2;
extern ushort g_200000C4;
extern uint g_20000144;
extern uint g_20000148;
extern ushort g_2000056C;
extern sbyte g_20000F9C[];
extern ushort g_200011B2[];
extern ushort g_20001342[];
extern uchar g_20002D1C;
extern uchar g_2000004D;
extern uchar g_2000004E;
extern uchar g_20000060;
extern uchar g_20000061;
extern uchar g_2000008D;
extern uchar g_2000008E;
extern ushort g_200000CC;
extern ushort g_200000D0;
extern uchar g_200000D4;
extern uchar g_200000D5;
extern uchar g_200000DE;
extern uchar g_200000F1;
extern ushort g_20000110;
extern ushort g_20000120;
extern ushort g_20000128;
extern ushort g_20000158;
extern ushort g_2000015A;
extern ushort g_20000160;
extern uchar g_20000162;
extern int g_2000110C[];
extern ushort g_200011A8[];
extern ushort g_200011B2[];
extern ushort g_20001342[];
extern int g_200032CC[];
extern ushort g_20003BBA[];
void ADC_SoftwareStartConvCmd(int, int);
void TIM_ClearFlag(int, ushort);
void runtime_timers_tick(void);
void capacity_integrate_current(void);
void USART_ClearITPendingBit(ushort *, uint);
undefined4 USART_GetITStatus(ushort *, uint);
void sub_08014B2E(void);
void sub_08014BCC(int);
void USART_ClearFlag(ushort *, ushort);
bool USART_GetFlagStatus(ushort *, ushort);
ushort USART_ReceiveData(int);
void sub_08014AFA(void);
void coordination_receive_byte(uint);
void coordination_transmit_interrupt(void);
void sub_08014B92(void);
void sub_0801D6E4(int);
void sub_08020D52(void);
void ADC_ClearITPendingBit(int, int);
void EXTI_ClearITPendingBit();
undefined4 EXTI_GetITStatus(uint);
void OSTimeTick(void);
void board_set_pb2(int);
void board_set_pa8(int);
bool GPIO_ReadInputDataBit(int, uint);
void board_set_pb5(int);
void PWR_ClearFlag(int);
bool PWR_GetFlagStatus(uint);
void RTC_ClearITPendingBit(ushort);
bool RTC_GetFlagStatus(ushort);
void RTC_WaitForLastTask(void);
void UART4_IRQHandler();
uint board_check_pa0_high(uint poll_delay);
void TIM3_IRQHandler();
void board_delay_poll_loops(uint loop_count);
uint board_check_disabled_output_voltage();
int board_poll_startup_voltage();
void NMI_Handler(void) {
}

void HardFault_Handler(void) {
    while (1) {
    }
}

void MemManage_Handler(void) {
    while (1) {
    }
}

void BusFault_Handler(void) {
    while (1) {
    }
}

void UsageFault_Handler(void) {
    while (1) {
    }
}

void SVC_Handler(void) {
}

void DebugMon_Handler();
void USART3_IRQHandler();
void EXTI1_IRQHandler();
void SysTick_Handler();
void TIM4_IRQHandler();
void EXTI0_IRQHandler();
void USART1_IRQHandler();
void USART2_IRQHandler();
void ADC1_2_IRQHandler();
void RTCAlarm_IRQHandler();

/* at: adc_avg_count 0x20000000, adc_ring_index 0x20000002, adc_ring_clear 0x20000004, adc_avg_sum 0x20000008 */
uchar adc_avg_count = 0;
ushort adc_ring_index = 0;
ushort adc_ring_clear = 0;
int adc_avg_sum = 0;
void DebugMon_Handler() {

    return;
}

void TIM3_IRQHandler() {

    TIM_ClearFlag(0x40000400, 1);
    ADC_SoftwareStartConvCmd(0x40012400, 1);
    return;
}

#define g_200000D4_v g_200000D4
#define g_200000D5_v g_200000D5
#define g_200000F1_v g_200000F1

void TIM4_IRQHandler() {

    TIM_ClearFlag(0x40000800, 1);
    runtime_timers_tick();
    if (((g_200000D4_v == '\x01') && (g_200000D5_v == '\x01')) && (g_200000F1_v == '\x01')) {
        capacity_integrate_current();
        return;
    }
    return;
}
#undef g_200000D4_v
#undef g_200000D5_v
#undef g_200000F1_v

#define DAT_40004400 (*(volatile uchar *)0x40004400)

void USART2_IRQHandler() {

    if ((USART_GetITStatus((ushort *)&DAT_40004400, 0x626)) != 0) {
        USART_ClearITPendingBit((ushort *)&DAT_40004400, 0x626);
        sub_08014B2E();
    }
    if ((USART_GetITStatus((ushort *)&DAT_40004400, 0x525)) != 0) {
        sub_08014BCC(1);
        return;
    }
    return;
}
#undef DAT_40004400

#define DAT_40013800 (*(volatile ushort *)0x40013800)

void USART1_IRQHandler() {
    uchar status_byte;

    status_byte = DAT_40013800;
    if ((USART_GetFlagStatus(&DAT_40013800, 8))) {
        USART_ReceiveData(0x40013800);
        USART_ClearFlag(&DAT_40013800, 8);
        return;
    }
    if ((status_byte & 0xb) != 0) {
        USART_ReceiveData(0x40013800);
        return;
    }
    if ((USART_GetITStatus(&DAT_40013800, 0x626)) != 0) {
        USART_ClearITPendingBit(&DAT_40013800, 0x626);
        sub_08014AFA();
        return;
    }
    if ((USART_GetITStatus(&DAT_40013800, 0x525)) != 0) {
        sub_08014BCC(0);
        return;
    }
    USART_ReceiveData(0x40013800);
    return;
}
#undef DAT_40013800

#define DAT_40004800 (*(volatile ushort *)0x40004800)

void USART3_IRQHandler() {

    if ((DAT_40004800 & 0xb) != 0) {
        USART_ReceiveData(0x40004800);
        return;
    }
    if ((USART_GetITStatus(&DAT_40004800, 0x727)) != 0) {
        USART_ClearITPendingBit(&DAT_40004800, 0x727);
        coordination_transmit_interrupt();
        return;
    }
    if ((USART_GetITStatus(&DAT_40004800, 0x525)) != 0) {
        coordination_receive_byte((uint)(byte)(USART_ReceiveData(0x40004800)));
        return;
    }
    USART_ReceiveData(0x40004800);
    return;
}
#undef DAT_40004800

#define DAT_40004c00 (*(volatile ushort *)0x40004C00)
#define g_200000DE_v g_200000DE

void UART4_IRQHandler() {

    if ((DAT_40004c00 & 0xb) != 0) {
        USART_ReceiveData(0x40004c00);
        return;
    }
    if ((USART_GetITStatus(&DAT_40004c00, 0x727)) != 0) {
        USART_ClearITPendingBit(&DAT_40004c00, 0x727);
        if (g_200000DE_v != '\x01') {
            sub_08014B92();
            return;
        }
        sub_08020D52();
        return;
    }
    if ((USART_GetITStatus(&DAT_40004c00, 0x525)) != 0) {
        sub_0801D6E4((uint)(byte)(USART_ReceiveData(0x40004c00)));
        return;
    }
    USART_ReceiveData(0x40004c00);
    return;
}
#undef DAT_40004c00
#undef g_200000DE_v

#define DAT_20000000 (adc_avg_count)
#define DAT_20000002 (adc_ring_index)
#define DAT_20000008 (adc_avg_sum)
#define DAT_2000008d g_2000008D
#define DAT_2000008e g_2000008E
#define DAT_200000cc g_200000CC
#define DAT_200000d0 g_200000D0
#define DAT_20000120 g_20000120
#define DAT_20000128 g_20000128
#define DAT_20000158 g_20000158
#define DAT_2000015a g_2000015A
#define DAT_20000160 g_20000160
#define DAT_20000162 g_20000162
#define DAT_2000110c (g_2000110C[0])
#define DAT_200011a8 (g_200011A8[0])
#define DAT_200011aa (g_200011A8[1])
#define DAT_200011ac (g_200011A8[2])
#define DAT_200011ae (g_200011A8[3])
#define DAT_200011b0 (g_200011A8[4])
#define DAT_200011b2 (g_200011B2[0])
#define DAT_20001342 (g_20001342[0])
#define DAT_20003bba (g_20003BBA[0])
#define DAT_20003bbc (g_20003BBA[1])
#define DAT_20003bbe (g_20003BBA[2])
#define DAT_20003bc0 (g_20003BBA[3])
#define DAT_20003bc2 (g_20003BBA[4])
#define DAT_20003bc4 (g_20003BBA[5])
#define DAT_20003bc6 (g_20003BBA[6])
#define DAT_20003bc8 (g_20003BBA[7])
#define DAT_20003bca (g_20003BBA[8])
#define DAT_20003bcc (g_20003BBA[9])
#define DAT_20003bce (g_20003BBA[10])

void ADC1_2_IRQHandler() {
    ADC_ClearITPendingBit(0x40012400, 0x220);
    (&DAT_200011b2)[DAT_20000002] = DAT_200000cc - DAT_20003bc2 - DAT_2000015a;
    (&DAT_20001342)[DAT_20000002] = 0x800 - DAT_20003bc6 - DAT_20000158;
    DAT_20000002++;
    if (DAT_20000002 >= 200) {
        DAT_20000002 = 0;
    }
    DAT_20000120 = DAT_20003bc8;
    DAT_20000128 = DAT_20003bca - 0x800;
    DAT_200011ae = DAT_20003bba;
    DAT_200011a8 = DAT_20003bbc;
    DAT_200011aa = DAT_20003bbe;
    DAT_200011ac = DAT_20003bc0;
    DAT_200011b0 = DAT_20003bcc;
    if (DAT_2000008d == DAT_2000008e && DAT_2000008d < 16 && DAT_2000008d < DAT_20000162) {
        if (DAT_200000d0 == 0) {
            if (DAT_20000000 < 50) {
                DAT_20000008 += DAT_20003bc4;
            }
            DAT_20000000++;
            if (DAT_20000000 >= 50) {
                DAT_20000000 = 50;
                (&DAT_2000110c)[DAT_2000008d] = DAT_20000008;
            }
        }
    } else if (DAT_2000008e != DAT_2000008d) {
        DAT_20000008 = 0;
        DAT_20000000 = 0;
    }
    DAT_20000160 = DAT_20003bce;
    DAT_2000008e = DAT_2000008d;
}
#undef DAT_20000000
#undef DAT_20000002
#undef DAT_20000008
#undef DAT_2000008d
#undef DAT_2000008e
#undef DAT_200000cc
#undef DAT_200000d0
#undef DAT_20000120
#undef DAT_20000128
#undef DAT_20000158
#undef DAT_2000015a
#undef DAT_20000160
#undef DAT_20000162
#undef DAT_2000110c
#undef DAT_200011a8
#undef DAT_200011aa
#undef DAT_200011ac
#undef DAT_200011ae
#undef DAT_200011b0
#undef DAT_200011b2
#undef DAT_20001342
#undef DAT_20003bba
#undef DAT_20003bbc
#undef DAT_20003bbe
#undef DAT_20003bc0
#undef DAT_20003bc2
#undef DAT_20003bc4
#undef DAT_20003bc6
#undef DAT_20003bc8
#undef DAT_20003bca
#undef DAT_20003bcc
#undef DAT_20003bce

#define DAT_2000335c (g_200032CC[36])

void EXTI1_IRQHandler() {

    if ((EXTI_GetITStatus(2)) != 0) {
        (**(code **)(DAT_2000335c + 0x14))();
        EXTI_ClearITPendingBit(2);
        return;
    }
    return;
}
#undef DAT_2000335c

#define g_2000004D_v g_2000004D
#define g_2000004E_v g_2000004E

void SysTick_Handler() {

    OSTimeTick();
    if (g_2000004D_v != 0) {
        if (g_2000004D_v < 2) {
            g_2000004D_v = g_2000004D_v + 1;
        } else {
            board_set_pb2(0);
        }
    }
    if (g_2000004E_v != 0) {
        if (g_2000004E_v < 2) {
            g_2000004E_v++;
        } else
            board_set_pa8(0);
    }
    return;
}
#undef g_2000004D_v
#undef g_2000004E_v

/* Runs nested busy-wait loops without a calibrated time claim. */
void board_delay_poll_loops(uint loop_count) {
    uint spin_count;
    uint delay_count;

    for (delay_count = 0; delay_count < loop_count; delay_count = delay_count + 1 & 0xffff) {
        spin_count = 0;
        do {
            spin_count = spin_count + 1;
        } while (spin_count < 8000);
    }
    return;
}

/* Returns success only after fifty PA0-high checks with polling delays. */
uint board_check_pa0_high(uint poll_delay) {
    uchar sample_count;

    sample_count = 0;
    do {
        board_set_pb5(0);
        if (!(GPIO_ReadInputDataBit(0x40010800, 1))) {
            return 0;
        }
        sample_count = sample_count + 1;
        board_delay_poll_loops(poll_delay);
    } while (sample_count < 0x32);
    return 1;
}

#define g_20000110_v g_20000110
extern uchar g_20000124[];
#define g_20000124_v (*(uint *)(g_20000124 + 0))
extern uchar g_20000273[];
#define g_20000273_v (*(volatile uchar *)(g_20000273 + 0))

/* Checks disabled output bits and the auxiliary/pack-voltage relation. */
uint board_check_disabled_output_voltage() {

    if ((-1 < (int)((uint)g_20000273_v << 0x1e)) && (-1 < (int)((uint)g_20000273_v << 0x1d))) {
        if ((int)(g_20000124_v - (uint)g_20000110_v) < -3000) {
            return 0;
        }
        return 1;
    }
    return 0;
}
#undef g_20000110_v
#undef g_20000124_v
#undef g_20000273_v

#define DAT_20000110 g_20000110
#define DAT_20000120 g_20000120
extern uchar g_20000124[];
#define DAT_20000124 (*(int *)(g_20000124 + 0))
#define DAT_20000128 g_20000128
#define DAT_200011a8 (g_200011A8[0])
#define DAT_200011aa (g_200011A8[1])
#define DAT_200011ac (g_200011A8[2])
#define DAT_200011ae (g_200011A8[3])
#define DAT_200011b0 (g_200011A8[4])
#define DAT_20003bba (g_20003BBA[0])
#define DAT_20003bbc (g_20003BBA[1])
#define DAT_20003bbe (g_20003BBA[2])
#define DAT_20003bc0 (g_20003BBA[3])
#define DAT_20003bc8 (g_20003BBA[7])
#define DAT_20003bca (g_20003BBA[8])
#define DAT_20003bcc (g_20003BBA[9])

/* Triggers ADC scans and polls voltage conditions with bounded retries.
   match: The same limit serves the retry count and voltage-margin comparison. */
int board_poll_startup_voltage() {
    int settle_count;
    ushort attempt_count;
    int poll_limit;
    int adc_base;
    attempt_count = 0;
    adc_base = 0x40012400;
    poll_limit = 0x3e8;
    while (1) {
        ADC_SoftwareStartConvCmd(adc_base, 1);
        settle_count = 0;
        do {
            settle_count++;
        } while ((uint)settle_count < 0xa);
        DAT_20000120 = DAT_20003bc8;
        DAT_20000128 = DAT_20003bca - 0x800;
        DAT_200011ae = DAT_20003bba;
        DAT_200011a8 = DAT_20003bbc;
        DAT_200011aa = DAT_20003bbe;
        DAT_200011ac = DAT_20003bc0;
        DAT_200011b0 = DAT_20003bcc;
        measurement_update_voltage_summary();
        if (DAT_20000124 - DAT_20000110 < poll_limit) {
            attempt_count++;
            if ((uint)attempt_count % 0xa == 0 && board_check_disabled_output_voltage() != 0) {
                return 0;
            }
            if ((uint)attempt_count >= poll_limit) {
                return 1;
            }
        } else {
            return 0;
        }
        board_delay_poll_loops(1);
    }
}
#undef DAT_20000110
#undef DAT_20000120
#undef DAT_20000124
#undef DAT_20000128
#undef DAT_200011a8
#undef DAT_200011aa
#undef DAT_200011ac
#undef DAT_200011ae
#undef DAT_200011b0
#undef DAT_20003bba
#undef DAT_20003bbc
#undef DAT_20003bbe
#undef DAT_20003bc0
#undef DAT_20003bc8
#undef DAT_20003bca
#undef DAT_20003bcc

#define DAT_20000060 g_20000060
#define DAT_20000061 g_20000061

void EXTI0_IRQHandler() {

    EXTI_ClearITPendingBit(1);
    if ((uint)DAT_20000060 >= 6) {
        DAT_20000061 = 1;
    }
}
#undef DAT_20000060
#undef DAT_20000061

void RTCAlarm_IRQHandler() {

    if ((RTC_GetFlagStatus(2))) {
        EXTI_ClearITPendingBit(0x20000);
        RTC_ClearITPendingBit(2);
        RTC_WaitForLastTask();
        EXTI_ClearITPendingBit(0x80);
        EXTI_ClearITPendingBit(2);
        if ((PWR_GetFlagStatus(1))) {
            PWR_ClearFlag(1);
            return;
        }
    }
    return;
}

void EXTI_ClearITPendingBit(undefined4);
void GPIO_ResetBits(int, undefined4);
void GPIO_SetBits(int, undefined4);
void runtime_timer_start(int, undefined4);
void sub_08015A76(void);
uint USBH_OTG_ISR_Handler(byte *);
void TIM2_IRQHandler(void);
void EXTI4_IRQHandler(void);
void OTG_FS_IRQHandler(void);

#define DAT_20000004 (adc_ring_clear)
#define g_2000005A_v g_2000005A
#define g_2000005B_v g_2000005B
#define g_2000005C_v g_2000005C
#define g_2000005E_v g_2000005E
#define g_20000077_v g_20000077
#define g_20000090_v g_20000090
#define g_20000092_v g_20000092
#define g_200000C2_v g_200000C2
#define g_200000C4_v g_200000C4
#define g_20000144_v g_20000144
#define g_20000148_v g_20000148
extern uchar g_20000271[];
#define g_20000271_v (*(volatile uchar *)(g_20000271 + 0))
extern uchar g_20000273[];
#define g_20000273_v (*(uchar *)(g_20000273 + 0))
#define g_2000056C_v g_2000056C
#define DAT_20000ffc (g_20000F9C[96])
#define g_200011B2_v (g_200011B2[0])
#define g_20001342_v (g_20001342[0])

void EXTI4_IRQHandler(void) {

    EXTI_ClearITPendingBit(0x10);
    g_20000273_v = g_20000273_v & 0xfb;
    GPIO_ResetBits(0x40011400, 4);
    g_2000005C_v = 0;
    GPIO_SetBits(0x40011000, 0x100);
    g_2000005A_v = 1;
    g_2000005E_v = 0;
    g_20000273_v = g_20000273_v & 0xfc;
    GPIO_ResetBits(0x40010c00, 8);
    g_2000005B_v = 0;
    DAT_20000004 = 0;
    do {
        (&g_200011B2_v)[DAT_20000004] = 0;
        (&g_20001342_v)[DAT_20000004] = 0;
        DAT_20000004++;
    } while (DAT_20000004 < 200);
    g_20000148_v = 0;
    g_20000144_v = 0;
    g_2000056C_v = 1;
    if (g_200000C2_v > 100) {
        if ((((g_20000092_v != '\0') && (g_20000077_v == 'B')) &&
             (-1 < (int)((uint)g_20000271_v << 0x19))) &&
            (g_20000090_v < 6)) {
            g_20000090_v = g_20000090_v + 1;
        }
        g_200000C4_v = 0;
        g_200000C2_v = 0;
        GPIO_ResetBits(0x40011000, 0x100);
        g_2000005A_v = 0;
        g_20000271_v |= 0x40;
        g_20000273_v &= 0xfb;
        g_20000271_v &= 0xfa;
        if (DAT_20000ffc == '\0') {
            runtime_timer_start(0xc, 72000);
            return;
        }
    }
    return;
}
#undef DAT_20000004
#undef g_2000005A_v
#undef g_2000005B_v
#undef g_2000005C_v
#undef g_2000005E_v
#undef g_20000077_v
#undef g_20000090_v
#undef g_20000092_v
#undef g_200000C2_v
#undef g_200000C4_v
#undef g_20000144_v
#undef g_20000148_v
#undef g_20000271_v
#undef g_20000273_v
#undef g_2000056C_v
#undef DAT_20000ffc
#undef g_200011B2_v
#undef g_20001342_v

void TIM2_IRQHandler(void) {

    sub_08015A76();
}

#define g_20002D1C_v g_20002D1C

void OTG_FS_IRQHandler(void) {

    USBH_OTG_ISR_Handler(&g_20002D1C_v);
    return;
}
#undef g_20002D1C_v
