/* Board GPIO, UART, ADC/DMA, timers, SPI and interrupt initialization.
   I recovered this complete unsplit unit using the A316 board source as a crib. */
/* at: board_initialize_pb8 0x08023D78, board_initialize_uarts 0x08023DC0, board_set_pb2 0x08023FB6, board_set_pa8 0x08023FD2, board_set_pb5 0x08023FEE, board_initialize_pb5 0x08024004, board_initialize_control_outputs 0x08024030, board_set_control_output 0x08024076, board_initialize_pb9 0x080240AC, board_set_pb9 0x080240D4, board_set_pc9 0x080240EA, board_initialize_pc9 0x08024100, board_initialize_auxiliary_outputs 0x0802412E, board_initialize_pb12_to_pb15_inputs 0x080241B4, board_initialize_pa0_input 0x080241D4, board_set_pc13 0x080241F2, board_initialize_pc13 0x08024208, board_initialize_pc4 0x08024236, board_set_pc4 0x0802425A, board_sample_pc4 0x08024270, board_initialize_debug_pins 0x08024278, board_enter_standby 0x0802428E, board_initialize_rtc_alarm_interrupt 0x080242E0, board_initialize_pa0_interrupt 0x08024326, board_initialize_pb4_interrupt 0x08024388, board_initialize_adc_dma 0x080243EA, board_initialize_afe_chip_select 0x080245A2, board_set_afe_chip_select 0x080245F6, board_initialize_spi1 0x0802460C, board_initialize_tim3 0x0802469C, board_initialize_tim4 0x080246DE, board_initialize_interrupts 0x08024722, board_initialize_usb_line_settings 0x080247D8, board_initialize_peripherals 0x080247EC, RCC_APB2PeriphClockCmd 0x08028EF2, GPIO_Init 0x080289AE, GPIO_SetBits 0x08028A8C, USART_Init 0x0802A090, USART_ITConfig 0x0802A19E, USART_Cmd 0x0802A186, USART_ClearITPendingBit 0x0802A378, sub_08014AC8 0x08014AC8, RCC_APB1PeriphClockCmd 0x08028F08, g_2000004D 0x2000004D, GPIO_ResetBits 0x08028A90, g_2000004E 0x2000004E, GPIO_ReadInputDataBit 0x08028A64, GPIO_PinRemapConfig 0x08028AD0, g_2000009D 0x2000009D, os_task_dly_idx 0x2000061E, RTC_GetCounter 0x08028FF2, RTC_SetAlarm 0x0802903A, RTC_WaitForLastTask 0x08029068, RTC_ITConfig 0x08028FC0, RCC_APB2PeriphResetCmd 0x08028F34, os_task_delay 0x0802A78A, PWR_WakeUpPinCmd 0x08028BA6, PWR_EnterSTANDBYMode 0x08028BD8, NVIC_PriorityGroupConfig 0x08027C88, NVIC_Init 0x08027C92, EXTI_ClearITPendingBit 0x08028426, EXTI_Init 0x08028368, GPIO_EXTILineConfig 0x08028B26, adc_dma_buf 0x20003BD2, RCC_ADCCLKConfig 0x08028DC0, RCC_AHBPeriphClockCmd 0x08028EDC, DMA_DeInit 0x08028164, DMA_Init 0x08028252, DMA_Cmd 0x080282A6, ADC_Init 0x08027D98, ADC_RegularChannelConfig 0x08027EAC, ADC_TempSensorVrefintCmd 0x0802805C, ADC_DMACmd 0x08027E00, ADC_ITConfig 0x08027E18, ADC_Cmd 0x08027DE8, ADC_ResetCalibration 0x08027E2E, ADC_GetResetCalibrationStatus 0x08027E38, ADC_StartCalibration 0x08027E46, ADC_GetCalibrationStatus 0x08027E50, SPI_Init 0x0802912C, SPI_Cmd 0x08029278, SPI_I2S_ClearFlag 0x0802937E, TIM_TimeBaseInit 0x080295A4, TIM_ITConfig 0x08029B7A, TIM_Cmd 0x08029B46, NVIC_SetVectorTable 0x08027CF2, sys_irq_enable 0x08014230, usb_line_config 0x200006AC */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I keep board GPIO, USART/UART, ADC/DMA, SPI and timer setup together.
   The pin wrappers and board_initialize_peripherals share this matched unit;
   RAM objects are caller bindings owned elsewhere. Measurement and serial tasks use them.
   name: I use a descriptive filename; I have no original vendor filename evidence. */
