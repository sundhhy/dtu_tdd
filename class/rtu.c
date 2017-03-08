
#include "cmsis_os.h"                                           // CMSIS RTOS header file
#include "osObjects.h"                      // RTOS object definitions
#include "rtu.h"
#include "sdhError.h"
#include "dtuConfig.h"
#include "string.h"
#include "debug.h"
#include "sw_filesys.h"
#include "TTextConfProt.h"
#include "times.h"
#include "led.h"
#include "modbusRTU_cli.h"
#include "adc.h"
#include "times.h"

/*    rtu code */
static void ModbusRTURegTpye3_wrCB(void)
{
	uint16_t u32_h, u32_l ;
	uint16_t val16, i;
	Dtu_config.rtu_addr = regType3_read( 0, REG_LINE);
	
	u32_l = regType3_read( 1, REG_LINE);
	u32_h = regType3_read( 2, REG_LINE);
	Dtu_config.the_485cfg.USART_BaudRate = ( u32_h << 16) |  u32_l;
	
	val16 = regType3_read( 3, REG_LINE);
	if( val16 == 8)
		Dtu_config.the_485cfg.USART_WordLength = USART_WordLength_8b;
	else if( val16 == 9)
		Dtu_config.the_485cfg.USART_WordLength = USART_WordLength_9b;
		
	val16 = regType3_read( 4, REG_LINE);
	Dtu_config.the_485cfg.USART_StopBits = ( val16 & 0x3) << 12;
	val16 = regType3_read( 5, REG_LINE);
	if( val16 == 0)
		Dtu_config.the_485cfg.USART_Parity = USART_Parity_No;
	else if( val16 == 1)
		Dtu_config.the_485cfg.USART_Parity = USART_Parity_Odd;
	else if( val16 == 2)
		Dtu_config.the_485cfg.USART_Parity = USART_Parity_Even;
	
	for( i = 0; i < 3; i++)
	{
		Dtu_config.chn_type[i] = regType3_read( 8 + i * 3, REG_LINE);
		Dtu_config.sign_range[i].rangeH = regType3_read( 9 + i * 3, REG_LINE);
		Dtu_config.sign_range[i].rangeL = regType3_read( 10 + i * 3, REG_LINE);
		
	}
	
	fs_lseek( DtuCfg_file, WR_SEEK_SET, 0);
	fs_write( DtuCfg_file, (uint8_t *)&Dtu_config, sizeof( DtuCfg_t));
	fs_flush();
	osDelay(10);
	
	
}




static void Read_rtuval( char chn, float f_val)
{
	
	uint32_t *temp = (uint32_t *)&f_val;
	uint16_t float_h ;
	uint16_t float_l ;
	
	chn = chn * 2;
	float_l =  *temp & 0xffff;
	regType4_write( chn, REG_LINE, float_l);
	
	float_h = ( *temp & 0xffff0000) >> 16;
	regType4_write( chn + 1, REG_LINE, float_h);
}

static void Read_rtursv( char chn, uint16_t rsv)
{
	
	uint16_t temp = rsv;
	
	
	chn = chn + 6;
	regType4_write( chn, REG_LINE, temp);
	
	
}

static void Read_rtualarm( char chn, uint16_t alarm)
{
	
	uint16_t temp = alarm;
	
	
	chn = chn + 9;
	regType4_write( chn, REG_LINE, temp);
	
	
}

void Init_rtu(void)
{
	uint16_t u32_half ;
	uint16_t val16;
	uint8_t stopbits[4] = { 1, 0, 2, 0};
	int i = 0;
	
	regType3_write( 0, REG_LINE, Dtu_config.rtu_addr);
	u32_half = Dtu_config.the_485cfg.USART_BaudRate & 0xffff;
	regType3_write( 1, REG_LINE, u32_half);
	u32_half = ( Dtu_config.the_485cfg.USART_BaudRate & 0xffff0000) >> 16;
	regType3_write( 2, REG_LINE, u32_half);
	
	if( Dtu_config.the_485cfg.USART_WordLength == USART_WordLength_8b)
		regType3_write( 3, REG_LINE, 8);
	else if( Dtu_config.the_485cfg.USART_WordLength == USART_WordLength_9b)
		regType3_write( 3, REG_LINE, 9);
	else
		regType3_write( 3, REG_LINE, 0);
	val16 = ( Dtu_config.the_485cfg.USART_StopBits >> 12) & 0x3;
	regType3_write( 4, REG_LINE, stopbits[ val16]);
	
	if( Dtu_config.the_485cfg.USART_Parity == USART_Parity_Odd)
		regType3_write( 5, REG_LINE, 2);
	else if( Dtu_config.the_485cfg.USART_Parity == USART_Parity_Even)
		regType3_write( 5, REG_LINE, 1);
	else
		regType3_write( 5, REG_LINE, 0);
	
	for( i = 0; i < 3; i++)
	{
		regType3_write( 8 + i * 3, REG_LINE, Dtu_config.chn_type[i]);
		regType3_write( 9 + i * 3, REG_LINE, Dtu_config.sign_range[i].rangeH);
		regType3_write( 10 + i * 3, REG_LINE, Dtu_config.sign_range[i].rangeL);
//		Set_chnType( i, Dtu_config.chn_type[i]);
//		Set_rangH( i, Dtu_config.sign_range[i].rangeH);
//		Set_rangL( i, Dtu_config.sign_range[i].rangeL);
//		Set_alarmH( i, Dtu_config.sign_range[i].alarmH);
//		Set_alarmL( i, Dtu_config.sign_range[i].alarmL);

	}
	
	
	
	Regist_reg3_wrcb( ModbusRTURegTpye3_wrCB);
	Regist_get_val( Read_rtuval);
	Regist_get_rsv( Read_rtursv);
	Regist_get_alarm( Read_rtualarm);
}
