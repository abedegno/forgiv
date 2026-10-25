// clang-format off
/* at: NMI_Handler 0x0801432C, HardFault_Handler 0x0801432E, MemManage_Handler 0x08014352, BusFault_Handler 0x08014356, UsageFault_Handler 0x0801435A, SVC_Handler 0x0801435E, DebugMon_Handler 0x08014360, TIM3_IRQHandler 0x08014362, TIM4_IRQHandler 0x080143BC, aux_uart_rx_byte 0x080143C8, USART1_IRQHandler 0x080145BE, USART2_IRQHandler 0x0801460A, USART3_IRQHandler 0x08014658, UART4_IRQHandler 0x080146A6, ADC1_2_IRQHandler 0x080146F4, EXTI1_IRQHandler 0x08014B4C, SysTick_Handler 0x08014B68, TIM2_IRQHandler 0x08014C8A, OTG_FS_IRQHandler 0x08014C92 */
/* at: adc_start_toggle 0x2000000E, buzzer_div 0x20000010, drm_bits_halves 0x20000011, grid_country 0x200001AB, output_enabled 0x200001D8, rs485_modbus_de_hold 0x20000242, rs485_meter_de_hold 0x20000243, drm_bits 0x20000298, aux_rx_state 0x200002C0, aux_rx_bit 0x200002C1, usb_task_tick_count 0x2000030F, modbus_usb_wait 0x20000394, uart_rx_buf 0x2000175C, cfg_settings 0x20001ABA, aux_wave_buf 0x2000206C, USB_OTG_Core 0x20003480, USB_Host 0x20003A30, adc_sample_ch0 0x20000000, adc_sample_ch1 0x20000002, adc_sample_ch5 0x20000004, adc_offset_samples 0x20000012, adc_sumsq_ch0 0x20000014, adc_sumsq_ch1 0x20000018, adc_sumsq_ch5 0x2000001C, pd2_low_count 0x20000020, pd2_high_count 0x20000022, aux_sync_count 0x20000024, adc_sample_ch7 0x20000026, adc_sumsq_ch7 0x20000028, adc_sumprod_ch0_ch7 0x2000002C, adc_cycle_samples 0x20000030, adc_cycle_end 0x20000032, adc_isr_phase 0x20000033, adc_cycle_sumsq_ch0 0x2000014C, adc_cycle_sumsq_ch1 0x20000154, adc_cycle_sumsq_ch5 0x20000158, adc_sum_ch0 0x20000178, adc_sum_ch1 0x2000017C, adc_sum_ch5 0x20000180, adc_offset_state 0x20000184, adc_offset_ch0 0x20000186, adc_offset_ch1 0x20000188, adc_offset_ch5 0x2000018A, adc_wave_idx 0x2000029A, aux_tx_byte 0x2000029C, aux_tx_repeat_count 0x2000029D, aux_tx_restart_count 0x2000029E, adc_cycle_n 0x200002A0, adc_cycle_sumsq_ch7 0x200002A4, adc_cycle_sumprod 0x200002A8, usb_reset_state 0x200002BC, aux_rx_idx 0x200002C2, aux_pages_seen 0x200002D4, adc_ntc_charger 0x200002FA, adc_ntc_heatsink 0x200002FE, adc_dma_buf 0x20000706, adc_wave_buf 0x200021EC */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* stm32f10x_it.c: the interrupt handlers. ST's template order first (NMI, HardFault, MemManage,
   BusFault, UsageFault, SVC, DebugMon; PendSV is in rtos/os_cpu_a.s), then the application's:
   the timers, the four UARTs, the ADC, EXTI1, SysTick and USB OTG.

   Entry points: the handlers, through the vector table in startup/startup_stm32f10x_cl.s;
   aux_uart_rx_byte from USART1_IRQHandler.
   Data: the ADC accumulators (adc_sample_*, adc_sumsq_*, adc_cycle_*, adc_offset_*), the
   waveform capture adc_wave_buf, the aux link receive state (aux_rx_*, aux_wave_buf, drm_bits).
   Neighbours: drivers/uart.c (the queue code the UART handlers call), control/measure.c and
   control/monitor.c (which use the ADC sums and the aux pages), board.c (pins, buzzer, DE pins),
   rtos (OSTimeTick, os_flags_post), the USB host library.

   The measuring scheme, as the code shows it: TIM3 starts an 8-channel ADC scan at about 9.6
   kHz; each end of scan, ADC1_2_IRQHandler takes channels 0, 5 and 7 (offset removed and
   scaled), squares and sums them, and closes a window on each debounced level change of PD2. At
   the end of a window it copies the sums to adc_cycle_* and posts event flag 2 to task_measure.
   During the first 0xE400 samples it averages the zero offsets of channels 0, 1 and 5
   (adc_offset_state 0, 1; task_measure divides the sums and sets state 2). Which signal each
   channel carries is not in the code; channel 0 is the one compared with the aux MCU's waveform
   for the CT power (monitor.c).

   name: original. ST's template file stm32f10x_it.c, second file of the ST USER group
   (map/filenames.tsv). */
