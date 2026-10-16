// clang-format off
/* at: sys_runtime_vars_init 0x080234B4, uart_init_all 0x08023530, gpio_pc9_set 0x0802382C, gpio_pc9_init 0x0802384A, adc_dma_init 0x08023878, tim3_init 0x08023A76, cal_gain_validate 0x08023AB8, cal_gains_validate_all 0x08023ADA, grid_load_country_bytes 0x08023B5C, grid_load_country_limits 0x08023B92, grid_country_from_setting 0x08023EC4, cfg_defaults_grid 0x08023F38, cfg_load_defaults 0x08024064, cfg_load_all 0x08024368, gpio_pc3_set 0x080246A4, gpio_pc3_init 0x080246BE, buzzer_pin_set 0x080246EA, buzzer_gpio_init 0x08024704, i2c_gpio_init 0x08024730, gpio_pd2_init 0x08024758, led_fault_gpio_init 0x0802477E, led_set 0x080247A6, led_gpio_init 0x080247E8, panel_gpio_init 0x0802480C, board_init 0x08024896, hex_nibble_to_ascii 0x080248E4, cfg_limit_check 0x080248FA, rs485_modbus_de_set 0x08024900, rs485_meter_de_set 0x08024920, led_fault_set 0x08024940, tim4_init 0x0802495E */
/* at: g_2000018C 0x2000018C, grid_country_byte_a 0x2000019A, grid_country_byte_b 0x2000019B, g_2000019E 0x2000019E, grid_table_set 0x200001A3, g_200001A4 0x200001A4, eeprom_load_failed 0x200001A6, eeprom_load_ok 0x200001A7, hr31_value 0x200001AC, hr32_value 0x200001AD, inv_run_mode 0x200001AE, g_200001AF 0x200001AF, g_200001B0 0x200001B0, g_200001B2 0x200001B2, g_200001D2 0x200001D2, g_200001D6 0x200001D6, output_enabled 0x200001D8, pv1_voltage 0x200001E0, dsp_rx_pages_seen 0x2000022E, rs485_modbus_de_hold 0x20000242, rs485_meter_de_hold 0x20000243, eeprom_dirty 0x20000260, g_20000261 0x20000261, g_20000262 0x20000262, g_20000263 0x20000263, g_20000264 0x20000264, g_20000265 0x20000265, g_20000267 0x20000267, fault_recover_window 0x20000288, g_2000028A 0x2000028A, g_2000028B 0x2000028B, g_20000297 0x20000297, usb_start_delay 0x20000306, board_init_done 0x20000396, adc_dma_buf 0x200006FA, fault_log 0x200019B7, cfg_settings 0x20001AAE, cal_gains 0x20001B81, energy_totals 0x20001BC5, cfg_block6 0x20001C82, cfg_block7 0x20001D7C, cfg_block8 0x20001E76, dev11_data 0x20002028 */
/* opts: --cpu Cortex-M3 -O0 --loose_implicit_cast */
// clang-format on
/* board.c: board bring-up and the settings defaults. It sets up the GPIO, the four UARTs, the
   ADC with DMA, TIM3 (the ADC trigger), the LEDs and the buzzer; it loads every EEPROM block at
   boot, falling back to defaults block by block; and it holds the per-country grid limit tables.

   Entry points: board_init (from main); cfg_load_defaults (also from the Modbus write handlers
   for HR2 and HR163); grid_load_country_limits (also from HR2, HR28 and HR163 writes);
   calib_table_defaults lives in monitor.c. Small drivers used from everywhere: led_set,
   led_fault_set, buzzer_pin_set, gpio_pc3_set, gpio_pc9_set, rs485_modbus_de_set,
   rs485_meter_de_set. Dead: hex_nibble_to_ascii, tim4_init.
   Data: the four grid tables grid_limits_tbl..tbl4 (const, flash); it defines most of the RAM
   blocks the settings live in (cfg_settings, cal_gains, energy_totals, cfg_block6/7/8,
   fault_log; see settings.h, energy.h) and a set of run-time bytes zeroed by
   sys_runtime_vars_init.
   Neighbours: drivers/eeprom.c and drivers/i2c.c (the EEPROM it loads), monitor.c
   (calib_table_defaults, buzzer_off), the Modbus slave (the HR handlers that call back in here).

   Pins, as the code drives them (what each pin does on the board is inference from the names):
     PA9/PA10  USART1 aux MCU link          PA2/PA3   USART2 meter RS485, DE on PA4
     PB10/PB11 USART3 Modbus RS485, DE PB4  PC10/PC11 UART4 DSP link
     PB0, PB1  status LEDs (led_set 2, 1)   PB9       fault LED
     PB5       buzzer                       PB6/PB7   I2C SCL/SDA (EEPROM), PB8 EEPROM WP
     PB14/PB15 battery charge/discharge     PC6, PC7, PC9, PA8 panel indicators
     PC3, PC9  outputs set by gpio_pc3_set, gpio_pc9_set (purpose not known)
     PD2       input, sampled in the ADC interrupt (a mains-cycle sync, inference)
     ADC: PA0, PA1, PA6, PC0, PC1, PC4, PC5: 8 conversions a scan by DMA

   name: descriptive (map/filenames.tsv). The original file is not known. */
#include "../include/ghidra_types.h"
#include "../include/stdperiph_types.h"
#include "../include/board.h"
#include "../include/dsp_link.h"
#include "../include/eeprom.h"
#include "../include/energy.h"
#include "../include/faults.h"
#include "../include/measure.h"
#include "../include/meter.h"
#include "../include/modbus.h"
#include "../include/settings.h"
#include "../include/stdperiph.h"
#include "../include/sys.h"
#include "../include/uart.h"
#include "../include/usb.h"

/* The grid settings tables, one row per country code (0 to 28): voltage and frequency trip
   limits and times. The first table has a second set of 29 rows, all zero. */
