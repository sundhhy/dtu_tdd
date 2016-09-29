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
#include "debug.h"



static int set_sms2TextMode(gprs_t *self);
static int serial_cmmn( char *buf, int bufsize);

#define UART_SEND	gprs_Uart_write
#define UART_RECV	gprs_Uart_read

//fromt
#define SMS_MSG_PDU		0
#define SMS_MSG_TEXT		1

#define SMS_CHRC_SET_GSM	0
#define SMS_CHRC_SET_UCS2	1
#define SMS_CHRC_SET_IRA	2
#define SMS_CHRC_SET_HEX	3

#define	CMDBUF_LEN		512
#define RETRY_TIMES	5

#define Tx_RX_DELAY		1
static struct {
	int8_t	sms_msgFromt;
	int8_t	sms_chrcSet;
	
	
}Gprs_fromt;
static char Gprs_cmd_buf[CMDBUF_LEN];
static int	Gprs_currentState = SHUTDOWN;
int init(gprs_t *self)
{
	gprs_uart_init();
	gprs_Uart_ioctl( GPRSUART_SET_TXWAITTIME_MS, 800);
	gprs_Uart_ioctl( GPRSUART_SET_RXWAITTIME_MS, 2000);
	
	Gprs_fromt.sms_msgFromt = -1;
	Gprs_fromt.sms_chrcSet = -1;
	return ERR_OK;
}


int startup(gprs_t *self)
{
//	char *pp = NULL;
//	int		retry = RETRY_TIMES;
	GPIO_ResetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	osDelay(100);
	GPIO_SetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	osDelay(1100);
	GPIO_ResetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	
//	osDelay(5000);
	/// 检查开机是否成功
//	while( retry)
//	{
//		strcpy( Gprs_cmd_buf, "ATE0\r\n" );
//		UART_SEND( Gprs_cmd_buf, strlen(Gprs_cmd_buf));
//		UART_RECV( Gprs_cmd_buf, CMDBUF_LEN);
//		pp = strstr((const char*)Gprs_cmd_buf,"OK");
//		if(pp)
//		{
//			Gprs_currentState = GPRS_OPEN_FINISH;
//			DPRINTF(" %s gprs open successed \r\n", __func__);
//			return ERR_OK;
//		}
//		osDelay(1000);
//		retry --;
//	}
//	DPRINTF(" %s gprs open failed \r\n", __func__);
//	return ERR_FAIL;

	return ERR_OK;
	
}

void shutdown(gprs_t *self)
{
	
	GPIO_ResetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	osDelay(100);
	GPIO_SetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	osDelay(1500);
	GPIO_ResetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	
	Gprs_currentState = SHUTDOWN;
	
	osDelay(5000);
}

int	check_simCard( gprs_t *self)
{
	short step = 0;
	short	retry = RETRY_TIMES;
	char *pp = NULL;

	while(1)
	{
		switch(step)
		{
			
			case 0:
				strcpy( Gprs_cmd_buf, "ATE0\r\n" );
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");
				if(pp)
				{
					Gprs_currentState = GPRS_OPEN_FINISH;
					step ++;
					DPRINTF(" ATE0 succeed! \t\n");
					retry = RETRY_TIMES *10;
					break;
				}
				osDelay(100);
				retry --;
				if( retry == 0) {
					DPRINTF(" ATE0 fail \t\n");
					return ERR_FAIL;
					
				}
				break;
				
			case 1:
				step ++;
				osDelay(1000);

//				UART_RECV( Gprs_cmd_buf, CMDBUF_LEN);
//				pp = strstr((const char*)Gprs_cmd_buf,"Ready");
//				if(pp)
//				{
//					step ++;
//					DPRINTF(" wait ready  succeed! \t\n");
//					retry = RETRY_TIMES;
//					break;
//				}
//				osDelay(100);
//				retry --;
//				if( retry == 0) {
//					DPRINTF(" wait ready  fail \t\n");
//					return ERR_FAIL;
//					
//				}
//				break;	
			case 2:
				strcpy( Gprs_cmd_buf, "AT+CPIN?\x00D\x00A" );
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");
				if(pp)
				{
					
					step ++;
					retry = RETRY_TIMES * 10;
					DPRINTF(" AT+CPIN? succeed! \t\n");
					break;
				}
				osDelay(100);
				retry --;
				if( retry == 0) {
					DPRINTF(" AT+CPIN? fail \t\n");
					return ERR_FAIL;
				}
				break;
			case 3:
				strcpy( Gprs_cmd_buf, "AT+CREG?\x00D\x00A" );
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN);
				if(((Gprs_cmd_buf[9]=='0')&&
					(Gprs_cmd_buf[11]=='1'))||
				 ((Gprs_cmd_buf[9]=='0')&&
					(Gprs_cmd_buf[11]=='5')))
				{
						DPRINTF(" check_simCard succeed !\r\n");
						Gprs_currentState = INIT_FINISH_OK;
						return ERR_OK;
				}
				osDelay(500);
				retry --;
				if( retry == 0) {
					DPRINTF(" AT+CREG? fail %s \r\n", Gprs_cmd_buf );
					return ERR_FAIL;
				}
				break;
				
			default:
				step = 0;
				break;
				
		}		//switch
	}	//while(1)
	
}

