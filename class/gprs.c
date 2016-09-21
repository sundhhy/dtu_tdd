/**
* @file 		gprs.c
* @brief		实现gprs的各种功能.
* @details		1. 开机
* @author		sundh
* @date		16-09-15
* @version	A001
* @par Copyright (c): 
* 		XXX公司
* @par History:         
*	version: author, date, desc
*	A001:sundh,16-09-15，创建，实现开机功能
*/

#include "gprs.h"
#include "stm32f10x_gpio.h"
#include "hardwareConfig.h"
#include "cmsis_os.h"  
#include "debug.h"
#include "gprs_uart.h"
#include "sdhError.h"
#include <string.h>
#include "def.h"

#define UART_SEND	gprs_Uart_write
#define UART_RECV	gprs_Uart_read

//fromt
#define SMS_MSG_PDU		0
#define SMS_MSG_TEXT		1

#define SMS_CHRC_SET_GSM	0
#define SMS_CHRC_SET_UCS2	1
#define SMS_CHRC_SET_IRA	2
#define SMS_CHRC_SET_HEX	3

#define	CMDBUF_LEN		64
#define RETRY_TIMES	5


static struct {
	int8_t	sms_msgFromt;
	int8_t	sms_chrcSet;
	
	
}Gprs_fromt;
static char Gprs_cmd_buf[CMDBUF_LEN];
static int	Gprs_currentState = SHUTDOWN;
int init(gprs_t *self)
{
	gprs_uart_init();
	gprs_Uart_ioctl( GPRSUART_SET_RXWAITTIME_MS, 200);
	
	Gprs_fromt.sms_msgFromt = -1;
	Gprs_fromt.sms_chrcSet = -1;
	return ERR_OK;
}


int startup(gprs_t *self)
{
	char *pp = NULL;
	int		retry = RETRY_TIMES;
	GPIO_ResetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	osDelay(100);
	GPIO_SetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	osDelay(1100);
	GPIO_ResetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	
	osDelay(1000);
	/// 检查开机是否成功
	while( retry)
	{
		strcpy( Gprs_cmd_buf, "ATE0\r\n" );
		UART_SEND( Gprs_cmd_buf, strlen(Gprs_cmd_buf));
		UART_RECV( Gprs_cmd_buf, CMDBUF_LEN);
		pp = strstr((const char*)Gprs_cmd_buf,"OK");
		if(pp)
		{
			Gprs_currentState = GPRS_OPEN_FINISH;
			return ERR_OK;
		}
		osDelay(1000);
	}
	return ERR_FAIL;
	
}

void shutdown(gprs_t *self)
{
	
	GPIO_ResetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	osDelay(100);
	GPIO_SetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	osDelay(1500);
	GPIO_ResetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	
	Gprs_currentState = SHUTDOWN;
}

int	check_simCard( gprs_t *self)
{
	short step = 0;
	short	retry = RETRY_TIMES;
	char *pp = NULL;
	if( Gprs_currentState < GPRS_OPEN_FINISH)
		return ERR_UNINITIALIZED;
	while(1)
	{
		switch(step)
		{
			case 0:
				strcpy( Gprs_cmd_buf, "AT+CPIN?\x00D\x00A" );
				UART_SEND( Gprs_cmd_buf, strlen(Gprs_cmd_buf));
				UART_RECV( Gprs_cmd_buf, CMDBUF_LEN);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");
				if(pp)
				{
					
					step ++;
					retry = RETRY_TIMES;
					break;
				}
				osDelay(100);
				retry --;
				if( retry == 0)
					return ERR_FAIL;
				break;
			case 1:
				strcpy( Gprs_cmd_buf, "AT+CREG?\x00D\x00A" );
				UART_SEND( Gprs_cmd_buf, strlen(Gprs_cmd_buf));
				UART_RECV( Gprs_cmd_buf, CMDBUF_LEN);
				if(((Gprs_cmd_buf[9]=='0')&&
					(Gprs_cmd_buf[11]=='1'))||
				 ((Gprs_cmd_buf[9]=='0')&&
					(Gprs_cmd_buf[11]=='5')))
				{
						DPRINTF(" check_simCard \r\n");
						Gprs_currentState = INIT_FINISH_OK;
						return ERR_OK;
				}
				osDelay(100);
				retry --;
				if( retry == 0)
					return ERR_FAIL;
				break;
				
			default:
				step = 0;
				break;
				
		}		//switch
	}	//while(1)
	
}