const ushort grid_limits_tbl[2][29][12] = {
    {{1860, 2630, 4753, 5147, 8, 8, 6, 6, 30, 30, 2000, 50},
     {1860, 2630, 4753, 5147, 8, 8, 6, 6, 30, 30, 9000, 50},
     {1955, 2760, 4700, 5200, 4, 4, 3, 3, 70, 70, 2000, 3},
     {1800, 2650, 4700, 5200, 500, 50, 50, 7, 80, 80, 9000, 50},
     {1955, 2530, 4980, 5020, 74, 149, 4, 4, 30, 300, 1000, 30},
     {2160, 2440, 4703, 5097, 70, 70, 18, 18, 30, 200, 2000, 50},
     {1900, 2600, 4953, 5047, 20, 20, 20, 20, 30, 200, 2000, 50},
     {1860, 2630, 4749, 5151, 3, 3, 3, 3, 30, 180, 9000, 50},
     {1840, 2622, 4750, 5200, 125, 50, 1000, 25, 30, 30, 2000, 60},
     {1840, 2530, 4750, 5150, 100, 100, 100, 100, 70, 70, 2000, 3},
     {1960, 2520, 4953, 5018, 98, 98, 50, 50, 30, 60, 2000, 50},
     {1955, 2645, 4750, 5200, 65, 8, 20, 20, 70, 70, 2000, 3},
     {1840, 2622, 4750, 5200, 125, 50, 1000, 25, 30, 30, 2000, 60},
     {1840, 2530, 4750, 5150, 4, 100, 3, 3, 60, 60, 2000, 50},
     {1940, 2420, 4950, 5050, 100, 50, 8, 8, 30, 120, 2000, 50},
     {1995, 2530, 4800, 5200, 150, 25, 25, 50, 30, 30, 2000, 60},
     {1995, 2530, 4800, 5200, 150, 25, 25, 50, 30, 30, 2000, 60},
     {1955, 2530, 4800, 5200, 150, 25, 25, 50, 30, 30, 2000, 60},
     {1840, 2645, 4700, 5200, 500, 100, 10, 200, 70, 70, 2000, 3},
     {1800, 2650, 4500, 5500, 500, 50, 50, 7, 80, 80, 9000, 50},
     {1800, 2650, 4700, 5200, 500, 50, 50, 7, 80, 80, 9000, 50},
     {1800, 2650, 4500, 5500, 500, 50, 250, 7, 80, 80, 9000, 50},
     {1955, 2760, 4700, 5200, 4, 4, 3, 3, 70, 70, 2000, 3},
     {1955, 2760, 4700, 5200, 4, 4, 3, 3, 70, 70, 2000, 3},
     {1955, 2530, 4750, 5150, 2500, 3000, 8, 8, 180, 70, 2000, 3},
     {1955, 2530, 4750, 5200, 60, 140, 15, 15, 70, 70, 2000, 3},
     {1955, 2760, 4700, 5200, 4, 4, 3, 3, 70, 70, 2000, 3},
     {1955, 2760, 4700, 5200, 4, 4, 3, 3, 70, 70, 2000, 3},
     {1955, 2530, 4750, 5150, 2500, 3000, 8, 8, 180, 70, 2000, 3}}};
/* Second grid table, one row of 10 per country: limits copied to the "_in" set of settings
   (HR71-78) and two more words (byte 0x12 is grid_country_byte_b, sent to the DSP on page 3). */
const ushort grid_limits_tbl2[29][10] = {{1860, 2630, 4753, 5147, 8, 8, 6, 6, 2000, 50},
                                         {1860, 2630, 4753, 5147, 8, 8, 6, 6, 9000, 50},
                                         {460, 2990, 4700, 5200, 4, 4, 3, 3, 2000, 50},
                                         {700, 2750, 4700, 5200, 50, 7, 50, 7, 2000, 50},
                                         {345, 2645, 4750, 5150, 9, 9, 4, 4, 1000, 7},
                                         {2160, 2530, 4703, 5097, 70, 8, 18, 18, 2000, 50},
                                         {1900, 2600, 4953, 5047, 20, 20, 20, 20, 2000, 50},
                                         {1860, 2630, 4749, 5151, 3, 3, 3, 3, 2000, 50},
                                         {1800, 2730, 4700, 5200, 122, 25, 25, 25, 2000, 50},
                                         {1610, 2530, 4750, 5150, 8, 100, 100, 100, 2000, 50},
                                         {1160, 3090, 4803, 5047, 3, 1, 8, 8, 2000, 50},
                                         {1955, 2645, 4750, 5200, 65, 8, 20, 20, 2000, 50},
                                         {1800, 2730, 4700, 5200, 122, 25, 25, 25, 2000, 50},
                                         {1840, 2645, 4750, 5150, 4, 4, 3, 3, 2000, 50},
                                         {1100, 2640, 4950, 5050, 8, 8, 8, 8, 2000, 50},
                                         {1380, 2530, 4800, 5200, 150, 25, 25, 50, 2000, 50},
                                         {1380, 2530, 4800, 5200, 150, 25, 25, 50, 2000, 50},
                                         {1380, 2730, 4700, 5200, 100, 25, 25, 25, 2000, 50},
                                         {1150, 2760, 4700, 5200, 10, 8, 10, 200, 2000, 50},
                                         {700, 2750, 4500, 5500, 50, 7, 50, 7, 2000, 50},
                                         {700, 2750, 4700, 5200, 50, 7, 50, 7, 2000, 50},
                                         {700, 2750, 4500, 5500, 50, 7, 250, 7, 2000, 50},
                                         {460, 2990, 4700, 5200, 4, 4, 3, 3, 2000, 50},
                                         {460, 2990, 4700, 5200, 4, 4, 3, 3, 2000, 50},
                                         {1955, 2645, 4750, 5150, 2500, 8, 8, 8, 2000, 50},
                                         {1955, 2645, 4750, 5150, 60, 5, 15, 15, 2000, 50},
                                         {460, 2990, 4700, 5200, 4, 4, 3, 3, 2000, 50},
                                         {460, 2990, 4700, 5200, 4, 4, 3, 3, 2000, 50},
                                         {1955, 2645, 4760, 5150, 2500, 8, 8, 8, 2000, 50}};
/* Third grid table: four limits per country copied to HR79-82 (the "_c" set; HR81/82 get HR28 x
   1000 added like the others). */
const ushort grid_limits_tbl3[29][4] = {
    {1860, 2630, 4753, 5147}, {1860, 2630, 4753, 5147}, {1954, 2530, 4950, 5020},
    {2050, 2530, 4750, 5015}, {1978, 2507, 4990, 5010}, {2160, 2440, 4703, 5097},
    {1900, 2600, 4953, 5047}, {1960, 2525, 4750, 5016}, {1880, 2580, 4760, 5190},
    {1880, 2490, 4760, 5120}, {1960, 2520, 4953, 5018}, {2000, 2490, 4760, 5170},
    {1880, 2580, 4760, 5190}, {1970, 2510, 4753, 5047}, {1940, 2420, 4950, 5050},
    {2035, 2490, 4810, 5190}, {2035, 2490, 4810, 5190}, {1880, 2580, 4760, 5190},
    {1978, 2507, 4710, 5040}, {2050, 2530, 4750, 5015}, {2050, 2530, 4750, 5015},
    {2050, 2530, 4750, 5015}, {1954, 2530, 4950, 5020}, {1954, 2530, 4950, 5020},
    {1954, 2530, 4750, 5020}, {1980, 2500, 4770, 5030}, {1954, 2530, 4950, 5020},
    {1954, 2530, 4950, 5020}, {1954, 2530, 4750, 5050}};
/* HR83 (10-minute average voltage limit) per country. */
const ushort grid_limits_tbl4[29] = {2530, 2530, 2530, 2580, 2530, 2530, 2530, 2530, 2622, 2530,
                                     2530, 2530, 2622, 2530, 2530, 2530, 2530, 2622, 2530, 2490,
                                     2580, 2580, 2530, 2530, 2530, 2530, 2530, 2530, 2530};

/* Puts the run-time state in its start values: buzzer off, run mode 0, counters and flags
   cleared, and the 0x11 device's command block reset (power command 100, not pending). The
   uncommented g_ bytes are cleared here and used in a few places I could not tie to a meaning. */
