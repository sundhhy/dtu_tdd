#include "times.h"
#include "stm32f10x_tim.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cmsis_os.h"
#include "sdhError.h"
#include "osObjects.h"

TIME2_T g_time2;



static char	Set_AlarmClock_flag[MAX_ALARM_TOP] = {0};
static uint32_t	Alarmtims_ms[MAX_ALARM_TOP] = {0};
static uint32_t	AlarmStart_ms[MAX_ALARM_TOP] = {0};

uint32_t get_time_s(void)
{
	return g_time2.time_s ;
	
}

uint32_t get_time_ms(void)
{
	return g_time2.time_ms ;
	
}

//一次只允许3设置一个闹钟
//当已经设置了一个闹钟以后，只有它已经获取以后才能再次设置新的闹钟
void set_alarmclock_s(int alarm_id, int sec)
{
	if( alarm_id >= MAX_ALARM_TOP)
		return ;
	
	if( Set_AlarmClock_flag[alarm_id] == 0)
	{
		
		Alarmtims_ms[alarm_id] = sec * 1000;
		AlarmStart_ms[alarm_id] = g_time2.time_ms + g_time2.time_s *1000;
		Set_AlarmClock_flag[alarm_id] = 1;
	}
	
}

void set_alarmclock_ms(int alarm_id, int msec)
{
	if( alarm_id >= MAX_ALARM_TOP)
		return ;
	
	if( Set_AlarmClock_flag[alarm_id] == 0)
	{
		
		Alarmtims_ms[alarm_id] = msec;

		AlarmStart_ms[alarm_id] = g_time2.time_ms + g_time2.time_s *1000;
		Set_AlarmClock_flag[alarm_id] = 1;
	}
	
}
int Ringing(int alarm_id)
{
	uint32_t now_ms = 0;
	if( alarm_id >= MAX_ALARM_TOP)
		return ERR_BAD_PARAMETER;
	if( Set_AlarmClock_flag[alarm_id])
	{
		now_ms = g_time2.time_ms + g_time2.time_s *1000;
		if( now_ms - AlarmStart_ms[alarm_id] >= Alarmtims_ms[alarm_id])
		{
			
			Set_AlarmClock_flag[alarm_id] = 0;
			return ERR_OK;
		}
		
	}
	return ERR_UNKOWN;
	
	
}

void TIM2_IRQHandler(void)          //定时器中断约10ms
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    { 
			g_time2.time_ms += 10;
			if( g_time2.time_ms >= 1000)
			{
				g_time2.time_s ++;
				g_time2.time_ms = 0;
				feed_iwwg();
			}   

			TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
			TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}

void clean_time2_flags(void)
{
    g_time2.time_ms = 0;
    g_time2.time_s = 0;
   
}

void time_test(void)
{
	while(1)
	{
		set_alarmclock_s(0, 10);
		while(1)
		{
			if( Ringing(0) == ERR_OK)
				break;
			 
		}
		
		
	}
	
	
}



