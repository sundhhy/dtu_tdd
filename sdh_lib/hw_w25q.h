#ifndef __W25Q_H
#define __W25Q_H
#include "hardwareConfig.h"
#include "spi.h"
#include "osObjects.h"                      // RTOS object definitions



///移植时需要修改的接口
#define W25Q_DELAY_MS(ms)				osDelay(ms)	
#define    W25Q_Enable_CS          	GPIO_ResetBits(W25Q_csPin.Port, W25Q_csPin.pin)
#define    W25Q_Disable_CS         	GPIO_SetBits(W25Q_csPin.Port, W25Q_csPin.pin);
#define SPI_WRITE(data, len)		spi_write( &W25Q_Spi, data, len)
#define SPI_READ(buf, len)			spi_read( &W25Q_Spi, buf, len)


#define SPI_MASTER_INSTANCE          (0)                 /*! User change define to choose SPI instance */
#define TRANSFER_SIZE               (64)
#define TRANSFER_BAUDRATE           (5000000U)           /*! Transfer baudrate - 1000k 这个波特率下，波形还算可以*/
#define MASTER_TRANSFER_TIMEOUT     (5000U)             /*! Transfer timeout of master - 5s */

#define PAGE_SIZE						256
#define SECTOR_SIZE					4096
#define BLOCK_SIZE					65536
#define SECTOR_HAS_PAGES		16
#define BLOCK_HAS_SECTORS		16


#define    SPIFLASH_CMD_LENGTH        0x03

//erase,program instructions
#define W25Q_INSTR_WR_ENABLE								0x06
#define W25Q_INSTR_ENWR_FOR_VlatlReg				0x50
#define W25Q_INSTR_WR_DISABLE								0x04
#define W25Q_INSTR_RD_Status_Reg1						0x05
#define W25Q_INSTR_RD_Status_Reg2						0x35
#define W25Q_INSTR_WR_Status_Reg						0x01
#define W25Q_INSTR_Page_Program							0x02
#define W25Q_INSTR_Sector_Erase_4K					0x20
#define W25Q_INSTR_BLOCK_Erase_32K					0x52
#define W25Q_INSTR_BLOCK_Erase_64K					0xD8
#define W25Q_INSTR_Chip_Erase_C7						0xC7
#define W25Q_INSTR_Chip_Erase_60						0x60
#define W25Q_INSTR_Erase_Program_Sup				0x75
#define W25Q_INSTR_Erase_Program_Res				0x7A
#define W25Q_INSTR_Page_Down								0xB5
#define W25Q_INSTR_ContinuousRD_Reset				0xff

#define W25Q_INSTR_READ_DATA								0x03


#define W25Q_STATUS1_BUSYBIT								0x01
#define W25Q_STATUS1_WEL									0x02
#define W25Q_STATUS1_BP0									0x04
#define W25Q_STATUS1_BP1									0x08
#define W25Q_STATUS1_BP2									0x10
#define W25Q_STATUS1_TB										0x20
#define W25Q_STATUS1_SEC									0x40
#define W25Q_STATUS1_SRP0									0x80


#define W25Q_STATUS2_SPR1								0x01
#define W25Q_STATUS2_QE									0x02
#define W25Q_STATUS2_RESERVED									0x04
#define W25Q_STATUS2_LB1									0x08
#define W25Q_STATUS2_LB2									0x10
#define W25Q_STATUS2_LB3									0x20
#define W25Q_STATUS2_CMP									0x40
#define W25Q_STATUS2_SUS									0x80
					

typedef struct {
	uint8_t		id[2];
	uint8_t 	recv[2];
	int			page_num;	
	int 		sector_num;
	int 		block_num;
	
	
	
}w25q_instance_t;
typedef struct {
	int32_t		page_size;						///一页的长度
	int32_t		total_pagenum;					///整个存储器的页数量
	
	uint16_t		sector_pagenum;
	uint16_t		block_pagenum;
}w25qInfo_t;

void w25q_init_cs(void);
void w25q_init_spi(void);

int w25q_init(void);

int w25q_Erase_Sector(uint16_t Sector_Number);

//该接口不会擦除扇区，调用方要事先进行擦除
//地址必须按照w25q的页大小对齐
//返回成功写入的字节数
int W25Q_write_flash(uint32_t addr, uint8_t *buf, uint32_t bytes);
//地址必须按照w25q的页大小对齐
//返回读取的字节数
int W25Q_read_flash(uint32_t addr, uint8_t *buf, uint32_t bytes);

#if NO_FILESYS == 0
void w25q_info(void *info);

int w25q_erase(uint32_t offset, uint32_t len);
int w25q_Write_Sector_Data(uint8_t *pBuffer, uint16_t Sector_Num);
int w25q_Read_Sector_Data(uint8_t *pBuffer, uint16_t Sector_Num);
int w25q_Read_page_Data(uint8_t *pBuffer, uint16_t num_page);


int w25q_Write(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t WriteBytesNum);
int w25q_rd_data(uint8_t *pBuffer, uint32_t rd_add, int len);
int w25q_close(void);
int w25q_Erase_chip_c7(void);
int w25q_Erase_chip_60(void);
#endif

#endif