int	send_text_sms(  gprs_t *self, char *phnNmbr, char *sms){
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
				if( set_sms2TextMode( self) == ERR_OK)
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
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN);
//				
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
				retry = 30 ;			///短信应答的最长延时是60s，接口的接收超时是2s。
				break;
			case 3:
				UART_RECV( Gprs_cmd_buf, CMDBUF_LEN);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");
				if(pp)
				{
					return ERR_OK;
				}
				pp = strstr((const char*)Gprs_cmd_buf,"ERROR");
				if(pp)
				{
					return ERR_FAIL;
				}
					
				osDelay(200);
				retry --;
				if( retry == 0)
					return ERR_FAIL;
				
		} //switch
		
		
	}	//while(1)
	
}

/**
 * @brief 从SIM卡中读取指定的号码的TEXT格式的信息
 *
 * @details 返回SIM卡中指定号码的未读信息，如果指定的号码为空，就返回SIM卡中第一条未读信息.
 * 
 * @param[in]	self.
 * @param[in]	phnNmbr 指定的号码，可以为空.
 * @param[in]	bufsize 缓存的大小.
 * @param[out]	buf 读取的短信存放在此内存中. 
 * @retval	> 0,短信编号
 * @retval  = 0 无短信
 * @retval	ERROR	< 0
 */
int	read_phnNmbr_TextSMS( gprs_t *self, char *phnNmbr, char *buf, int *len)
{
	char step = 0;
	char i = 0;
	short retry = RETRY_TIMES;
	
	char *pp = NULL;
	
	short number = 0;
	char  text_begin = 0, text_end = 0;
			
	int tmp = 0;
	
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
				if( set_sms2TextMode( self) == ERR_OK)
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
			case 1:		///获取短信的总数量
				strcpy( Gprs_cmd_buf, "AT+CPMS?\x00D\x00A" );
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN);
				tmp = strcspn( Gprs_cmd_buf, "0123456789");			///查找接受到的字符串中的第一个字符串中的偏移	
				number = atoi( Gprs_cmd_buf + tmp);			///第一个数字就是短信的数量
				if( number < 1)
					return 0;	
				i = 1;		///短信从1开始读取
				step ++;
				retry = RETRY_TIMES;
				break;
				
			case 2:		///一次读取短信，并从中读取发送方是指定号码的短信
				sprintf( Gprs_cmd_buf, "AT+CMGR=%d\x00D\x00A", i);
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN);
			
				pp = Gprs_cmd_buf;			///假定pp的值是正常的
				if( phnNmbr)
					pp = strstr((const char*)Gprs_cmd_buf,phnNmbr);
				
				
				if( pp)
				{
					number = 0;
					while( *pp != '\0')
					{
						if( *pp == '\x00A')			/// 内容从第一个\00A 开始， 第二个\00A结束
						{
							if( text_begin == 0) {
								text_begin = 1;
								pp ++;
								continue;
							}
							else {
								text_end = 1;
								
							}
						}
						if( text_end || number > ( *len - 1))
						{
							*len = number;
							return i;
						}
						
						
						if( text_begin) {
							number ++;
							*buf = *pp;
							buf ++;
						}
						
						pp ++;
							
					}		// while( *pp != '\0')
					
					
					return 0;
				}
				i ++;
				if( i > number)
					return 0;
				break;
			default:
				step = 0;
				break;
		}  //switch(step)
		
	}	//while(1)
		
		
		
}


