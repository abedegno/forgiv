/* at: ADC_DeInit 0x0802D7C4, ADC_Init 0x0802D812, ADC_StructInit 0x0802D858, ADC_Cmd 0x0802D86A, ADC_DMACmd 0x0802D880, ADC_ITConfig 0x0802D896, ADC_ResetCalibration 0x0802D8AE, ADC_GetResetCalibrationStatus 0x0802D8B8, ADC_StartCalibration 0x0802D8CC, ADC_GetCalibrationStatus 0x0802D8D6, ADC_SoftwareStartConvCmd 0x0802D8EA, ADC_GetSoftwareStartConvStatus 0x0802D900, ADC_DiscModeChannelCountConfig 0x0802D914, ADC_DiscModeCmd 0x0802D92C, ADC_RegularChannelConfig 0x0802D942, ADC_ExternalTrigConvCmd 0x0802D9FA, ADC_GetConversionValue 0x0802DA10, ADC_GetDualModeConversionValue 0x0802DA18, ADC_AutoInjectedConvCmd 0x0802DA20, ADC_InjectedDiscModeCmd 0x0802DA36, ADC_ExternalTrigInjectedConvConfig 0x0802DA4C, ADC_ExternalTrigInjectedConvCmd 0x0802DA5C, ADC_SoftwareStartInjectedConvCmd 0x0802DA72, ADC_GetSoftwareStartInjectedConvCmdStatus 0x0802DA88, ADC_InjectedChannelConfig 0x0802DA9C, ADC_InjectedSequencerLengthConfig 0x0802DB1E, ADC_SetInjectedOffset 0x0802DB36, ADC_GetInjectedConversionValue 0x0802DB4A, ADC_AnalogWatchdogCmd 0x0802DB66, ADC_AnalogWatchdogThresholdsConfig 0x0802DB76, ADC_AnalogWatchdogSingleChannelConfig 0x0802DB7C, ADC_TempSensorVrefintCmd 0x0802DB8C, ADC_GetFlagStatus 0x0802DBCC, ADC_ClearFlag 0x0802DBDE, ADC_GetITStatus 0x0802DBE4, ADC_ClearITPendingBit 0x0802DC08 */
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
