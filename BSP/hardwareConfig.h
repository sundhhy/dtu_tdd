#ifndef __HARDWARECONFIG_H__
#define __HARDWARECONFIG_H__
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"


#define	GPRS_COM			3			///< gprs模块使用的串口号	
#define	DEBUG_COM			1

#if GPRS_COM == 3
#define GPRS_USART	USART3
#elif GPRS_COM == 1
#define GPRS_USART	USART1
#endif

#if DEBUG_COM == 3
#define DEBUG_USART	USART3
#elif DEBUG_COM == 1
#define DEBUG_USART	USART1
#endif



typedef struct 
{
	GPIO_TypeDef	*Port;
	uint16_t		pin;
}gpio_pins;




extern gpio_pins	Gprs_powerkey;





extern USART_InitTypeDef USART_InitStructure;
extern USART_InitTypeDef Conf_GprsUsart;

#endif
