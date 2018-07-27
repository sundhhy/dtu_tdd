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

//------------------------------------------------------------------------------
// global variable declarations
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// function prototypes
//------------------------------------------------------------------------------
//void ModbusMaster_begin(void);
//void ModbusMaster_beginTransmission(uint16_t u16Address);
//uint8_t ModbusMaster_requestFrom(uint16_t address, uint16_t quantity);
//void  ModbusMaster_sendBit(uint8_t data);
//void ModbusMaster_send16(uint16_t data);
//void ModbusMaster_send32(uint32_t data);
//void ModbusMaster_send8(uint8_t data);
//uint8_t ModbusMaster_available(void);
//uint16_t ModbusMaster_receive(void);
//uint16_t ModbusMaster_getResponseBuffer(uint8_t u8Index);
//void ModbusMaster_clearResponseBuffer();
//uint8_t ModbusMaster_setTransmitBuffer(uint8_t u8Index, uint16_t u16Value);
//void ModbusMaster_clearTransmitBuffer();
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
#endif
