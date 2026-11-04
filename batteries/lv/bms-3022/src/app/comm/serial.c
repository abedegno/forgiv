/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast -DSTM32F10X_CL -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x */
/* at: serial_rx_initialize 0x08014AC8, serial_usart1_send_next 0x08014AFA, serial_usart2_send_next 0x08014B2E, serial_usart3_send_next 0x08014B5C, serial_uart4_send_next 0x08014B92, serial_receive_byte 0x08014BCC, serial_read_byte 0x08014F36, serial_send 0x08014F94, ascii_hex_to_byte 0x08014FD6, ascii_byte_to_hex 0x08015034, ascii_length_checksum_valid 0x08015058, ascii_frame_checksum_valid 0x0801508A, ascii_set_length_field 0x080150EE, ascii_set_rtn_field 0x0801512E */
/* at: update_parser_state 0x20000020, update_end_value 0x20000044, usart3_tx_done 0x2000004D, usart1_tx_done 0x2000004E, update_mode 0x200000E1, update_block_ready 0x200000E2, update_ack_received 0x200000E4, update_end_received 0x200000E6, update_timeout_ticks 0x200000EA, hex_pair_buffer 0x2000010D, pack_address 0x20000167, serial_rx_buffers 0x20000D0F, serial_rx_queues 0x20000F6B, serial_tx_queues 0x20000F8F, firmware_update_payload 0x2000164F */
/* I follow the four serial ports' byte queues here, the firmware-update parsers on USART1 and
   the hex helpers of the ASCII protocol. Each port has a nine-byte transmit queue and a
   nine-byte receive queue over a 151-byte buffer; the interrupt handlers call the send-next
   and receive routines, and tasks call serial_send and serial_read_byte. On USART1 the pack
   at address 0 parses "OK"/"NO" acknowledgements, and other packs parse "STA" (enter update
   mode), "BMS" block frames and "END" frames into firmware_update_payload.
   The ASCII helpers convert hex digits, check the length and frame checksums and fill the
   length and return-code fields; ascii_protocol.c calls them through its own caller views.
   Neighbours: stm32f10x_it.c (interrupts), ascii_protocol.c, register_protocol.c, usbh_usr.c.
   name: I use a descriptive filename; I have no original vendor filename evidence. */
#include "../../include/ghidra_types.h"
#include "stm32f10x_usart.h"
#include "../../include/tasks.h"

/* A transmit queue: busy while bytes remain; index is the next byte to send. */
typedef __packed struct {
    uchar busy;
    ushort length, index;
    uchar *buffer;
} SerialTxQueue;
/* A receive queue: empty is 1 when nothing is queued; count is the number of unread bytes. */
typedef __packed struct {
    uchar empty, count, write_index, read_index;
    uchar *buffer;
    uchar size;
} SerialRxQueue;
extern uchar serial_rx_queues[], serial_tx_queues[];
#define serial_rx ((SerialRxQueue *)serial_rx_queues)
#define serial_tx ((SerialTxQueue *)serial_tx_queues)

extern uchar serial_rx_buffers[];

/* Clears the port's receive buffer for size bytes and resets its queue to empty. */
void serial_rx_initialize(int port, uint size) {
    SerialRxQueue *queue;
    uint i;
    for (i = 0; i < size; i = (i + 1) & 0xffff)
        (serial_rx_buffers + port * 151)[i] = 0;
    queue = &serial_rx[port];
    queue->empty = 1;
    queue->count = 0;
    queue->write_index = 0;
    queue->read_index = 0;
    queue->buffer = serial_rx_buffers + port * 151;
    queue->size = size;
}

extern uchar usart1_tx_done;

/* USART1 transmit step: sends the next queued byte, or marks the queue idle and sets
   usart1_tx_done. */
