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
#include "dtuConfig.h"

#include "CircularBuffer.h"



static int set_sms2TextMode(gprs_t *self);
static int serial_cmmn( char *buf, int bufsize, int delay_ms);
void read_event(void *buf, void *arg);
static int prepare_ip(gprs_t *self);
static int get_sms_phNO(char *databuf, char *phbuf);

#define UART_SEND	gprs_Uart_write
#define UART_RECV	gprs_Uart_read

//fromt
#define SMS_MSG_PDU		0
#define SMS_MSG_TEXT		1
#define SMS_MSG_ERR			2

#define SMS_CHRC_SET_GSM	0
#define SMS_CHRC_SET_UCS2	1
#define SMS_CHRC_SET_IRA	2
#define SMS_CHRC_SET_HEX	3

#define	CMDBUF_LEN		64


//ip connect state
#define CNNT_DISCONNECT		0
#define	CNNT_ESTABLISHED	1
#define CNNT_SENDERROR		2


static struct {
	int8_t	sms_msgFromt;
	int8_t	sms_chrcSet;
	
	
}Gprs_state;
static struct {
	
//	int8_t	cnn_num[IPMUX_NUM];
	
	int8_t	cnn_state[IPMUX_NUM];
}Ip_cnnState;

static char Gprs_cmd_buf[CMDBUF_LEN];
static short	Gprs_currentState = SHUTDOWN;
static short	FlagSmsReady = 0;
static int RcvSms_seq = 0;

int init(gprs_t *self)
{
	gprs_uart_init();
	gprs_Uart_ioctl( GPRSUART_SET_TXWAITTIME_MS, 800);
	gprs_Uart_ioctl( GPRSUART_SET_RXWAITTIME_MS, 2000);
	
	self->event_cbuf->buf = malloc( EVENT_MAX * sizeof(tElement));
	if( self->event_cbuf->buf == NULL)
	{
		DPRINTF("gprs malloc event buf failed !\n");
		return ERR_MEM_UNAVAILABLE;
	}
	self->event_cbuf->read = 0;
	self->event_cbuf->write = 0;
	self->event_cbuf->size = EVENT_MAX;
	
	regRxIrq_cb( read_event, (void *)self);
	Gprs_state.sms_msgFromt = -1;
	Gprs_state.sms_chrcSet = -1;
	return ERR_OK;
}


void startup(gprs_t *self)
{
//	char *pp = NULL;
//	int		retry = RETRY_TIMES;
	GPIO_ResetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	osDelay(100);
	GPIO_SetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	osDelay(1100);
	GPIO_ResetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	
	
	
	
	return ;
	
}

void shutdown(gprs_t *self)
{
	
	char *pp;
	while(1)
	{
		strcpy( Gprs_cmd_buf, "AT+CPOWD=1\r\n" );		//正常关机
		serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,5000);
		pp = strstr((const char*)Gprs_cmd_buf,"NORMAL POWER DOWN");
		if( pp)
			return;
	}
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
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1);
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
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1);
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
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1);
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
//	uint8_t i = 0;
	char	step = 0;
	short retry = RETRY_TIMES;
	char *pp = NULL;
	int  sms_len = strlen( sms);
	if( phnNmbr == NULL || sms == NULL)
		return ERR_BAD_PARAMETER;
	if( FlagSmsReady == 0)
		return ERR_UNINITIALIZED;
	if( Gprs_currentState < INIT_FINISH_OK)
		return ERR_UNINITIALIZED;
	
	if( check_phoneNO( phnNmbr) != ERR_OK)
		return ERR_BAD_PARAMETER;	
	
	
	
	while(1)
	{
		switch(step)
		{
			case 0:		//set msg mode
				if( set_sms2TextMode( self) == ERR_OK)
				{
					
					step ++;
					Gprs_state.sms_msgFromt = SMS_MSG_TEXT;
					retry = RETRY_TIMES;
					break;
				}
				osDelay(100);
				retry --;
				if( retry == 0)
					return ERR_DEV_TIMEOUT;
				break;
			case 1:		//set char mode
				if( Gprs_state.sms_chrcSet == SMS_CHRC_SET_GSM)
				{	
					step ++;
					retry = RETRY_TIMES;
					break;
				}
			
				strcpy( Gprs_cmd_buf, "AT+CSCS=\"GSM\"\x00D\x00A" );
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1);
//				
				pp = strstr((const char*)Gprs_cmd_buf,"OK");
				if(pp)
				{
					
					step ++;
					Gprs_state.sms_chrcSet = SMS_CHRC_SET_GSM;
					retry = RETRY_TIMES;
					break;
				}
				osDelay(100);
				retry --;
				if( retry == 0)
					return ERR_DEV_TIMEOUT;
				break;
			case 2:
				sprintf(Gprs_cmd_buf,"AT+CMGS=\"%s\"\x00D\x00A",phnNmbr);
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
//					sprintf(Gprs_cmd_buf,"AT+CMGS=\"%s\"\x00D\x00A",phnNmbr);
//					UART_SEND( Gprs_cmd_buf, strlen(Gprs_cmd_buf));
//					osDelay(100);
//					UART_SEND( sms, sms_len + 1);
					Gprs_state.sms_msgFromt = SMS_MSG_ERR;
					return ERR_FAIL;
				}
					
				osDelay(200);
				retry --;
				if( retry == 0)
					return ERR_DEV_TIMEOUT;
				
		} //switch
		
		
	}	//while(1)
	
}

