//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//

#include<string.h>

#include "crc.h"


#include "modbus_master.h"
//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// module global vars
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// global function prototypes
//------------------------------------------------------------------------------

//============================================================================//
//            P R I V A T E   D E F I N I T I O N S                           //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------
//static const uint8_t ku8MBIllegalFunction            = 0x01;
//static const uint8_t ku8MBIllegalDataAddress         = 0x02;
//static const uint8_t ku8MBIllegalDataValue           = 0x03;
//static const uint8_t ku8MBSlaveDeviceFailure         = 0x04;

//static const uint8_t ku8MBSuccess                    = 0x00;
//static const uint8_t ku8MBInvalidSlaveID             = 0xE0;
//static const uint8_t ku8MBInvalidFunction            = 0xE1;
//static const uint8_t ku8MBResponseTimedOut           = 0xE2;
//static const uint8_t ku8MBInvalidCRC                 = 0xE3;


//static const uint8_t ku8MaxBufferSize                = 64;   ///< size of response/transmit buffers    


// Modbus function codes for bit access
//static const uint8_t ku8MBReadCoils                  = 0x01; ///< Modbus function 0x01 Read Coils
//static const uint8_t ku8MBReadDiscreteInputs         = 0x02; ///< Modbus function 0x02 Read Discrete Inputs
//static const uint8_t ku8MBWriteSingleCoil            = 0x05; ///< Modbus function 0x05 Write Single Coil
//static const uint8_t ku8MBWriteMultipleCoils         = 0x0F; ///< Modbus function 0x0F Write Multiple Coils

// Modbus function codes for 16 bit access
//static const uint8_t ku8MBReadHoldingRegisters       = 0x03; ///< Modbus function 0x03 Read Holding Registers
//static const uint8_t ku8MBReadInputRegisters         = 0x04; ///< Modbus function 0x04 Read Input Registers
//static const uint8_t ku8MBWriteSingleRegister        = 0x06; ///< Modbus function 0x06 Write Single Register
//static const uint8_t ku8MBWriteMultipleRegisters     = 0x10; ///< Modbus function 0x10 Write Multiple Registers
//static const uint8_t ku8MBMaskWriteRegister          = 0x16; ///< Modbus function 0x16 Mask Write Register
//static const uint8_t ku8MBReadWriteMultipleRegisters = 0x17; ///< Modbus function 0x17 Read Write Multiple Registers

// Modbus timeout [milliseconds]
//static const uint16_t ku16MBResponseTimeout          = 2000; ///< Modbus timeout [milliseconds]
//------------------------------------------------------------------------------
// local types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------
uint8_t  _u8MBSlave;                                         ///< Modbus slave (1..255) initialized in begin()
uint16_t _u16ReadAddress;                                    ///< slave register from which to read
uint16_t _u16ReadQty;                                        ///< quantity of words to read
//uint16_t _u16ResponseBuffer[ku8MaxBufferSize];               ///< buffer to store Modbus slave response; read via GetResponseBuffer()
uint16_t _u16WriteAddress;                                   ///< slave register to which to write
uint16_t _u16WriteQty;                                       ///< quantity of words to write
//uint16_t _u16TransmitBuffer[ku8MaxBufferSize];               ///< buffer containing data to transmit to Modbus slave; set via SetTransmitBuffer()
uint16_t* txBuffer; // from Wire.h -- need to clean this up Rx
uint8_t _u8TransmitBufferIndex;
uint16_t u16TransmitBufferLength;
uint16_t* rxBuffer; // from Wire.h -- need to clean this up Rx
uint8_t _u8ResponseBufferIndex;
uint8_t _u8ResponseBufferLength;


static mdm_up_read_ack	func_slave_update;
static mdm_up_write_ack	func_up_write;
static mdm_up_err					func_err_update;

//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------

static int ModbusMaster_ModbusMasterTransaction(uint8_t u8MBFunction, uint8_t *buf, int buf_size);

static uint8_t lowByte(uint16_t val);
static uint8_t highByte(uint16_t val);

