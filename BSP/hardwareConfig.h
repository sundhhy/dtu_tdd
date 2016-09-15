#ifndef __HARDWARECONFIG_H__
#define __HARDWARECONFIG_H__
#include "stm32f10x_gpio.h"

typedef struct 
{
	GPIO_TypeDef	*Port;
	uint16_t		pin;
}gpio_pins;




extern gpio_pins	Gprs_powerkey;

#endif
