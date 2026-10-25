/* stdperiph_types.h: the ST StdPeriph V3.5.0 init structures A316's code builds on its stack,
   with the layouts armcc gives them (enums are one byte: armcc sizes an enum to its values).
   Used by drafts that pass a local to a StdPeriph *_Init function (tools/drafts.py, structify).
   name: original (StdPeriph type names). */
#ifndef STDPERIPH_TYPES_H
#define STDPERIPH_TYPES_H
typedef struct {
    unsigned short GPIO_Pin;
    unsigned char GPIO_Speed;
    unsigned char GPIO_Mode;
} GPIO_InitTypeDef;
typedef struct {
    unsigned char NVIC_IRQChannel;
    unsigned char NVIC_IRQChannelPreemptionPriority;
    unsigned char NVIC_IRQChannelSubPriority;
    unsigned char NVIC_IRQChannelCmd;
} NVIC_InitTypeDef;
typedef struct {
    unsigned int USART_BaudRate;
    unsigned short USART_WordLength;
    unsigned short USART_StopBits;
    unsigned short USART_Parity;
    unsigned short USART_Mode;
    unsigned short USART_HardwareFlowControl;
} USART_InitTypeDef;
typedef struct {
    unsigned short TIM_Prescaler;
    unsigned short TIM_CounterMode;
    unsigned short TIM_Period;
    unsigned short TIM_ClockDivision;
    unsigned char TIM_RepetitionCounter;
} TIM_TimeBaseInitTypeDef;
typedef struct {
    unsigned int DMA_PeripheralBaseAddr;
    unsigned int DMA_MemoryBaseAddr;
    unsigned int DMA_DIR;
    unsigned int DMA_BufferSize;
    unsigned int DMA_PeripheralInc;
    unsigned int DMA_MemoryInc;
    unsigned int DMA_PeripheralDataSize;
    unsigned int DMA_MemoryDataSize;
    unsigned int DMA_Mode;
    unsigned int DMA_Priority;
    unsigned int DMA_M2M;
} DMA_InitTypeDef;
typedef struct {
    unsigned int ADC_Mode;
    unsigned char ADC_ScanConvMode;
    unsigned char ADC_ContinuousConvMode;
    unsigned int ADC_ExternalTrigConv;
    unsigned int ADC_DataAlign;
    unsigned char ADC_NbrOfChannel;
} ADC_InitTypeDef;
typedef struct {
    unsigned int EXTI_Line;
    unsigned char EXTI_Mode;
    unsigned char EXTI_Trigger;
    unsigned char EXTI_LineCmd;
} EXTI_InitTypeDef;
typedef struct {
    unsigned short TIM_OCMode;
    unsigned short TIM_OutputState;
    unsigned short TIM_OutputNState;
    unsigned short TIM_Pulse;
    unsigned short TIM_OCPolarity;
    unsigned short TIM_OCNPolarity;
    unsigned short TIM_OCIdleState;
    unsigned short TIM_OCNIdleState;
} TIM_OCInitTypeDef;
#endif