//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//



int MDM_register_update(mdm_up_read_ack up_read, mdm_up_write_ack up_write, mdm_up_err up_err)
{
	func_slave_update = up_read;
	func_up_write = up_write;
	func_err_update = up_err;
	
	return 0;
}


//成功返回0
//crc错误返灰modubus 协议的错误码

int ModbusMaster_decode_pkt(uint8_t *adu, int adu_len)
{
	uint8_t  	slave_addr;
	uint8_t		func_code;
	uint16_t	tmp_u16;
	uint16_t	reg_addr;
	uint16_t	calc_crc;
	int 		err_ret;
	
	//校验crc
	tmp_u16 = adu[adu_len - 1] + (adu[adu_len - 2] << 8);	  //read crc
	calc_crc = CRC16(adu, (adu_len - 2));
	if(tmp_u16 != calc_crc)
	{
		err_ret = -1;
		goto err;
	}
	
	slave_addr = adu[0];
	func_code = adu[1];
	
	if(func_code & 0x80)
	{
//		if(func_err_update)
			func_err_update(slave_addr, func_code &0x70, adu[2]);
		err_ret = adu[2];
		goto err;
		
	}
	//读取命令的返回格式是：功能码，字节数，数据
	//写单个寄存器命令的返回格式是：功能码，寄存器地址，值
	//连续写入返回的格式是：功能码，起始地址，寄存器数量
	//取出地址和值，上传
	
	func_code &= 0x7f;
	
	if(func_code < 5)
	{
		//读取命令的返回
//		if(func_slave_update)
			func_slave_update(slave_addr, func_code, adu[2], &adu[3]);
	}
	else
	{
		//写命令的返回
//		if(func_up_write)
		reg_addr = adu[3] | (adu[2] << 8);
		tmp_u16 = adu[5] | (adu[4] << 8);
		func_up_write(slave_addr, func_code, reg_addr, tmp_u16);
		
	}

exit:	
	return 0;
	
err:
	
	return err_ret;
}



/**
Modbus function 0x01 Read Coils.
This function code is used to read from 1 to 2000 contiguous status of 
coils in a remote device. The request specifies the starting address, 
i.e. the address of the first coil specified, and the number of coils. 
Coils are addressed starting at zero.
The coils in the response buffer are packed as one coil per bit of the 
data field. Status is indicated as 1=ON and 0=OFF. The LSB of the first 
data word contains the output addressed in the query. The other coils 
follow toward the high order end of this word and from low order to high 
order in subsequent words.
If the returned quantity is not a multiple of sixteen, the remaining 
bits in the final data word will be padded with zeros (toward the high 
order end of the word).
@param u16ReadAddress address of first coil (0x0000..0xFFFF)
@param u16BitQty quantity of coils to read (1..2000, enforced by remote device)
@return 0 on success; exception number on failure
@ingroup discrete
*/
uint8_t ModbusMaster_readCoils(uint8_t SlaveID,uint16_t u16ReadAddress, uint16_t u16BitQty, uint8_t *buf, int buf_size)
{
	_u8MBSlave = SlaveID;
  _u16ReadAddress = u16ReadAddress;
  _u16ReadQty = u16BitQty;
  return ModbusMaster_ModbusMasterTransaction(MDM_U8_READ_COILS, buf, buf_size);
}


