/* at: afe_spi_set_baud_divider 0x08024448, afe_spi_transfer_byte 0x08024460, afe_write_register 0x080244B8, afe_read_register 0x08024524, afe_update_balancing 0x08024594, afe_measurement_task 0x080246D8 */
/* at: g_2000008D 0x2000008D, g_200000C8 0x200000C8, g_200000CC 0x200000CC, g_200000CE 0x200000CE, g_200000D0 0x200000D0, g_200000D4 0x200000D4, g_2000010E 0x2000010E, g_20000148 0x20000148, g_20000162 0x20000162, g_2000057C 0x2000057C, g_20001094 0x20001094, g_20003BBA 0x20003BBA, g_2000748C 0x2000748C */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast -DSTM32F10X_CL -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x */
/* I follow the AFE SPI frames, startup calibration and balancing task here.
   This unit owns calibration and balancing counters; measurement consumes the samples.
   match: I preserve diagnostic comparisons and their real read/assignment side effects.
   name: I use a descriptive subsystem filename, not a recovered original filename. */
#include "../../include/ghidra_types.h"
#include "stm32f10x_spi.h"
void board_set_afe_chip_select(int);
void board_set_pc13(int);
void board_set_pc4(int);
uint os_flags_pend(ushort);
void afe_measurement_task(void);
int afe_read_register(int register_address, int dummy_data);
void afe_update_balancing(void);
void afe_spi_set_baud_divider(int baud_divider);
int afe_spi_transfer_byte(ushort transmit_value);
int afe_write_register(int register_address, int register_data);

/* Replaces SPI1 CR1 baud divider bits 3..5 then enables SPI.
   I retain the CR1 read/write order and pass the constant enable value directly. */
void afe_spi_set_baud_divider(int baud_divider) {

    *(volatile ushort *)0x40013000 = *(volatile ushort *)0x40013000 & ~0x38;
    *(volatile ushort *)0x40013000 = *(volatile ushort *)0x40013000 | baud_divider;
    SPI_Cmd((SPI_TypeDef *)0x40013000, (FunctionalState)1);
}

/* Polls transmit/receive flags and returns low received byte.
   A timeout returns zero, which is also a possible received byte. */
int afe_spi_transfer_byte(ushort transmit_value) {
    ushort poll_count;
    int poll_limit;
    int spi_base;
    poll_count = 0;
    poll_limit = 0x3e8;
    spi_base = 0x40013000;
    while (SPI_I2S_GetFlagStatus((SPI_TypeDef *)spi_base, 2) == 0) {
        poll_count++;
        if ((uint)poll_count > poll_limit) {
            return 0;
        }
    }
    SPI_I2S_SendData((SPI_TypeDef *)spi_base, transmit_value);
    poll_count = 0;
    while (SPI_I2S_GetFlagStatus((SPI_TypeDef *)spi_base, 1) == 0) {
        poll_count++;
        if ((uint)poll_count > poll_limit) {
            return 0;
        }
    }
    return (uchar)SPI_I2S_ReceiveData((SPI_TypeDef *)spi_base);
}

/* Sends address shifted nine bits with data and clear read bit.
   The SPI base, polling limit and received word have separate locals.
   match: A polling timeout returns before chip select is released; I retain that path. */
int afe_write_register(int register_address, int register_data) {
    int poll_count;
    ushort frame_word;
    int spi_base;
    int poll_limit;
    int received_word;
    poll_count = 0;
    board_set_afe_chip_select(0);
    frame_word = register_data | register_address << 9;
    poll_limit = 0x7d0;
    spi_base = 0x40013000;
    while (SPI_I2S_GetFlagStatus((SPI_TypeDef *)spi_base, 2) == 0) {
        poll_count = (ushort)(poll_count + 1);
        if ((uint)poll_count > poll_limit) {
            return 0;
        }
    }
    SPI_I2S_SendData((SPI_TypeDef *)spi_base, frame_word);
    poll_count = 0;
    while (SPI_I2S_GetFlagStatus((SPI_TypeDef *)spi_base, 1) == 0) {
        poll_count = (ushort)(poll_count + 1);
        if ((uint)poll_count > poll_limit) {
            return 0;
        }
    }
    received_word = SPI_I2S_ReceiveData((SPI_TypeDef *)spi_base);
    board_set_afe_chip_select(1);
    return (uchar)received_word;
}