/**
 * @brief 从SIM卡中读取指定的号码的TEXT格式的信息
 *
 * @details 返回SIM卡中指定号码的未读信息，如果输入的号码内存为空或者号码不合法就返回第一条短信
 *					如果号码非法，将读取的短信的发送方号码放入短信内存
 * 
 * @param[in]	self.
 * @param[in]	phnNmbr 指定的号码，可以为空.
 * @param[in]	bufsize 缓存的大小.
 * @param[out]	buf 读取的短信存放在此内存中. 
 * @param[out] phnNmbr	当没有指定一个合法的号码的时候，将读取的短信的号码放入其中
 * @retval	> 0,短信编号
 * @retval  = 0 无短信
 * @retval	ERROR	< 0
 */

int	read_phnNmbr_TextSMS( gprs_t *self, char *phnNmbr, char *in_buf, char *out_buf, int *len)
{
	char step = 0;
	char i = 0;
	short retry = RETRY_TIMES;
	
	char *pp = NULL;
	char	*ptarget = out_buf;
	
	short number = 0;
	char  text_begin = 0, text_end = 0;
			
	short tmp = 0;
	short legal_phno = 0;
	int	phnoend_offset = 0;
	
	if( check_phoneNO( phnNmbr) == ERR_OK)
		legal_phno = 1;
	
	
	while(1)
	{
		switch(step)
		{
			case 0:		//set msg mode
				if( set_sms2TextMode( self) == ERR_OK)
				{
					
					step ++;
					Gprs_state.sms_msgFromt = SMS_MSG_TEXT;
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
//				strcpy( Gprs_cmd_buf, "AT+CNMI?\x00D\x00A" );
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1);
				tmp = strcspn( Gprs_cmd_buf, "0123456789");			///查找接受到的字符串中的第一个字符串中的偏移	
				number = atoi( Gprs_cmd_buf + tmp);			///第一个数字就是短信的数量
				if( number < 1)
					return 0;	
				i = RcvSms_seq;		///短信从短信CMIT通知开始读取
				step ++;
				retry = RETRY_TIMES;
				break;
				
			case 2:		///一次读取短信，并从中读取发送方是指定号码的短信
				sprintf( in_buf, "AT+CMGR=%d\x00D\x00A", i);
				serial_cmmn( in_buf, *len, 1000);
			
				pp = in_buf;			///假定pp的值是正常的
				if(  legal_phno)			//输入的号码合法，就进行匹配
				{
					pp = strstr((const char*)in_buf,phnNmbr);
				}
				else if( phnNmbr)	//没有指定要接收短信的发送方号码时，把发送方号码放入传入的号码中去
				{
					
					
					phnoend_offset = get_sms_phNO( pp, phnNmbr);
					if( phnoend_offset > 0)
						pp += phnoend_offset ;
					else			//找不到任何有效的号码，就不收这个短信了
						pp = NULL;
				}
				
				
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
							
							*ptarget  = '\0';
							*len = number;
							return i;
						}
						
						
						if( text_begin) {
							number ++;
							*ptarget = *pp;
							ptarget ++;
						}
						
						pp ++;
							
					}		// while( *pp != '\0')
					
//todo  如果出现接收到的数据没有结尾怎么办？
					return 0;
				}
				pp = strstr((const char*)in_buf,"REC");		//当接收的数据中有REC的时候，可能是串口数据没收全，再试几次
				if( pp && retry)
				{
					retry --;
				}
				else
				{
					retry = RETRY_TIMES;
					i ++;
				}
				if( i > number)
					return ERR_FAIL;
				break;
			default:
				step = 0;
				break;
		}  //switch(step)
		
	}	//while(1)
		
		
		
}
//读取指定序列号的短信
int	read_seq_TextSMS( gprs_t *self, char *phnNmbr, int seq, char *buf, int *len)
{
	char step = 0;
	char i = 0;
	short retry = RETRY_TIMES;
	
	char *pp = NULL;
	char	*ptarget = buf;
	
	short number = 0;
	char  text_begin = 0, text_end = 0;
			
	short legal_phno = 0;
	int	phnoend_offset = 0;
	
	if( check_phoneNO( phnNmbr) == ERR_OK)
		legal_phno = 1;
	
	
	while(1)
	{
		switch(step)
		{
			case 0:		//set msg mode
				if( set_sms2TextMode( self) == ERR_OK)
				{
					
					step ++;
					Gprs_state.sms_msgFromt = SMS_MSG_TEXT;
					retry = RETRY_TIMES;
					break;
				}
				osDelay(100);
				retry --;
				if( retry == 0)
					return ERR_FAIL;
				break;
			case 1:		
				sprintf( buf, "AT+CMGR=%d\x00D\x00A", seq);
				serial_cmmn( buf, *len, 1000);
			
				pp = buf;			
				if(  legal_phno)			//输入的号码合法，就进行匹配
				{
					pp = strstr((const char*)buf,phnNmbr);
				}
				else if( phnNmbr)	//没有指定要接收短信的发送方号码时，把发送方号码放入传入的号码中去
				{
					
					
					phnoend_offset = get_sms_phNO( pp, phnNmbr);
					if( phnoend_offset > 0)
						pp += phnoend_offset ;
					else			//找不到任何有效的号码，就不收这个短信了
						pp = NULL;
				}
				
				
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
							
							*ptarget  = '\0';
							*len = number;
							return i;
						}
						
						
						if( text_begin) {
							number ++;
							*ptarget = *pp;
							ptarget ++;
						}
						
						pp ++;
							
					}		// while( *pp != '\0')
					
//todo  如果出现接收到的数据没有结尾怎么办？
					return ERR_OK;
				}
				pp = strstr((const char*)buf,"REC");		//当接收的数据中有REC的时候，可能是串口数据没收全，再试几次
				if( pp && retry)
				{
					retry --;
				}
				else
				{
					return ERR_FAIL;
				}
				
				break;
			default:
				step = 0;
				return ERR_FAIL;
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
		serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1);
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
 * @param[in]	cnnt_num.  连接序号
 * @param[in]	addr. 
 * @param[in]	portnum. 
 * @retval	>=0 连接号
 * @retval	ERR_FAIL 发送失败	
 * @retval	ERR_BAD_PARAMETER 输入的连接号超出范围
 * @retval	ERR_ADDR_ERROR 输入的地址无法连接
 */
 int tcpip_cnnt( gprs_t *self, int cnnt_num,char *prtl, char *addr, int portnum)
 {
	short step = 0;
	short	retry = RETRY_TIMES;
	int ret = 0;
	char *pp = NULL;

	if( cnnt_num > IPMUX_NUM)
		return ERR_BAD_PARAMETER;
	while(1)
	{
		switch( step)
		{
			case 0:
				ret = prepare_ip( self);
				if( ret != ERR_OK)
					return ERR_FAIL;
				step ++;
			case 1:		//先关闭这个连接
				sprintf( Gprs_cmd_buf, "AT+CIPSTATUS=%d\x00D\x00A", cnnt_num);
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,500);
				pp = strstr((const char*)Gprs_cmd_buf,"CONNECTED");	
				if( pp)
				{
					step ++;
					break;
				}
				step += 2;
				break;
			case 2:
				sprintf( Gprs_cmd_buf, "AT+CIPCLOSE=%d,0\x00D\x00A", cnnt_num);		//quick close
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1000);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");	
				if( pp)
					step ++;
				break;
			case 3:
				sprintf( Gprs_cmd_buf, "AT+CIPSTART=%d,\"TCP\",\"%s\",\"%d\"\x00D\x00A", cnnt_num, addr, portnum);
				DPRINTF("  %s ", Gprs_cmd_buf);
				UART_SEND( Gprs_cmd_buf, strlen( Gprs_cmd_buf));
				osDelay(10);
				retry = RETRY_TIMES * 10;
				step++;
			case 4:
				
				ret = UART_RECV( Gprs_cmd_buf, CMDBUF_LEN);
				if( ret > 0)
				{
					pp = strstr((const char*)Gprs_cmd_buf,"ERROR");	
					if( pp)
						return ERR_ADDR_ERROR;
					pp = strstr((const char*)Gprs_cmd_buf,"CONNECT OK");	
					if( pp)
					{
						Ip_cnnState.cnn_state[ cnnt_num] = CNNT_ESTABLISHED;
						osDelay(1000);
						return ERR_OK;
					}
					pp = strstr((const char*)Gprs_cmd_buf,"CONNECT FAIL");
					if( pp)
					{	
						return ERR_BAD_PARAMETER;
					}
					
				
				}
				retry --;
				if( retry == 0)
					return ERR_DEV_TIMEOUT;
//				osDelay(1000);
				
				
			default:break;
		}
	}
		
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
 * @retval	ERR_BAD_PARAMETER 连接号非法
 * @retval	ERR_UNINITIALIZED 连接未建立
 * @retval	ERR_FAIL 发送命令没有收到正确的回复
 */
