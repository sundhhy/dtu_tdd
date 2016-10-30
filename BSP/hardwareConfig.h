#ifndef __HARDWARECONFIG_H__
#define __HARDWARECONFIG_H__
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "spi.h"
#include "stm32f10x_adc.h"

#define	GPRS_COM			3			///< gprs模块使用的串口号	
#define	DEBUG_COM			1
#define	SERAIL_485_COM		2
#define W25Q_SPI			SPI1
#define ADC_BASE			ADC1

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

#if SERAIL_485_COM == 2
#define SER485_USART	USART2
#endif

typedef struct 
{
	GPIO_TypeDef	*Port;
	uint16_t		pin;
}gpio_pins;


///stm32的外设的DMA请求与DMA通道的对应关系是固定的，不是随便配置的。参考STM32的参考手册
typedef struct 
{
	
	DMA_Channel_TypeDef		*dma_tx_base;
	int32_t					dma_tx_flag;
	int32_t 					dma_tx_irq;
	
	DMA_Channel_TypeDef		*dma_rx_base;
	int32_t					dma_rx_flag;
	int32_t 					dma_rx_irq;
	

}Dma_source;


extern gpio_pins	Gprs_powerkey;

extern gpio_pins	W25Q_csPin;
extern SPI_instance W25Q_Spi ;
extern gpio_pins 	ADC_pins;
extern int32_t ADC_chn;

extern USART_InitTypeDef USART_InitStructure;
extern USART_InitTypeDef Conf_GprsUsart;
extern USART_InitTypeDef Conf_S485Usart_default;

extern Dma_source DMA_gprs_usart;
extern Dma_source DMA_s485_usart;
extern Dma_source DMA_adc;

#endif