void sys_runtime_vars_init() {

    buzzer_off();
    g_200001AF = 0;
    pv1_voltage = 0;
    pv1_voltage = 0;
    g_200001D2 = 0;
    inv_run_mode = 0;
    g_20000265 = 0;
    g_20000264 = 0;
    fault_recover_window = 0;
    g_20000261 = 0;
    g_20000267[0] = 0;
    g_20000262 = 0;
    g_20000263 = 0;
    g_2000019E = 0;
    g_200001D6 = 0;
    g_200001B0 = 0xff;
    g_200001B2 = 0;
    dsp_rx_pages_seen = 0;
    g_2000018C = 0;
    output_enabled = 0;
    g_20000297 = 0;
    g_2000028A = 0;
    g_2000028B = 0;
    g_200001A4 = 0;
    usb_start_delay = 0;
    dev11_data.cmd_pending = 0;
    dev11_data.cmd = 0x64;
    dev11_data.fresh = 0;
}

/* Sets up the four ports at 9600 baud with receive and transmit-complete interrupts, and resets
   their receive queues to 150 bytes. USART1 (aux MCU) and USART3 (Modbus slave) and UART4 (DSP)
   are 8N1. USART2 (meters) is 8N1 when HR47 meter_type is 1 and 9 bits with even parity (8E1)
   otherwise. The RS485 driver-enable pins PA4 and PB4 are push-pull outputs. */
void uart_init_all() {
    GPIO_InitTypeDef gpio_config;
    USART_InitTypeDef uart_config;

    RCC_APB2PeriphClockCmd(0x4000, 1);
    RCC_APB2PeriphClockCmd(4, 1);
    gpio_config.GPIO_Pin = 0x200;
    gpio_config.GPIO_Mode = 0x18;
    gpio_config.GPIO_Speed = 3;
    GPIO_Init(GPIOA_BASE, &gpio_config);
    gpio_config.GPIO_Pin = 0x400;
    gpio_config.GPIO_Mode = 4;
    gpio_config.GPIO_Speed = 3;
    GPIO_Init(GPIOA_BASE, &gpio_config);
    uart_config.USART_BaudRate = 0x2580;
    uart_config.USART_WordLength = 0;
    uart_config.USART_StopBits = 0;
    uart_config.USART_Parity = 0;
    uart_config.USART_HardwareFlowControl = 0;
    uart_config.USART_Mode = 0xc;
    USART_Init(USART1_BASE, &uart_config);
    USART_ITConfig(USART1_BASE, 0x525, 1);
    USART_ITConfig(USART1_BASE, 0x626, 1);
    USART_Cmd(USART1_BASE, 1);
    USART_ClearITPendingBit(USART1_BASE, 0x626);
    uart_rx_reset(0, 0x96);
    RCC_APB2PeriphClockCmd(4, 1);
    gpio_config.GPIO_Pin = 0x10;
    gpio_config.GPIO_Speed = 3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init(GPIOA_BASE, &gpio_config);
    RCC_APB1PeriphClockCmd(0x20000, 1);
    RCC_APB2PeriphClockCmd(4, 1);
    gpio_config.GPIO_Pin = 4;
    gpio_config.GPIO_Mode = 0x18;
    gpio_config.GPIO_Speed = 3;
    GPIO_Init(GPIOA_BASE, &gpio_config);
    gpio_config.GPIO_Pin = 8;
    gpio_config.GPIO_Mode = 4;
    gpio_config.GPIO_Speed = 3;
    GPIO_Init(GPIOA_BASE, &gpio_config);
    if (cfg_settings.meter_type == 1) {
        uart_config.USART_BaudRate = 0x2580;
        uart_config.USART_WordLength = 0;
        uart_config.USART_StopBits = 0;
        uart_config.USART_Parity = 0;
        uart_config.USART_HardwareFlowControl = 0;
        uart_config.USART_Mode = 0xc;
    } else {
        uart_config.USART_BaudRate = 0x2580;
        uart_config.USART_WordLength = 0x1000;
        uart_config.USART_StopBits = 0;
        uart_config.USART_Parity = 0x400;
        uart_config.USART_HardwareFlowControl = 0;
        uart_config.USART_Mode = 0xc;
    }
    USART_Init(USART2_BASE, &uart_config);
    USART_ITConfig(USART2_BASE, 0x525, 1);
    USART_ITConfig(USART2_BASE, 0x626, 1);
    USART_Cmd(USART2_BASE, 1);
    USART_ClearITPendingBit(USART2_BASE, 0x626);
    uart_rx_reset(1, 0x96);
    RCC_APB2PeriphClockCmd(8, 1);
    gpio_config.GPIO_Pin = 4;
    gpio_config.GPIO_Speed = 3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init(GPIOB_BASE, &gpio_config);
    RCC_APB1PeriphClockCmd(0x40000, 1);
    RCC_APB2PeriphClockCmd(8, 1);
    gpio_config.GPIO_Pin = 0x400;
    gpio_config.GPIO_Mode = 0x18;
    gpio_config.GPIO_Speed = 3;
    GPIO_Init(GPIOB_BASE, &gpio_config);
    gpio_config.GPIO_Pin = 0x800;
    gpio_config.GPIO_Mode = 4;
    gpio_config.GPIO_Speed = 3;
    GPIO_Init(GPIOB_BASE, &gpio_config);
    uart_config.USART_BaudRate = 0x2580;
    uart_config.USART_WordLength = 0;
    uart_config.USART_StopBits = 0;
    uart_config.USART_Parity = 0;
    uart_config.USART_HardwareFlowControl = 0;
    uart_config.USART_Mode = 0xc;
    USART_Init(USART3_BASE, &uart_config);
    USART_ITConfig(USART3_BASE, 0x525, 1);
    USART_ITConfig(USART3_BASE, 0x626, 1);
    USART_Cmd(USART3_BASE, 1);
    USART_ClearITPendingBit(USART3_BASE, 0x626);
    uart_rx_reset(2, 0x96);
    RCC_APB1PeriphClockCmd(0x80000, 1);
    RCC_APB2PeriphClockCmd(0x10, 1);
    gpio_config.GPIO_Pin = 0x400;
    gpio_config.GPIO_Mode = 0x18;
    gpio_config.GPIO_Speed = 3;
    GPIO_Init(GPIOC_BASE, &gpio_config);
    gpio_config.GPIO_Pin = 0x800;
    gpio_config.GPIO_Mode = 4;
    gpio_config.GPIO_Speed = 3;
    GPIO_Init(GPIOC_BASE, &gpio_config);
    uart_config.USART_BaudRate = 0x2580;
    uart_config.USART_WordLength = 0;
    uart_config.USART_StopBits = 0;
    uart_config.USART_Parity = 0;
    uart_config.USART_HardwareFlowControl = 0;
    uart_config.USART_Mode = 0xc;
    USART_Init(UART4_BASE, &uart_config);
    USART_ITConfig(UART4_BASE, 0x525, 1);
    USART_ITConfig(UART4_BASE, 0x626, 1);
    USART_Cmd(UART4_BASE, 1);
    USART_ClearITPendingBit(UART4_BASE, 0x626);
    uart_rx_reset(3, 0x96);
}

/* Drives PC9. panel_led_pc9_set (panel_led.c) drives the same pin; sys_tick_1s sets it to 1
   every pass after the first 3 s and status_outputs_update then sets it from the load power. */