int sendto_tcp( gprs_t *self, int cnnt_num, char *data, int len)
{
	char *pp;
	int retry = RETRY_TIMES;
	if( cnnt_num > IPMUX_NUM)
		return ERR_BAD_PARAMETER;
	if( Ip_cnnState.cnn_state[ cnnt_num] != CNNT_ESTABLISHED)
		return ERR_UNINITIALIZED;
	
	sprintf( Gprs_cmd_buf, "AT+CIPSEND=%d,%d\x00D\x00A", cnnt_num, len);
	UART_SEND( Gprs_cmd_buf, strlen( Gprs_cmd_buf));
	osDelay(100);
	UART_SEND( data, len);
	while(1)
	{
		
		UART_RECV( Gprs_cmd_buf, CMDBUF_LEN);
		
		pp = strstr((const char*)Gprs_cmd_buf,"OK");		
		if( pp)
			return ERR_OK;
		pp = strstr((const char*)Gprs_cmd_buf,"FAIL");		
		if( pp)
			return ERR_FAIL;
		pp = strstr((const char*)Gprs_cmd_buf,"ERROR");		
		if( pp)
		{
			Ip_cnnState.cnn_state[ cnnt_num] = CNNT_SENDERROR;
			return ERR_FAIL;
		}
		
		retry --;
		
		osDelay(500);
		if( retry == 0)
			return ERR_OK;
	}
	
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
	#if 0
	int ret;
	char *pp;
	int recv_seq = -1;
	int tmp = 0;
	int buf_size = *lsize ;
	*lsize = 0;
	
	ret = UART_RECV( buf, buf_size);
	if( ret < 1)
	 return ERR_FAIL;

	pp = strstr((const char*)buf,"CLOSED");
	if( pp)
	{
		tmp = strcspn( buf, "0123456789");	
		recv_seq = atoi( buf + tmp);
		Ip_cnnState.cnn_state[ recv_seq] = CNNT_DISCONNECT;
		return recv_seq;
	}
	pp = strstr((const char*)buf,"RECEIVE");	
	if( pp == NULL)
		return ERR_FAIL;
	
	tmp = strcspn( buf, "0123456789");	
	recv_seq = atoi( buf + tmp);
	*lsize = atoi( buf + tmp + 1);
	
	while( *pp != '\x00A')
		pp++;
	memcpy( buf, pp, *lsize);

	return recv_seq;
	#endif
	return 0;
}
int deal_tcpclose_event( gprs_t *self, char *data, int len)
{
	char *pp;
	short tmp, close_seq;
//	self->event = CLR_EVENT( self->event, tcp_close);
	pp = strstr((const char*)data,"CLOSED");
	if( pp)
	{
		tmp = strcspn( data, "0123456789");	
		close_seq = atoi( data + tmp);
		Ip_cnnState.cnn_state[ close_seq] = CNNT_DISCONNECT;
		return close_seq;
	}
	return ERR_BAD_PARAMETER;
}