/**
Modbus function 0x02 Read Discrete Inputs.
This function code is used to read from 1 to 2000 contiguous status of 
discrete inputs in a remote device. The request specifies the starting 
address, i.e. the address of the first input specified, and the number 
of inputs. Discrete inputs are addressed starting at zero.
The discrete inputs in the response buffer are packed as one input per 
bit of the data field. Status is indicated as 1=ON; 0=OFF. The LSB of 
the first data word contains the input addressed in the query. The other 
inputs follow toward the high order end of this word, and from low order 
to high order in subsequent words.
If the returned quantity is not a multiple of sixteen, the remaining 
bits in the final data word will be padded with zeros (toward the high 
order end of the word).
@param u16ReadAddress address of first discrete input (0x0000..0xFFFF)
@param u16BitQty quantity of discrete inputs to read (1..2000, enforced by remote device)
@return 0 on success; exception number on failure
@ingroup discrete
*/
uint8_t ModbusMaster_readDiscreteInputs(uint8_t SlaveID,uint16_t u16ReadAddress,
  uint16_t u16BitQty, uint8_t *buf, int buf_size)
{
	_u8MBSlave = SlaveID;
  _u16ReadAddress = u16ReadAddress;
  _u16ReadQty = u16BitQty;
  return ModbusMaster_ModbusMasterTransaction(MDM_U8_READ_DISCRETEINPUTS, buf, buf_size);
}


/**
Modbus function 0x03 Read Holding Registers.
This function code is used to read the contents of a contiguous block of 
holding registers in a remote device. The request specifies the starting 
register address and the number of registers. Registers are addressed 
starting at zero.
The register data in the response buffer is packed as one word per 
register.
@param u16ReadAddress address of the first holding register (0x0000..0xFFFF)
@param u16ReadQty quantity of holding registers to read (1..125, enforced by remote device)
@return 0 on success; exception number on failure
@ingroup register
*/
uint8_t ModbusMaster_readHoldingRegisters(uint8_t SlaveID,uint16_t u16ReadAddress,
  uint16_t u16ReadQty, uint8_t *buf, int buf_size)
{
	_u8MBSlave = SlaveID;
  _u16ReadAddress = u16ReadAddress;
  _u16ReadQty = u16ReadQty;
  return ModbusMaster_ModbusMasterTransaction(MDM_U16_READ_HOLD_REG, buf, buf_size);
}


/**
Modbus function 0x04 Read Input Registers.
This function code is used to read from 1 to 125 contiguous input 
registers in a remote device. The request specifies the starting 
register address and the number of registers. Registers are addressed 
starting at zero.
The register data in the response buffer is packed as one word per 
register.
@param u16ReadAddress address of the first input register (0x0000..0xFFFF)
@param u16ReadQty quantity of input registers to read (1..125, enforced by remote device)
@return 0 on success; exception number on failure
@ingroup register
*/
uint8_t ModbusMaster_readInputRegisters(uint8_t SlaveID,uint16_t u16ReadAddress,
  uint8_t u16ReadQty, uint8_t *buf, int buf_size)
{
	_u8MBSlave = SlaveID;
  _u16ReadAddress = u16ReadAddress;
  _u16ReadQty = u16ReadQty;
  return ModbusMaster_ModbusMasterTransaction(MDM_U16_READ_INPUT_REG, buf, buf_size);
}


/**
Modbus function 0x05 Write Single Coil.
This function code is used to write a single output to either ON or OFF 
in a remote device. The requested ON/OFF state is specified by a 
constant in the state field. A non-zero value requests the output to be 
ON and a value of 0 requests it to be OFF. The request specifies the 
address of the coil to be forced. Coils are addressed starting at zero.
@param u16WriteAddress address of the coil (0x0000..0xFFFF)
@param u8State 0=OFF, non-zero=ON (0x00..0xFF)
@return 0 on success; exception number on failure
@ingroup discrete
*/
uint8_t ModbusMaster_writeSingleCoil(uint8_t SlaveID,uint16_t u16WriteAddress, uint8_t u8State, uint8_t *buf, int buf_size)
{
	_u8MBSlave = SlaveID;
  _u16WriteAddress = u16WriteAddress;
  _u16WriteQty = (u8State ? 0xFF00 : 0x0000);
  return ModbusMaster_ModbusMasterTransaction(MDM_WRITE_SINGLE_COIL, buf, buf_size);
}