#include "bms_hardware_types.h"

/* Configures PB8 output and sets it high. */
void board_initialize_pb8(void)

{
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(8, 1);
    gpio_config.GPIO_Pin = 0xc0;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x14;
    GPIO_Init((void *)0x40010C00, &gpio_config);
    gpio_config.GPIO_Pin = 0x100;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((void *)0x40010C00, &gpio_config);
    GPIO_SetBits((void *)0x40010c00, 0x100);
    return;
}

/* Configures USART1, UART4 and UART5 at 9600 baud. */
void board_initialize_uarts(void)

{
    GPIO_InitTypeDef gpio_config;
    USART_InitTypeDef usart_config;

    RCC_APB2PeriphClockCmd(4, 1);
    gpio_config.GPIO_Pin = 0x100;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((void *)0x40010800, &gpio_config);
    RCC_APB2PeriphClockCmd(0x4000, 1);
    RCC_APB2PeriphClockCmd(4, 1);
    gpio_config.GPIO_Pin = 0x200;
    gpio_config.GPIO_Mode = 0x18;
    gpio_config.GPIO_Speed = 0x3;
    GPIO_Init((void *)0x40010800, &gpio_config);
    gpio_config.GPIO_Pin = 0x400;
    gpio_config.GPIO_Mode = 0x4;
    gpio_config.GPIO_Speed = 0x3;
    GPIO_Init((void *)0x40010800, &gpio_config);
    usart_config.USART_BaudRate = 0x2580;
    usart_config.USART_WordLength = 0x0;
    usart_config.USART_StopBits = 0x0;
    usart_config.USART_Parity = 0x0;
    usart_config.USART_HardwareFlowControl = 0x0;
    usart_config.USART_Mode = 0xc;
    USART_Init(&(*(uint *)0x40013800), &usart_config);
    USART_ITConfig((void *)0x40013800, 0x525, 1);
    USART_ITConfig((void *)0x40013800, 0x626, 1);
    USART_Cmd((void *)0x40013800, 1);
    USART_ClearITPendingBit(&(*(uint *)0x40013800), 0x626);
    sub_08014AC8(0, 0x96);
    RCC_APB2PeriphClockCmd(8, 1);
    gpio_config.GPIO_Pin = 0x4;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((void *)0x40010C00, &gpio_config);
    RCC_APB1PeriphClockCmd(0x40000, 1);
    RCC_APB2PeriphClockCmd(8, 1);
    gpio_config.GPIO_Pin = 0x400;
    gpio_config.GPIO_Mode = 0x18;
    gpio_config.GPIO_Speed = 0x3;
    GPIO_Init((void *)0x40010C00, &gpio_config);
    gpio_config.GPIO_Pin = 0x800;
    gpio_config.GPIO_Mode = 0x4;
    gpio_config.GPIO_Speed = 0x3;
    GPIO_Init((void *)0x40010C00, &gpio_config);
    usart_config.USART_BaudRate = 0x2580;
    usart_config.USART_WordLength = 0x0;
    usart_config.USART_StopBits = 0x0;
    usart_config.USART_Parity = 0x0;
    usart_config.USART_HardwareFlowControl = 0x0;
    usart_config.USART_Mode = 0xc;
    USART_Init(&(*(uint *)0x40004800), &usart_config);
    USART_ITConfig((void *)0x40004800, 0x525, 1);
    USART_Cmd((void *)0x40004800, 1);
    sub_08014AC8(2, 0x96);
    RCC_APB1PeriphClockCmd(0x80000, 1);
    RCC_APB2PeriphClockCmd(0x10, 1);
    gpio_config.GPIO_Pin = 0x400;
    gpio_config.GPIO_Mode = 0x18;
    gpio_config.GPIO_Speed = 0x3;
    GPIO_Init((void *)0x40011000, &gpio_config);
    gpio_config.GPIO_Pin = 0x800;
    gpio_config.GPIO_Mode = 0x4;
    gpio_config.GPIO_Speed = 0x3;
    GPIO_Init((void *)0x40011000, &gpio_config);
    usart_config.USART_BaudRate = 0x2580;
    usart_config.USART_WordLength = 0x0;
    usart_config.USART_StopBits = 0x0;
    usart_config.USART_Parity = 0x0;
    usart_config.USART_HardwareFlowControl = 0x0;
    usart_config.USART_Mode = 0xc;
    USART_Init(&(*(uint *)0x40004C00), &usart_config);
    USART_ITConfig((void *)0x40004c00, 0x525, 1);
    USART_Cmd((void *)0x40004c00, 1);
    sub_08014AC8(3, 0x96);
    return;
}