void serial_usart1_send_next(void) {
    SerialTxQueue *queue = serial_tx;
    if (queue->index >= queue->length) {
        queue->busy = 0;
        usart1_tx_done = 1;
        return;
    }
    USART_SendData(USART1, queue->buffer[queue->index]);
    queue->index++;
    queue->busy = 1;
}

/* USART2 transmit step. */
void serial_usart2_send_next(void) {
    SerialTxQueue *queue = &serial_tx[1];
    if (queue->index >= queue->length) {
        queue->busy = 0;
        return;
    }
    USART_SendData(USART2, queue->buffer[queue->index]);
    queue->index++;
    queue->busy = 1;
}

extern uchar usart3_tx_done;

/* USART3 transmit step; sets usart3_tx_done when the queue is sent. */
void serial_usart3_send_next(void) {
    SerialTxQueue *queue = &serial_tx[2];
    if (queue->index >= queue->length) {
        queue->busy = 0;
        usart3_tx_done = 1;
        return;
    }
    USART_SendData(USART3, queue->buffer[queue->index]);
    queue->index++;
    queue->busy = 1;
}

/* UART4 transmit step; disables the transmit interrupt when the queue is sent. */
void serial_uart4_send_next(void) {
    SerialTxQueue *queue = &serial_tx[3];
    if (queue->index >= queue->length) {
        queue->busy = 0;
        USART_ITConfig(UART4, USART_IT_TXE, DISABLE);
        return;
    }
    USART_SendData(UART4, queue->buffer[queue->index]);
    queue->index++;
    queue->busy = 1;
}

/* USART1 firmware-update state. The four bytes of update_parser_state hold the
   acknowledgement step, the start-command step and the payload index, all volatile. */
extern uchar update_parser_state[];
#define update_ack_state (*(volatile uchar *)(update_parser_state + 0))
#define update_start_state (*(volatile uchar *)(update_parser_state + 1))
#define update_frame_index (*(volatile ushort *)(update_parser_state + 2))
extern uint update_end_value;
extern uchar update_mode[];
extern uchar update_block_ready;
extern uchar update_ack_received;
extern uchar update_end_received;
extern ushort update_timeout_ticks;
extern uchar pack_address;
extern uchar firmware_update_payload[];

/* Receive interrupt step for one port: queues the received byte. On USART1 in update mode
   the pack at address 0 instead follows "OK"/"NO" acknowledgements, and other packs collect
   "BMS" + address + 1024 data bytes + 2-byte checksum (1030 bytes, checked over the first
   1028) or "END" + address + a 32-bit value. Outside update mode USART1 bytes are queued
   while "STA" + this pack's address is watched for. */
