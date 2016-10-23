#ifndef _TIMERS_H_
#define _TIMERS_H_
#include "stdint.h"
#define MAX_COMMA 256

typedef struct TIME2_T
{
	uint32_t		time_ms;
    uint32_t    time_s;
   
}TIME2_T;


void clean_time2_flags(void);
uint32_t get_time_s(void);
uint32_t get_time_ms(void);
#endif
