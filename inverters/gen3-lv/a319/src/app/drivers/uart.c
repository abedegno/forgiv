// clang-format off
/* at: uart_rx_reset 0x08014F24 */
/* at: dsp_flash_ok_state 0x20000048, rs485_modbus_de_hold 0x20000242, rs485_meter_de_hold 0x20000243, dsp_flash_step 0x20000308, dsp_flash_reply 0x20000309, dsp_flash_mode 0x200003A4, dsp_flash_ok 0x200003A6, OSRdyGrp 0x200004D4, uart_rx_buf 0x2000175C, uart_rx_q 0x20001A72, uart_tx_q 0x20001A96, task_tbl 0x20007EB0 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* uart.c: interrupt-driven transmit and receive queues for the four ports.

     ch 0  USART1  aux MCU link (receive bytes go to aux_uart_rx_byte instead of a queue)
     ch 1  USART2  meter RS485 (Modbus master, task_meter)
     ch 2  USART3  Modbus slave RS485 (task_modbus_rs485)
     ch 3  UART4   DSP link (task_dsp_rx, task_dsp_tx, firmware updates)

   Entry points: uart_tx_start, uart_rx_getc, uart_rx_reset (tasks); uart_tx_isr_* and
   uart_rx_isr (the USART handlers in stm32f10x_it.c).
   Data: uart_tx_q[4] and uart_rx_q[4] (9-byte records, application_types.h), uart_rx_buf[4][151].
   Neighbours: stm32f10x_it.c, board.c (uart_init_all, the RS485 driver enables), the tasks
   above, rtos (it sets task flags directly: task 2 flag 0x10 when a DSP frame has gone out, task
   1 flag 8 when 45 bytes of a 0x44 frame have come in).

   The receive queues of ch 0-2 are a linear buffer: the ISR appends at b and counts a; the task
   reads at idx and the queue empties itself when a reaches 0. Queue fields: busy = 1 means "no
   data" (0 after the ISR stored a byte), a = bytes not yet read, b = write index, idx = read
   index. For ch 3 a counts up to b instead. A transmit queue sends buf[0..len-1] one byte per
   transmit-complete interrupt; uart_tx_start waits while the port is busy.

   name: likely (map/filenames.tsv). */
#include "../../include/firmware_types.h"
#include "../../include/stdperiph_types.h"
#include "../../include/application_types.h"
#include "../../include/dsp_link.h"
#include "../../include/stdperiph.h"
#include "../../include/tasks.h"
#include "../../include/uart.h"

/* Clears receive buffer ch and resets its queue to empty with the given size (at most 151). */
void uart_rx_reset(int ch, int size) {
    uart_rx *p;
    ushort i;

    for (i = 0; i < size; i++) {
        uart_rx_buf[ch][i] = 0;
    }
    p = &uart_rx_q[ch];
    p->busy = 1;
    p->pending_count = 0;
    p->write_index = 0;
    p->idx = 0;
    p->buf = uart_rx_buf[ch];
    p->size = size;
}

/* Sends the next byte of the USART1 queue, or marks it idle when all are sent. */
void uart_tx_isr_usart1() {
    uart_tx *p = &uart_tx_q[0];

    if (p->idx >= p->len) {
        p->busy = 0;
        return;
    }
    USART_SendData(USART1_BASE, p->buf[p->idx]);
    p->idx++;
    p->busy = 1;
}

/* The same for USART2; at the end it starts the release count of the meter RS485 driver enable
   (SysTick_Handler drops PA4 two ticks later). */
void uart_tx_isr_usart2() {
    uart_tx *p = uart_tx_q;

    p += 1;

    if (p->idx >= p->len) {
        p->busy = 0;
        rs485_meter_de_hold = 1;
        return;
    }
    USART_SendData(USART2_BASE, p->buf[p->idx]);
    p->idx++;
    p->busy = 1;
}

/* The same for USART3; at the end it starts the release count of the Modbus driver enable (PB4). */
void uart_tx_isr_usart3() {
    uart_tx *p = uart_tx_q;

    p += 2;

    if (p->idx >= p->len) {
        p->busy = 0;
        rs485_modbus_de_hold = 1;
        return;
    }
    USART_SendData(USART3_BASE, p->buf[p->idx]);
    p->idx++;
    p->busy = 1;
}

/* The same for UART4; at the end it sets flag 0x10 of task 2 (task_dsp_tx may send again). */
void uart_tx_isr_uart4() {
    uart_tx *p = uart_tx_q;

    p += 3;

    if (p->idx >= p->len) {
        p->busy = 0;
        task_tbl[TASK_DSP_TX].FlagsRdy = task_tbl[TASK_DSP_TX].FlagsRdy | 0x10;
        return;
    }
    USART_SendData(UART4_BASE, p->buf[p->idx]);
    p->idx++;
    p->busy = 1;
}

/* Stores one received byte in queue ch. For ch 0-2 the byte is appended while there is room.
   UART4 is special:
   - in the DSP firmware relay (dsp_flash_mode 1) it only watches for "OK" (0x4F 0x4B) and sets
     dsp_flash_ok;
   - while task 1 still has flag 8 (a frame not yet taken) bytes are dropped;
   - during a DSP update (dsp_flash_step >= 2) it collects replies that start 0x0A and sets
     dsp_flash_reply after 5 bytes;
   - otherwise it collects a frame that starts 0x44 and, at 45 bytes, sets flag 8 of task 1 and
     its ready bit; a first byte other than 0x44 restarts the frame.
   In that last case it clears uart_rx_buf[2][0], the first byte of the Modbus RS485 buffer, not
   UART4's (likely a slip for [3]; docs/findings.md). */