void serial_receive_byte(int port) {
    SerialRxQueue *queue;
    uint address, mode, state;
    uchar *end;
    ushort expected;
    uchar ack_char;
    uchar received;
    uint next_index;
    uint frame_index;
    ushort sum;
    uint sum_index;

    sum = 0;
    sum_index = 0;
    queue = &serial_rx[port];
    if (port == 0) {
        const uchar *address_ptr = &pack_address;
        const uchar *mode_ptr = update_mode;
        uint mode_address_diagnostic = 0;
        /* match: the compiler drops these unread pointer comparisons after allocation, but
       their lifetimes keep the matched order of literal loads. */
        mode_address_diagnostic = (uint)(update_parser_state != (const uchar *)queue);
        address = *address_ptr;
        mode = *mode_ptr;
        mode_address_diagnostic = (uint)(mode_ptr != (const uchar *)queue);
        mode_address_diagnostic = (uint)(address_ptr != (const uchar *)queue);
        (void)mode_address_diagnostic;
        if (address == 0) {
            if (mode == 1) {
                ack_char = (uchar)(USART_ReceiveData(USART1));
                state = update_ack_state;
                if (state == 0) {
                    if (ack_char == 'O' || ack_char == 'N')
                        update_ack_state = 1;
                    else
                        update_ack_state = 0;
                } else if (state == 1) {
                    if (ack_char == 'K' || ack_char == 'O')
                        update_ack_state = state + 1;
                    else
                        update_ack_state = 0;
                }
                if (update_ack_state >= 2) {
                    update_ack_state = 0;
                    update_ack_received = 1;
                    update_timeout_ticks = 0;
                }
            } else {
                if (queue->write_index < queue->size) {
                    queue->buffer[queue->write_index] = (uchar)(USART_ReceiveData(USART1));
                    queue->write_index++;
                    queue->count++;
                } else {
                    USART_ReceiveData(USART1);
                }
                queue->empty = 0;
            }
        } else if (((mode == 1) && (update_block_ready == 0)) && (update_end_received == 0)) {
            received = (uchar)(USART_ReceiveData(USART1));
            frame_index = (uint)update_frame_index;
            if (frame_index == 0) {
                if ((received == 'B') || (received == 'E')) {
                    firmware_update_payload[0] = received;
                    update_frame_index = 1;
                } else {
                    update_frame_index = 0;
                }
            } else if (frame_index == 1) {
                if ((received == 'M') || (received == 'N')) {
                    firmware_update_payload[1] = received;
                    update_frame_index = 2;
                } else {
                    update_frame_index = 0;
                }
            } else if (frame_index == 2) {
                if ((received == 'S') || (received == 'D')) {
                    firmware_update_payload[2] = received;
                    update_frame_index = 3;
                } else {
                    update_frame_index = 0;
                }
            } else if (frame_index == 3) {
                if (((firmware_update_payload[0] == 'E') && (firmware_update_payload[1] == 'N')) &&
                    (firmware_update_payload[2] == 'D')) {
                    firmware_update_payload[3] = received;
                    update_frame_index = frame_index + 1;
                } else if (received == pack_address) {
                    if (((firmware_update_payload[0] == 'B') &&
                         (firmware_update_payload[1] == 'M')) &&
                        (firmware_update_payload[2] == 'S')) {
                        firmware_update_payload[3] = received;
                        update_frame_index = frame_index + 1;
                    }
                } else {
                    update_frame_index = 0;
                }
                if (received < 6) {
                    update_timeout_ticks = 0;
                }
            } else {
                firmware_update_payload[frame_index] = received;
                next_index = frame_index + 1 & 0xffff;
                update_frame_index = next_index;
                if (((firmware_update_payload[0] == 'E') && (firmware_update_payload[1] == 'N')) &&
                    (firmware_update_payload[2] == 'D')) {
                    if (next_index >= 9) {
                        update_frame_index = 0;
                        update_end_received = 1;
                        update_end_value = (uint)firmware_update_payload[7] +
                                           (uint)firmware_update_payload[4] * 0x1000000 +
                                           (uint)firmware_update_payload[5] * 0x10000 +
                                           (uint)firmware_update_payload[6] * 0x100;
                    }
                } else if (next_index >= 0x406) {
                    do {
                        sum = (ushort)firmware_update_payload[sum_index] + sum;
                        sum_index = sum_index + 1 & 0xffff;
                    } while (sum_index < 0x404);
                    end = firmware_update_payload + next_index;
                    expected = (ushort)((int)(sbyte)end[-2] * 256);
                    expected += end[-1];
                    if (expected == sum)
                        update_block_ready = 1;
                    else
                        update_block_ready = 0;
                    update_frame_index = 0;
                }
            }
        } else {
            received = (uchar)(USART_ReceiveData(USART1));
            state = update_start_state;
            if (state == 0) {
                if (received == 'S') {
                    update_start_state = 1;
                } else {
                    update_start_state = 0;
                }
            } else if (state == 1) {
                if (received == 'T') {
                    update_start_state = 2;
                } else {
                    update_start_state = 0;
                }
            } else if (state == 2) {
                if (received == 'A') {
                    update_start_state = 3;
                    update_mode[0] = 1;
                } else {
                    update_start_state = 0;
                }
            } else if (state == 3) {
                if (received == pack_address) {
                    update_start_state = 4;
                } else {
                    update_start_state = 0;
                }
            } else {
                update_start_state = 0;
            }
            if ((uint)queue->write_index < (uint)queue->size) {
                queue->buffer[queue->write_index] = received;
                queue->write_index++;
                queue->count++;
            } else {
                USART_ReceiveData(USART1);
            }
            queue->empty = 0;
        }
    } else if (port == 1) {
        if (queue->write_index < queue->size) {
            queue->buffer[queue->write_index] = (uchar)(USART_ReceiveData(USART2));
            queue->write_index++;
            queue->count++;
        } else {
            USART_ReceiveData(USART2);
        }
        queue->empty = 0;
    } else if (port == 2) {
        if (queue->write_index < queue->size) {
            queue->buffer[queue->write_index] = (uchar)(USART_ReceiveData(USART3));
            queue->write_index++;
            queue->count++;
        } else {
            USART_ReceiveData(USART3);
        }
        queue->empty = 0;
    } else if (port == 3) {
        if (queue->write_index < queue->size) {
            queue->buffer[queue->write_index] = (uchar)(USART_ReceiveData(UART4));
            queue->write_index++;
            queue->count++;
        } else {
            USART_ReceiveData(UART4);
        }
        queue->empty = 0;
    }
    return;
}