void gpio_pc9_set(int level) {

    if (level == 0) {
        GPIO_ResetBits(GPIOC_BASE, 0x200);
    } else {
        GPIO_SetBits(GPIOC_BASE, 0x200);
    }
    return;
}

/* PC9 as a push-pull output, low. */
void gpio_pc9_init() {
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(0x10, 1);
    gpio_config.GPIO_Pin = 0x200;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((uint *)GPIOC_BASE, &gpio_config);
    gpio_pc9_set(0);
    return;
}

/* ADC1 scans 8 channels into adc_dma_buf by DMA1 channel 1 (circular, 16-bit), started by
   software (TIM3_IRQHandler), with the end-of-conversion interrupt on, then calibrates. ADC
   clock PCLK2/6. Scan order: ch0, ch1, (rank 3 unset), ch6, ch10, ch11, ch14, ch15; the
   temperature sensor is enabled but is not in the scan. ADC1_2_IRQHandler reads the buffer. */
void adc_dma_init() {
    ADC_InitTypeDef ADC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    RCC_APB2PeriphClockCmd(0x201, 1);
    RCC_ADCCLKConfig(0x8000);
    RCC_AHBPeriphClockCmd(1, 1);
    RCC_APB2PeriphClockCmd(0x10, 1);
    GPIO_InitStructure.GPIO_Pin = 0x33;
    GPIO_InitStructure.GPIO_Mode = 0;
    GPIO_Init(GPIOC_BASE, &GPIO_InitStructure);
    RCC_APB2PeriphClockCmd(4, 1);
    GPIO_InitStructure.GPIO_Pin = 0x43;
    GPIO_InitStructure.GPIO_Mode = 0;
    GPIO_Init(GPIOA_BASE, &GPIO_InitStructure);
    DMA_DeInit(DMA1_Channel1_BASE);
    DMA_InitStructure.DMA_PeripheralBaseAddr = 0x4001244C;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint)adc_dma_buf;
    DMA_InitStructure.DMA_DIR = 0;
    DMA_InitStructure.DMA_BufferSize = 8;
    DMA_InitStructure.DMA_PeripheralInc = 0;
    DMA_InitStructure.DMA_MemoryInc = 0x80;
    DMA_InitStructure.DMA_PeripheralDataSize = 0x100;
    DMA_InitStructure.DMA_MemoryDataSize = 0x400;
    DMA_InitStructure.DMA_Mode = 0x20;
    DMA_InitStructure.DMA_Priority = 0x2000;
    DMA_InitStructure.DMA_M2M = 0;
    DMA_Init(DMA1_Channel1_BASE, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel1_BASE, 1);
    ADC_InitStructure.ADC_Mode = 0;
    ADC_InitStructure.ADC_ScanConvMode = 1;
    ADC_InitStructure.ADC_ContinuousConvMode = 0;
    ADC_InitStructure.ADC_ExternalTrigConv = 0xe0000;
    ADC_InitStructure.ADC_DataAlign = 0;
    ADC_InitStructure.ADC_NbrOfChannel = 8;
    ADC_Init(ADC1_BASE, &ADC_InitStructure);
    ADC_RegularChannelConfig(ADC1_BASE, 0, 1, 2);
    ADC_RegularChannelConfig(ADC1_BASE, 1, 2, 2);
    ADC_RegularChannelConfig(ADC1_BASE, 6, 4, 2);
    ADC_RegularChannelConfig(ADC1_BASE, 10, 5, 2);
    ADC_RegularChannelConfig(ADC1_BASE, 11, 6, 2);
    ADC_RegularChannelConfig(ADC1_BASE, 14, 7, 2);
    ADC_RegularChannelConfig(ADC1_BASE, 15, 8, 2);
    ADC_TempSensorVrefintCmd(1);
    ADC_DMACmd(ADC1_BASE, 1);
    ADC_ITConfig(ADC1_BASE, 0x220, 1);
    ADC_Cmd(ADC1_BASE, 1);
    ADC_ResetCalibration(ADC1_BASE);
    while (ADC_GetResetCalibrationStatus(ADC1_BASE))
        ;
    ADC_StartCalibration(ADC1_BASE);
    while (ADC_GetCalibrationStatus(ADC1_BASE))
        ;
}

/* TIM3 update interrupt at 72 MHz / 15 / 251, about 19.1 kHz. TIM3_IRQHandler starts an ADC scan
   on every second interrupt, so the ADC runs at about 9.6 kHz, 191 samples per 50 Hz cycle
   (inference from the clock tree; SystemInit sets 72 MHz). */
void tim3_init() {
    TIM_TimeBaseInitTypeDef timer_config;

    RCC_APB1PeriphClockCmd(2, 1);
    timer_config.TIM_Prescaler = 0xe;
    timer_config.TIM_CounterMode = 0;
    timer_config.TIM_Period = 0xfa;
    timer_config.TIM_ClockDivision = 0;
    timer_config.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3_BASE, &timer_config);
    TIM_ITConfig(TIM3_BASE, 1, 1);
    TIM_Cmd(TIM3_BASE, 1);
}

/* Checks one calibration gain: 0x2D0..0x533 (0.70 to 1.30 in Q10) passes and returns 1; anything
   else is replaced by 0x400 (1.0) and returns 0. */
uint cal_gain_validate(ushort *gain) {
    uint unused_gain;

    if (!((*gain < 0x2d0) || (0x533 < *gain))) {
        return 1;
    } else {
        *gain = 0x400;
        return 0;
    }
}

/* Runs cal_gain_validate over the 12 gains of block 2 and the four unnamed words at +0x24,
   +0x28, +0x2C and +0x30. Called once from cfg_load_all. */
void cal_gains_validate_all() {

    cal_gain_validate(&cal_gains.gain0);
    cal_gain_validate(&cal_gains.gain1);
    cal_gain_validate(&cal_gains.gain2);
    cal_gain_validate(&cal_gains.gain3);
    cal_gain_validate(&cal_gains.gain4);
    cal_gain_validate(&cal_gains.gain5);
    cal_gain_validate(&cal_gains.gain6);
    cal_gain_validate(&cal_gains.gain7);
    cal_gain_validate(&cal_gains.gain8);
    cal_gain_validate(&cal_gains.gain_24);
    cal_gain_validate(&cal_gains.gain_28);
    cal_gain_validate(&cal_gains.gain_2C);
    cal_gain_validate(&cal_gains.gain_30);
    cal_gain_validate(&cal_gains.gain11);
    cal_gain_validate(&cal_gains.gain9);
    cal_gain_validate(&cal_gains.gain10);
    return;
}

/* Loads the two per-country bytes the DSP gets on pages 2 and 3: the low byte of word 11 of the
   country's row in grid_limits_tbl[grid_table_set] and of word 9 of its row in grid_limits_tbl2. */
void grid_load_country_bytes() {

    grid_country_byte_a = (uchar)grid_limits_tbl[grid_table_set][grid_country][11];
    grid_country_byte_b = (uchar)grid_limits_tbl2[grid_country][9];
}