/* Writes PB2 and clears its software state byte. */
void board_set_pb2(int level)

{
    g_2000004D = 0;
    if (level != 0) {
        GPIO_SetBits((void *)0x40010c00, 4);
        return;
    }
    GPIO_ResetBits((void *)0x40010c00, 4);
    return;
}

/* Writes PA8 and clears its software state byte. */
void board_set_pa8(int level)

{
    g_2000004E = 0;
    if (level != 0) {
        GPIO_SetBits((void *)0x40010800, 0x100);
        return;
    }
    GPIO_ResetBits((void *)0x40010800, 0x100);
    return;
}

/* Writes PB5 according to nonzero input. */
void board_set_pb5(int level)

{
    if (level != 0) {
        GPIO_SetBits((void *)0x40010c00, 0x20);
        return;
    }
    GPIO_ResetBits((void *)0x40010c00, 0x20);
    return;
}

/* Configures PB5 output and drives it low. */
void board_initialize_pb5(void)

{
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(8, 1);
    gpio_config.GPIO_Pin = 0x20;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((void *)0x40010C00, &gpio_config);
    board_set_pb5(0);
    return;
}

/* Configures PC6, PC7, PC12 and PA15 outputs. */
void board_initialize_control_outputs(void)

{
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(0x10, 1);
    gpio_config.GPIO_Pin = 0x10c0;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((void *)0x40011000, &gpio_config);
    RCC_APB2PeriphClockCmd(4, 1);
    gpio_config.GPIO_Pin = 0x8000;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((void *)0x40010800, &gpio_config);
    return;
}

/* Selects PC6 or PC7 for channel 1 or 2. */
void board_set_control_output(int channel, int level)

{
    if (channel == 1) {
        if (level == 1) {
            GPIO_SetBits((void *)0x40011000, 0x40);
            return;
        }
        if (level == 0) {
            GPIO_ResetBits((void *)0x40011000, 0x40);
            return;
        }
    } else if (channel == 2) {
        if (level == 1) {
            GPIO_SetBits((void *)0x40011000, 0x80);
            return;
        }
        if (level == 0) {
            GPIO_ResetBits((void *)0x40011000, 0x80);
            return;
        }
    }
    return;
}

/* Configures PB9 output. */
void board_initialize_pb9(void)

{
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(8, 1);
    gpio_config.GPIO_Pin = 0x200;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((void *)0x40010C00, &gpio_config);
    return;
}

/* Writes PB9 according to nonzero input. */
void board_set_pb9(int level)

{
    if (level != 0) {
        GPIO_SetBits((void *)0x40010c00, 0x200);
        return;
    }
    GPIO_ResetBits((void *)0x40010c00, 0x200);
    return;
}

/* Writes PC9 according to nonzero input. */
void board_set_pc9(int level)

{
    if (level != 0) {
        GPIO_SetBits((void *)0x40011000, 0x200);
        return;
    }
    GPIO_ResetBits((void *)0x40011000, 0x200);
    return;
}

/* Configures PC9 output and drives it high. */
void board_initialize_pc9(void)

{
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(0x10, 1);
    gpio_config.GPIO_Pin = 0x200;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((void *)0x40011000, &gpio_config);
    board_set_pc9(1);
    return;
}

/* Configures PB3, PC8 and PD2 outputs. */
void board_initialize_auxiliary_outputs(void)

{
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(8, 1);
    gpio_config.GPIO_Pin = 0x8;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((void *)0x40010C00, &gpio_config);
    RCC_APB2PeriphClockCmd(0x10, 1);
    gpio_config.GPIO_Pin = 0x100;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((void *)0x40011000, &gpio_config);
    RCC_APB2PeriphClockCmd(0x20, 1);
    gpio_config.GPIO_Pin = 0x4;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((void *)0x40011400, &gpio_config);
    return;
}

/* Configures PB12..PB15 as pull-up inputs. */
void board_initialize_pb12_to_pb15_inputs(void)

