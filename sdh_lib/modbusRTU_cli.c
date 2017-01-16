/**
* @file 		modbusRTU_cli.c
* @brief		实现modbusRTU从站功能.
* @details	由寄存器的访问部分和协议解析两部分组成.
* @author		author
* @date		date
* @version	A001
* @par Copyright (c): 
* 		XXX??
* @par History:         
*	version: author, date, desc\n
*	V0.1:sundh,17-01-15,创建实现0和3号命令
*/
#include "modbusRTU_cli.h"
#include "sdhError.h"

//uint16_t coil_buf[COIL_SIZE/16];
static uint16_t state_buf[STATE_SIZE/16];			//2区数据内存
uint16_t input_buf[INPUT_SIZE];
uint16_t hold_buf[HOLD_SIZE];

//uint16_t *COIL_ADDRESS=coil_buf;
uint16_t *STATE_ADDRESS=state_buf;
uint16_t *INPUT_ADDRESS=input_buf;
uint16_t *HOLD_ADDRESS=hold_buf;


/* 寄存器访问接口			*/

uint16_t regTyppe2_read(uint16_t addr, uint16_t reg_type)
{
	uint16_t i,j;

	if(reg_type == REG_MODBUS)
	{
	  addr-=10001;
	}
	 
	i = addr/16;
	j = addr%16;
    
	i = *(STATE_ADDRESS+i) & (1<<j);
	
	return (i!=0);
	
}

int regType2_write(uint16_t addr, uint16_t reg_type, int val)
{
	return ERR_OK;
}



uint16_t regType3_read(uint16_t hold_address, uint16_t reg_type)
{
	if(reg_type==REG_MODBUS)
	{
		hold_address-=40001;
	}

	return *(HOLD_ADDRESS + hold_address);
}

uint16_t regType3_write(uint16_t hold_address, uint16_t reg_type, uint16_t val)
{
	if(reg_type==REG_MODBUS)
	{
		hold_address-=40001;
	}

	*(HOLD_ADDRESS + hold_address) = val;
	return ERR_OK;
}


uint16_t regType4_read(uint16_t input_address, uint16_t reg_type)
{
	if(reg_type==REG_MODBUS)
	{
	  input_address-=30001;
	}
	  
	return *(INPUT_ADDRESS + input_address);
}

uint16_t regType4_write(uint16_t input_address, uint16_t reg_type)
{
	
	  
	return ERR_FAIL;
}