int	create_newContact( gprs_t *self, char *name, char *phonenum)
{
	
	return ERR_OK;
}

int	modify_contact( gprs_t *self, char *name, char *phonenum)
{
	
	return ERR_OK;
}

int	delete_contact( gprs_t *self, char *name)
{
	
	return ERR_OK;
}
	
//返回值是读取的短信的编号
int deal_smsrecv_event( gprs_t *self, void *event, char *buf, int *lsize, char *phno)
{
	gprs_event_t *t_event = ( gprs_event_t *)event;
	int ret = self->read_seq_TextSMS( self, phno, t_event->arg, buf, lsize);
	if( ret == ERR_OK)
		return t_event->arg;
	else
		return ERR_FAIL;
}
int deal_tcprecv_event( gprs_t *self, char *in_buf, char *out_buf, int *len)
{
//	char *pp;
//	int recv_seq = -1;
//	int tmp = 0;
//	self->event = CLR_EVENT( self->event, tcp_receive);
//	pp = strstr((const char*)in_buf,"RECEIVE");	
//	if( pp == NULL)
//		return ERR_FAIL;
//	
//	tmp = strcspn( pp, "0123456789");	
//	recv_seq = atoi( pp + tmp);
//	*len = atoi( pp + tmp + 2);
//	while( *pp != '\x00A')
//		pp++;
//	
//	memcpy( out_buf, pp + 1, *len);
//	out_buf[ *len] = 0;
//	return recv_seq;
	return 0;
}




static int get_cmit_seq( char *data)
{
	//+CMTI:"SM",1
	int tmp;
	tmp = strcspn( data, "0123456789");	
	return  atoi( data + tmp);
}

