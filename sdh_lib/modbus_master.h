//------------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------------
#ifndef __INC_modbus_master_H
#define __INC_modbus_master_H

#include <stdint.h>
//------------------------------------------------------------------------------
// check for correct compilation options
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// typedef
//------------------------------------------------------------------------------
typedef int (*mdm_update_slave_data)(uint8_t SlaveID, uint16_t reg_addr, void *data);		//modbus从站返回的数据的处理
//------------------------------------------------------------------------------
// global variable declarations
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// function prototypes
//------------------------------------------------------------------------------

uint8_t ModbusMaster_readCoils(uint8_t SlaveID,uint16_t u16ReadAddress, uint16_t u16BitQty, uint8_t *buf, int buf_size);
uint8_t ModbusMaster_readDiscreteInputs(uint8_t SlaveID,uint16_t u16ReadAddress,uint16_t u16BitQty, uint8_t *buf, int buf_size);
uint8_t ModbusMaster_readHoldingRegisters(uint8_t SlaveID,uint16_t u16ReadAddress,uint16_t u16ReadQty, uint8_t *buf, int buf_size);
uint8_t ModbusMaster_readInputRegisters(uint8_t SlaveID,uint16_t u16ReadAddress,uint8_t u16ReadQty, uint8_t *buf, int buf_size);	
uint8_t ModbusMaster_writeSingleCoil(uint8_t SlaveID,uint16_t u16WriteAddress, uint8_t u8State, uint8_t *buf, int buf_size);
uint8_t ModbusMaster_writeSingleRegister(uint8_t SlaveID,uint16_t u16WriteAddress,uint16_t u16WriteValue, uint8_t *buf, int buf_size);
uint8_t ModbusMaster_writeMultipleCoils(uint8_t SlaveID,uint16_t u16WriteAddress,uint16_t u16BitQty, uint8_t *buf, int buf_size);
uint8_t ModbusMaster_writeMultipleRegisters(uint8_t SlaveID,uint16_t u16WriteAddress,uint16_t u16WriteQty, uint8_t *buf, int buf_size);
//写值的时候，buf会被内部临时用于参数传递,来节省本模块的资源消耗
uint8_t ModbusMaster_maskWriteRegister(uint8_t SlaveID,uint16_t u16WriteAddress,uint16_t u16AndMask, uint16_t u16OrMask, uint8_t *buf, int buf_size);
//写值的时候，buf会被内部临时用于参数传递,来节省本模块的资源消耗
uint8_t ModbusMaster_readWriteMultipleRegisters(uint8_t SlaveID,uint16_t u16ReadAddress,uint16_t u16ReadQty, uint16_t u16WriteAddress, uint16_t u16WriteQty, uint8_t *buf, int buf_size);

int ModbusMaster_decode_pkt(uint8_t *buf, int buf_size);

int MDM_register_update(mdm_update_slave_data func_update);
#endif