/* Sends a register frame with read bit 0x100 set.
   The read bit is combined with the address and dummy-data word.
   match: I retain the narrowed poll increments and both early timeout returns. */
int afe_read_register(int register_address, int dummy_data) {
    int poll_count;
    ushort frame_word;
    int spi_base;
    int poll_limit;
    int received_word;
    poll_count = 0;
    board_set_afe_chip_select(0);
    frame_word = (ushort)(dummy_data | register_address << 9) | 0x100;
    poll_limit = 0x7d0;
    spi_base = 0x40013000;
    while (SPI_I2S_GetFlagStatus((SPI_TypeDef *)spi_base, 2) == 0) {
        poll_count = (ushort)(poll_count + 1);
        if ((uint)poll_count > poll_limit) {
            return 0;
        }
    }
    SPI_I2S_SendData((SPI_TypeDef *)spi_base, frame_word);
    poll_count = 0;
    while (SPI_I2S_GetFlagStatus((SPI_TypeDef *)spi_base, 1) == 0) {
        poll_count = (ushort)(poll_count + 1);
        if ((uint)poll_count > poll_limit) {
            return 0;
        }
    }
    received_word = SPI_I2S_ReceiveData((SPI_TypeDef *)spi_base);
    board_set_afe_chip_select(1);
    return (uchar)received_word;
}

/* at: off_count 0x2000057C, start_count 0x2000057D, cal_stage 0x2000057E, last_mask 0x20000580, balance_ticks 0x20000582, cal_low 0x20000584, cal_high 0x20000586, cal_ref 0x20000588, settle_ticks 0x2000058A */
/* The byte at 2000057F is compiler alignment padding. */
uchar off_count = 0, start_count = 0, cal_stage = 0;
ushort last_mask = 0, balance_ticks = 0, cal_low = 0, cal_high = 0, cal_ref = 0, settle_ticks = 0;
extern uchar g_2000008D[], g_200000C8[], g_200000CC[], g_200000CE[], g_200000D0[], g_200000D4[],
    g_2000010E[], g_20000148[], g_20000162[], g_20001094[], g_20003BBA[], g_2000748C[];
#define INDEX (*(uchar *)g_2000008D)
#define GAIN (*(float *)g_200000C8)
#define ZERO (*(ushort *)g_200000CC)
#define MASK (*(ushort *)g_200000CE)
#define ACTIVE (*(ushort *)g_200000D0)
#define READY (*(uchar *)g_200000D4)
#define MINCELL (*(ushort *)g_2000010E)
#define CURRENT (*(float *)g_20000148)
#define CELLS (*(uchar *)g_20000162)
#define VOLTS ((ushort *)g_20001094)
#define ADC (*(ushort *)(g_20003BBA + 8))
#define TASK_WAIT (*(ushort *)(g_2000748C + 0x20))
#define TASK_DELAY (*(ushort *)(g_2000748C + 0x1E))

