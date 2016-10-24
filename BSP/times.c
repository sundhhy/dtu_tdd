#include "times.h"
#include "stm32f10x_tim.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cmsis_os.h"
#include "sdhError.h"
TIME2_T g_time2;



static char	Set_AlarmClock_S[MAX_ALARM_TOP] = {0};
static uint32_t	Alarmtims_s[MAX_ALARM_TOP] = {0};
static uint32_t	AlarmStart_s[MAX_ALARM_TOP] = {0};


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
	
	if( Set_AlarmClock_S[alarm_id] == 0)
	{
		
		Alarmtims_s[alarm_id] = sec;
		AlarmStart_s[alarm_id] = g_time2.time_s;
		Set_AlarmClock_S[alarm_id] = 1;
	}
	
}
int Ringing_s(int alarm_id)
{
	if( alarm_id >= MAX_ALARM_TOP)
		return ERR_BAD_PARAMETER;
	if( Set_AlarmClock_S[alarm_id])
	{
		if( g_time2.time_s - AlarmStart_s[alarm_id] > Alarmtims_s[alarm_id])
		{
			
			Set_AlarmClock_S[alarm_id] = 0;
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