#include "../include/firmware_types.h"
#include "../include/stdperiph_types.h"
#include "../include/board.h"
#include "../include/measure.h"
#include "../include/modbus.h"
#include "../include/settings.h"
#include "../include/stdperiph.h"
#include "../include/tasks.h"
#include "../include/uart.h"
#include "../include/usb.h"

/* NMI: returns. */
void NMI_Handler() { return; }

/* Hard fault: requests a system reset (SCB AIRCR VECTKEY | SYSRESETREQ, keeping the priority
   grouping) and waits for it. So a hard fault restarts the inverter's ARM. */
void HardFault_Handler() {

    while (1) {
        __disable_fiq();
        __nop();
        *(volatile uint *)SCB_AIRCR_ADDR =
            (*(volatile uint *)SCB_AIRCR_ADDR & 0x700 | 0x5fa0000) + 4;
        __dsb(0xf);
        __nop();
        while (1) {
        }
    }
}

/* Memory management fault: loops for ever. */
void MemManage_Handler() {

    do {
        /* WARNING: Do nothing block with infinite loop */
    } while (true);
}

/* Bus fault: loops for ever. */
void BusFault_Handler() {

    do {
        /* WARNING: Do nothing block with infinite loop */
    } while (true);
}

/* Usage fault: loops for ever. */
void UsageFault_Handler() {

    do {
        /* WARNING: Do nothing block with infinite loop */
    } while (true);
}

/* SVC: returns. */
void SVC_Handler() { return; }

/* Debug monitor: returns. */
void DebugMon_Handler() { return; }

/* TIM3 update, about 19.1 kHz: starts an ADC scan on every second interrupt. While
   output_enabled is 1 it also drives the buzzer pin on for two interrupts and off for one
   (inference: a tone of about 6.4 kHz while the output relay is in, buzzer_on/off in monitor.c). */
void TIM3_IRQHandler() {

    TIM_ClearFlag(TIM3_BASE, 1);
    if (adc_start_toggle == 0) {
        adc_start_toggle = 1;
        ADC_SoftwareStartConvCmd(ADC1_BASE, 1);
    } else {
        adc_start_toggle = 0;
    }
    if (output_enabled == '\x01') {
        if (buzzer_div < 2) {
            buzzer_div = buzzer_div + 1;
            buzzer_on();
        } else {
            buzzer_div = '\0';
            buzzer_off();
        }
    } else {
        buzzer_div = '\0';
    }
    return;
}

/* TIM4 update: clears the flag only. TIM4 is never started (tim4_init is dead). */
void TIM4_IRQHandler() {

    TIM_ClearFlag(TIM4_BASE, 1);
    return;
}

