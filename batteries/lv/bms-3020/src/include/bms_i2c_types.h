#ifndef BMS_I2C_TYPES_H
#define BMS_I2C_TYPES_H
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
#include "bms_dates.h"
typedef struct {
    uchar reserved0, cells;
    ushort cell[16];
    uchar temps, reserved35;
    ushort temp[10];
    ushort voltage, current, soc, reserved62, soh;
    uchar reserved66[4], alarm70, alarm71, reserved72[2], alarm74;
    uchar reserved75[2], alarm77, alarm78, reserved79;
} measure_t;
extern measure_t measure;
extern uchar log_pending, log_interval, flags273;
extern volatile uchar device_status[];
extern ushort log_index, os_task_dly_idx;
extern volatile uchar alarm271, alarm272, alarm274, alarm277, alarm278;
extern uchar log_buffer[65];
void GPIO_SetBits(uint, ushort);
void GPIO_ResetBits(uint, ushort);
int GPIO_ReadInputDataBit(uint, ushort);
void os_task_delay(int, ushort, ushort);
void snapshot_publish(void);
void runtime_publish_date(void);
uint bcd_to_byte(uint);
void runtime_timer_start(uint, uint);
#endif
