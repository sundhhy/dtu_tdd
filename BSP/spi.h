#ifndef __SPI_H_
#define __SPI_H_
#include "stm32f10x_spi.h"

#define SPI_RXBUF_LEN	64
#define CMD_SET_TXBLOCK	1
#define CMD_CLR_TXBLOCK	2
#define CMD_SET_RXBLOCK	3
#define CMD_CLR_RXBLOCK	4
#define SET_TXWAITTIME_MS	5
#define SET_RXWAITTIME_MS	6

typedef struct  {
	short	tx_block;		//×èÈû±êÖ¾
	short	rx_block;
	short	tx_waittime_ms;
	short	rx_waittime_ms;

}io_ctl;

typedef struct {
	void 	*spi_base;
	void	*config;
	int 	irq;
	io_ctl	*pctl;
	uint8_t	*rx_buf;
	
}SPI_instance;




int	spi_init( SPI_instance	*spi);
int	spi_close( SPI_instance	*spi);
int spi_write( SPI_instance *spi, uint8_t *data, int len);
int spi_read( SPI_instance *spi, uint8_t *data, int len);
void spi_ioctl(SPI_instance *spi, int cmd, ...);
#endif