/* Receives the auxiliary MCU's pages on USART1, one byte per call. A page is 101 bytes:

     [0]       0x77
     [1]       DRM bits, half a word (bit 7..0 or 15..8 by page parity) unless it is 0x77
     [2..97]   48 big-endian words, stored at aux_wave_buf[48 * page]
     [98]      page number 0..3
     [99..100] CRC16/MODBUS over bytes 0..98, high byte first

   Bytes are only taken while the ARM's own capture is complete (adc_wave_idx >= 1000). A good
   page is answered with "1".."4" (page + 0x31). When both DRM halves have arrived and the grid
   code is 2, 3, 8, 20 or 21 with HR41 set, drm_mode_eval runs. A page with byte 1 = 0x77 sets
   aux_rx_bit instead: the last word of the capture then carries the battery NTC or the DRM word
   (task_monitor). After page 3 with all four seen, aux_rx_state = 2 hands the capture to
   task_monitor. Inference: the aux MCU samples the CT clamp and the DRM RJ45 inputs. */
void aux_uart_rx_byte() {
    int crc = 0;
    volatile int unused_value = 0;
    uchar i = 0;
    uchar c;

    c = USART_ReceiveData(USART1_BASE);
    if (aux_rx_state == 0) {
        switch (aux_rx_idx) {
        case 0:
            if (c == 0x77) {
                uart_rx_buf[0][aux_rx_idx] = c;
                aux_rx_idx = aux_rx_idx + 1;
            } else {
                aux_rx_idx = 0;
            }
            break;
        default:
            if (aux_rx_idx < 0x65 && uart_rx_buf[0][0] == 0x77 && adc_wave_idx >= 1000) {
                uart_rx_buf[0][aux_rx_idx] = c;
                aux_rx_idx = aux_rx_idx + 1;
                if (aux_rx_idx == 0x65 && uart_rx_buf[0][98] <= 3) {
                    if (uart_rx_buf[0][0] == 0x77) {
                        crc = crc16_modbus(uart_rx_buf[0], 0x63);
                        if (uart_rx_buf[0][100] + uart_rx_buf[0][99] * 256 == crc) {
                            for (i = 0; i < 0x30; i++) {
                                aux_wave_buf[i + uart_rx_buf[0][98] * 48] =
                                    (short)(uart_rx_buf[0][(i + 1) * 2 + 1] +
                                            uart_rx_buf[0][(i + 1) * 2] * 256);
                            }
                            if (uart_rx_buf[0][1] != 0x77) {
                                aux_rx_bit = 0;
                                if (uart_rx_buf[0][98] % 2 == 0) {
                                    (ushort) drm_bits = (uchar)drm_bits + uart_rx_buf[0][1] * 256;
                                    drm_bits_halves = drm_bits_halves | 1;
                                } else {
                                    (ushort) drm_bits =
                                        ((ushort)drm_bits & 0xff00) + uart_rx_buf[0][1];
                                    drm_bits_halves = drm_bits_halves | 2;
                                }
                                if (drm_bits_halves == 3) {
                                    drm_bits_halves = 0;
                                    if ((grid_country == 3 || grid_country == 0x14 ||
                                         grid_country == 0x15 || grid_country == 8 ||
                                         grid_country == 2) &&
                                        cfg_settings.enable_drm_rj45 == 1) {
                                        drm_mode_eval();
                                    }
                                }
                            } else {
                                aux_rx_bit = 1;
                            }
                            aux_pages_seen = (1 << uart_rx_buf[0][98]) + aux_pages_seen;
                            if (uart_rx_buf[0][98] >= 3 && aux_pages_seen == 0xf) {
                                aux_rx_state = 2;
                                aux_pages_seen = 0;
                            }
                            aux_tx_byte = uart_rx_buf[0][98] + 0x31;
                            USART_SendData(USART1_BASE, aux_tx_byte);
                            aux_tx_repeat_count = 0;
                        }
                    }
                    aux_rx_idx = 0;
                }
            } else {
                aux_rx_idx = 0;
                aux_pages_seen = 0;
            }
            break;
        }
    }
}

/* USART1: on a receive error (PE, FE or ORE in SR) reads the data register to clear it;
   otherwise transmit-complete feeds the next byte, receive goes to aux_uart_rx_byte. */