/* Copies the grid code's trip limits into the settings: HR63-70 and HR62/HR61 from
   grid_limits_tbl, HR71-78 from tbl2, HR79-82 from tbl3, HR83 from tbl4, and two more words to
   +0x69/+0x6B. The frequency limits get HR28 enable_60hz_freq_mode x 1000 added, so a 60 Hz grid
   uses the 50 Hz rows shifted by 10.00 Hz (inference from the units, 0.01 Hz). */
void grid_load_country_limits() {
    cfg_settings.inverter_start_time = grid_limits_tbl[grid_table_set][grid_country][8];
    cfg_settings.inverter_restart_delay = grid_limits_tbl[grid_table_set][grid_country][9];
    cfg_settings.v_ac_low_out = grid_limits_tbl[grid_table_set][grid_country][0];
    cfg_settings.v_ac_high_out = grid_limits_tbl[grid_table_set][grid_country][1];
    cfg_settings.f_ac_low_out = grid_limits_tbl[grid_table_set][grid_country][2] +
                                cfg_settings.enable_60hz_freq_mode * 1000;
    cfg_settings.f_ac_high_out = grid_limits_tbl[grid_table_set][grid_country][3] +
                                 cfg_settings.enable_60hz_freq_mode * 1000;
    cfg_settings.v_ac_low_out_time = grid_limits_tbl[grid_table_set][grid_country][4];
    cfg_settings.v_ac_high_out_time = grid_limits_tbl[grid_table_set][grid_country][5];
    cfg_settings.f_ac_low_out_time = grid_limits_tbl[grid_table_set][grid_country][6];
    cfg_settings.f_ac_high_out_time = grid_limits_tbl[grid_table_set][grid_country][7];
    cfg_settings.v_ac_low_in = grid_limits_tbl2[grid_country][0];
    cfg_settings.v_ac_high_in = grid_limits_tbl2[grid_country][1];
    cfg_settings.f_ac_low_in =
        grid_limits_tbl2[grid_country][2] + cfg_settings.enable_60hz_freq_mode * 1000;
    cfg_settings.f_ac_high_in =
        grid_limits_tbl2[grid_country][3] + cfg_settings.enable_60hz_freq_mode * 1000;
    cfg_settings.v_ac_low_in_time = grid_limits_tbl2[grid_country][4];
    cfg_settings.v_ac_high_in_time = grid_limits_tbl2[grid_country][5];
    cfg_settings.f_ac_low_in_time = grid_limits_tbl2[grid_country][6];
    cfg_settings.f_ac_high_in_time = grid_limits_tbl2[grid_country][7];
    cfg_settings.v_ac_low_c = grid_limits_tbl3[grid_country][0];
    cfg_settings.v_ac_high_c = grid_limits_tbl3[grid_country][1];
    cfg_settings.f_ac_low_c =
        grid_limits_tbl3[grid_country][2] + cfg_settings.enable_60hz_freq_mode * 1000;
    cfg_settings.f_ac_high_c =
        grid_limits_tbl3[grid_country][3] + cfg_settings.enable_60hz_freq_mode * 1000;
    cfg_settings.v_10_min_protection = grid_limits_tbl4[grid_country];
    cfg_settings.f69 = grid_limits_tbl[grid_table_set][grid_country][10];
    cfg_settings.f6B = grid_limits_tbl2[grid_country][8];
}

/* Takes the grid code from HR2: grid_country is the high byte when it is below 29 and the low
   byte (the model) is at most 0x32. Otherwise grid_country becomes 0 and it returns 1. It also
   sets HR308 = 3600 and HR309 = 65 whatever the model byte is (the three branches are the same). */
int grid_country_from_setting() {
    uchar module_code = 0;

    grid_table_set = 0;
    module_code = (uchar)cfg_settings.inverter_module_l;
    if (module_code == 0x24) {
        cfg_settings.hr308 = 0xe10;
        cfg_settings.hr309 = 0x41;
    } else {
        if (module_code == 0x2e) {
            cfg_settings.hr308 = 0xe10;
            cfg_settings.hr309 = 0x41;
        } else {
            cfg_settings.hr308 = 0xe10;
            cfg_settings.hr309 = 0x41;
        }
    }
    if (0x1d > (int)((uint)cfg_settings.inverter_module_l >> 8) &&
        (uchar)cfg_settings.inverter_module_l <= 0x32) {
        grid_country = (uint)cfg_settings.inverter_module_l >> 8;
        return 0;
    }
    grid_country = 0;
    return 1;
}

/* Defaults of the grid-code and power-quality settings that depend on the country: HR123 (50 for
   countries 17 and 28, 28 for 24, else 24), HR321 (5040 for 8 and 12, 5050 for 18 and 28, else
   5020), the power rates (active 100 %, reactive 0, PF 0), HR130-145 and others. */
void cfg_defaults_grid() {

    cfg_settings.hr128 = 1;
    if ((grid_country == '\x11') || (grid_country == '\x1c')) {
        cfg_settings.hr123 = 0x32;
    } else if (grid_country == '\x18') {
        cfg_settings.hr123 = 0x1c;
    } else {
        cfg_settings.hr123 = 0x18;
    }
    cfg_settings.hr129 = 1;
    cfg_settings.active_power_rate = 100;
    cfg_settings.reactive_power_rate = 0;
    cfg_settings.power_factor = 0;
    cfg_settings.f09 = 0;
    cfg_settings.inverter_state = 1;
    cfg_settings.hr130 = 0xff;
    cfg_settings.hr131 = 20000;
    cfg_settings.hr132 = 0xff;
    cfg_settings.hr133 = 20000;
    cfg_settings.hr134 = 0xff;
    cfg_settings.hr135 = 20000;
    cfg_settings.hr136 = 0xff;
    cfg_settings.hr137 = 20000;
    cfg_settings.hr327 = 0x28;
    cfg_settings.hr122 = 0;
    cfg_settings.hr145 = 0x8fc;
    cfg_settings.hr144 = 0x96f;
    cfg_settings.hr142 = 0x14;
    cfg_settings.hr143 = 5;
    cfg_settings.hr138 = 0x9b4;
    cfg_settings.hr139 = 0x9e2;
    cfg_settings.hr140 = 0x844;
    cfg_settings.hr141 = 0x816;
    cfg_settings.hr125 = 1;
    cfg_settings.hr121 = 0;
    if ((grid_country == '\f') || (grid_country == '\b')) {
        cfg_settings.hr321 = 0x13b0;
    } else if ((grid_country == '\x12') || (grid_country == '\x1c')) {
        cfg_settings.hr321 = 0x13ba;
    } else {
        cfg_settings.hr321 = 0x139c;
    }
    cfg_settings.hr326 = 0x1374;
    cfg_settings.f74 = 200;
    return;
}

typedef __packed struct {
    volatile uchar f : 8;
} BF8_08024064;
typedef __packed struct {
    volatile ushort f : 16;
} BF16_08024064;

