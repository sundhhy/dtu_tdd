/**
* @file 		hardwareConfig.c
* @brief		系统的硬件配置
* @details		1. gprs开机引脚配置
* @author		sundh
* @date		16-09-15
* @version	A001
* @par Copyright (c): 
* 		XXX公司
* @par History:         
*	version: author, date, desc
*	A001:sundh,16-09-15,gprs的开机引脚配置
*/
#include "hardwareConfig.h"


gpio_pins	Gprs_powerkey =  {
	GPIOB,
	GPIO_Pin_0
};