void USART1_IRQHandler() {
    uchar status;

    status = *(volatile ushort *)USART1_BASE;
    if ((status & 0xb) != 0) {
        USART_ReceiveData(USART1_BASE);
    } else {
        if (USART_GetITStatus(USART1_BASE, 0x626) != 0) {
            USART_ClearITPendingBit(USART1_BASE, 0x626);
            uart_tx_isr_usart1();
        } else {
            if (USART_GetITStatus(USART1_BASE, 0x525) != 0) {
                aux_uart_rx_byte();
            } else {
                USART_ReceiveData(USART1_BASE);
            }
        }
    }
}

/* USART2 (meter RS485): the same, receive goes to queue 1. */
void USART2_IRQHandler() {
    uchar status;

    status = *(volatile ushort *)USART2_BASE;
    if ((status & 0xb) != 0) {
        USART_ReceiveData(USART2_BASE);
    } else {
        if (USART_GetITStatus(USART2_BASE, 0x626) != 0) {
            USART_ClearITPendingBit(USART2_BASE, 0x626);
            uart_tx_isr_usart2();
        } else {
            if (USART_GetITStatus(USART2_BASE, 0x525) != 0) {
                uart_rx_isr(1);
            } else {
                USART_ReceiveData(USART2_BASE);
            }
        }
    }
}

/* USART3 (Modbus RS485): the same, receive goes to queue 2. */
void USART3_IRQHandler() {
    uchar status;

    status = *(volatile ushort *)USART3_BASE;
    if ((status & 0xb) != 0) {
        USART_ReceiveData(USART3_BASE);
    } else {
        if (USART_GetITStatus(USART3_BASE, 0x626) != 0) {
            USART_ClearITPendingBit(USART3_BASE, 0x626);
            uart_tx_isr_usart3();
        } else {
            if (USART_GetITStatus(USART3_BASE, 0x525) != 0) {
                uart_rx_isr(2);
            } else {
                USART_ReceiveData(USART3_BASE);
            }
        }
    }
}

/* UART4 (DSP link): the same, receive goes to queue 3. */
void UART4_IRQHandler() {
    uchar status;

    status = *(volatile ushort *)UART4_BASE;
    if ((status & 0xb) != 0) {
        USART_ReceiveData(UART4_BASE);
    } else {
        if (USART_GetITStatus(UART4_BASE, 0x626) != 0) {
            USART_ClearITPendingBit(UART4_BASE, 0x626);
            uart_tx_isr_uart4();
        } else {
            if (USART_GetITStatus(UART4_BASE, 0x525) != 0) {
                uart_rx_isr(3);
            } else {
                USART_ReceiveData(UART4_BASE);
            }
        }
    }
}

/* End of an ADC scan (about 9.6 kHz). Takes channel 0, 5 and 7 samples (channel 1 is zeroed; the
   "/ 1" averaging is a leftover that does nothing), stores the heatsink and charger NTC readings
   (channels at buffer 1 and 6) for monitor.c, debounces PD2 over 5 scans and closes a
   measurement window on each level change. On the low edge it also counts 50 windows and then
   restarts the aux link with "0" (0x30) and a new waveform capture. Per window it accumulates
   ch0^2, ch5^2, ch7^2 and ch0 x ch7 (offset removed, scaled by 0x90A, 0x198 and 0x93B in Q10;
   ch5 clamped to +-2000), and at the window end copies them to adc_cycle_* and posts flag 2 to
   task 0 (task_measure). A window over 250 scans is thrown away. The first 191 ch0 samples after
   a restart go to adc_wave_buf for the CT comparison in task_monitor. */