/* Writes the factory defaults of one block into RAM: block 1 (cfg_settings) when the argument is
   1, block 6 (cfg_block6, the time slots) when it is 6. Block 1 starts from the grid code (HR2):
   grid_country_from_setting, grid_load_country_limits and cfg_defaults_grid, then the battery
   and schedule defaults:

     HR54 battery type 1 (lithium)   HR55 capacity 100 Ah    HR27 battery power mode 1
     HR26 6000 W                     HR97 43.20 V            HR98 57.20 V
     HR110 4 %   HR111/112 50 %      HR114 4 %               HR108 6, HR109 1, HR176 7
     HR58 auto judge 1               HR47 meter type 1       HR42 CT reverse mode 2 (auto)
     slots and their enables 0       HR310 100 %             HR116 4 %

   Block 6: charge slot SOCs (HR242, b1-b9) 100 %, discharge slot SOCs (HR272, HR275, a1-a8) 4 %,
   all slot times 0, HR230 1, HR353 12000, and the tail +0x66.. (148 bytes) cleared. It does not
   mark anything dirty; the callers do. */
void cfg_load_defaults(int block) {
    int unused_value;

    if (block == 1) {
        grid_country_from_setting();
        grid_load_country_limits();
        cfg_defaults_grid();
        cfg_settings.v_pv_input_start = 0x514;
        cfg_settings.hr31 = 0x14;
        cfg_settings.f06 = 1;
        cfg_settings.hr166 = 0;
        cfg_settings.f73 = 1;
        cfg_settings.battery_type = 1;
        cfg_settings.battery_capacity = 0x64;
        cfg_settings.battery_power_mode = 1;
        cfg_settings.meter_type = 1;
        cfg_settings.grid_port_max_power = 0x1770;
        cfg_settings.battery_low_voltage_limit = 0x10e0;
        cfg_settings.battery_high_voltage_limit = 0x1658;
        cfg_settings.enable_60hz_freq_mode = 0;
        cfg_settings.enable_drm_rj45 = 0;
        cfg_settings.enable_ammeter = 0;
        cfg_settings.enable_reverse_ct_clamp = 2;
        cfg_settings.battery_soc_reserve = 4;
        cfg_settings.hr108 = 6;
        cfg_settings.hr109 = 1;
        cfg_settings.battery_charge_limit = 0x32;
        cfg_settings.battery_discharge_limit = 0x32;
        cfg_settings.battery_discharge_min_power_reserve = 4;
        ((BF8_08024064 *)&cfg_settings.hr176)->f = 7;
        cfg_settings.hr113 = 0;
        cfg_settings.hr115 = 1;
        cfg_settings.enable_discharge = 0;
        cfg_settings.discharge_slot_1_start = 0;
        cfg_settings.discharge_slot_1_end = 0;
        cfg_settings.discharge_slot_2_start = 0;
        cfg_settings.discharge_slot_2_end = 0;
        cfg_settings.enable_charge = 0;
        cfg_settings.charge_slot_1_start = 0;
        cfg_settings.charge_slot_1_end = 0;
        cfg_settings.soc_force_adjust = 0;
        cfg_settings.hr177 = 0;
        cfg_settings.reverse_115_meter_dir = 0;
        cfg_settings.reverse_418_meter_dir = 0;
        cfg_settings.fA2 = 0;
        cfg_settings.enable_auto_judge_battery_type = 1;
        cfg_settings.enable_charge_target = 0;
        cfg_settings.charge_target_soc = 4;
        if (grid_country == 7) {
            cfg_settings.hr300 = 0;
            cfg_settings.hr303 = 0;
            cfg_settings.hr312 = 0;
        } else {
            cfg_settings.hr300 = 0xa;
            cfg_settings.hr303 = 0x3a98;
            cfg_settings.hr312 = 0x3a98;
        }
        cfg_settings.hr301 = 0x96;
        cfg_settings.hr178 = 0;
        cfg_settings.hr179 = 0;
        cfg_settings.hr305 = 0;
        cfg_settings.hr307 = 0x8fc;
        cfg_settings.hr310 = 0x64;
        cfg_settings.hr311 = 0;
        cfg_settings.hr313 = 0x64;
        cfg_settings.hr314 = 0x64;
        cfg_settings.hr315 = 0x47e;
        cfg_settings.hr316 = 0xac8;
        ((BF16_08024064 *)&cfg_settings.hr102)->f = 0xffbf;
        cfg_settings.hr318 = 0;
        cfg_settings.hr319 = 0;
        cfg_settings.hr320 = 0;
        if (grid_country == 3) {
            cfg_settings.hr328 = 0x1397;
        } else {
            cfg_settings.hr328 = 0x1392;
        }
        cfg_settings.hr330 = 0;
        ((BF8_08024064 *)&cfg_settings.hr331)->f = 0;
        cfg_settings.hr101 = 0x64;
        cfg_settings.fD2 = 0;
        if (grid_country == 4 || grid_country == 0x12) {
            cfg_settings.hr302 = 0x64;
            cfg_settings.hr306 = 0x64;
        } else {
            cfg_settings.hr302 = 0x1388;
            cfg_settings.hr306 = 0x1770;
        }
    } else {
        if (block == 6) {
            cfg_block6.hr242 = 0x64;
            cfg_block6.slot_b1_soc = 0x64;
            cfg_block6.slot_b2_soc = 0x64;
            cfg_block6.slot_b3_soc = 0x64;
            cfg_block6.slot_b4_soc = 0x64;
            cfg_block6.slot_b5_soc = 0x64;
            cfg_block6.slot_b6_soc = 0x64;
            cfg_block6.slot_b7_soc = 0x64;
            cfg_block6.slot_b8_soc = 0x64;
            cfg_block6.slot_b9_soc = 0x64;
            cfg_block6.hr272 = 4;
            cfg_block6.hr275 = 4;
            cfg_block6.slot_a1_soc = 4;
            cfg_block6.slot_a2_soc = 4;
            cfg_block6.slot_a3_soc = 4;
            cfg_block6.slot_a4_soc = 4;
            cfg_block6.slot_a5_soc = 4;
            cfg_block6.slot_a6_soc = 4;
            cfg_block6.slot_a7_soc = 4;
            cfg_block6.slot_a8_soc = 4;
            cfg_block6.hr322 = 0;
            cfg_block6.hr325 = 0;
            cfg_block6.hr323 = 0;
            cfg_block6.hr324 = 0;
            cfg_block6.slot_b1_start = 0;
            cfg_block6.slot_b1_end = 0;
            cfg_block6.slot_b2_start = 0;
            cfg_block6.slot_b2_end = 0;
            cfg_block6.slot_b3_start = 0;
            cfg_block6.slot_b3_end = 0;
            cfg_block6.slot_b4_start = 0;
            cfg_block6.slot_b4_end = 0;
            cfg_block6.slot_b5_start = 0;
            cfg_block6.slot_b5_end = 0;
            cfg_block6.slot_b6_start = 0;
            cfg_block6.slot_b6_end = 0;
            cfg_block6.slot_b7_start = 0;
            cfg_block6.slot_b7_end = 0;
            cfg_block6.slot_b8_start = 0;
            cfg_block6.slot_b8_end = 0;
            cfg_block6.slot_b9_start = 0;
            cfg_block6.slot_b9_end = 0;
            cfg_block6.slot_a1_start = 0;
            cfg_block6.slot_a1_end = 0;
            cfg_block6.slot_a2_start = 0;
            cfg_block6.slot_a2_end = 0;
            cfg_block6.slot_a3_start = 0;
            cfg_block6.slot_a3_end = 0;
            cfg_block6.slot_a4_start = 0;
            cfg_block6.slot_a4_end = 0;
            cfg_block6.slot_a5_start = 0;
            cfg_block6.slot_a5_end = 0;
            cfg_block6.slot_a6_start = 0;
            cfg_block6.slot_a6_end = 0;
            cfg_block6.slot_a7_start = 0;
            cfg_block6.slot_a7_end = 0;
            cfg_block6.slot_a8_start = 0;
            cfg_block6.slot_a8_end = 0;
            cfg_block6.hr230 = 1;
            cfg_block6.hr124 = 0;
            cfg_block6.hr353 = 0x2ee0;
            __aeabi_memclr(&cfg_block6.hr322 + 0x66, 0x94);
        }
    }
}