{
    GPIO_InitTypeDef gpio_config;

    gpio_config.GPIO_Pin = 0xf000;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x48;
    GPIO_Init((void *)0x40010C00, &gpio_config);
    return;
}

/* Configures PA0 as a pull-down input. */
void board_initialize_pa0_input(void)

{
    GPIO_InitTypeDef gpio_config;

    gpio_config.GPIO_Pin = 0x1;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x28;
    GPIO_Init((void *)0x40010800, &gpio_config);
    return;
}

/* Writes PC13 according to nonzero input. */
void board_set_pc13(int level)

{
    if (level != 0) {
        GPIO_SetBits((void *)0x40011000, 0x2000);
        return;
    }
    GPIO_ResetBits((void *)0x40011000, 0x2000);
    return;
}

/* Configures PC13 output and drives it low. */
void board_initialize_pc13(void)

{
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(0x10, 1);
    gpio_config.GPIO_Pin = 0x2000;
    gpio_config.GPIO_Speed = 0x2;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((void *)0x40011000, &gpio_config);
    board_set_pc13(0);
    return;
}

/* Configures PC4 output. */
void board_initialize_pc4(void)

{
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(0x10, 1);
    gpio_config.GPIO_Pin = 0x10;
    gpio_config.GPIO_Speed = 0x3;
    gpio_config.GPIO_Mode = 0x10;
    GPIO_Init((void *)0x40011000, &gpio_config);
    return;
}

/* Writes PC4 according to nonzero input. */
void board_set_pc4(int level)

{
    if (level != 0) {
        GPIO_SetBits((void *)0x40011000, 0x10);
        return;
    }
    GPIO_ResetBits((void *)0x40011000, 0x10);
    return;
}

/* Performs a PC4 input read with unused return value.
   match: I retain the peripheral read even though its result is discarded. */
void board_sample_pc4(void)

{
    GPIO_ReadInputDataBit((void *)0x40011000, 0x10);
    return;
}

/* Enables AFIO GPIO clocks and applies SWJ remap. */
void board_initialize_debug_pins(void)

{
    RCC_APB2PeriphClockCmd(0xd, 1);
    GPIO_PinRemapConfig(0x300200, 1);
    return;
}

/* Optionally schedules RTC alarm then enters standby. */
void board_enter_standby(void)

{
    int rtc_counter;

    if ((int)((uint)g_2000009D << 0x1d) < 0) {
        rtc_counter = RTC_GetCounter();
        RTC_SetAlarm(rtc_counter + 10);
        RTC_WaitForLastTask();
        RTC_ITConfig(2, 1);
        RTC_WaitForLastTask();
    }
    RCC_APB2PeriphResetCmd(0x1fc, 0);
    os_task_delay((uint)(byte)os_task_dly_idx, 0x5dc, 10);
    RCC_APB1PeriphClockCmd(0x10000000, 1);
    PWR_WakeUpPinCmd(1);
    PWR_EnterSTANDBYMode();
    return;
}