void ADC1_2_IRQHandler() {
    int unused_value;

    ADC_ClearITPendingBit(ADC1_BASE, 0x220);
    if (adc_isr_phase == 0) {
        adc_sample_ch0 = 0;
        adc_sample_ch1 = 0;
        adc_sample_ch5 = 0;
        adc_sample_ch7 = 0;
    }
    adc_sample_ch0 = (short)(adc_dma_buf[0] + adc_sample_ch0);
    adc_sample_ch1 = 0;
    adc_ntc_heatsink = adc_dma_buf[1];
    adc_sample_ch5 = (short)(adc_dma_buf[5] + adc_sample_ch5);
    adc_ntc_charger = adc_dma_buf[6];
    adc_sample_ch7 = (short)(adc_dma_buf[7] + adc_sample_ch7);
    adc_isr_phase = adc_isr_phase + 1;
    if (adc_isr_phase >= 1) {
        adc_isr_phase = 0;
        adc_sample_ch0 = adc_sample_ch0 / 1;
        adc_sample_ch1 = adc_sample_ch1 / 1;
        adc_sample_ch5 = adc_sample_ch5 / 1;
        adc_sample_ch7 = adc_sample_ch7 / 1;
    } else {
        return;
    }
    if (GPIO_ReadInputDataBit(GPIOD_BASE, 4) == 0) {
        pd2_high_count = 0;
        if (pd2_low_count < 5) {
            pd2_low_count = pd2_low_count + 1;
        } else {
            if (pd2_low_count == 5) {
                pd2_low_count = 6;
                adc_cycle_end = 1;
                aux_sync_count = aux_sync_count + 1;
                if (aux_sync_count >= 0x32) {
                    aux_sync_count = 0;
                    aux_tx_byte = 0x30;
                    USART_SendData(USART1_BASE, aux_tx_byte);
                    aux_pages_seen = 0;
                    aux_tx_repeat_count = 0;
                    adc_wave_idx = 0;
                    aux_tx_restart_count = 0;
                }
                if (aux_sync_count == 0x2d) {
                    if (usb_reset_state == 1) {
                        USART_SendData(USART1_BASE, 0xab);
                    } else {
                        USART_SendData(USART1_BASE, 0);
                    }
                }
            }
        }
    } else {
        pd2_low_count = 0;
        if (pd2_high_count < 5) {
            pd2_high_count = pd2_high_count + 1;
        } else {
            if (pd2_high_count == 5) {
                pd2_high_count = 6;
                adc_cycle_end = 1;
            }
        }
    }
    if (adc_offset_samples == 0) {
        adc_cycle_sumsq_ch0 = 0;
        adc_cycle_sumsq_ch1 = 0;
        adc_cycle_sumsq_ch5 = 0;
        adc_offset_state = 0;
        adc_offset_ch0 = 0x746;
        adc_offset_ch1 = 0x746;
        adc_offset_ch5 = 0x746;
    }
    if (adc_offset_samples < 0xe400) {
        adc_offset_samples = adc_offset_samples + 1;
        if (adc_offset_samples >= 0xe100) {
            adc_sum_ch0 = adc_sum_ch0 + adc_sample_ch0;
            adc_sum_ch5 = adc_sum_ch5 + adc_sample_ch5;
            adc_sum_ch1 = adc_sum_ch1 + adc_sample_ch1;
        }
    } else {
        adc_offset_state = 1;
    }
    adc_sample_ch0 = adc_sample_ch0 - (short)adc_offset_ch0;
    adc_sample_ch5 = adc_sample_ch5 - (short)adc_offset_ch5;
    if (adc_sample_ch5 > 0x7d0) {
        adc_sample_ch5 = 0x7d0;
    }
    if (adc_sample_ch5 < -2000) {
        adc_sample_ch5 = 0xf830;
    }
    adc_sample_ch1 = adc_sample_ch1 - (short)adc_offset_ch1;
    adc_sample_ch7 = (short)(adc_sample_ch7 - 0x746);
    adc_sample_ch5 = adc_sample_ch5 * 0x198 >> 0xa;
    adc_sample_ch1 = 0;
    adc_sample_ch0 = (short)(adc_sample_ch0 * 0x90a >> 0xa);
    adc_sample_ch7 = (short)(adc_sample_ch7 * 0x93b >> 0xa);
    adc_sumsq_ch7 = adc_sample_ch7 * adc_sample_ch7 + adc_sumsq_ch7;
    adc_sumprod_ch0_ch7 = adc_sample_ch0 * adc_sample_ch7 + adc_sumprod_ch0_ch7;
    adc_sumsq_ch0 = adc_sample_ch0 * adc_sample_ch0 + adc_sumsq_ch0;
    adc_sumsq_ch5 = adc_sample_ch5 * adc_sample_ch5 + adc_sumsq_ch5;
    adc_sumsq_ch1 = adc_sample_ch1 * adc_sample_ch1 + adc_sumsq_ch1;
    adc_cycle_samples = adc_cycle_samples + 1;
    if (adc_cycle_end == 1) {
        adc_cycle_end = 0;
        adc_cycle_n = adc_cycle_samples;
        adc_cycle_sumsq_ch7 = adc_sumsq_ch7;
        adc_cycle_sumprod = adc_sumprod_ch0_ch7;
        adc_cycle_samples = 0;
        adc_sumsq_ch7 = 0;
        adc_sumprod_ch0_ch7 = 0;
        adc_cycle_sumsq_ch0 = adc_sumsq_ch0;
        adc_cycle_sumsq_ch1 = adc_sumsq_ch1;
        adc_cycle_sumsq_ch5 = adc_sumsq_ch5;
        adc_sumsq_ch0 = 0;
        adc_sumsq_ch1 = 0;
        adc_sumsq_ch5 = 0;
        os_flags_post(0, 2);
    } else {
        if (adc_cycle_samples > 0xfa) {
            adc_cycle_samples = 0;
            adc_sumsq_ch7 = 0;
            adc_sumprod_ch0_ch7 = 0;
            adc_sumsq_ch0 = 0;
            adc_sumsq_ch1 = 0;
            adc_sumsq_ch5 = 0;
        }
    }
    if (adc_wave_idx < 0x3e8) {
        if (adc_wave_idx < 0xcc) {
            if (adc_wave_idx >= 0xd) {
                adc_wave_buf[adc_wave_idx - 0xd] = adc_sample_ch0;
            }
            adc_wave_idx = adc_wave_idx + 1;
        } else {
            adc_wave_idx = 0x3e8;
        }
    }
    return;
}

