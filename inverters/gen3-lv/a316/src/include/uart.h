/* uart.h: the four UARTs (drivers/uart.c): transmit and receive queues, RS485 driver enables,
   the aux link on USART1 and the interrupt handlers (stm32f10x_it.c). Ports: 0 USART1 aux MCU, 1
   USART2 meters, 2 USART3 Modbus, 3 UART4 DSP.
   name: descriptive (readability step 1). */
#ifndef UART_H
#define UART_H

#include "ghidra_types.h"
#include "a316_types.h"

/* data */
extern uchar aux_pages_seen;
extern uchar aux_rx_bit;
extern ushort aux_rx_idx;
extern uchar aux_rx_state;
extern uchar aux_sync_count;
extern uchar aux_tx_byte;
extern short aux_wave_buf[192]; /* 4 pages x 48 words from the aux MCU */
extern short adc_sample_ch0;
extern short adc_sample_ch1;
extern short adc_sample_ch5;
extern int adc_sumsq_ch1;
extern short adc_sample_ch7;
extern uchar adc_isr_phase;
extern int adc_sum_ch0;
extern int adc_sum_ch1;
extern int adc_sum_ch5;
extern uchar output_enabled;
extern uchar aux_tx_repeat_count;
extern ushort aux_tx_restart_count;
extern uchar g_2000030F;
extern ushort pd2_high_count;
extern ushort pd2_low_count;
extern uchar rs485_meter_de_hold;
extern uchar rs485_modbus_de_hold;
extern uchar uart_rx_buf[][151]; /* one per port; [0] also holds the aux pages */
extern uart_rx uart_rx_q[];
extern uart_tx uart_tx_q[];

/* functions */
void ADC1_2_IRQHandler();
void BusFault_Handler();
void DebugMon_Handler();
void EXTI1_IRQHandler();
void HardFault_Handler();
void MemManage_Handler();
void NMI_Handler();
void OTG_FS_IRQHandler();
void SVC_Handler();
void TIM2_IRQHandler();
void TIM3_IRQHandler();
void TIM4_IRQHandler();
void UART4_IRQHandler();
void USART1_IRQHandler();
void USART2_IRQHandler();
void USART3_IRQHandler();
void UsageFault_Handler();
void aux_uart_rx_byte();
void rs485_meter_de_set(int level);
void rs485_modbus_de_set(int level);
void uart_init_all();
int uart_rx_getc(int ch, uchar *out);
void uart_rx_isr(int ch);
void uart_rx_reset(int ch, int size);
void uart_tx_isr_uart4();
void uart_tx_isr_usart1();
void uart_tx_isr_usart2();
void uart_tx_isr_usart3();
void uart_tx_start(int ch, uchar *buf, ushort len);

#endif