void read_event(void *buf, void *arg)
{
	char *pp;
	gprs_t *cthis;
	int tmp = 0;		
	gprs_event_t	*event;
	if( arg == NULL)
		return ;
	
	cthis = ( gprs_t *)arg;
	

	event = malloc(	sizeof( gprs_event_t));
	if( event == NULL)
		return ;
	pp = strstr((const char*)buf,"CLOSED");
	if( pp)
	{
		event = malloc(	sizeof( gprs_event_t));
		if( event)
		{
			event->type = tcp_close;
			event->arg = atoi( pp);
			CBWrite( cthis->event_cbuf, event);
		}
	}
	//+RECEIVE:0,6 \n
	//123456
	pp = strstr((const char*)buf,"RECEIVE");
	if( pp)
	{
		event = malloc(	sizeof( gprs_event_t));
		if( event)
		{
			
			event->type = tcp_receive;
			event->arg = atoi( pp);
			pp = strstr(buf,",");
			tmp = atoi( pp) ;
			event->data = malloc( tmp + 1);
			if( event->data)
			{
				
				while( *pp != '\x00A')
					pp++;
				
				memcpy( event->data, pp + 1, tmp);
				event->data[tmp] = '\0';
	
			}
			CBWrite( cthis->event_cbuf, event);
		}
		
	}
	pp = strstr((const char*)buf,"CMTI");
	if( pp)
	{
		
		event = malloc(	sizeof( gprs_event_t));
		if( event)
		{
			event->type = sms_urc;
			event->arg = get_cmit_seq(pp);;
			CBWrite( cthis->event_cbuf, event);
		}
		
		
	}
	pp = strstr((const char*)buf,"SMS Ready");
	if( pp)
	{
		FlagSmsReady = 1;
		
		
	}
	
	
	
}

int get_event( gprs_t *self, void **event)
{
	
	return CBRead( self->event_cbuf, event);
	
//	char *pp;
//	
//	if( self->event == 0)
//		return 0;
//	gprs_Uart_ioctl( GPRS_UART_CMD_CLR_RXBLOCK);
//	UART_RECV( buf, *lsize);
//	gprs_Uart_ioctl( GPRS_UART_CMD_SET_RXBLOCK);
//	return self->event;
	
//	if( ret < 1)
//	 return ERR_UNKOWN;
//	
//	*lsize = ret;
//	pp = strstr((const char*)buf,"CLOSED");
//	if( pp)
//	{
//		
//		return tcp_close;
//	}
//	pp = strstr((const char*)buf,"RECEIVE");
//	if( pp)
//	{
//		return tcp_receive;
//	}
//	pp = strstr((const char*)buf,"CMTI");
//	if( pp)
//	{
//		return sms_urc;
//		
//	}
//	return ERR_UNKOWN;
	
	//每次都去读取下短信吧，防止被垃圾短信塞满短信存储区而导致无法接收短信了
//	return sms_urc;
}
int linkRecv_seq( gprs_t *self, void *event)
{
	gprs_event_t *this_event = (gprs_event_t *)event;
	if( CKECK_EVENT( this_event, tcp_receive) )
	{
		return this_event->arg;
		
	}
	
	return ERR_FAIL;
}
void deal_link_event( gprs_t *self, void *event, char *out_buf, int *lsize)
{
	int tmp = 0;
	gprs_event_t *this_event = (gprs_event_t *)event;
	if( CKECK_EVENT( this_event, tcp_receive) )
	{
		tmp = sizeof( this_event->data);
		if( *lsize > tmp)
			*lsize = tmp;
		memcpy( out_buf, this_event->data, *lsize);
		
		return ;
		
	}
	if( CKECK_EVENT( this_event, tcp_close) )
	{
		
		Ip_cnnState.cnn_state[ this_event->arg] = CNNT_DISCONNECT;
		return ;
	}
	
	
}

void free_event( gprs_t *self, void *event)
{
	gprs_event_t *this_event = (gprs_event_t *)event;
	if( CKECK_EVENT( this_event, tcp_receive) )
	{
		if( this_event->data)
			free( this_event->data);
		
	}
	free(event);
}

//返回第一个未处于连接状态的序号
int	get_firstDiscnt_seq( gprs_t *self)
{
	int i = 0; 
	for( i = 0 ; i < IPMUX_NUM; i ++)
	{
		if( Ip_cnnState.cnn_state[ i] != CNNT_ESTABLISHED)
			return i;
		
	}
	
	return ERR_FAIL;
	
}
int	get_firstCnt_seq( gprs_t *self)
{
	int i = 0; 
	for( i = 0 ; i < IPMUX_NUM; i ++)
	{
		if( Ip_cnnState.cnn_state[ i] == CNNT_ESTABLISHED)
			return i;
		
	}
	
	return ERR_FAIL;
	
}

int set_dns_ip( gprs_t *self, char *dns_ip)
{
	short	retry = RETRY_TIMES;
	char *pp = NULL;
	
	if( check_ip( dns_ip) != ERR_OK)
		return ERR_BAD_PARAMETER;

	while(1)
	{

		sprintf( Gprs_cmd_buf,"%s%s\r\n",AT_SET_DNSIP, dns_ip);
		serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1000);
		pp = strstr((const char*)Gprs_cmd_buf,"OK");	
		if( pp)
		{
			return ERR_OK;
		}
		pp = strstr((const char*)Gprs_cmd_buf,"ERROR");	
		if( pp)
		{
			return ERR_BAD_PARAMETER;
		}
		if(	retry)
			retry --;
		else
			return ERR_FAIL;
				
			
	}
	
}


int get_apn( gprs_t *self, char *buf)
{
	if( buf == NULL )
		return ERR_BAD_PARAMETER;
	if( Dtu_config.apn[0] == 0)
		strcpy(buf, "CNMT");
	else
		strcpy(buf,Dtu_config.apn);
	
	return ERR_OK;
	
	
}



