#ifndef BMS_DRIVER_TYPES_H
#define BMS_DRIVER_TYPES_H
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef struct {
    uchar sec, min, hour, date, month, year, week;
} rtc_cal_t;
extern rtc_cal_t rtc_time;
extern ushort rtc_daycnt, os_task_dly_idx;
void os_task_delay(int, ushort, ushort);
uint RTC_GetCounter(void);
int RTC_GetITStatus(ushort);
void RTC_ClearITPendingBit(ushort);
void RTC_WaitForLastTask(void);
void RTC_WaitForSynchro(void);
void RTC_EnterConfigMode(void);
void RTC_ExitConfigMode(void);
void RTC_SetPrescaler(uint);
void RTC_SetCounter(uint);
void RTC_SetAlarm(uint);
void RCC_APB1PeriphClockCmd(uint, int);
void PWR_BackupAccessCmd(int);
ushort BKP_ReadBackupRegister(ushort);
void BKP_WriteBackupRegister(ushort, ushort);
void BKP_DeInit(void);
void RCC_LSEConfig(uchar);
int RCC_GetFlagStatus(uchar);
void RCC_RTCCLKConfig(uint);
void RCC_RTCCLKCmd(int);
#endif