/* Configures EXTI17 and RTC alarm IRQ41. */
void board_initialize_rtc_alarm_interrupt(void) {
    NVIC_InitTypeDef interrupt_config;
    EXTI_InitTypeDef exti_config;
    NVIC_PriorityGroupConfig(0x600);
    interrupt_config.NVIC_IRQChannel = 0x29;
    interrupt_config.NVIC_IRQChannelPreemptionPriority = 1;
    interrupt_config.NVIC_IRQChannelSubPriority = 0;
    interrupt_config.NVIC_IRQChannelCmd = 1;
    NVIC_Init(&interrupt_config);
    EXTI_ClearITPendingBit(0x20000);
    exti_config.EXTI_Line = 0x20000;
    exti_config.EXTI_LineCmd = 1;
    exti_config.EXTI_Mode = 0;
    exti_config.EXTI_Trigger = 8;
    EXTI_Init(&exti_config);
}
/* Routes PA0 to EXTI0 and enables IRQ6. */
void board_initialize_pa0_interrupt(void) {
    GPIO_InitTypeDef gpio_config;
    NVIC_InitTypeDef interrupt_config;
    EXTI_InitTypeDef exti_config;
    RCC_APB2PeriphClockCmd(5, 1);
    gpio_config.GPIO_Pin = 1;
    gpio_config.GPIO_Mode = 40;
    GPIO_Init((uint *)1073809408, &gpio_config);
    GPIO_EXTILineConfig(0, 0);
    exti_config.EXTI_Line = 1;
    exti_config.EXTI_Mode = 0;
    exti_config.EXTI_Trigger = 8;
    exti_config.EXTI_LineCmd = 1;
    EXTI_Init(&exti_config);
    NVIC_PriorityGroupConfig(1536);
    interrupt_config.NVIC_IRQChannel = 6;
    interrupt_config.NVIC_IRQChannelPreemptionPriority = 2;
    interrupt_config.NVIC_IRQChannelSubPriority = 2;
    interrupt_config.NVIC_IRQChannelCmd = 1;
    NVIC_Init(&interrupt_config);
}
/* Routes PB4 to EXTI4 and enables IRQ10. */
void board_initialize_pb4_interrupt(void) {
    GPIO_InitTypeDef gpio_config;
    NVIC_InitTypeDef interrupt_config;
    EXTI_InitTypeDef exti_config;
    RCC_APB2PeriphClockCmd(9, 1);
    gpio_config.GPIO_Pin = 16;
    gpio_config.GPIO_Mode = 72;
    GPIO_Init((uint *)1073810432, &gpio_config);
    GPIO_EXTILineConfig(1, 4);
    exti_config.EXTI_Line = 16;
    exti_config.EXTI_Mode = 0;
    exti_config.EXTI_Trigger = 12;
    exti_config.EXTI_LineCmd = 1;
    EXTI_Init(&exti_config);
    NVIC_PriorityGroupConfig(1792);
    interrupt_config.NVIC_IRQChannel = 10;
    interrupt_config.NVIC_IRQChannelPreemptionPriority = 0;
    interrupt_config.NVIC_IRQChannelSubPriority = 0;
    interrupt_config.NVIC_IRQChannelCmd = 1;
    NVIC_Init(&interrupt_config);
}

/* Configures eleven ADC1 channels and DMA1 channel1 then calibrates.
   match: I keep ADC, GPIO and DMA local declarations and assignments in their matched order. */
void board_initialize_adc_dma(void)

{
    bool calibration_busy;
    ADC_InitTypeDef adc_config;
    GPIO_InitTypeDef gpio_config;
    DMA_InitTypeDef dma_config;

    RCC_APB2PeriphClockCmd(0x201, 1);
    RCC_ADCCLKConfig(0x8000);
    RCC_AHBPeriphClockCmd(1, 1);
    RCC_APB2PeriphClockCmd(0x10, 1);
    gpio_config.GPIO_Pin = 0xf;
    gpio_config.GPIO_Mode = 0x0;
    GPIO_Init((void *)0x40011000, &gpio_config);
    RCC_APB2PeriphClockCmd(4, 1);
    gpio_config.GPIO_Pin = 0x1e;
    gpio_config.GPIO_Mode = 0x0;
    GPIO_Init((void *)0x40010800, &gpio_config);
    RCC_APB2PeriphClockCmd(4, 1);
    gpio_config.GPIO_Pin = 0x1;
    gpio_config.GPIO_Mode = 0x28;
    GPIO_Init((void *)0x40010800, &gpio_config);
    RCC_APB2PeriphClockCmd(8, 1);
    gpio_config.GPIO_Pin = 0x3;
    gpio_config.GPIO_Mode = 0x0;
    GPIO_Init((void *)0x40010C00, &gpio_config);
    DMA_DeInit((void *)0x40020008);
    dma_config.DMA_PeripheralBaseAddr = 0x4001244c;
    dma_config.DMA_MemoryBaseAddr = (uint)adc_dma_buf;
    dma_config.DMA_DIR = 0;
    dma_config.DMA_BufferSize = 0xb;
    dma_config.DMA_PeripheralInc = 0;
    dma_config.DMA_MemoryInc = 0x80;
    dma_config.DMA_PeripheralDataSize = 0x100;
    dma_config.DMA_MemoryDataSize = 0x400;
    dma_config.DMA_Mode = 0x20;
    dma_config.DMA_Priority = 0x2000;
    dma_config.DMA_M2M = 0;
    DMA_Init((void *)0x40020008, &dma_config);
    DMA_Cmd((void *)0x40020008, 1);
    adc_config.ADC_Mode = 0x0;
    adc_config.ADC_ScanConvMode = 0x1;
    adc_config.ADC_ContinuousConvMode = 0x0;
    adc_config.ADC_ExternalTrigConv = 0xe0000;
    adc_config.ADC_DataAlign = 0x0;
    adc_config.ADC_NbrOfChannel = 0xb;
    ADC_Init((void *)0x40012400, &adc_config);
    ADC_RegularChannelConfig((void *)0x40012400, 1, 1, 4);
    ADC_RegularChannelConfig((void *)0x40012400, 2, 2, 4);
    ADC_RegularChannelConfig((void *)0x40012400, 3, 3, 4);
    ADC_RegularChannelConfig((void *)0x40012400, 4, 4, 4);
    ADC_RegularChannelConfig((void *)0x40012400, 8, 5, 4);
    ADC_RegularChannelConfig((void *)0x40012400, 9, 6, 4);
    ADC_RegularChannelConfig((void *)0x40012400, 10, 7, 4);
    ADC_RegularChannelConfig((void *)0x40012400, 0xb, 8, 4);
    ADC_RegularChannelConfig((void *)0x40012400, 0xc, 9, 4);
    ADC_RegularChannelConfig((void *)0x40012400, 0xd, 10, 4);
    ADC_RegularChannelConfig((void *)0x40012400, 0, 0xb, 4);
    ADC_TempSensorVrefintCmd(1);
    ADC_DMACmd((void *)0x40012400, 1);
    ADC_ITConfig((void *)0x40012400, 0x220, 1);
    ADC_Cmd((void *)0x40012400, 1);
    ADC_ResetCalibration((void *)0x40012400);
    do {
        calibration_busy = ADC_GetResetCalibrationStatus((void *)0x40012400);
    } while (calibration_busy);
    ADC_StartCalibration((void *)0x40012400);
    do {
        calibration_busy = ADC_GetCalibrationStatus((void *)0x40012400);
    } while (calibration_busy);
    return;
}

