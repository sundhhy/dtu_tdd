#include "times.h"
#include "stm32f10x_tim.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cmsis_os.h"
TIME2_T g_time2;





uint32_t get_time_s(void)
{
	return g_time2.time_s ;
	
}

uint32_t get_time_ms(void)
{
	return g_time2.time_ms ;
	
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





