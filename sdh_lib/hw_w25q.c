/*************************************************
Copyright (C), 
File name: w25q.c
Author:		sundh
Version:	v1.0
Date: 		14-02-27
Description: 
w25Q的驱动程序，实现W25Q的标准SPI操作模式
使用MCU的SPI0来与w25q通讯
w25q要提供很多复杂的接口。
w25q不实现驱动适配器接口。
为了尽量减少对flash的操作，所有对flash的写操作都以扇区为单位进行。对于读取提供了随机地址读的方法。
Others: 
Function List: 
1. w25q_init
初始化flash，并获取id和容量信息
History: 
1. Date:
Author:
Modification:
2. w25q_Erase_Sector
擦除flash中的选定的某一个扇区
History: 
1. Date:
Author:
Modification:
3. w25q_Write_Sector_Data
像一个指定的扇区写入数据
History: 
1. Date:
Author:
Modification:
4. w25q_Read_Sector_Data
读取一个扇区
History: 
1. Date:
Author:
Modification:
5. w25q_rd_data
读取随机地址
History: 
1. Date:
Author:
Modification:
*************************************************/
#include "hw_w25q.h"
#include <string.h>
#include "sdhError.h"




w25q_instance_t  W25Q_flash;


//--------------本地私有数据--------------------------------
static uint8_t	W25Q_tx_buf[16];
static uint8_t	W25Q_rx_buf[16];
//------------本地私有函数------------------------------
static int w25q_wr_enable(void);
static uint8_t w25q_ReadSR(void);
static int w25q_write_waitbusy(uint8_t *data, int len);
//static void w25q_Write_Data(uint8_t *pBuffer,uint16_t Block_Num,uint16_t Page_Num,uint32_t WriteBytesNum);

//--------------------------------------------------------------

int w25q_init(void)
{
	static char first = 1;
	if( first)
	{
		w25q_init_cs();
		w25q_init_spi();
			
			
		W25Q_Disable_CS;
		first = 0;
	}
		
	return w25q_read_id();


}

int w25q_read_id(void)
{
	
	//read id
	W25Q_tx_buf[0] = 0x90;
	W25Q_tx_buf[1] = 0;
	W25Q_tx_buf[2] = 0;
	W25Q_tx_buf[3] = 0;
	W25Q_Enable_CS;
	
	SPI_WRITE( W25Q_tx_buf, 4);
	
	SPI_READ( W25Q_rx_buf, 2);

	W25Q_Disable_CS;
	W25Q_flash.id[0] =  W25Q_rx_buf[0];
	W25Q_flash.id[1] =  W25Q_rx_buf[1];
	if( W25Q_rx_buf[0] == 0xEF && W25Q_rx_buf[1] == 0x17)		//w25Q128
	{
		W25Q_flash.page_num = 65536;
		W25Q_flash.sector_num = W25Q_flash.page_num/SECTOR_HAS_PAGES;
		W25Q_flash.block_num = W25Q_flash.sector_num/BLOCK_HAS_SECTORS;
		return ERR_OK;
	}
	

	return ERR_FAIL;
	
	
}

int w25q_close(void)
{
	
	return ERR_OK;
	
}




//将提供的扇区进行擦除操作。扇区号的范围是0 - 4096 （w25q128）

int w25q_Erase_Sector(uint16_t Sector_Number)
{

	uint8_t Block_Num = Sector_Number / BLOCK_HAS_SECTORS;
	
	if( Sector_Number > W25Q_flash.sector_num)
			return ERR_BAD_PARAMETER;
	
	Sector_Number %= BLOCK_HAS_SECTORS;
	W25Q_tx_buf[1] = Block_Num;
	W25Q_tx_buf[2] = Sector_Number<<4;
	W25Q_tx_buf[3] = 0;
	
	return w25q_write_waitbusy( W25Q_tx_buf, 4);
	
}


int w25q_Erase_block(uint16_t block_Number)
{

	if( block_Number > W25Q_flash.block_num)
		return ERR_BAD_PARAMETER;
	
	W25Q_tx_buf[0] = W25Q_INSTR_BLOCK_Erase_64K;
	W25Q_tx_buf[1] = block_Number;
	W25Q_tx_buf[2] = 0;
	W25Q_tx_buf[3] = 0;
	return w25q_write_waitbusy( W25Q_tx_buf, 4);
	
}




