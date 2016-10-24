#ifndef _TIMERS_H_
#define _TIMERS_H_
#include "stdint.h"
//#define MAX_COMMA 256
#define MAX_ALARM_TOP		4			//支持的最多的闹铃数量
typedef struct TIME2_T
{
	uint32_t		time_ms;
    uint32_t    time_s;
   
}TIME2_T;


void clean_time2_flags(void);
uint32_t get_time_s(void);
uint32_t get_time_ms(void);
void set_alarmclock_s(int alarm_id, int sec);
int Ringing_s(int alarm_id);
#endif