/* Takes the next byte from the port's receive queue into *out. Returns 1 when the queue is
   empty, otherwise 0; the queue resets when its last byte is read or the read passes its size. */
uint serial_read_byte(int port, uchar *out) {
    SerialRxQueue *queue = &serial_rx[port];

    if (queue->empty == 1) {
        return 1;
    }
    if (queue->read_index < queue->size) {
        *out = queue->buffer[queue->read_index];
        queue->read_index++;
        OS_ENTER_CRITICAL();
        if (queue->count != 0) {
            queue->count--;
        }
        if (queue->count == 0) {
            queue->empty = 1;
            queue->write_index = 0;
            queue->read_index = 0;
        }
        OS_EXIT_CRITICAL();
    } else {
        OS_ENTER_CRITICAL();
        queue->empty = 1;
        queue->write_index = 0;
        queue->read_index = 0;
        queue->count = 0;
        OS_EXIT_CRITICAL();
    }
    return 0;
}

/* Waits until the port's transmitter is idle, queues length bytes and sends the first. */
void serial_send(int port, uchar *data, ushort length) {
    SerialTxQueue *queue = &serial_tx[port];
    while (queue->busy == 1) {
    }
    queue->busy = 1;
    queue->length = length;
    queue->index = 0;
    queue->buffer = data;
    if (port == 0)
        serial_usart1_send_next();
    else if (port == 1)
        serial_usart2_send_next();
    else if (port == 2)
        serial_usart3_send_next();
    else if (port == 3)
        serial_uart4_send_next();
}

/* Combines two ASCII hex digits, either case, into a byte; any other character counts as 0. */
int ascii_hex_to_byte(int high, int low) {
    int value;
    int digit;
    digit = high - '0';
    if ((uint)digit <= 9) {
        value = (uchar)digit;
    } else {
        if ((uint)(high - 'A') <= 5) {
            value = (uchar)(high - ('A' - 10));
        } else {
            if ((uint)(high - 'a') <= 5) {
                value = (uchar)(high - ('a' - 10));
            } else {
                value = 0;
            }
        }
    }
    value = ((uint)value << 0x1c) >> 0x18;
    if ((uint)(low - '0') <= 9) {
        return (uchar)(value + low - '0');
    }
    if ((uint)(low - 'A') <= 5) {
        return (uchar)(value + low - ('A' - 10));
    }
    if ((uint)(low - 'a') > 5)
        return value;
    return (uchar)(value + low - ('a' - 10));
}

