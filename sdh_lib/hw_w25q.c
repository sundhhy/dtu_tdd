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
uint8_t	W25Q_tx_buf[16];
uint8_t	W25Q_rx_buf[16];
//------------本地私有函数------------------------------
static int w25q_wr_enable(void);
static uint8_t w25q_ReadSR(void);
static uint8_t w25_Wait_Busy(void);
static void w25q_Write_Data(uint8_t *pBuffer,uint16_t Block_Num,uint16_t Page_Num,uint32_t WriteBytesNum);

//--------------------------------------------------------------

int w25q_init(void)
{
	uint32_t calculatedBaudRate;

	w25q_init_cs();
	w25q_init_spi();
		
	W25Q_Disable_CS;
		
	

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
	static char step = 0;
	char count = 0;
	uint8_t Block_Num = Sector_Number / BLOCK_HAS_SECTORS;
	while(1)
	{
	if( step == 0) {
		if( Sector_Number > W25Q_flash.sector_num)
			return ERR_BAD_PARAMETER;
		if( w25q_wr_enable() < 0)
			return ERR_DRI_OPTFAIL;
		W25Q_Enable_CS;
		W25Q_tx_buf[0] = W25Q_INSTR_Sector_Erase_4K;
		
		Sector_Number %= BLOCK_HAS_SECTORS;

		W25Q_tx_buf[1] = Block_Num;
		W25Q_tx_buf[2] = Sector_Number<<4;
		W25Q_tx_buf[3] = 0;
		
		SPI_WRITE( W25Q_tx_buf, 4);
	
	SPI_READ( W25Q_rx_buf, 2);
		
		
		step = 1;
		W25Q_Disable_CS;
		return RET_DELAY;
		
	}
	
	if( step == 1) {
		if( w25q_ReadSR() & W25Q_WRITE_BUSYBIT)
			return RET_DELAY;
		else {
			
			step = 0;
			
		}
			
	}
}
	return RET_SUCCEED;
}


error_t w25q_Erase_block(uint16_t block_Number)
{
	static char step = 0;
	char count = 0;
	if( step == 0) {
		if( block_Number > W25Q_flash.block_num)
			return ERROR_T(ERR_BLOCK_OUT_OF_LIMIT);
		if( w25q_wr_enable() < 0)
			return ERROR_T(ERR_SPI_SEND_FAIL);
		W25Q_Enable_CS;
		
		W25Q_tx_buf[0] = W25Q_INSTR_BLOCK_Erase_64K;
		
	

		W25Q_tx_buf[1] = block_Number;
		W25Q_tx_buf[2] = 0;
		W25Q_tx_buf[3] = 0;
		SPI_DRV_DmaMasterTransfer(SPI_MASTER_INSTANCE, NULL, W25Q_tx_buf, NULL, 4);
		while (SPI_DRV_DmaMasterGetTransferStatus(SPI_MASTER_INSTANCE, NULL) == kStatus_SPI_Busy)
		{
			count ++;
			if( count > 40) 
			{
				W25Q_Disable_CS;
				SPI_DRV_DmaMasterAbortTransfer(SPI_MASTER_INSTANCE);
				return ERROR_T(ERR_SPI_SEND_FAIL);
				
			}
			
		}
		step = 1;
		W25Q_Disable_CS;
//		return RET_DELAY;
		
	}
	
	if( step == 1) {
		if( w25q_ReadSR() & W25Q_WRITE_BUSYBIT)
			return RET_DELAY;
		else {
			
			step = 0;
			
		}
			
	}
	return RET_SUCCEED;
}




