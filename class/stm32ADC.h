#ifndef __STM32ADC_H__
#define __STM32ADC_H__
#include "lw_oopc.h"
#include "stdint.h"
#include "CircularBuffer.h"

CLASS( stm32ADC)
{
	
	int (*init)( stm32ADC *self, void *arg);
	
};

#endif