int read_smscAddr(gprs_t *self, char *addr)
{
	short step = 0;
	short	retry = RETRY_TIMES;
	char *pp = NULL;
//	int tmp = 0;
	if( check_phoneNO( addr) == ERR_OK)
		return ERR_OK;
	
	//返回SIM卡的默认短信中心号码
	while(1)
	{
		switch( step)
		{
			case 0:
				strcpy( Gprs_cmd_buf,"AT+CSCA=?\r\n");
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1000);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");	
				if( pp)
				{
					step ++;
					retry = RETRY_TIMES;
					break;
				}
				if( retry)
					retry --;
				else
					return ERR_FAIL;
				break;
			case 1:
				sprintf( Gprs_cmd_buf,"AT+CSCA?\r\n");
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1000);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");	
				if( pp)
				{
					//+CSCA: "+8613800571500",145
//					tmp = strcspn( pp, "0123456789");	
					
					pp = strstr((const char*)Gprs_cmd_buf,"+CSCA:");
					if( pp)
					{
						pp += strlen("+CSCA:") + 1;
						//"+8613800571500",145
						while( *pp != ',')
						{
							
							*addr = *pp;
							addr ++;
							pp ++;
							
						}

						return ERR_OK;
					}
					return ERR_FAIL;
				}
				pp = strstr((const char*)Gprs_cmd_buf,"ERROR");	
				if( pp)
				{
					return ERR_FAIL;
				}
				if(	retry)
					retry --;
				else
					return ERR_FAIL;
				break;
			default:
				return ERR_FAIL;
		}
	}
	
	
}


int set_smscAddr(gprs_t *self, char *addr)
{
	short step = 0;
	short	retry = RETRY_TIMES;
	char *pp = NULL;
	if( check_phoneNO( addr) != ERR_OK)
		return ERR_BAD_PARAMETER;
	while(1)
	{
		switch( step)
		{
			case 0:
				strcpy( Gprs_cmd_buf,"AT+CSCA=?\r\n");
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1000);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");	
				if( pp)
				{
					step ++;
					retry = RETRY_TIMES;
					break;
				}
				if( retry)
					retry --;
				else
					return ERR_FAIL;
				break;
			case 1:
				sprintf( Gprs_cmd_buf,"AT+CSCA=\"%s\"\r\n", addr);
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1000);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");	
				if( pp)
				{
					
					return ERR_OK;
					
				}
				pp = strstr((const char*)Gprs_cmd_buf,"ERROR");	
				if( pp)
				{
					return ERR_FAIL;
				}
				if(	retry)
					retry --;
				else
					return ERR_FAIL;
				break;
			default:
				return ERR_FAIL;
		}
	}
	
	
}

int check_phoneNO(char *NO)
{
	int j = 0;
	char *pp = NO;
	
	if( NO == NULL)
		return ERR_BAD_PARAMETER;
	//跳过"
	if( NO[0] == '\"')
	{
		pp ++;
	}
	//跳过区号：+86
	if( pp[0] == '+')
	{
		pp += 3;
	}
	while(pp[j] != '\0')
	{
		if( pp[j] < '0' || pp[j] > '9')
			break;
		j ++;
		
	}
	if( j == 11)
		return ERR_OK;
	return ERR_BAD_PARAMETER;
	
}
int compare_phoneNO(char *NO1, char *NO2)
{
	int j = 0;
	char *pp1 = NO1;
	char *pp2 = NO2;
	
	if( NO1 == NULL || NO2 == NULL)
		return ERR_BAD_PARAMETER;
	//跳过"
	if( NO1[0] == '\"')
	{
		pp1 ++;
	}
	//跳过区号：+86
	if( pp1[0] == '+')
	{
		pp1 += 3;
	}
	
	//跳过"
	if( NO2[0] == '\"')
	{
		pp2 ++;
	}
	//跳过区号：+86
	if( pp2[0] == '+')
	{
		pp2 += 3;
	}
	if( strlen( pp1 ) != strlen( pp2 ))
		return 1;
	while( pp1[j] != '\0' && pp2[j] != '\0')
	{
		if( pp1[j] != pp2[j])
			return 1;
		j ++;
		
	}
	
	return 0;
	
}