void uart_rx_isr(int ch) {
    uart_rx *p = &uart_rx_q[ch];
    uchar c;
    uchar f;

    if (ch == 0) {
        if (p->write_index < p->size) {
            p->buf[p->write_index] = USART_ReceiveData(USART1_BASE);
            p->write_index++;
            p->pending_count++;
        } else {
            USART_ReceiveData(USART1_BASE);
        }
        p->busy = 0;
    } else if (ch == 1) {
        if (p->write_index < p->size) {
            p->buf[p->write_index] = USART_ReceiveData(USART2_BASE);
            p->write_index++;
            p->pending_count++;
        } else {
            USART_ReceiveData(USART2_BASE);
        }
        p->busy = 0;
    } else if (ch == 2) {
        if (p->write_index < p->size) {
            p->buf[p->write_index] = USART_ReceiveData(USART3_BASE);
            p->write_index++;
            p->pending_count++;
        } else {
            USART_ReceiveData(USART3_BASE);
        }
        p->busy = 0;
    } else if (ch == 3) {
        f = task_tbl[TASK_DSP_RX].FlagsRdy & 8;
        if (dsp_flash_mode == 1) {
            c = USART_ReceiveData(UART4_BASE);
            if (dsp_flash_ok_state == 0) {
                if (c == 0x4f)
                    dsp_flash_ok_state = 1;
                else
                    dsp_flash_ok_state = 0;
            } else if (dsp_flash_ok_state == 1) {
                if (c == 0x4b)
                    dsp_flash_ok_state = 2;
                else
                    dsp_flash_ok_state = 0;
            }
            if (dsp_flash_ok_state >= 2) {
                dsp_flash_ok_state = 0;
                dsp_flash_ok = 1;
            }
        } else if (f == 0) {
            c = USART_ReceiveData(UART4_BASE);
            p->buf[p->write_index] = c;
            if (dsp_flash_step > 0) {
                if (dsp_flash_step >= 2 && uart_rx_buf[ch][0] == 0xa) {
                    if (p->write_index < p->size)
                        p->write_index++;
                    p->busy = 0;
                    if (p->write_index >= 5)
                        dsp_flash_reply = 1;
                }
            } else if (uart_rx_buf[ch][0] == 0x44) {
                if (p->write_index < p->size)
                    p->write_index++;
                p->busy = 0;
                if (p->write_index >= 0x2d) {
                    task_tbl[TASK_DSP_RX].FlagsRdy |= 8;
                    OSRdyGrp |= 2;
                }
            } else {
                p->write_index = 0;
                uart_rx_buf[2][0] = 0;
            }
        } else {
            USART_ReceiveData(UART4_BASE);
        }
    }
}

/* Takes the next received byte of queue ch into *out and returns 0, or returns 1 when there is
   none. When the queue has been read up, it resets itself to empty. */
int uart_rx_getc(int ch, uchar *out) {
    uart_rx *p;

    if (ch == 0 || ch == 1 || ch == 2) {
        p = &uart_rx_q[ch];
        if (p->busy == 1)
            return 1;
        if (p->idx < p->size) {
            *out = p->buf[p->idx];
            p->idx++;
            OS_ENTER_CRITICAL();
            if (p->pending_count > 0)
                p->pending_count--;
            if (p->pending_count == 0) {
                p->busy = 1;
                p->write_index = 0;
                p->idx = 0;
            }
            OS_EXIT_CRITICAL();
        } else {
            OS_ENTER_CRITICAL();
            p->busy = 1;
            p->write_index = 0;
            p->idx = 0;
            p->pending_count = 0;
            OS_EXIT_CRITICAL();
        }
        return 0;
    }
    if (ch == 3) {
        p = &uart_rx_q[ch];
        if (p->busy == 1)
            return 1;
        if (p->idx < p->size) {
            *out = p->buf[p->idx];
            p->idx++;
            OS_ENTER_CRITICAL();
            if (p->pending_count < p->size)
                p->pending_count++;
            if (p->pending_count == p->write_index) {
                p->busy = 1;
                p->write_index = 0;
                p->idx = 0;
                p->pending_count = 0;
            }
            OS_EXIT_CRITICAL();
        } else {
            OS_ENTER_CRITICAL();
            p->busy = 1;
            p->write_index = 0;
            p->idx = 0;
            p->pending_count = 0;
            OS_EXIT_CRITICAL();
        }
        return 0;
    }
    return 1;
}

/* Waits until port ch is free, then starts sending len bytes from buf (the ISR does the rest). */
void uart_tx_start(int ch, uchar *buf, ushort len) {
    uart_tx *p = &uart_tx_q[ch];

    while (p->busy == 1) {
    }
    p->busy = 1;
    p->len = len;
    p->idx = 0;
    p->buf = buf;
    if (ch == 0) {
        uart_tx_isr_usart1();
    } else if (ch == 1) {
        uart_tx_isr_usart2();
    } else if (ch == 2) {
        uart_tx_isr_usart3();
    } else if (ch == 3) {
        uart_tx_isr_uart4();
    }
}