int delete_sms( gprs_t *self, int seq) 
{
	short retry = RETRY_TIMES;
	
	char *pp = NULL;
	
	while(1)
	{
		sprintf( Gprs_cmd_buf, "AT+CMGD=%d\x00D\x00A", seq);
		serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN);
		pp = strstr((const char*)Gprs_cmd_buf,"OK");		
		if( pp)
			return ERR_OK;
		
		pp = strstr((const char*)Gprs_cmd_buf,"ERROR");	
		if( pp)
			return ERR_FAIL;
		
		retry --;
		if( retry == 0)
			return ERR_FAIL;
		
		osDelay(100);
		
		
	}
	
}
/**
 * @brief 向指定的地址发起连接
 *
 * @details 1. .
 *			2. 连接成功后向服务器发送数据.
 *			3. 重复1和2，直到建立4个连接。同一个服务器可以建立多个连接。
 *			4. 回显服务器发送的数据.
 *			5. 服务器段发送finish来结束测试
 * 
 * @param[in]	self.
 * @param[in]	addr. 
 * @param[in]	portnum. 
 * @retval	>=0 连接号
 * @retval	ERR_FAIL 发送失败	
 */
 int tcp_cnnt( gprs_t *self, char *addr, int portnum)
 {
	 
	 
 }

/**
 * @brief 像指定的连接发送tcp数据.
 *
 * @details 1. .
 *			2. 连接成功后向服务器发送数据.
 *			3. 重复1和2，直到建立4个连接。同一个服务器可以建立多个连接。
 *			4. 回显服务器发送的数据.
 *			5. 服务器段发送finish来结束测试
 * 
 * @param[in]	self.
 * @param[in]	cnnt_num. 连接号
 * @param[in]	data. 发送数据地址
 * @param[in]	len. 发送的数据长度
 * @retval	ERR_OK 成功
 * @retval	ERR_FAIL 发送失败	
 */
 int sendto_tcp( gprs_t *self, int cnnt_num, char *data, int len)
 {
	 
	 
 }
 /**
 * @brief 从gprs接收数据.
 *
 * @details 1. .
 *			2. 连接成功后向服务器发送数据.
 *			3. 重复1和2，直到建立4个连接。同一个服务器可以建立多个连接。
 *			4. 回显服务器发送的数据.
 *			5. 服务器段发送finish来结束测试
 * 
 * @param[in]	self.
 * @param[in]	buf. 接收的缓存地址
 * @param[in]	lsize. 缓存长度
 * @param[out]	lsize. 接收到的数据长度
 * @retval	>0 数据的连接号
 * @retval	ERR_FAIL 接收失败	
 */
 int recvform_tcp( gprs_t *self, char *buf, int *lsize)
 {
	 
	 
 }


int sms_test( gprs_t *self, char *phnNmbr, char *buf, int bufsize)
{
	int ret = 0;
	int count = 0;
	char *pp = NULL;
	int len = bufsize;
	strcpy( buf, "sms test ! reply \" Succeed \" in 30 second!\r\n"); 
	
//	for( count = 0; count < 1024; count ++)
//	{
//		
//		buf[count] = 8 + '0';
//	}
//	buf[count] = '\0';
//	count = 0;
	ret = self->send_text_sms( self, phnNmbr, buf);
	if( ret != ERR_OK)
		return ret;
	
	memset( buf, 0, bufsize);
	
	while(1)
	{
		
		ret = self->read_phnNmbr_TextSMS( self, phnNmbr, buf, &len);
		if( ret> 0)
			break;
		
		osDelay(1000);
		count ++;
		
		if( count > 10)
			break;
		
	}
	
	
	pp = strstr((const char*)buf,"Succeed");
	if(pp)
	{
		self->delete_sms( self, ret);
		return ERR_OK;
	}
	
	return ERR_UNKOWN;
}


