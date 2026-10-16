/* stdperiph.h: StdPeriph V3.5.0 functions as the application's callers declare them (loose
   types: the callers compiled against these, not ST's headers), and the peripheral base
   addresses they pass. src/stdperiph holds the functions.
   name: descriptive (readability step 1). */
#ifndef STDPERIPH_H
#define STDPERIPH_H

#include "ghidra_types.h"
#include "stdperiph_types.h"

/* peripheral base addresses (STM32F105/107, RM0008), as the callers pass them: plain int
   literals, not ST's typed pointers (the callers compiled against int parameters) */
#define TIM2_BASE 0x40000000
#define TIM3_BASE 0x40000400
#define TIM4_BASE 0x40000800
#define USART2_BASE 0x40004400
#define USART3_BASE 0x40004800
#define UART4_BASE 0x40004C00
#define GPIOA_BASE 0x40010800
#define GPIOB_BASE 0x40010C00
#define GPIOC_BASE 0x40011000
#define GPIOD_BASE 0x40011400
#define GPIOE_BASE 0x40011800
#define ADC1_BASE 0x40012400
#define USART1_BASE 0x40013800
#define DMA1_Channel1_BASE 0x40020008
#define RCC_BASE 0x40021000
#define FLASH_R_BASE 0x40022000
/* Cortex-M3 system control registers the code writes directly */
#define SCB_VTOR_ADDR 0xE000ED08
#define SCB_AIRCR_ADDR 0xE000ED0C
#define NVIC_ISER_ADDR 0xE000E100
#define SYSTICK_CTRL_ADDR 0xE000E010

/* functions */
void ADC_ClearITPendingBit(int, int);
void ADC_Cmd(int, int);
void ADC_DMACmd(int, int);
int ADC_GetCalibrationStatus(int);
int ADC_GetResetCalibrationStatus(int);
void ADC_ITConfig(int, int, int);
void ADC_Init(int, ADC_InitTypeDef *);
void ADC_RegularChannelConfig(int, int, int, int);
void ADC_ResetCalibration(int);
void ADC_SoftwareStartConvCmd(int, int);
void ADC_StartCalibration(int);
void ADC_TempSensorVrefintCmd(int);
void BKP_DeInit(void);
int BKP_ReadBackupRegister(int);
void BKP_WriteBackupRegister(int, int);
void DMA_Cmd(int, int);
void DMA_DeInit(int);
void DMA_Init(int, DMA_InitTypeDef *);
void EXTI_ClearITPendingBit(undefined4);
undefined4 EXTI_GetITStatus(uint);
void FLASH_ClearFlag(int);
int FLASH_ProgramWord(int, int);
void FLASH_Unlock(void);
uint GPIO_Init(uint *, ushort *);
#ifndef OWN_GPIO_ReadInputDataBit
int GPIO_ReadInputDataBit(int, int);
#endif
void GPIO_ResetBits(int, undefined4);
void GPIO_SetBits(int, undefined4);
void NVIC_Init(byte *);
void NVIC_PriorityGroupConfig(uint);
void NVIC_SetVectorTable(int, int);
void PWR_BackupAccessCmd(int);
void RCC_ADCCLKConfig(int);
void RCC_AHBPeriphClockCmd(int, int);
void RCC_APB1PeriphClockCmd(int, int);
void RCC_APB2PeriphClockCmd();
int RCC_GetFlagStatus(int);
void RCC_LSEConfig(int);
void RCC_OTGFSCLKConfig(undefined4);
void RCC_RTCCLKCmd(int);
void RCC_RTCCLKConfig(int);
void RTC_ClearITPendingBit(ushort);
void RTC_EnterConfigMode(void);
void RTC_ExitConfigMode(void);
void RTC_WaitForLastTask(void);
void RTC_WaitForSynchro(void);
void SysTick_Handler();
void TIM_ARRPreloadConfig(int, int);
void TIM_ClearFlag(int, ushort);
void TIM_ClearITPendingBit();
void TIM_Cmd(int, int);
undefined4 TIM_GetITStatus(int, ushort);
void TIM_ITConfig(int, int, int);
void TIM_TimeBaseInit(int, int);
void USART_ClearITPendingBit(int, int);
void USART_Cmd(int, int);
int USART_GetITStatus(int, int);
void USART_ITConfig(int, int, int);
void USART_Init(int, int);
int USART_ReceiveData(int);
void USART_SendData(int, int);

#endif
