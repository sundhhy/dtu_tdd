#ifndef __STM32_CHIPSET_H
#define __STM32_CHIPSET_H

typedef unsigned char  u8;
typedef unsigned short u16;

void enbale_ncss_gpioclk(void);


extern void IWDG_Configuration(void);
extern void RCC_Configuration(void);
extern void NVIC_Configuration(void);
extern void GPIO_Configuration(void);
extern void USART_Configuration(void);
extern void Init_TIM2(void);
extern void adc_Configuration(void);
#endif/* __STM32_CHIPSET_H */