int w25q_Write(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t WriteBytesNum)
{

		static char step = 0;
		char count = 0;
		switch( step)
		{
			case 0:
				if( w25q_wr_enable() < 0)
					return ERROR_T(ERR_SPI_SEND_FAIL);
				W25Q_Enable_CS;
				
				W25Q_tx_buf[0] = W25Q_INSTR_Page_Program;
				W25Q_tx_buf[1] = (uint8_t)((WriteAddr&0x00ff0000)>>16);
				W25Q_tx_buf[2] = (uint8_t)((WriteAddr&0x0000ff00)>>8);
				W25Q_tx_buf[3] = (uint8_t)WriteAddr;
				SPI_DRV_DmaMasterTransfer(SPI_MASTER_INSTANCE, NULL, W25Q_tx_buf, NULL, 4);
				while (SPI_DRV_DmaMasterGetTransferStatus(SPI_MASTER_INSTANCE, NULL) == kStatus_SPI_Busy)
				{
					count ++;
					if( count > 100) 
					{
						W25Q_Disable_CS;
						SPI_DRV_DmaMasterAbortTransfer(SPI_MASTER_INSTANCE);
						return ERROR_T(ERR_SPI_SEND_FAIL);
						
					}
				}
				SPI_DRV_DmaMasterTransfer(SPI_MASTER_INSTANCE, NULL, pBuffer, NULL, WriteBytesNum);
				step ++;
				return RET_PROCESSING;
			case 1:
				W25Q_Disable_CS;
				step ++;
			case 2:
				if( w25q_ReadSR() & W25Q_WRITE_BUSYBIT)
						return RET_DELAY;
				else {
					
					step = 0;
					return RET_SUCCEED;
				}
			
			
			
		}

}


error_t w25q_Write_Sector_Data(uint8_t *pBuffer, uint16_t Sector_Num)
{
	static char step = 0;
	static char wr_page = 0;
	error_t		ret;
	while(1)
	{
		switch( step)
		{
//			write_pg:
			case 0:
				ret = w25q_Write(pBuffer + wr_page*PAGE_SIZE, Sector_Num*SECTOR_SIZE + wr_page * PAGE_SIZE, PAGE_SIZE);
				if( ret == RET_SUCCEED)
					step ++;
				else
					return ret;
			case 1:
				wr_page ++;
				if( wr_page < SECTOR_HAS_PAGES)
				{
					step = 0;
					break;
				}
				else {
					step = 0;
					wr_page = 0;
					return RET_SUCCEED;
				}	
		}
	}
   
}


error_t w25q_Read_Sector_Data(uint8_t *pBuffer, uint16_t Sector_Num)
{
	uint8_t Block_Num = Sector_Num / BLOCK_HAS_SECTORS;
	static char step = 0;
	int count = 0;
	switch(step) 
	{
		case 0:
			
			if( Sector_Num > W25Q_flash.sector_num)
				return ERROR_T(ERR_SECTOR_OUT_OF_LIMIT);
			memset( pBuffer, 0xff, SECTOR_SIZE);
			Sector_Num %= BLOCK_HAS_SECTORS;
			W25Q_Enable_CS;
			W25Q_tx_buf[0] = W25Q_INSTR_READ_DATA;
			W25Q_tx_buf[1] = Block_Num;
			W25Q_tx_buf[2] = Sector_Num<<4;
			W25Q_tx_buf[3] = 0;
				
				
			SPI_DRV_DmaMasterTransfer(SPI_MASTER_INSTANCE, NULL, W25Q_tx_buf, NULL, 4);
			while (SPI_DRV_DmaMasterGetTransferStatus(SPI_MASTER_INSTANCE, NULL) == kStatus_SPI_Busy)
			{
				count ++;
				if( count > 50)
				{
					W25Q_Disable_CS;
					SPI_DRV_DmaMasterAbortTransfer(SPI_MASTER_INSTANCE);
					return RET_DELAY;
				}
			}
			
			SPI_DRV_DmaMasterTransfer(SPI_MASTER_INSTANCE, NULL, NULL,  pBuffer, SECTOR_SIZE);	
			step = 1;
			return RET_PROCESSING;
		case 1:
			step = 0;
			W25Q_Disable_CS;
			return RET_SUCCEED;
		
		
		
	}
	


}