int copy_phoneNO(char *dest_NO, char *src_NO)
{
	int j = 0;
	int count = 0;
	char *pp = src_NO;
	
	//跳过"
	if( src_NO[0] == '\"')
	{
		pp ++;
	}
	//跳过区号：+86
	if( pp[0] == '+')
	{
		
		dest_NO[0] = pp[0];
		dest_NO[1] = pp[1];
		dest_NO[2] = pp[2];
		j = 3;
		
	}
	
	while(pp[j] != '\0')
	{
		if( pp[j] >= '0' && pp[j] <= '9')
		{
			dest_NO[j] = pp[j];
		}
		else
			break;
		j ++;
		count ++;
		if( count == 11)
			break;
	}
	dest_NO[j] = 0;
	return ERR_OK;
	
}
//从字符串中能找到4个点，并且没有处理数字以外的数据，就认为ip地址是合法的
int check_ip(char *ip)
{
	int j = 0;
	int dit_count = 0;
	while(ip[j] != '\0')
	{
		if( ip[j] == '.')
		{
			dit_count ++;
			
			
		}
		if( ip[j] < '0' && ip[j] > '9')
			return ERR_BAD_PARAMETER;
		j ++;
		
	}
	if( dit_count == 3)
		return ERR_OK;
	return ERR_BAD_PARAMETER;
	
}
int sms_test( gprs_t *self, char *phnNmbr, char *buf, int bufsize)
{
	int ret = 0;
	int count = 0;
	char *pp = NULL;
	int len = bufsize;
	DPRINTF("sms test ... \n");
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
		
		ret = self->read_phnNmbr_TextSMS( self, phnNmbr, buf,  buf, &len);
		if( ret> 0)
			break;
		
		osDelay(1000);
		count ++;
		
		if( count > 10)
			break;
		
	}
	self->delete_sms( self, ret);
	DPRINTF(" recv sms : %s \n",buf);
	pp = strstr((const char*)buf,"Succeed");
	if(pp)
	{
		
		return ERR_OK;
	}
	DPRINTF("recv sms : %s \n",buf);
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
	char step = 0;
	char sum = 0;
	short i = 0;
	int ret = 0;
	int len = 0;
	char *pp;
	char finish[IPMUX_NUM] = { 0};
	while(1)
	{
		switch( step)
		{
			case 0:
				ret = self->tcpip_cnnt( self, i, "TCP", tets_addr, portnum);
				if( ret < 0)
					return ERR_FAIL;
				step ++;
				break;
			case 1:
				sprintf( buf, "the %d'st connect is established\n", i);
				DPRINTF("%s \n", buf);
				ret = self->sendto_tcp( self, i, buf, strlen( buf) + 1);
				if( ret != ERR_OK)
				{
					DPRINTF("send to connect %d tcp data fail \n", i);
					return ERR_FAIL;
				}
				i ++;
				if( i < 4)			///重复连接4次
				{
					step = 0;
					break;
				}
				step ++;
			case 2:
				len = bufsize;
			//TODO:16-12-19 重写事件处理后此处未进行相应的改进
//				ret = self->guard_serial( self, buf, &len);
				if( ret == tcp_receive)
				{
					ret = self->deal_tcprecv_event( self, buf,  buf, &len);
					if( len >= 0)
					{
						pp = strstr((const char*)buf,"finished");
						if( pp)
						{
							finish[ ret] = 1;
							sum = 0;
							for( i = 0; i < IPMUX_NUM; i++)
							{
								sum += finish[i];
								
							}
							if( sum == IPMUX_NUM)
								return ERR_OK;
							
						}
						DPRINTF(" recv : %s \n", buf);
						self->sendto_tcp( self, ret, buf, len);
					}
				
					
				}
				if( ret == tcp_close)
				{
					ret = self->deal_tcpclose_event( self, buf, len);
					if( ret >= 0)
						self->tcpip_cnnt( self, ret, "TCP", tets_addr, portnum);
				}
				break;
			default:break;
			
			
		}		//switch
		
		
	}		//while(1)
	
	
	
}

static int serial_cmmn( char *buf, int bufsize, int delay_ms)
{
	UART_SEND( buf, strlen(buf));
	if( delay_ms)
		osDelay( delay_ms);

	return UART_RECV( buf, bufsize);
}

static int set_sms2TextMode(gprs_t *self)
{
	int retry = RETRY_TIMES;
	char *pp = NULL;
	int step = 0;
	
	if( Gprs_state.sms_msgFromt == SMS_MSG_TEXT)
	{
		return ERR_OK;
	}
	while(1)
	{
		switch( step)
		{
			case 0:
				self->set_smscAddr( self, Dtu_config.smscAddr);
				step ++;
			case 1:
				strcpy( Gprs_cmd_buf, "AT+CMGF=1\x00D\x00A" );
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");
				if(pp)
				{
					
					Gprs_state.sms_msgFromt = SMS_MSG_TEXT;
					step ++;
					retry = RETRY_TIMES;
					return ERR_OK;
				}
				
				retry --;
				if( retry == 0)
					return ERR_FAIL;
				break;
			default:
				step = 0;
				break;
			
			
			
		}
		
		osDelay(100);
	}
	
}

static int check_apn(char *apn)
{
	while( *apn != '\0' && apn != NULL)
	{
		if( *apn++ != ' ')
			return 1;
	}
	
	return 0;
	
}