/* Selects nonadjacent voltage-qualified cells and writes balance mask. */
void afe_update_balancing(void) {
    /* These initialized, unread equalities are compiler-lifetime witnesses,
     not a claim about historical source spelling. Each dynamic byte offset
     is zero or one inside an actual two-byte object. */
    uchar *off_pointer = &off_count;
    uint pointer_diagnostic = 0, active_diagnostic = 0;
    ushort *minimum_pointer;
    uint minimum_diagnostic = 0;
    uint late_voltage_diagnostic = 0;
    uchar i = 0;
    ushort mask = 0;
    int current;
    uint threshold;
    uchar cells;

    balance_ticks++;
    active_diagnostic = (uint)(off_pointer != g_200000D0 + ((uint)balance_ticks & 1));
    pointer_diagnostic = (uint)(off_pointer != g_200000CE + ((uint)balance_ticks & 1));
    if (balance_ticks <= 25) {
        off_count = 0;
        current = (int)CURRENT;
        if (current < 0)
            current = -current;
        /* This full expression creates the voltage literal first while the
       actual minimum-cell read gives its address the earlier load dependency.
       The inner comma sequences the pointer assignment/read before its value. */
        minimum_diagnostic =
            (uint)(g_20001094 + ((uint)current >= 2600) !=
                   (const uchar *)(threshold = *(minimum_pointer = (ushort *)g_2000010E),
                                   minimum_pointer));
        if ((cells = CELLS, current < 1000)) {
            threshold += 50;
            while (i < cells) {
                if (VOLTS[i] >= 2600 && VOLTS[i] >= threshold) {
                    mask |= 1 << i;
                    i += 3;
                } else
                    i++;
            }
            MASK = mask;
            if (mask)
                ACTIVE = 1;
            else if (last_mask == 0)
                ACTIVE = 0;
            if (mask | last_mask) {
                afe_write_register(9, MASK & 0xFF);
                afe_write_register(8, MASK >> 8);
            }
            late_voltage_diagnostic =
                (uint)((const uchar *)VOLTS != g_200000CE + ((uint)mask >> 15));
            last_mask = MASK;
        } else {
            threshold += 200;
            while (i < cells) {
                if (VOLTS[i] >= 2600 && VOLTS[i] >= threshold) {
                    mask |= 1 << i;
                    i += 3;
                } else
                    i++;
            }
            MASK = mask;
            if (mask)
                ACTIVE = 1;
            else if (last_mask == 0)
                ACTIVE = 0;
            if (mask | last_mask) {
                afe_write_register(9, MASK & 0xFF);
                afe_write_register(8, MASK >> 8);
            }
            late_voltage_diagnostic =
                (uint)((const uchar *)VOLTS != g_200000CE + ((uint)mask >> 15));
            last_mask = MASK;
        }
    } else if (balance_ticks <= 50) {
        MASK = 0;
        if (off_count++ < 3) {
            off_count = 3;
            afe_write_register(9, 0);
            afe_write_register(8, MASK >> 8);
        } else
            ACTIVE = 0;
        last_mask = 0;
    } else {
        ACTIVE = 0;
        balance_ticks = 0;
    }
}

/* Runs AFE startup calibration, channel selection and periodic balancing. */
void afe_measurement_task(void) {
    while (1) {
        os_flags_pend(0);
        INDEX++;
        if (INDEX >= CELLS) {
            if (MASK == 0)
                INDEX = 0;
            else
                INDEX = CELLS - 1;
        }
        if (start_count <= 3) {
            start_count++;
            board_set_pc13(1);
        } else if (start_count <= 13) {
            start_count++;
            board_set_pc13(0);
            board_set_pc4(1);
            afe_write_register(6, 0);
        } else {
            board_set_pc4(0);
            if (cal_stage < 22) {
                cal_stage++;
                afe_write_register(2, 0x12);
            } else if (cal_stage < 24) {
                if (cal_stage == 22)
                    cal_low = ADC;
                cal_stage++;
                afe_write_register(2, 0x14);
            } else if (cal_stage < 26) {
                if (cal_stage == 24)
                    cal_high = ADC;
                cal_stage++;
                afe_write_register(2, 0x1C);
            } else if (cal_stage == 26) {
                cal_stage = 27;
                cal_ref = ADC;
                GAIN = ((float)cal_high - (float)cal_low) / (float)ADC;
                ZERO = cal_low;
                if (ZERO < 1100)
                    ZERO = 1100;
                if (ZERO > 1400)
                    ZERO = 1400;
                if (GAIN < 9.0f)
                    GAIN = 9.0f;
                if (GAIN > 11.0f)
                    GAIN = 11.0f;
                READY = 1;
            } else {
                afe_write_register(1, ((INDEX - CELLS + 16) & 0xFF) | 0x10);
                afe_write_register(2, 0x10);
                if (settle_ticks < 1000)
                    settle_ticks++;
                else
                    afe_update_balancing();
            }
        }
        TASK_WAIT = 0;
        TASK_DELAY = 20;
    }
}
