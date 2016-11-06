/**
* @file 		spi.c
* @brief		spi驱动程序.
* @details		中断用于错误处理，而不做为数据传输使用
* @author		author
* @date		date
* @version	A001
* @par Copyright (c): 
* 		XXX??
* @par History:         
*	version: author, date, desc\n
*/
#include "spi.h"
#include "hardwareConfig.h"
#include "sdhError.h"
#include "stdlib.h"
#include "stm32f10x_spi.h"
#include "osObjects.h"                      // RTOS object definitions
#include <stdarg.h>

//sCircularBuffer	Spi_rxCb;

//osSemaphoreId SemId_spiRx;                         // Semaphore ID
//uint32_t os_semaphore_cb_Sem_spiRx[2] = { 0 }; 
//const osSemaphoreDef_t os_semaphore_def_Sem_spiRx = { (os_semaphore_cb_Sem_spiRx) };

/**
 * @brief 初始化spi驱动.
 *
 * @details This is a detail description.
 * 
 * @param[in]	inArgName input argument description.
 * @param[out]	outArgName output argument description. 
 * @retval	OK	??
 * @retval	ERROR	?? 
 */
int	spi_init( SPI_instance	*spi)
{	
	spi->pctl = malloc( sizeof( io_ctl));
	if( spi->pctl == NULL)
		return ERR_MEM_UNAVAILABLE;
	
//	spi->rx_buf = malloc( SPI_RXBUF_LEN);
//	if( spi->pctl == NULL)
//	{
//		free( spi->pctl);
//		return ERR_MEM_UNAVAILABLE;
//	}
//	Spi_rxCb.buf = spi->rx_buf;
//	Spi_rxCb.read = 0;
//	Spi_rxCb.write = 0;
//	Spi_rxCb.size = SPI_RXBUF_LEN;
	
	SPI_Init( spi->spi_base, spi->config);
	SPI_I2S_ITConfig( spi->spi_base, SPI_I2S_IT_OVR, ENABLE);
	SPI_Cmd( spi->spi_base, ENABLE);
	
	return ERR_OK;
}

int	spi_close( SPI_instance	*spi)
{	
	
	
	free( spi->pctl);
	
	
	return ERR_OK;
}

int spi_sendByteForRead( SPI_instance *spi)
{
	int count_ms = spi->pctl->rx_waittime_ms;
	
		/*! Loop while DR register in not emplty */
	while( SPI_I2S_GetFlagStatus( spi->spi_base, SPI_I2S_FLAG_TXE) == RESET)
	{
		if( spi->pctl->rx_block == 0)
			return ERR_DEV_BUSY;
		
		osDelay(1);
		if( count_ms)
			count_ms --;
		else
			return ERR_DEV_TIMEOUT;
		;
	}
	 
	/*!Send byte through the SPI1 peripheral */
	SPI_I2S_SendData(spi->spi_base, 0xff);
	 
	/*! Wait to receive a byte */
	while (SPI_I2S_GetFlagStatus(spi->spi_base, SPI_I2S_FLAG_RXNE) == RESET)
	{
		osDelay(1);
		if( count_ms)
			count_ms --;
		else
			return ERR_DEV_TIMEOUT;
		
	}
	 
	/*! Return the byte read from the SPI bus */
	return SPI_I2S_ReceiveData(spi->spi_base);
}

int spi_write( SPI_instance *spi, uint8_t *data, int len)
{
	short i = 0;
	short count_ms = spi->pctl->tx_waittime_ms;
	while( SPI_I2S_GetFlagStatus( spi->spi_base, SPI_I2S_FLAG_BSY))
	{
		if( spi->pctl->tx_block == 0)
			return ERR_DEV_BUSY;
		
		osDelay(1);
		if( count_ms)
			count_ms --;
		else
			return ERR_DEV_TIMEOUT;
	}
	for( i = 0; i < len; i++)
	{
		//等待spi的发送缓冲区为空闲的时候开始发送
		while( SPI_I2S_GetFlagStatus( spi->spi_base, SPI_I2S_FLAG_TXE) != SET)
		{
			;
		}
		SPI_I2S_SendData( spi->spi_base, data[i]);
		while( SPI_I2S_GetFlagStatus( spi->spi_base, SPI_I2S_FLAG_RXNE) != SET)
		{
			;
		}
		SPI_I2S_ReceiveData( spi->spi_base); 
	}
	count_ms = spi->pctl->tx_waittime_ms;
	while( SPI_I2S_GetFlagStatus( spi->spi_base, SPI_I2S_FLAG_BSY))
	{
		if( spi->pctl->tx_block == 0)
			return ERR_DEV_BUSY;
		
		osDelay(1);
		if( count_ms)
			count_ms --;
		else
			return ERR_DEV_TIMEOUT;
	}
	
	return ERR_OK;

	
}

/**
 * @brief 初始化spi驱动.
 *
 * @details This is a detail description.
 * 
 * @param[in]	inArgName input argument description.
 * @param[out]	outArgName output argument description. 
 * @retval	OK	读取成功
 * @retval	ERROR	?? 
 */int spi_read( SPI_instance *spi, uint8_t *data, int len)
{
	int i = 0;
	int ret = 0;
	
	
	for( i = 0; i < len; i++)
	{

		ret = spi_sendByteForRead( spi);
		if( ret < 0)
			return ret;
		data[i] = ret;

		
		
	}
	return ERR_OK;	
	
}



void spi_ioctl(SPI_instance *spi, int cmd, ...)
{
	int int_data;
	va_list arg_ptr; 
	va_start(arg_ptr, cmd); 
	
	switch(cmd)
	{
		case CMD_SET_TXBLOCK:
			spi->pctl->tx_block = 1;
			break;
		case CMD_CLR_TXBLOCK:
			spi->pctl->tx_block = 0;
			break;
		case CMD_SET_RXBLOCK:
			spi->pctl->rx_block = 1;
			break;
		case CMD_CLR_RXBLOCK:
			spi->pctl->rx_block = 0;
			break;
		case SET_TXWAITTIME_MS:
			int_data = va_arg(arg_ptr, int);
			va_end(arg_ptr); 
			spi->pctl->tx_waittime_ms = int_data;
			break;
		case SET_RXWAITTIME_MS:
			int_data = va_arg(arg_ptr, int);
			va_end(arg_ptr); 
			spi->pctl->rx_waittime_ms = int_data;
			break;
		default: break;
		
	}
	
	
}



void SPI1_IRQHandler(void)
{
//	char rdata = 0;
//	if( SPI_I2S_GetFlagStatus( W25Q_SPI, SPI_I2S_FLAG_RXNE))
//	{
//		rdata = SPI_I2S_ReceiveData( W25Q_SPI);
//		if( CBWrite( &Spi_rxCb, rdata) == ERR_OK)
//			osSemaphoreRelease( SemId_spiRx);
//		
//	}
	if( SPI_I2S_GetFlagStatus( W25Q_SPI, SPI_I2S_FLAG_OVR))
	{
		//依次读取SPI_DR和SPI_SR来清除OVR
		SPI_I2S_ReceiveData( W25Q_SPI);
		SPI_I2S_GetFlagStatus( W25Q_SPI, SPI_I2S_FLAG_OVR);
	}
	
}