static int prepare_ip(gprs_t *self)
{
	short retry = RETRY_TIMES;
	short step = 0;
	char *pp = NULL;
	
	if( Gprs_currentState == TCP_IP_OK)
		return ERR_OK;
	
	while(1)
	{
		switch( step )
		{
			case 0:
				strcpy( Gprs_cmd_buf, "AT+CIPMUX=1\x00D\x00A" );		//设置连接为多连接
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");
				if(pp)
				{
					retry = RETRY_TIMES;
					step ++;
					break;;
				}
				
				pp = strstr((const char*)Gprs_cmd_buf,"ERROR");
				if(pp)
				{
					strcpy( Gprs_cmd_buf, "AT+CIPSHUT\x00D\x00A" );		//Deactivate GPRS PDP context
					serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1);
					break;
					
				}
				retry --;
				if( retry == 0)
					return ERR_FAIL;
				osDelay(100);
				break;
			
			case 1:
				if( !check_apn( Dtu_config.apn))
					strcpy( Gprs_cmd_buf, "AT+CSTT=\"CMNET\"\x00D\x00A" );		//设置默认gprs接入点
				else
					sprintf( Gprs_cmd_buf, "AT+CSTT=%s\x00D\x00A", Dtu_config.apn );
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");
				if(pp)
				{
					retry = RETRY_TIMES;
					step ++;
					break;;
				}
				
				retry --;
				if( retry == 0)
					return ERR_FAIL;
				osDelay(100);
				break;
			case 2:
				strcpy( Gprs_cmd_buf, "AT+CIICR\x00D\x00A" );		//激活移动场景
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");
				if(pp)
				{
					retry = RETRY_TIMES;
					step ++;
					break;
				}
				
				retry --;
				if( retry == 0)
					return ERR_FAIL;
				osDelay(100);
				break;
			case 3:
				strcpy( Gprs_cmd_buf, "AT+CIFSR\x00D\x00A" );			//获取ip地址
				serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1);
				pp = strstr((const char*)Gprs_cmd_buf,".");
				if(pp)
				{
					Gprs_currentState = TCP_IP_OK;
					return ERR_OK;
				}
				
				retry --;
				if( retry == 0)
					return ERR_FAIL;
				osDelay(100);
				break;
			case 4:
				
					
				self->set_dns_ip( self, Dtu_config.dns_ip);
				return ERR_OK;
				
			default:
				retry = RETRY_TIMES;
				step = 0;
				break;
		}		//switch
		osDelay(1000);
	}		//while
	
}

//接收的短信的格式是：
//+CMGR: "REC UNREAD","+8613918186089", "","02/01/30,20:40:31+00",This is a test
//
//OK
//返回值是号码结束的地方
static int get_sms_phNO(char *databuf, char *phbuf)
{
	
	char *pp;
	int tmp = 0;
	
	tmp = strcspn( databuf, "0123456789");			///查找接受到的字符串中的第一个字符串中的偏移	
	if( tmp == strlen( databuf))			//找不到数字
		return -1;
	if( databuf[ tmp -1] == '+')		//说明带区号
		tmp --;
	
	pp = databuf + tmp;	///第一个数字就是号码开始的地方,把区号页放入号码中
	while( *pp != '"' && *pp != '\0')
	{
		*phbuf = *pp;
		phbuf ++;
		pp ++;
		tmp ++;
	}
	return tmp;
}


CTOR(gprs_t)
FUNCTION_SETTING(init, init);
FUNCTION_SETTING(startup, startup);
FUNCTION_SETTING(shutdown, shutdown);
FUNCTION_SETTING(check_simCard, check_simCard);
FUNCTION_SETTING(send_text_sms, send_text_sms);
FUNCTION_SETTING(read_phnNmbr_TextSMS, read_phnNmbr_TextSMS);
FUNCTION_SETTING(read_seq_TextSMS, read_seq_TextSMS);


FUNCTION_SETTING(delete_sms, delete_sms);
FUNCTION_SETTING(sms_test, sms_test);

FUNCTION_SETTING(get_apn, get_apn);
FUNCTION_SETTING(deal_tcpclose_event, deal_tcpclose_event);
FUNCTION_SETTING(deal_tcprecv_event, deal_tcprecv_event);
FUNCTION_SETTING(get_event, get_event);
FUNCTION_SETTING(linkRecv_seq, linkRecv_seq);
FUNCTION_SETTING(deal_link_event, deal_link_event);
FUNCTION_SETTING(free_event, free_event);
FUNCTION_SETTING(get_firstDiscnt_seq, get_firstDiscnt_seq);
FUNCTION_SETTING(get_firstCnt_seq, get_firstCnt_seq);
FUNCTION_SETTING(read_smscAddr, read_smscAddr);
FUNCTION_SETTING(set_smscAddr, set_smscAddr);
FUNCTION_SETTING(set_dns_ip, set_dns_ip);
FUNCTION_SETTING(tcpip_cnnt, tcpip_cnnt);
FUNCTION_SETTING(sendto_tcp, sendto_tcp);
FUNCTION_SETTING(deal_smsrecv_event, deal_smsrecv_event);
FUNCTION_SETTING(recvform_tcp, recvform_tcp);
FUNCTION_SETTING(tcp_test, tcp_test);

END_CTOR