/**
 * @brief 测试tcp的数据收发.
 *
 * @details 1. 向服务器发起连接.
 *			2. 连接成功后向服务器发送数据.
 *			3. 重复1和2，直到建立4个连接。同一个服务器可以建立多个连接。
 *			4. 回显服务器发送的数据.
 *			5. 服务器段发送finish来结束测试
 * 
 * @param[in]	self.
 * @param[in]	tets_addr. 服务器的地址
 * @param[in]	portnum. 服务器的端口号
 * @param[in]	buf. 测试缓存地址
 * @param[in]	bufsize. 测试缓存的大小
 * @retval	OK	
 * @retval	ERROR	
 */
int tcp_test( gprs_t *self, char *tets_addr, int portnum, char *buf, int bufsize)
{
	short step = 0;
	short i = 0;
	int ret = 0;
	int seq[4] = {-1};
	int len = 0;
	char *pp;
	while(1)
	{
		switch( step)
		{
			case 0:
				ret = self->tcp_cnnt( self, tets_addr, portnum);
				if( ret < 0)
					return ERR_FAIL;
				step ++;
				seq[i] = ret;
				break;
			case 1:
				sprintf( buf, "the %d'st connect is established\n", i);
				DPRINTF("%s \n", buf);
				ret = self->sendto_tcp( self, seq[i], buf, strlen( buf) + 1);
				if( ret != ERR_OK)
				{
					DPRINTF("send to connect %d tcp data fail \n", i);
					return ERR_FAIL;
				}
				i ++;
				if( i < 3)			///重复连接4次
				{
					step = 0;
					break;
				}
				step ++;
			case 2:
				len = bufsize;
				ret = self->recvform_tcp( self, buf, &len);
				if( ret > 0)
				{
					pp = strstr((const char*)buf,"finished");
					if( pp)
						return ERR_OK;
					
					DPRINTF(" recv : %s \n", buf);
					self->sendto_tcp( self, seq[i], buf, strlen( buf) + 1);
				}
				break;
			default:break;
			
			
		}		//switch
		
		
	}		//while(1)
	
	
	
}

static int serial_cmmn( char *buf, int bufsize)
{
	UART_SEND( buf, strlen(buf));
	#ifdef Tx_RX_DELAY
	osDelay( Tx_RX_DELAY);
#endif
	return UART_RECV( buf, bufsize);
}

static int set_sms2TextMode(gprs_t *self)
{
	int retry = 1;
	char *pp = NULL;
	
	if( Gprs_fromt.sms_msgFromt == SMS_MSG_TEXT)
	{
		return ERR_OK;
	}
	while(1)
	{
		strcpy( Gprs_cmd_buf, "AT+CMGF=1\x00D\x00A" );
		serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN);
		pp = strstr((const char*)Gprs_cmd_buf,"OK");
		if(pp)
		{
			
			Gprs_fromt.sms_msgFromt = SMS_MSG_TEXT;
			return ERR_OK;
		}
		
		retry --;
		if( retry == 0)
			return ERR_FAIL;
		osDelay(100);
	}
	
}
CTOR(gprs_t)
FUNCTION_SETTING(init, init);
FUNCTION_SETTING(startup, startup);
FUNCTION_SETTING(shutdown, shutdown);
FUNCTION_SETTING(check_simCard, check_simCard);
FUNCTION_SETTING(send_text_sms, send_text_sms);
FUNCTION_SETTING(read_phnNmbr_TextSMS, read_phnNmbr_TextSMS);
FUNCTION_SETTING(delete_sms, delete_sms);
FUNCTION_SETTING(sms_test, sms_test);


FUNCTION_SETTING(tcp_cnnt, tcp_cnnt);
FUNCTION_SETTING(sendto_tcp, sendto_tcp);
FUNCTION_SETTING(recvform_tcp, recvform_tcp);
FUNCTION_SETTING(tcp_test, tcp_test);

END_CTOR
