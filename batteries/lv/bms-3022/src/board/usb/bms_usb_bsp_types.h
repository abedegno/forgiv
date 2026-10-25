#ifndef BMS_USB_BSP_TYPES_H
#define BMS_USB_BSP_TYPES_H
typedef unsigned int uint;
typedef struct {
    unsigned char NVIC_IRQChannel, NVIC_IRQChannelPreemptionPriority, NVIC_IRQChannelSubPriority,
        NVIC_IRQChannelCmd;
} NVIC_InitTypeDef;
typedef struct {
    unsigned short TIM_Prescaler, TIM_CounterMode, TIM_Period, TIM_ClockDivision;
    unsigned char TIM_RepetitionCounter;
} TIM_TimeBaseInitTypeDef;
#define TIM2_BASE 0x40000000
extern volatile uint BSP_delay;
void NVIC_PriorityGroupConfig(uint);
void NVIC_Init(NVIC_InitTypeDef *);
void RCC_APB1PeriphClockCmd(uint, int);
void RCC_OTGFSCLKConfig(uint);
void RCC_AHBPeriphClockCmd(uint, int);
void TIM_Cmd(uint, int);
void TIM_ITConfig(uint, unsigned short, int);
void TIM_TimeBaseInit(uint, TIM_TimeBaseInitTypeDef *);
void TIM_ClearITPendingBit(uint, unsigned short);
void TIM_ARRPreloadConfig(uint, int);
int TIM_GetITStatus(uint, unsigned short);
#endif