/* EXTI line 1: calls the USB host user callback at offset 0x14 of USB_Host's user callback table
   (OverCurrentDetected in ST's usbh_usr layout) and clears the line. From ST's USB host example. */
void EXTI1_IRQHandler() {
    int unused_value;

    if (EXTI_GetITStatus(2) != 0) {
        (**(code **)(*(int *)&USB_Host[144] + 0x14))();
        EXTI_ClearITPendingBit(2);
    }
    return;
}

/* SysTick, 1 ms: the scheduler tick (OSTimeTick), a counter usb_task_tick_count that task_usb resets,
   and two timing jobs: modbus_usb_wait is cleared at once (not counted down, so the 20-tick wait
   in modbus_slave_process lasts at most one tick), and each RS485 driver enable is released two
   ticks after its transmitter finished. */
void SysTick_Handler() {

    OSTimeTick();
    if (usb_task_tick_count < 0x32) {
        usb_task_tick_count = usb_task_tick_count + 1;
    }
    if (modbus_usb_wait > 0) {
        modbus_usb_wait = 0;
    }
    if (rs485_modbus_de_hold > 0) {
        if (rs485_modbus_de_hold < 2) {
            rs485_modbus_de_hold = rs485_modbus_de_hold + 1;
        } else {
            rs485_modbus_de_set(0);
        }
    }
    if (rs485_meter_de_hold > 0) {
        if (rs485_meter_de_hold < 2) {
            rs485_meter_de_hold = rs485_meter_de_hold + 1;
        } else {
            rs485_meter_de_set(0);
        }
    }
    return;
}

/* TIM2: the USB host library delay timer (usb_bsp.c). */
void TIM2_IRQHandler() {

    USB_OTG_BSP_TimerIRQ();
    return;
}

/* USB OTG FS: ST's host interrupt handler. */
void OTG_FS_IRQHandler() {

    USBH_OTG_ISR_Handler(&USB_OTG_Core[0]);
    return;
}