/**
Modbus function 0x06 Write Single Register.
This function code is used to write a single holding register in a 
remote device. The request specifies the address of the register to be 
written. Registers are addressed starting at zero.
@param u16WriteAddress address of the holding register (0x0000..0xFFFF)
@param u16WriteValue value to be written to holding register (0x0000..0xFFFF)
@return 0 on success; exception number on failure
@ingroup register
*/
uint8_t ModbusMaster_writeSingleRegister(uint8_t SlaveID,uint16_t u16WriteAddress,
  uint16_t u16WriteValue, uint8_t *buf, int buf_size)
{
	
		uint16_t *p_u16 = (uint16_t *)buf;

	_u8MBSlave = SlaveID;
  _u16WriteAddress = u16WriteAddress;
  _u16WriteQty = 0;
  p_u16[0] = u16WriteValue;
  return ModbusMaster_ModbusMasterTransaction(MDM_WRITE_SINGLE_REG, buf, buf_size);
}


/**
Modbus function 0x0F Write Multiple Coils.
This function code is used to force each coil in a sequence of coils to 
either ON or OFF in a remote device. The request specifies the coil 
references to be forced. Coils are addressed starting at zero.
The requested ON/OFF states are specified by contents of the transmit 
buffer. A logical '1' in a bit position of the buffer requests the 
corresponding output to be ON. A logical '0' requests it to be OFF.
@param u16WriteAddress address of the first coil (0x0000..0xFFFF)
@param u16BitQty quantity of coils to write (1..2000, enforced by remote device)
@return 0 on success; exception number on failure
@ingroup discrete
*/
//uint8_t ModbusMaster_writeMultipleCoils(uint8_t SlaveID,uint16_t u16WriteAddress,
//  uint16_t u16BitQty, uint8_t *buf, int buf_size)
//{
//	_u8MBSlave = SlaveID;
//  _u16WriteAddress = u16WriteAddress;
//  _u16WriteQty = u16BitQty;
//  return ModbusMaster_ModbusMasterTransaction(MDM_WRITE_MULT_COILS, buf, buf_size);
//}
/*
uint8_t ModbusMaster_writeMultipleCoils()
{
  _u16WriteQty = u16TransmitBufferLength;
  return ModbusMaster_ModbusMasterTransaction(ku8MBWriteMultipleCoils);
}
*/


/**
Modbus function 0x10 Write Multiple Registers.
This function code is used to write a block of contiguous registers (1 
to 123 registers) in a remote device.
The requested written values are specified in the transmit buffer. Data 
is packed as one word per register.
@param u16WriteAddress address of the holding register (0x0000..0xFFFF)
@param u16WriteQty quantity of holding registers to write (1..123, enforced by remote device)
@return 0 on success; exception number on failure
@ingroup register
*/
//uint8_t ModbusMaster_writeMultipleRegisters(uint8_t SlaveID,uint16_t u16WriteAddress,
//  uint16_t u16WriteQty, uint8_t *buf, int buf_size)
//{
//	_u8MBSlave = SlaveID;
//  _u16WriteAddress = u16WriteAddress;
//  _u16WriteQty = u16WriteQty;
//  return ModbusMaster_ModbusMasterTransaction(MDM_WRITE_MULT_REGS, buf, buf_size);
//}

// new version based on Wire.h

/*uint8_t ModbusMaster_writeMultipleRegisters()
{
  _u16WriteQty = _u8TransmitBufferIndex;
  return ModbusMaster_ModbusMasterTransaction(ku8MBWriteMultipleRegisters);
}
*/

/**
Modbus function 0x16 Mask Write Register.
This function code is used to modify the contents of a specified holding 
register using a combination of an AND mask, an OR mask, and the 
register's current contents. The function can be used to set or clear 
individual bits in the register.
The request specifies the holding register to be written, the data to be 
used as the AND mask, and the data to be used as the OR mask. Registers 
are addressed starting at zero.
The function's algorithm is:
Result = (Current Contents && And_Mask) || (Or_Mask && (~And_Mask))
@param u16WriteAddress address of the holding register (0x0000..0xFFFF)
@param u16AndMask AND mask (0x0000..0xFFFF)
@param u16OrMask OR mask (0x0000..0xFFFF)
@return 0 on success; exception number on failure
@ingroup register
*/
//uint8_t ModbusMaster_write_input_reg(uint8_t SlaveID,uint16_t u16WriteAddress,
//  uint16_t u16_val, uint8_t *buf, int buf_size)
//{
//	uint16_t *p_u16 = (uint16_t *)buf;
//	_u8MBSlave = SlaveID;
//  _u16WriteAddress = u16WriteAddress;
//  p_u16[0] = u16_val;
//  return ModbusMaster_ModbusMasterTransaction(MDM_WRITE_SINGLE_REG, buf, buf_size);
//}


