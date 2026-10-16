/* rtc.h: the RTC calendar (drivers/rtc.c, the common STM32 RTC_Get/RTC_Set code) and the time
   the application keeps.
   name: descriptive (readability step 1). */
#ifndef RTC_H
#define RTC_H

#include "ghidra_types.h"

/* the calendar RTC_Get fills from the RTC counter (the common STM32 RTC calendar code); year is years since 2000, as HR35 shows it */
typedef struct {
    uchar sec;   /* +0x00 */
    uchar min;   /* +0x01 */
    uchar hour;  /* +0x02 */
    uchar date;  /* +0x03 */
    uchar month; /* +0x04 */
    uchar year;  /* +0x05 */
    uchar week;  /* +0x06 computed with the wrong century and never read */
} rtc_cal_t;

/* data */
extern ushort rtc_daycnt;
extern uchar rtc_ready[]; /* 1 once RTC_Init has run (or failed twice) */
extern rtc_cal_t rtc_time;

/* functions */
int Is_Leap_Year(int year);
int RTC_Alarm_Set(ushort syear, uchar smon, uchar sday, uchar hour, uchar min, uchar sec);
int RTC_Get();
int RTC_GetCounter(void);
undefined4 RTC_GetITStatus(ushort);
uchar RTC_Get_Week(int year, int month, int day);
void RTC_IRQHandler();
int RTC_Init();
void RTC_NVIC_Config();
int RTC_Set(ushort syear, uchar smon, uchar sday, uchar hour, uchar min, uchar sec);
void RTC_SetAlarm(int);
void RTC_SetCounter(int);
void RTC_SetPrescaler(int);
uchar bin_to_bcd(int value, int digits);

#endif