int w25q_Write(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t WriteBytesNum)
{

		short step = 0;
		short count = 100;
		while(1)
		{
			switch( step)
			{
				case 0:
					if( w25q_wr_enable() != ERR_OK)
						return ERR_DRI_OPTFAIL;
					W25Q_Enable_CS;
				
					W25Q_tx_buf[0] = W25Q_INSTR_Page_Program;
					W25Q_tx_buf[1] = (uint8_t)((WriteAddr&0x00ff0000)>>16);
					W25Q_tx_buf[2] = (uint8_t)((WriteAddr&0x0000ff00)>>8);
					W25Q_tx_buf[3] = (uint8_t)WriteAddr;
					if( SPI_WRITE( W25Q_tx_buf, 4) != ERR_OK)
						return ERR_DRI_OPTFAIL;
					if( SPI_WRITE( pBuffer, WriteBytesNum) != ERR_OK)
						return ERR_DRI_OPTFAIL;
					W25Q_Disable_CS;
					step++;
					break;
				case 1:
					if( w25q_ReadSR() & W25Q_WRITE_BUSYBIT)
					{
						W25Q_DELAY_MS(1);
						if( count)
							count --;
						else
							return ERR_DEV_TIMEOUT;
						break;
					}
					return ERR_OK;
				default:
					step = 0;
					break;
				
			}		//switch
			
			
		}		//while(1)
		
}


int w25q_Write_Sector_Data(uint8_t *pBuffer, uint16_t Sector_Num)
{
	int wr_page = 0;
	int		ret;
	while(1)
	{
		ret = w25q_Write(pBuffer + wr_page*PAGE_SIZE, Sector_Num*SECTOR_SIZE + wr_page * PAGE_SIZE, PAGE_SIZE);
		if( ret != ERR_OK)
			return ret;
		wr_page ++;
		if( wr_page >= SECTOR_HAS_PAGES)
			return ERR_OK;
	}
   
}


int w25q_Read_Sector_Data(uint8_t *pBuffer, uint16_t Sector_Num)
{
	uint8_t Block_Num = Sector_Num / BLOCK_HAS_SECTORS;
	if( Sector_Num > W25Q_flash.sector_num)
		return ERR_BAD_PARAMETER;
	memset( pBuffer, 0xff, SECTOR_SIZE);
	
	W25Q_Enable_CS;
	W25Q_tx_buf[0] = W25Q_INSTR_READ_DATA;
	W25Q_tx_buf[1] = Block_Num;
	W25Q_tx_buf[2] = Sector_Num<<4;
	W25Q_tx_buf[3] = 0;
	if( SPI_WRITE( W25Q_tx_buf, 4) != ERR_OK)
		return ERR_DRI_OPTFAIL;
	
	if( SPI_READ( pBuffer, SECTOR_SIZE) != SECTOR_SIZE)
		return ERR_DRI_OPTFAIL;
	W25Q_Disable_CS;
	return ERR_OK;
}



int w25q_rd_data(uint8_t *pBuffer, uint32_t rd_add, int len)
{
	if( len > PAGE_SIZE)
		return ERR_BAD_PARAMETER;
	W25Q_tx_buf[0] = W25Q_INSTR_READ_DATA;
	W25Q_tx_buf[1] = (uint8_t)((rd_add&0x00ff0000)>>16);
	W25Q_tx_buf[2] = (uint8_t)((rd_add&0x0000ff00)>>8);
	W25Q_tx_buf[3] = (uint8_t)rd_add;
	W25Q_Enable_CS;
	
	if( SPI_WRITE( W25Q_tx_buf, 4) != ERR_OK)
		return ERR_DRI_OPTFAIL;
	if( SPI_READ( pBuffer, len) != SECTOR_SIZE)
		return ERR_DRI_OPTFAIL;
	W25Q_Disable_CS;
	return ERR_OK;

}






//-------------------------------------------------------------------
//本地函数
//--------------------------------------------------------------------

static int w25q_wr_enable(void)
{
	int ret;
	
	W25Q_tx_buf[0] = W25Q_INSTR_WR_ENABLE;
	W25Q_Enable_CS;
	ret = SPI_WRITE( W25Q_tx_buf,1);
	W25Q_Disable_CS;
	return ret;
}


static uint8_t w25q_ReadSR(void)
{
    uint8_t retValue = 0;
	
    W25Q_Enable_CS;
		W25Q_tx_buf[0] = W25Q_INSTR_WR_Status_Reg;
		
		if( SPI_WRITE( W25Q_tx_buf, 1) != ERR_OK)
			return 0xff;
		if( SPI_READ( &retValue, 1) != 1)
			return 0xff;
    W25Q_Disable_CS;
    return retValue;
}

static int w25q_write_waitbusy(uint8_t *data, int len)
{
	short step = 0;
	short count = 100;
	
	
	while(1)
	{
		switch( step)
		{	
			case 0:
				if( w25q_wr_enable() != ERR_OK)
					return ERR_DRI_OPTFAIL;
				if( SPI_WRITE( data, len) != ERR_OK)
					return ERR_DRI_OPTFAIL;
				step ++;
				W25Q_DELAY_MS(1);
				break;
			case 1:
				if( w25q_ReadSR() & W25Q_WRITE_BUSYBIT)
				{
					W25Q_DELAY_MS(1);
					if( count)
						count --;
					else
						return ERR_DEV_TIMEOUT;
					break;
				}
				return ERR_OK;
			default:
				step = 0;
				break;
		}
	}
		

}