/* Boot-time load of every EEPROM block (eeprom_load, five tries each), with defaults where a
   block fails, and the dirty bit set so the defaults get written back:

     block 1 settings: on failure zero it, serial "XXXXXXXXXX", defaults; then try the 32-byte
             header copy (module HR1/HR2, Modbus address), else model 3/0x32, address 0x11
     block 2 gains and fault log: gains to 0x400/0 and the log cleared as needed
     block 3 event log, block 4 energy counters (zeroed), block 6 slots (defaults),
     block 7 (zeroed), block 8 (zeroed, the first 14 bytes "X")

   eeprom_load_failed is set when anything was dirty after this, eeprom_load_ok otherwise;
   task_monitor uses them to retry the reads before writing defaults over the chip.

   Then the fix-ups, every boot:
   - a bad grid code or byte +0x06 != 1 reloads the country limits;
   - the gains are range checked (cal_gains_validate_all);
   - HR53 inverter_state is forced to 1: f09 is cleared just before it is tested, so the "0"
     branch is dead;
   - version byte +0xA6 (fA6): below 0x13 sets HR47 = 1, HR109 = 1; at or below 0x14 sets
     HR97 = 43.20 V, HR98 = 58.50 V and marks block 1 dirty. Nothing in the image ever sets fA6
     above 0x14, so this runs on every boot and HR97/HR98 written over Modbus last only until the
     next restart (docs/findings.md);
   - HR124 above 1 becomes 0, and HR101 outside 1..255 becomes 100 with bit 6 of HR102 cleared. */
void cfg_load_all() {
    int i;
    int load_result;
    int block_len;
    uchar *block_data;

    load_result = 0;
    i = 0;
    eeprom_wp_set(0);
    cfg_load_defaults(1);
    load_result = eeprom_load(1, 5);
    if (load_result == 0) {
        block_len = 0xd3;
        block_data = (uchar *)&cfg_settings;
        for (i = 0; i < block_len;) {
            block_data[i] = 0;
            i = (uchar)(i + 1);
        }
        cfg_settings.user_code = 0;
        for (i = 0; i < 0xa; i = (uchar)(i + 1)) {
            cfg_settings.inverter_serial[0][i] = 0x58;
        }
        cfg_load_defaults(1);
        i = eeprom_load(5, 5);
        if (i == 0) {
            cfg_settings.inverter_module_h = 3;
            cfg_settings.inverter_module_l = 0x32;
            cfg_settings.modbus_address = 0x11;
        }
        grid_country_from_setting();
        grid_load_country_limits();
    }
    load_result = eeprom_load(2, 5);
    if (load_result == 2) {
        calib_table_defaults();
        eeprom_dirty = eeprom_dirty | 2;
    } else {
        if (load_result == 1) {
            block_len = 0x2d;
            block_data = &fault_log[0];
            for (i = 0; i < block_len; i = (uchar)(i + 1)) {
                block_data[i] = 0;
            }
            eeprom_dirty = eeprom_dirty | 2;
        } else {
            if (load_result == 0) {
                calib_table_defaults();
                block_len = 0x2d;
                block_data = &fault_log[0];
                for (i = 0; i < block_len; i = (uchar)(i + 1)) {
                    block_data[i] = 0;
                }
                eeprom_dirty = eeprom_dirty | 2;
            }
        }
    }
    load_result = eeprom_load(3, 5);
    if (load_result == 0) {
        eeprom_dirty = eeprom_dirty | 4;
    }
    load_result = eeprom_load(4, 5);
    if (load_result == 0) {
        block_len = 0xbd;
        block_data = (uchar *)&energy_totals;
        for (i = 0; i < block_len; i = (uchar)(i + 1)) {
            block_data[i] = 0;
        }
        eeprom_dirty = eeprom_dirty | 8;
    }
    cfg_load_defaults(6);
    load_result = eeprom_load(6, 5);
    if (load_result == 0) {
        block_len = 0xfa;
        block_data = &cfg_block6.hr322;
        for (i = 0; i < block_len; i = (uchar)(i + 1)) {
            block_data[i] = 0;
        }
        cfg_load_defaults(6);
        eeprom_dirty = eeprom_dirty | 0x20;
    }
    load_result = eeprom_load(7, 5);
    if (load_result == 0) {
        block_len = 0xfa;
        block_data = &cfg_block7[0];
        for (i = 0; i < block_len; i = (uchar)(i + 1)) {
            block_data[i] = 0;
        }
        eeprom_dirty = eeprom_dirty | 0x40;
    }
    load_result = eeprom_load(8, 5);
    if (load_result == 0) {
        block_len = 0xf0;
        block_data = &cfg_block8[0];
        for (i = 0; i < block_len; i = (uchar)(i + 1)) {
            block_data[i] = 0;
        }
        for (i = 0; i < 0xe; i = (uchar)(i + 1)) {
            cfg_block8[i] = 0x58;
        }
        eeprom_dirty = eeprom_dirty | 0x80;
    }
    if (eeprom_dirty > 0) {
        eeprom_load_failed = 1;
    } else {
        eeprom_load_ok = 1;
    }
    if (grid_country_from_setting() != 0) {
        grid_load_country_limits();
        eeprom_dirty = eeprom_dirty | 1;
    }
    if (cfg_settings.f06 != 1) {
        grid_load_country_limits();
        eeprom_dirty = eeprom_dirty | 1;
    }
    grid_load_country_bytes();
    cal_gains_validate_all();
    hr31_value = cfg_settings.hr31 >> 4;
    hr32_value = cfg_settings.hr31 & 0xf;
    cfg_settings.f09 = 0;
    if (cfg_settings.f09 == 1) {
        cfg_settings.inverter_state = 0;
    } else {
        cfg_settings.inverter_state = 1;
    }
    hr31_value = cfg_settings.hr31 >> 4;
    hr32_value = cfg_settings.hr31 & 0xf;
    if (cfg_settings.fA6 < 0x13) { /* settings layout version (inference) */
        cfg_settings.fA6 = 0x14;
        cfg_settings.meter_type = 1;
        cfg_settings.hr109 = 1;
        eeprom_dirty = eeprom_dirty | 1;
    }
    if (cfg_settings.fA6 <= 0x14) {
        cfg_settings.fA6 = 0x14;
        cfg_settings.battery_low_voltage_limit = 0x10e0;
        cfg_settings.battery_high_voltage_limit = 0x16da;
        eeprom_dirty = eeprom_dirty | 1;
    }
    if (cfg_block6.hr124 > 1) {
        cfg_block6.hr124 = 0;
        eeprom_dirty = eeprom_dirty | 0x20;
    }
    if (cfg_settings.hr101 > 0xff || cfg_settings.hr101 < 1) {
        cfg_settings.hr101 = 0x64;
        cfg_settings.hr102 = cfg_settings.hr102 & ~0x40;
        eeprom_dirty = eeprom_dirty | 1;
    }
}