/* Configures PC5 output high; SPI register frames drive it low/high. */
void board_initialize_afe_chip_select(void)

{
    GPIO_InitTypeDef gpio_config;

    RCC_APB2PeriphClockCmd(0x10, 1);
    gpio_config.GPIO_Pin = 0x20;
    gpio_config.GPIO_Mode = 0x10;
    gpio_config.GPIO_Speed = 0x3;
    GPIO_Init((void *)0x40011000, &gpio_config);
    GPIO_SetBits((void *)0x40011000, 0x20);
    return;
}

/* Writes PC5 level around AFE SPI frames. */
void board_set_afe_chip_select(int level)

{
    if (level != 0) {
        GPIO_SetBits((void *)0x40011000, 0x20);
        return;
    }
    GPIO_ResetBits((void *)0x40011000, 0x20);
    return;
}

/* Configures PA5..PA7 and SPI1.
   match: The ten halfwords retain their original array representation and order. */
void board_initialize_spi1(void)

{
    GPIO_InitTypeDef gpio_config;
    ushort spi_config_words[10];

    RCC_APB2PeriphClockCmd(4, 1);
    RCC_APB2PeriphClockCmd(0x1000, 1);
    gpio_config.GPIO_Pin = 0xe0;
    gpio_config.GPIO_Mode = 0x18;
    gpio_config.GPIO_Speed = 0x3;
    GPIO_Init((void *)0x40010800, &gpio_config);
    GPIO_SetBits((void *)0x40010800, 0xe0);
    spi_config_words[0] = 0;
    spi_config_words[1] = 0x104;
    spi_config_words[2] = 0x800;
    spi_config_words[3] = 0;
    spi_config_words[4] = 0;
    spi_config_words[5] = 0x200;
    spi_config_words[6] = 0x28;
    spi_config_words[7] = 0;
    spi_config_words[8] = 7;
    SPI_Init(&(*(uint *)0x40013000), spi_config_words);
    SPI_Cmd(&(*(uint *)0x40013000), 1);
    SPI_I2S_ClearFlag((void *)0x40013000, 2);
    SPI_I2S_ClearFlag((void *)0x40013000, 1);
    return;
}