/**
Modbus function 0x17 Read Write Multiple Registers.
This function code performs a combination of one read operation and one 
write operation in a single MODBUS transaction. The write operation is 
performed before the read. Holding registers are addressed starting at 
zero.
The request specifies the starting address and number of holding 
registers to be read as well as the starting address, and the number of 
holding registers. The data to be written is specified in the transmit 
buffer.
@param u16ReadAddress address of the first holding register (0x0000..0xFFFF)
@param u16ReadQty quantity of holding registers to read (1..125, enforced by remote device)
@param u16WriteAddress address of the first holding register (0x0000..0xFFFF)
@param u16WriteQty quantity of holding registers to write (1..121, enforced by remote device)
@return 0 on success; exception number on failure
@ingroup register
*/
//uint8_t ModbusMaster_readWriteMultipleRegisters(uint8_t SlaveID,uint16_t u16ReadAddress,
//  uint16_t u16ReadQty, uint16_t u16WriteAddress, uint16_t u16WriteQty, uint8_t *buf, int buf_size)
//{
//	_u8MBSlave = SlaveID;
//  _u16ReadAddress = u16ReadAddress;
//  _u16ReadQty = u16ReadQty;
//  _u16WriteAddress = u16WriteAddress;
//  _u16WriteQty = u16WriteQty;
//  return ModbusMaster_ModbusMasterTransaction(ku8MBReadWriteMultipleRegisters, buf, buf_size);
//}





//=========================================================================//
//                                                                         //
//          P R I V A T E   D E F I N I T I O N S                          //
//                                                                         //
//=========================================================================//
/// \name Private Functions
/// \{


/**
Modbus transaction engine.
Sequence:
  - assemble Modbus Request Application Data Unit (ADU),
    based on particular function called
  - transmit request over selected serial port
  - wait for/retrieve response
  - evaluate/disassemble response
  - return status (success/exception)
@param u8MBFunction Modbus function (0x01..0xFF)
@return 0 on success; exception number on failure
*/
static int ModbusMaster_ModbusMasterTransaction(uint8_t u8MBFunction, uint8_t *buf, int buf_size)
{
  uint8_t u8ModbusADU[128];
  uint8_t u8ModbusADUSize = 0;
  uint8_t i, u8Qty;
  uint16_t u16CRC;
	uint16_t write_val = *(uint16_t *)buf;
//  uint32_t u32StartTime;
//  uint8_t u8BytesLeft = 8;
//  uint8_t u8MBStatus = ku8MBSuccess;
	
	// assemble Modbus Request Application Data Unit
  u8ModbusADU[u8ModbusADUSize++] = _u8MBSlave;
  u8ModbusADU[u8ModbusADUSize++] = u8MBFunction;
	
	switch(u8MBFunction)
  {
    case MDM_U8_READ_COILS:
    case MDM_U8_READ_DISCRETEINPUTS:
    case MDM_U16_READ_INPUT_REG:
    case MDM_U16_READ_HOLD_REG:
//    case ku8MBReadWriteMultipleRegisters:
      u8ModbusADU[u8ModbusADUSize++] = highByte(_u16ReadAddress);
      u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16ReadAddress);
      u8ModbusADU[u8ModbusADUSize++] = highByte(_u16ReadQty);
      u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16ReadQty);
      break;
  }
	
  switch(u8MBFunction)
  {
    case MDM_WRITE_SINGLE_COIL:
    case MDM_WRITE_SINGLE_REG:
//    case MDM_WRITE_MULT_COILS:
//    case MDM_WRITE_MULT_REGS:
//    case ku8MBReadWriteMultipleRegisters:
      u8ModbusADU[u8ModbusADUSize++] = highByte(_u16WriteAddress);
      u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteAddress);
      break;
  }
	
	switch(u8MBFunction)
  {
    case MDM_WRITE_SINGLE_COIL:
      u8ModbusADU[u8ModbusADUSize++] = highByte(_u16WriteQty);
      u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteQty);
      break;
      
    case MDM_WRITE_SINGLE_REG:
      u8ModbusADU[u8ModbusADUSize++] = highByte(write_val);
      u8ModbusADU[u8ModbusADUSize++] = lowByte(write_val);
      break;
      
