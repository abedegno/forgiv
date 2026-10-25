/* at: ADC_DeInit 0x0802E044, ADC_Init 0x0802E092, ADC_StructInit 0x0802E0D8, ADC_Cmd 0x0802E0EA, ADC_DMACmd 0x0802E100, ADC_ITConfig 0x0802E116, ADC_ResetCalibration 0x0802E12E, ADC_GetResetCalibrationStatus 0x0802E138, ADC_StartCalibration 0x0802E14C, ADC_GetCalibrationStatus 0x0802E156, ADC_SoftwareStartConvCmd 0x0802E16A, ADC_GetSoftwareStartConvStatus 0x0802E180, ADC_DiscModeChannelCountConfig 0x0802E194, ADC_DiscModeCmd 0x0802E1AC, ADC_RegularChannelConfig 0x0802E1C2, ADC_ExternalTrigConvCmd 0x0802E27A, ADC_GetConversionValue 0x0802E290, ADC_GetDualModeConversionValue 0x0802E298, ADC_AutoInjectedConvCmd 0x0802E2A0, ADC_InjectedDiscModeCmd 0x0802E2B6, ADC_ExternalTrigInjectedConvConfig 0x0802E2CC, ADC_ExternalTrigInjectedConvCmd 0x0802E2DC, ADC_SoftwareStartInjectedConvCmd 0x0802E2F2, ADC_GetSoftwareStartInjectedConvCmdStatus 0x0802E308, ADC_InjectedChannelConfig 0x0802E31C, ADC_InjectedSequencerLengthConfig 0x0802E39E, ADC_SetInjectedOffset 0x0802E3B6, ADC_GetInjectedConversionValue 0x0802E3CA, ADC_AnalogWatchdogCmd 0x0802E3E6, ADC_AnalogWatchdogThresholdsConfig 0x0802E3F6, ADC_AnalogWatchdogSingleChannelConfig 0x0802E3FC, ADC_TempSensorVrefintCmd 0x0802E40C, ADC_GetFlagStatus 0x0802E44C, ADC_ClearFlag 0x0802E45E, ADC_GetITStatus 0x0802E464, ADC_ClearITPendingBit 0x0802E488 */
/* opts: --cpu Cortex-M3 -O0 -DSTM32F10X_CL -DUSE_STDPERIPH_DRIVER -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x -I{root}/lib/stdperiph35/Project/STM32F10x_StdPeriph_Template */
/* ST STM32F10x Standard Peripheral Library V3.5.0, stm32f10x_adc.c: the ADC driver.
   The file includes ST's source from lib/stdperiph35 and compiles it whole.
   Job in this firmware: board.c sets up and calibrates the ADCs for the measurement sampling (adc_dma_init);
   ADC1_2_IRQHandler in stm32f10x_it.c reads the results.
   Changes: none. The included file is V3.5.0 as shipped, built with STM32F10X_CL and
   USE_STDPERIPH_DRIVER.
   match: built without --split_sections, as the vendor's build was: the driver is one section,
   so armlink keeps every function in it, and the image holds functions nothing calls. The at:
   line gives each function's address for the proof.
   name: original. The function names and their order are ST's; the gate proves the file whole. */
#include "../../lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_adc.c"
