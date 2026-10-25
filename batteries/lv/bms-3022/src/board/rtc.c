/* at: RTC_Get_Week 0x08016600, Is_Leap_Year 0x0801664C, RTC_Get 0x0801667A, RTC_Set 0x0801674E, RTC_Init 0x08016810, RTC_IRQHandler 0x080168D0, RTC_Alarm_Set 0x080168FE, rtc_time 0x20000105, rtc_daycnt 0x20000218, os_task_dly_idx 0x2000061E, table_week 0x0803098A, mon_table 0x08030996 */
/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast */
/* I follow calendar conversion and RTC setup here. The weekday and month tables stay with these routines.
   Entry examples: RTC_Get_Week, Is_Leap_Year, RTC_Get.
   name: I use this descriptive subsystem filename; I have no original filename evidence. */
#include "bms_driver_types.h"

/* the month correction table for the day of the week, and the days of each month */
const uchar table_week[12] = {0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5};
const uchar mon_table[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/* Day of the week (0 Sunday) for a year, month and day. RTC_Get passes the year as years since
   2000, so the century test (c > 19) never adds the 2000s correction and the weekday is wrong;
   nothing reads rtc_time.week. */
uchar RTC_Get_Week(uint year, uint month, int day) {
    uchar c;
    uchar y;
    ushort w;

    c = year / 100;
    y = year % 100;
    if (c > 19) {
        y = y + 100;
    }
    w = y + y / 4;
    w = w % 7;
    w = w + day + table_week[month - 1];
    if (y % 4 == 0 && month < 3) {
        w--;
    }
    return w % 7;
}

/* 1 for a leap year. RTC_Get also calls it with years since 2000, which gives the right answer
   for 2000-2099 because the remainder by 4 is the same. */
int Is_Leap_Year(uint year) {

    if (year % 4 == 0) {
        if (year % 0x64 == 0) {
            if (year % 0x190 == 0) {
                return 1;
            }
            return 0;
        }
        return 1;
    }
    return 0;
}

/* Converts the RTC counter into rtc_time. The date part is recomputed only when the day count
   changes. */
int RTC_Get() {
    uint timecount = 0;
    uint temp = 0;
    ushort temp1 = 0;

    timecount = RTC_GetCounter();
    temp = timecount / 86400;
    if (rtc_daycnt != temp) {
        rtc_daycnt = temp;
        temp1 = 1970;
        while (temp >= 365) {
            if (Is_Leap_Year(temp1)) {
                if (temp >= 366)
                    temp -= 366;
                else {
                    temp1++;
                    break;
                }
            } else
                temp -= 365;
            temp1++;
        }
        rtc_time.year = temp1 - 2000;
        temp1 = 0;
        while (temp >= 28) {
            if (Is_Leap_Year(rtc_time.year) && temp1 == 1) {
                if (temp >= 29)
                    temp -= 29;
                else
                    break;
            } else {
                if (temp >= mon_table[temp1])
                    temp -= mon_table[temp1];
                else
                    break;
            }
            temp1++;
        }
        rtc_time.month = temp1 + 1;
        rtc_time.date = temp + 1;
    }
    temp = timecount % 86400;
    rtc_time.hour = temp / 3600;
    rtc_time.min = (temp % 3600) / 60;
    rtc_time.sec = (temp % 3600) % 60;
    rtc_time.week = RTC_Get_Week(rtc_time.year, rtc_time.month, rtc_time.date);
    return 0;
}

/* Sets the RTC counter from a date and time in 1970-2099. Returns 1 for a year outside that
   range. */
int RTC_Set(ushort syear, uchar smon, uchar sday, uchar hour, uchar min, uchar sec) {
    ushort t;
    uint seccount = 0;

    if (syear < 1970 || syear > 2099)
        return 1;
    for (t = 1970; t < syear; t++) {
        if (Is_Leap_Year(t))
            seccount += 31622400;
        else
            seccount += 31536000;
    }
    smon -= 1;
    for (t = 0; t < smon; t++) {
        seccount += (uint)mon_table[t] * 86400;
        if (Is_Leap_Year(syear) && t == 1)
            seccount += 86400;
    }
    seccount += (uint)(sday - 1) * 86400;
    seccount += (uint)hour * 3600;
    seccount += (uint)min * 60;
    seccount += sec;
    RCC_APB1PeriphClockCmd(0x18000000, 1);
    PWR_BackupAccessCmd(1);
    RTC_SetCounter(seccount);
    RTC_WaitForLastTask();
    return 0;
}

/* Starts the RTC. When BKP DR4 does not hold the "set" mark it resets the backup domain, starts
   the LSE (waiting up to 250 x 10 ticks; returns 0 when it does not start), selects it, sets the
   1 Hz prescaler, sets 2017-03-01 02:03:04 and writes the mark. Then reads the time. Returns 1
   when the RTC runs. */
int RTC_Init() {
    uchar wait_count;

    wait_count = 0;
    RCC_APB1PeriphClockCmd(0x18000000, 1);
    PWR_BackupAccessCmd(1);
    if (BKP_ReadBackupRegister(4) != 0xa5a5 && BKP_ReadBackupRegister(4) != 0x5050) {
        BKP_DeInit();
        RCC_LSEConfig(1);
        while (RCC_GetFlagStatus(0x41) == 0 && wait_count < 0xfa) {
            wait_count++;
            os_task_delay((uchar)os_task_dly_idx, 0xa, 0x1f4);
        }
        if (wait_count >= 0xfa) {
            return 0;
        }
        RCC_RTCCLKConfig(0x100);
        RCC_RTCCLKCmd(1);
        RTC_WaitForLastTask();
        RTC_WaitForSynchro();
        RTC_EnterConfigMode();
        RTC_SetPrescaler(0x7fff);
        RTC_WaitForLastTask();
        RTC_Set(0x7e1, 3, 1, 2, 3, 4);
        RTC_ExitConfigMode();
        BKP_WriteBackupRegister(4, 0xa5a5);
    } else {
        RTC_WaitForSynchro();
        RTC_WaitForLastTask();
    }
    RTC_Get();
    return 1;
}

/* RTC interrupt: reads the time on the second or alarm flag (never enabled; see the header). */
void RTC_IRQHandler() {
    if (RTC_GetITStatus(1) != 0) {
        RTC_Get();
    }
    if (RTC_GetITStatus(2) != 0) {
        RTC_ClearITPendingBit(2);
        RTC_Get();
    }
    RTC_ClearITPendingBit(5);
    RTC_WaitForLastTask();
    return;
}

/* Dead. Sets the RTC alarm to a date and time, like RTC_Set. */
int RTC_Alarm_Set(ushort syear, uchar smon, uchar sday, uchar hour, uchar min, uchar sec) {
    ushort t;
    uint seccount = 0;

    if (syear < 1970 || syear > 2099)
        return 1;
    for (t = 1970; t < syear; t++) {
        if (Is_Leap_Year(t))
            seccount += 31622400;
        else
            seccount += 31536000;
    }
    smon -= 1;
    for (t = 0; t < smon; t++) {
        seccount += (uint)mon_table[t] * 86400;
        if (Is_Leap_Year(syear) && t == 1)
            seccount += 86400;
    }
    seccount += (uint)(sday - 1) * 86400;
    seccount += (uint)hour * 3600;
    seccount += (uint)min * 60;
    seccount += sec;
    RCC_APB1PeriphClockCmd(0x18000000, 1);
    PWR_BackupAccessCmd(1);
    RTC_SetAlarm(seccount);
    RTC_WaitForLastTask();
    return 0;
}