//    case MDM_WRITE_MULT_COILS:
//      u8ModbusADU[u8ModbusADUSize++] = highByte(_u16WriteQty);
//      u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteQty);
//      u8Qty = (_u16WriteQty % 8) ? ((_u16WriteQty >> 3) + 1) : (_u16WriteQty >> 3);
//      u8ModbusADU[u8ModbusADUSize++] = u8Qty;
//      for (i = 0; i < u8Qty; i++)
//      {
//        switch(i % 2)
//        {
//          case 0: // i is even
//            u8ModbusADU[u8ModbusADUSize++] = lowByte(write_val);
//            break;
//            
//          case 1: // i is odd
//            u8ModbusADU[u8ModbusADUSize++] = highByte(write_val);
//            break;
//        }
//      }
//      break;
      
//    case MDM_WRITE_MULT_REGS:
//    case ku8MBReadWriteMultipleRegisters:
//      u8ModbusADU[u8ModbusADUSize++] = highByte(_u16WriteQty);
//      u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteQty);
//      u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteQty << 1);
//      
//      for (i = 0; i < lowByte(_u16WriteQty); i++)
//      {
//        u8ModbusADU[u8ModbusADUSize++] = highByte(p_u16[i]);
//        u8ModbusADU[u8ModbusADUSize++] = lowByte(p_u16[i]);
//      }
//      break;
      
   
  }
	
	 // append CRC
  u16CRC = CRC16(u8ModbusADU, u8ModbusADUSize);
  
//  u8ModbusADU[u8ModbusADUSize++] = lowByte(u16CRC);
//  u8ModbusADU[u8ModbusADUSize++] = highByte(u16CRC);
  u8ModbusADU[u8ModbusADUSize++] = highByte(u16CRC);
  u8ModbusADU[u8ModbusADUSize++] = lowByte(u16CRC);
  u8ModbusADU[u8ModbusADUSize] = 0;
	
	if(u8ModbusADUSize > buf_size)
		return -1;
	
	memcpy(buf, u8ModbusADU, u8ModbusADUSize);
	
	return u8ModbusADUSize;
	