/* Drives PC3 (task_measure sets it to 1 on every pass; its purpose is not known). */
void gpio_pc3_set(int level) {

    if (level == 0) {
        GPIO_ResetBits(GPIOC_BASE, 8);
    } else {
        GPIO_SetBits(GPIOC_BASE, 8);
    }
    return;
}

/* PC3 as a push-pull output, low. */
void gpio_pc3_init() {
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(0x10, 1);
    gpio_config.GPIO_Pin = 0x8;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((uint *)GPIOC_BASE, &gpio_config);
    gpio_pc3_set(0);
    return;
}

/* Drives PB5, the buzzer (inference from buzzer_on/off and TIM3_IRQHandler). */
void buzzer_pin_set(int level) {

    if (level == 0) {
        GPIO_ResetBits(GPIOB_BASE, 0x20);
    } else {
        GPIO_SetBits(GPIOB_BASE, 0x20);
    }
    return;
}

/* PB5 as a push-pull output, low. */
void buzzer_gpio_init() {
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(8, 1);
    gpio_config.GPIO_Pin = 0x20;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((uint *)GPIOB_BASE, &gpio_config);
    buzzer_pin_set(0);
    return;
}

/* PB6, PB7, PB8 (I2C SCL, SDA, EEPROM write protect) as push-pull outputs. */
void i2c_gpio_init() {
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(8, 1);
    gpio_config.GPIO_Pin = 0x1c0;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((uint *)GPIOB_BASE, &gpio_config);
    return;
}

/* PD2 as a floating input (sampled by ADC1_2_IRQHandler). */
void gpio_pd2_init() {
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(0x20, 1);
    gpio_config.GPIO_Pin = 0x4;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x4;
    GPIO_Init((uint *)GPIOD_BASE, &gpio_config);
    return;
}

/* PB9, the fault LED, as a push-pull output. */
void led_fault_gpio_init() {
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(8, 1);
    gpio_config.GPIO_Pin = 0x200;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((uint *)GPIOB_BASE, &gpio_config);
    return;
}

/* Sets status LED 1 (PB1) or 2 (PB0) on (1) or off (0); other numbers and values do nothing. */
void led_set(int led, int level) {

    if (led == 1) {
        if (level == 1) {
            GPIO_SetBits(GPIOB_BASE, 2);
        } else if (level == 0) {
            GPIO_ResetBits(GPIOB_BASE, 2);
        }
    } else if (led == 2) {
        if (level == 1) {
            GPIO_SetBits(GPIOB_BASE, 1);
        } else if (level == 0) {
            GPIO_ResetBits(GPIOB_BASE, 1);
        }
    }
    return;
}

/* PB0 and PB1, the status LEDs, as push-pull outputs. */
void led_gpio_init() {
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(8, 1);
    gpio_config.GPIO_Pin = 0x3;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((uint *)GPIOB_BASE, &gpio_config);
    return;
}

/* The panel indicator pins PB14, PB15, PC6, PC7, PC9 and PA8 as push-pull outputs. */
void panel_gpio_init() {
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(8, 1);
    RCC_APB2PeriphClockCmd(0x10, 1);
    RCC_APB2PeriphClockCmd(4, 1);
    gpio_config.GPIO_Pin = 0xc000;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((uint *)GPIOB_BASE, &gpio_config);
    gpio_config.GPIO_Pin = 0xc0;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((uint *)GPIOC_BASE, &gpio_config);
    gpio_config.GPIO_Pin = 0x200;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((uint *)GPIOC_BASE, &gpio_config);
    gpio_config.GPIO_Pin = 0x100;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((uint *)GPIOA_BASE, &gpio_config);
    return;
}

/* Board bring-up, in order: panel, LED, fault LED, PD2, I2C and buzzer pins, PC3, then
   cfg_load_all (settings from the EEPROM), TIM3, ADC with DMA, PC9, the UARTs and the run-time
   variables. Sets board_init_done = 1; the meter code reuses that byte as its "no meter answer"
   flag (meter_poll, meter_rx_frame), so it is not only an init flag. */
void board_init() {

    panel_gpio_init();
    led_gpio_init();
    led_set(1, 0);
    led_set(2, 0);
    led_fault_gpio_init();
    gpio_pd2_init();
    i2c_gpio_init();
    buzzer_gpio_init();
    gpio_pc3_init();
    cfg_load_all();
    tim3_init();
    adc_dma_init();
    gpio_pc9_init();
    uart_init_all();
    sys_runtime_vars_init();
    board_init_done = 1;
    return;
}

/* Dead. 0-9 to "0"-"9", 10-15 to "A"-"F". */
uchar hex_nibble_to_ascii(int nibble) {

    if (nibble < 0xa) {
        return (uchar)(nibble + 0x30);
    }
    return (uchar)(nibble + 0x37);
}

/* The range check the Modbus handlers call for grid voltage and frequency limits (HR63-66,
   71-74, 79-82): it accepts everything. Whether a real check was removed or never written is not
   known. */
uint cfg_limit_check(uint value, int lower_limit, int upper_limit) { return 1; }

/* Drives PB4, the Modbus RS485 driver enable, and clears the release counter; SysTick_Handler
   releases it two ticks after the last byte has gone (uart_tx_isr_usart3 starts the count). */
void rs485_modbus_de_set(int level) {

    rs485_modbus_de_hold = 0;
    if (level == 0) {
        GPIO_ResetBits(GPIOB_BASE, 4);
    } else {
        GPIO_SetBits(GPIOB_BASE, 4);
    }
    return;
}

/* Drives PA4, the meter RS485 driver enable, the same way for USART2. */
void rs485_meter_de_set(int level) {

    rs485_meter_de_hold = 0;
    if (level == 0) {
        GPIO_ResetBits(GPIOA_BASE, 0x10);
    } else {
        GPIO_SetBits(GPIOA_BASE, 0x10);
    }
    return;
}

/* Drives PB9, the fault LED. */
void led_fault_set(int level) {

    if (level == 0) {
        GPIO_ResetBits(GPIOB_BASE, 0x200);
    } else {
        GPIO_SetBits(GPIOB_BASE, 0x200);
    }
    return;
}

/* Dead. TIM4 update interrupt at 72 MHz / 10 / 37; nothing calls it, and TIM4_IRQHandler only
   clears the flag. */
void tim4_init() {
    TIM_TimeBaseInitTypeDef timer_config;

    RCC_APB1PeriphClockCmd(4, 1);
    timer_config.TIM_Prescaler = 9;
    timer_config.TIM_CounterMode = 0;
    timer_config.TIM_Period = 0x24;
    timer_config.TIM_ClockDivision = 0;
    timer_config.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM4_BASE, &timer_config);
    TIM_ITConfig(TIM4_BASE, 1, 1);
    TIM_Cmd(TIM4_BASE, 1);
}