/* Writes a byte as two upper-case ASCII hex digits. */
void ascii_byte_to_hex(int value, uchar *out) {
    int low;
    uint high;
    high = (uint)value >> 4;
    if (high < 0xa) {
        out[0] = high + '0';
    } else {
        out[0] = high + ('A' - 10);
    }
    low = value & 0xf;
    if ((uint)low < 0xa) {
        out[1] = low + '0';
        return;
    }
    out[1] = low + ('A' - 10);
}

/* Checks an ASCII frame's length field: its top digit must be the negated sum of the three
   length digits, modulo 16. Returns 1 when it is. */
int ascii_length_checksum_valid(uchar *frame) {
    int low;
    int high;
    high = ascii_hex_to_byte(frame[9], frame[10]);
    low = ascii_hex_to_byte(frame[11], frame[12]);
    if ((~(low + (high + ((uint)low >> 4))) + 1 & 0xf) == (uint)high >> 4) {
        return 1;
    }
    return 0;
}

/* match: the two inline helpers are matching forms (vendor-idioms.md); the word helper
   reads the four checksum digits, the other negates the sum. */
static __inline ushort serial_ascii_word(int p, uint mask) {
    uint hi = (ascii_hex_to_byte(*(uchar *)(p + 1), *(uchar *)(p + 2)) << 8) & mask;
    uint lo = ascii_hex_to_byte(*(uchar *)(p + 3), *(uchar *)(p + 4));
    return hi + lo;
}
static __inline ushort serial_negate_sum(ushort sum) {
    return (ushort)(~sum + 1);
}
/* Checks an ASCII frame's checksum: the negated 16-bit sum of every character after '~'
   must equal the four hex digits that follow the INFO. Returns 1 when it does. */
int ascii_frame_checksum_valid(int frame) {
    uint length, high;
    ushort received;
    ushort sum = 0, checksum, i;
    uint mask;
    uint pointer_diagnostic = 0;

    high = ascii_hex_to_byte(*(uchar *)(frame + 9), *(uchar *)(frame + 10));
    mask = 0xffff;
    high = (high << 8) & mask;
    length =
        ((high + ascii_hex_to_byte(*(uchar *)(frame + 11), *(uchar *)(frame + 12))) & 0xfff) + 12;
    for (i = 1; i <= length; i++)
        sum += *(uchar *)(frame + i);
    checksum = serial_negate_sum(sum);
    /* match: the reads above need at least 17 frame bytes, so frame + (sum & 1) is in
     bounds. This unread comparison keeps the sum's register lifetime as matched. */
    pointer_diagnostic = (uint)((((uchar *)frame) + (sum & 1)) != update_mode);
    (void)pointer_diagnostic;
    received = serial_ascii_word(frame + length, mask);
    if (checksum == received)
        return 1;
    return 0;
}

extern uchar hex_pair_buffer[];

/* Writes the INFO length (12 bits) and its checksum digit as the frame's four-digit length
   field. */
void ascii_set_length_field(uchar *frame, int length) {
    ushort field;
    field = length & 0xfff |
            (uchar)(~(length + ((uint)length >> 4) + ((uint)length >> 8) & 0xf) + 1) << 0xc;
    ascii_byte_to_hex((uint)field >> 8, &hex_pair_buffer[0]);
    frame[9] = hex_pair_buffer[0];
    frame[10] = hex_pair_buffer[1];
    ascii_byte_to_hex((uchar)field, &hex_pair_buffer[0]);
    frame[11] = hex_pair_buffer[0];
    frame[12] = hex_pair_buffer[1];
}

/* Writes the return code as two hex digits at frame bytes 7 and 8. */
void ascii_set_rtn_field(uchar *frame, int rtn) {

    ascii_byte_to_hex(rtn, &hex_pair_buffer[0]);
    frame[7] = hex_pair_buffer[0];
    frame[8] = hex_pair_buffer[1];
}