error_t w25q_rd_data(uint8_t *pBuffer, uint32_t rd_add, int len)
{
	if( len > PAGE_SIZE)
		return ERROR_T(ERR_RD_LEN_OUT_OF_LIMIT);
	W25Q_tx_buf[0] = W25Q_INSTR_READ_DATA;
	W25Q_tx_buf[1] = (uint8_t)((rd_add&0x00ff0000)>>16);
	W25Q_tx_buf[2] = (uint8_t)((rd_add&0x0000ff00)>>8);
	W25Q_tx_buf[3] = (uint8_t)rd_add;
	W25Q_Enable_CS;
	SPI_DRV_DmaMasterTransfer(SPI_MASTER_INSTANCE, NULL, W25Q_tx_buf, NULL, 4);
	while (SPI_DRV_DmaMasterGetTransferStatus(SPI_MASTER_INSTANCE, NULL) == kStatus_SPI_Busy)
	{
	}
	SPI_DRV_DmaMasterTransfer(SPI_MASTER_INSTANCE, NULL, NULL, pBuffer, len);	
	while (SPI_DRV_DmaMasterGetTransferStatus(SPI_MASTER_INSTANCE, NULL) == kStatus_SPI_Busy)
	{
	}
	W25Q_Disable_CS;
	return 0;
	
}






//-------------------------------------------------------------------
//本地函数
//--------------------------------------------------------------------

static int w25q_wr_enable(void)
{
	char count = 0;
	spi_status_t ret ;
	W25Q_tx_buf[0] = W25Q_INSTR_WR_ENABLE;
	W25Q_Enable_CS;
	ret = SPI_DRV_DmaMasterTransfer(SPI_MASTER_INSTANCE, NULL, W25Q_tx_buf,
                                        NULL, 1);
	while (SPI_DRV_DmaMasterGetTransferStatus(SPI_MASTER_INSTANCE, NULL) == kStatus_SPI_Busy)
	{
		count ++;
		if( count > 40)
			break;
	}
	
	if( count> 40)
	{

		W25Q_Disable_CS;
		SPI_DRV_DmaMasterAbortTransfer(SPI_MASTER_INSTANCE);
		return -1;
	}
	
	W25Q_Disable_CS;
	return 0;
}


//阻塞：因为只传输1字节的数据，还不如阻塞在这里，简化操作过程，并不会浪费多少时间
static uint8_t w25q_ReadSR(void)
{
    uint8_t retValue = 0;
		uint8_t	count = 0;
    W25Q_Enable_CS;
		W25Q_tx_buf[0] = W25Q_INSTR_WR_Status_Reg;
		SPI_DRV_DmaMasterTransfer(SPI_MASTER_INSTANCE, NULL, W25Q_tx_buf,
                                        NULL, 1);
		while (SPI_DRV_DmaMasterGetTransferStatus(SPI_MASTER_INSTANCE, NULL) == kStatus_SPI_Busy)
		{
			count ++;
			if( count > 100)
			{
				SPI_DRV_DmaMasterAbortTransfer(SPI_MASTER_INSTANCE);
				return 0xff;
			}
		}
		count = 0;
		SPI_DRV_DmaMasterTransfer(SPI_MASTER_INSTANCE, NULL, NULL,
															&retValue, 1);	
		while (SPI_DRV_DmaMasterGetTransferStatus(SPI_MASTER_INSTANCE, NULL) == kStatus_SPI_Busy)
		{
			count ++;
			if( count > 100)
			{
				SPI_DRV_DmaMasterAbortTransfer(SPI_MASTER_INSTANCE);
				return 0xff;
			}
		}
    W25Q_Disable_CS;
    return retValue;
}


static uint8_t w25_Wait_Busy(void)
{
    
	
		
		
	
    while((w25q_ReadSR()&W25Q_WRITE_BUSYBIT) == 0x01)
    {  
        
    }
    return 0;
}










