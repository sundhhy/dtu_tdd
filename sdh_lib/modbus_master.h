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
#define MDM_U8_READ_COILS									1
#define MDM_U8_READ_DISCRETEINPUTS				2
#define MDM_U16_READ_HOLD_REG							3
#define MDM_U16_READ_INPUT_REG						4

#define MDM_WRITE_SINGLE_COIL			5
//#define MDM_WRITE_MULT_COILS				0xf
#define MDM_WRITE_SINGLE_REG				6
//#define MDM_WRITE_MULT_REGS					0x10

//------------------------------------------------------------------------------
// typedef
//------------------------------------------------------------------------------
//modbus 从站对读取命令的返回处理
typedef int (*mdm_up_read_ack)(uint8_t slave_addr, uint8_t func_code, uint8_t num_byte, uint8_t *data);		
//modbus 从站对写命令的返回处理
typedef int (*mdm_up_write_ack)(uint8_t slave_addr, uint8_t func_code, uint16_t reg_addr, uint16_t val);		

typedef int (*mdm_up_err)(uint8_t slave_addr, uint8_t func_code, uint8_t err_code);
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
//uint8_t ModbusMaster_writeMultipleCoils(uint8_t SlaveID,uint16_t u16WriteAddress,uint16_t u16BitQty, uint8_t *buf, int buf_size);
//uint8_t ModbusMaster_writeMultipleRegisters(uint8_t SlaveID,uint16_t u16WriteAddress,uint16_t u16WriteQty, uint8_t *buf, int buf_size);
//写值的时候，buf会被内部临时用于参数传递,来节省本模块的资源消耗
//uint8_t ModbusMaster_write_input_reg(uint8_t SlaveID,uint16_t u16WriteAddress,uint16_t u16_val, uint8_t *buf, int buf_size);
//写值的时候，buf会被内部临时用于参数传递,来节省本模块的资源消耗
uint8_t ModbusMaster_readWriteMultipleRegisters(uint8_t SlaveID,uint16_t u16ReadAddress,uint16_t u16ReadQty, uint16_t u16WriteAddress, uint16_t u16WriteQty, uint8_t *buf, int buf_size);

int ModbusMaster_decode_pkt(uint8_t *adu, int adu_len);

int MDM_register_update(mdm_up_read_ack up_read, mdm_up_write_ack up_write, mdm_up_err up_err);
#endif
