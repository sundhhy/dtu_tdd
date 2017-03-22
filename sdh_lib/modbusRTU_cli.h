#ifndef __MODBUSRTU_CLI_H_
#define __MODBUSRTU_CLI_H_
#include <stdint.h>

#define CPU_LITTLE_END	1  

/*------------------------------------------------------------------------------------------*/
/*??RAM????  ??????1K  0x20003c00~0x20003fff                                    */
/*                                                                                          */
/*------------------------------------------------------------------------------------------*/ 
/*????????RAM?  0x20003c00~0x20003d00 256??--------------------------------------*/
#define USER_RAM		((uint8_t*)0x20003c00)




#define STATE_SIZE 		16//128										//×´Ì¬¼Ä´æÆ÷
#define INPUT_SIZE 		12//160										//ÊäÈë¼Ä´æÆ÷
#define COIL_SIZE 		32//256										//ÏßÈ¦¼Ä´æÆ÷
#define HOLD_SIZE 		23//160										//±£³Ö¼Ä´æÆ÷


/*????????RAM?  0x20003fc0~0x20003fff 64??---------------------------------------*/

/*MODBUS ??????-----------------------------------------------------------------------*/
#define REG_LINE 	0												//ÏßÐÔµØÖ·
#define REG_MODBUS 	1												//MODBUD???? ???0?00001,1?10001,3?30001,4?40001
#define COIL_ON  	PIO_ON											//?????
#define COIL_OFF 	PIO_OFF											//?????
#define STATE_ON 	1												//?????
#define STATE_OFF 	0												//?????

/*MODBUS cmd-------------------------------------------------------------------------*/
#define READ_COIL			1										//??????
#define READ_STATE			2										//??????
#define READ_HOLD			3										//??????
#define READ_INPUT			4										//??????
#define WRITE_1_COIL		5										//???????
#define WRITE_1_HOLD		6										//???????
#define WRITE_N_COIL		15										//???????
#define WRITE_N_HOLD		16										//??????? 

/* Modbus err ************************************************************************/
#define MB_CMD_ERR		1
#define MB_ADDR_ERR		2
#define MB_DATA_ERR		3
#define MB_MB_ERR		4


typedef void (*Reg3_write_cb)(void);

uint16_t regType3_read(uint16_t hold_address, uint16_t reg_type);
int regType3_write(uint16_t hold_address, uint16_t reg_type, uint16_t val);
uint16_t regType4_read(uint16_t input_address, uint16_t reg_type);
int regType4_write(uint16_t input_address, uint16_t reg_type, uint16_t val);
int Regist_reg3_wrcb( Reg3_write_cb cb);

uint16_t CRC16(uint8_t *puchMsg, uint16_t usDataLen);			         //16Î»crcÐ£Ñé

uint8_t 	modbusRTU_getID(uint8_t *command_buf);
uint16_t modbusRTU_data(uint8_t *command_buf, int cmd_len, uint8_t *ack_buf, int ackbuf_len);

#endif