/* Configures timer at 40000400. */
void board_initialize_tim3(void) {
    TIM_TimeBaseInitTypeDef timer_config;
    RCC_APB1PeriphClockCmd(2, 1);
    timer_config.TIM_Prescaler = 199;
    timer_config.TIM_CounterMode = 0;
    timer_config.TIM_Period = 72;
    timer_config.TIM_ClockDivision = 0;
    timer_config.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit((ushort *)1073742848, &timer_config);
    TIM_ITConfig((void *)1073742848, 1, 1);
    TIM_Cmd((ushort *)1073742848, 1);
}
/* Configures timer at 40000800. */
void board_initialize_tim4(void) {
    TIM_TimeBaseInitTypeDef timer_config;
    RCC_APB1PeriphClockCmd(4, 1);
    timer_config.TIM_Prescaler = 99;
    timer_config.TIM_CounterMode = 0;
    timer_config.TIM_Period = 18000;
    timer_config.TIM_ClockDivision = 0;
    timer_config.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit((ushort *)1073743872, &timer_config);
    TIM_ITConfig((void *)1073743872, 1, 1);
    TIM_Cmd((ushort *)1073743872, 1);
}

/* Sets vector base and peripheral interrupt priorities. */
void board_initialize_interrupts(void)

{
    NVIC_InitTypeDef interrupt_config;

    NVIC_SetVectorTable(0x8000000, 0x14000);
    NVIC_PriorityGroupConfig(0x600);
    interrupt_config.NVIC_IRQChannel = 0x1d;
    interrupt_config.NVIC_IRQChannelPreemptionPriority = 0x0;
    interrupt_config.NVIC_IRQChannelSubPriority = 0x0;
    interrupt_config.NVIC_IRQChannelCmd = 0x1;
    NVIC_Init(&interrupt_config);
    interrupt_config.NVIC_IRQChannel = 0x1e;
    interrupt_config.NVIC_IRQChannelPreemptionPriority = 0x0;
    interrupt_config.NVIC_IRQChannelSubPriority = 0x0;
    interrupt_config.NVIC_IRQChannelCmd = 0x1;
    NVIC_Init(&interrupt_config);
    interrupt_config.NVIC_IRQChannel = 0x12;
    interrupt_config.NVIC_IRQChannelPreemptionPriority = 0x0;
    interrupt_config.NVIC_IRQChannelSubPriority = 0x1;
    interrupt_config.NVIC_IRQChannelCmd = 0x1;
    NVIC_Init(&interrupt_config);
    interrupt_config.NVIC_IRQChannel = 0x25;
    interrupt_config.NVIC_IRQChannelPreemptionPriority = 0x0;
    interrupt_config.NVIC_IRQChannelSubPriority = 0x5;
    interrupt_config.NVIC_IRQChannelCmd = 0x1;
    NVIC_Init(&interrupt_config);
    interrupt_config.NVIC_IRQChannel = 0x27;
    interrupt_config.NVIC_IRQChannelPreemptionPriority = 0x0;
    interrupt_config.NVIC_IRQChannelSubPriority = 0x6;
    interrupt_config.NVIC_IRQChannelCmd = 0x1;
    NVIC_Init(&interrupt_config);
    interrupt_config.NVIC_IRQChannel = 0x34;
    interrupt_config.NVIC_IRQChannelPreemptionPriority = 0x0;
    interrupt_config.NVIC_IRQChannelSubPriority = 0x7;
    interrupt_config.NVIC_IRQChannelCmd = 0x1;
    NVIC_Init(&interrupt_config);
    sys_irq_enable();
    return;
}

/* Sets CDC line coding to 57600 baud 8-bit. */
void board_initialize_usb_line_settings(void)

{
    usb_line_config.baud = 0xe100;
    usb_line_config.bits = 8;
    usb_line_config.parity = 0;
    usb_line_config.stop = 0;
    return;
}

/* Calls board setup routines in original order. */
void board_initialize_peripherals(void) {
    board_initialize_debug_pins();
    board_initialize_control_outputs();
    board_set_control_output(1, 0);
    board_set_control_output(2, 0);
    board_initialize_pb9();
    board_initialize_pb8();
    board_initialize_auxiliary_outputs();
    board_initialize_pb12_to_pb15_inputs();
    board_initialize_tim3();
    board_initialize_tim4();
    board_initialize_adc_dma();
    board_initialize_pc9();
    board_initialize_uarts();
    board_initialize_afe_chip_select();
    board_initialize_pc13();
    board_initialize_pc4();
    board_initialize_spi1();
    board_initialize_pb4_interrupt();
    board_initialize_usb_line_settings();
    board_initialize_rtc_alarm_interrupt();
    board_initialize_interrupts();
}