//	 while (u8BytesLeft && !u8MBStatus)
//  {
//    if (Modbus_Master_Rece_Available())
//    {
//      u8ModbusADU[u8ModbusADUSize++] = Modbus_Master_Read();
//      u8BytesLeft--;
//    }
//    else
//    {
//      /*
//      if (_idle)
//      {
//        _idle();
//      }
//			*/
//    }
//    
//    // evaluate slave ID, function code once enough bytes have been read
//    if (u8ModbusADUSize == 5)
//    {
//      // verify response is for correct Modbus slave
//      if (u8ModbusADU[0] != _u8MBSlave)
//      {
//        u8MBStatus = ku8MBInvalidSlaveID;
//        break;
//      }
//      
//      // verify response is for correct Modbus function code (mask exception bit 7)
//      if ((u8ModbusADU[1] & 0x7F) != u8MBFunction)
//      {
//        u8MBStatus = ku8MBInvalidFunction;
//        break;
//      }
//      
//      // check whether Modbus exception occurred; return Modbus Exception Code
//      if (bitRead(u8ModbusADU[1], 7))
//      {
//        u8MBStatus = u8ModbusADU[2];
//        break;
//      }
//      
//      // evaluate returned Modbus function code
//      switch(u8ModbusADU[1])
//      {
//        case ku8MBReadCoils:
//        case ku8MBReadDiscreteInputs:
//        case ku8MBReadInputRegisters:
//        case ku8MBReadHoldingRegisters:
//        case ku8MBReadWriteMultipleRegisters:
//          u8BytesLeft = u8ModbusADU[2];
//          break;
//          
//        case ku8MBWriteSingleCoil:
//        case ku8MBWriteMultipleCoils:
//        case ku8MBWriteSingleRegister:
//        case ku8MBWriteMultipleRegisters:
//          u8BytesLeft = 3;
//          break;
//          
//        case ku8MBMaskWriteRegister:
//          u8BytesLeft = 5;
//          break;
//      }
//    }
//    if ((Modbus_Master_Millis() - u32StartTime) > ku16MBResponseTimeout)
//    {
//      u8MBStatus = ku8MBResponseTimedOut;
//    }
//  }
//  
//	// verify response is large enough to inspect further
//  if (!u8MBStatus && u8ModbusADUSize >= 5)
//  {
//    // calculate CRC
//    u16CRC = 0xFFFF;
//    for (i = 0; i < (u8ModbusADUSize - 2); i++)
//    {
//      u16CRC = crc16_update(u16CRC, u8ModbusADU[i]);
//    }
//    
//    // verify CRC
//    if (!u8MBStatus && (lowByte(u16CRC) != u8ModbusADU[u8ModbusADUSize - 2] ||
//      highByte(u16CRC) != u8ModbusADU[u8ModbusADUSize - 1]))
//    {
//      u8MBStatus = ku8MBInvalidCRC;
//    }	
//  }
//  
//	// disassemble ADU into words
//  if (!u8MBStatus)
//  {
//    // evaluate returned Modbus function code
//    switch(u8ModbusADU[1])
//    {
//      case ku8MBReadCoils:
//      case ku8MBReadDiscreteInputs:
//        // load bytes into word; response bytes are ordered L, H, L, H, ...
//        for (i = 0; i < (u8ModbusADU[2] >> 1); i++)
//        {
//          if (i < ku8MaxBufferSize)
//          {
//            _u16ResponseBuffer[i] = word(u8ModbusADU[2 * i + 4], u8ModbusADU[2 * i + 3]);
//          }
//          
//          _u8ResponseBufferLength = i;
//        }
//        
//        // in the event of an odd number of bytes, load last byte into zero-padded word
//        if (u8ModbusADU[2] % 2)
//        {
//          if (i < ku8MaxBufferSize)
//          {
//            _u16ResponseBuffer[i] = word(0, u8ModbusADU[2 * i + 3]);
//          }
//          
//          _u8ResponseBufferLength = i + 1;
//        }
//        break;
//        
//      case ku8MBReadInputRegisters:
//      case ku8MBReadHoldingRegisters:
//      case ku8MBReadWriteMultipleRegisters:
//        // load bytes into word; response bytes are ordered H, L, H, L, ...
//        for (i = 0; i < (u8ModbusADU[2] >> 1); i++)
//        {
//          if (i < ku8MaxBufferSize)
//          {
//            _u16ResponseBuffer[i] = word(u8ModbusADU[2 * i + 3], u8ModbusADU[2 * i + 4]);
//          }
//          
//          _u8ResponseBufferLength = i;
//        }
//        break;
//    }
//  }
//  
//  _u8TransmitBufferIndex = 0;
//  u16TransmitBufferLength = 0;
//  _u8ResponseBufferIndex = 0;
//  return u8MBStatus;
	
}


static uint8_t lowByte(uint16_t val)
{
	
	return (val) & 0xff;
}
static uint8_t highByte(uint16_t val)
{
	
	return (val >> 8) & 0xff;
}