int	send_text_sms(  gprs_t *self, char *phnNmbr, char *sms)
{
	uint8_t i = 0;
	char	step = 0;
	short retry = RETRY_TIMES;
	char *pp = NULL;
	int  sms_len = strlen( sms);
	if( phnNmbr == NULL || sms == NULL)
		return ERR_BAD_PARAMETER;
	if( Gprs_currentState < INIT_FINISH_OK)
		return ERR_UNINITIALIZED;
	for( i = 0; i < strlen( phnNmbr); i ++)
	{
		if( phnNmbr[i] >'9' || phnNmbr[i] < '0')
			return ERR_BAD_PARAMETER;	
	}
	
	
	while(1)
	{
		switch(step)
		{
			case 0:		//set msg mode
				if( Gprs_fromt.sms_msgFromt == SMS_MSG_TEXT)
				{
					step ++;
					retry = RETRY_TIMES;
					break;
				}
				
				strcpy( Gprs_cmd_buf, "AT+CMGF=1\x00D\x00A" );
				UART_SEND( Gprs_cmd_buf, strlen(Gprs_cmd_buf));
				UART_RECV( Gprs_cmd_buf, CMDBUF_LEN);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");
				if(pp)
				{
					
					step ++;
					Gprs_fromt.sms_msgFromt = SMS_MSG_TEXT;
					retry = RETRY_TIMES;
					break;
				}
				osDelay(100);
				retry --;
				if( retry == 0)
					return ERR_FAIL;
				break;
			case 1:		//set char mode
				if( Gprs_fromt.sms_chrcSet == SMS_CHRC_SET_GSM)
				{	
					step ++;
					retry = RETRY_TIMES;
					break;
				}
			
				strcpy( Gprs_cmd_buf, "AT+CSCS=\"GSM\"\x00D\x00A" );
				UART_SEND( Gprs_cmd_buf, strlen(Gprs_cmd_buf));
				UART_RECV( Gprs_cmd_buf, CMDBUF_LEN);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");
				if(pp)
				{
					
					step ++;
					Gprs_fromt.sms_chrcSet = SMS_CHRC_SET_GSM;
					retry = RETRY_TIMES;
					break;
				}
				osDelay(100);
				retry --;
				if( retry == 0)
					return ERR_FAIL;
				break;
			case 2:
				sprintf(Gprs_cmd_buf,"AT+CMGS=\"+86%s\"\x00D\x00A",phnNmbr);
				UART_SEND( Gprs_cmd_buf, strlen(Gprs_cmd_buf));
				osDelay(100);
				sms[ sms_len] = 0X1A;		//把字符串的结尾0替换成gprs模块的结束符0x1A
				UART_SEND( sms, sms_len + 1);
				step ++;
				break;
			case 3:
				return ERR_OK;
				
		} //switch
		
		
	}	//while(1)
	
}

/**
 * @brief 从SIM卡中读取指定的号码的信息.
 *
 * @details 返回SIM卡中指定号码的未读信息，如果指定的号码为空，就返回SIM卡中第一条未读信息.
 * 
 * @param[in]	self.
 * @param[in]	phnNmbr 指定的号码，可以为空.
 * @param[in]	bufsize 缓存的大小.
 * @param[out]	buf 读取的短信存放在此内存中. 
 * @retval	OK	> 0,短信的长度
 * @retval	ERROR	< 0
 */
int	read_phnNmbr_sms( gprs_t *self, char *phnNmbr, char *buf, int bufsize)
{
	
	return 0;
}



int sms_test( gprs_t *self, char *phnNmbr, char *buf, int bufsize)
{
	int ret = 0;
	int count = 0;
	char *pp = NULL;
	strcpy( buf, "sms test ! reply \" succeed \" in 30 second!\r\n"); 
	
	ret = self->send_text_sms( self, phnNmbr, buf);
	if( ret != ERR_OK)
		return ret;
	
	memset( buf, 0, bufsize);
	
	while(1)
	{
		
		ret = self->read_phnNmbr_sms( self, phnNmbr, buf, bufsize);
		if( ret> 0)
			break;
		
		osDelay(1000);
		count ++;
		
		if( count > 30)
			break;
		
	}
	
	
	pp = strstr((const char*)buf,"succeed");
	if(pp)
		return ERR_OK;
	
	return ERR_UNKOWN;
}


CTOR(gprs_t)
FUNCTION_SETTING(init, init);
FUNCTION_SETTING(startup, startup);
FUNCTION_SETTING(shutdown, shutdown);
FUNCTION_SETTING(check_simCard, check_simCard);
FUNCTION_SETTING(send_text_sms, send_text_sms);
FUNCTION_SETTING(read_phnNmbr_sms, read_phnNmbr_sms);
FUNCTION_SETTING(sms_test, sms_test);

END_CTOR
