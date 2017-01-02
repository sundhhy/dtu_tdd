#ifndef __LED_H__
#define __LED_H__
#include "lw_oopc.h"
#include "stdint.h"
#include "CircularBuffer.h"
#include "hardwareConfig.h"

CLASS( stm32LED)
{
	gpio_pins	*pin;
	int			led_status;
	int (*init)( stm32LED *self, gpio_pins *pin);
	void (*blink)( stm32LED *self);
	void (*test)( stm32LED *self);
};

extern stm32LED	*LED_run;
extern stm32LED	*LED_com; 
#endif
