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
#include "bufManager.h"
#include "CircularBuffer.h"

#include "times.h"
#include "system.h"

static int set_sms2TextMode(gprs_t *self);
static int serial_cmmn( char *buf, int bufsize, int delay_ms);
static int SerilTxandRx( char *buf, int bufsize, int count);
void read_event(void *buf, void *arg ,int len);
static int prepare_ip(gprs_t *self);
static int get_sms_phNO(char *databuf, char *phbuf);
static int get_seq( char **data);
static int check_apn(char *apn);
//void free_event( gprs_t *self, void *event);

//static gprs_event_t *malloc_event();
#define UART_SEND	gprs_Uart_write
#define UART_RECV	gprs_Uart_read


#define UART_RXWAIT_MS  100
#define UART_TXWAIT_MS  2000
//fromt
#define SMS_MSG_PDU		0
#define SMS_MSG_TEXT		1
#define SMS_MSG_ERR			2

#define SMS_CHRC_SET_ERR	-1
#define SMS_CHRC_SET_GSM	0xe
#define SMS_CHRC_SET_UCS2	1
#define SMS_CHRC_SET_IRA	2
#define SMS_CHRC_SET_HEX	3

#define	CMDBUF_LEN		60
#define	TCPDATA_LEN		1024

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


//RecvdataBuf	TcpRecvData;

static char Gprs_cmd_buf[CMDBUF_LEN];
static char TCP_data[TCPDATA_LEN];



osMutexDef (GprsMutex); 
osMutexId g_GprsMutex_id; 

vectorBufManager_t	g_TcpVbm;

void GprsTcpCnnectBeagin()
{
	dsys.gprs.flag_cnt = 0;
}

void GprsTcpCnnectFinish()
{
	dsys.gprs.flag_cnt = 0;
}

gprs_t *GprsGetInstance(void)
{
	static gprs_t *singleton = NULL;
	if( singleton == NULL)
	{
		singleton = gprs_t_new();
		singleton->init(singleton);
	}
	return singleton;
	
}

int Grps_SetCipmode( short mode)
{
	if( ( mode != CIPMODE_TRSP) && ( mode != CIPMODE_OPAQUE))
		return ERR_BAD_PARAMETER;
	
	
	dsys.gprs.cip_mode = mode;
	return ERR_OK;
}


int Grps_SetCipmux( short mux)
{
	if( ( mux != 0) && ( mux != 1))
		return ERR_BAD_PARAMETER;
	
	
	dsys.gprs.cip_mux = mux;
	return ERR_OK;
}

int Gprs_init(gprs_t *self)
{
	dsys.gprs.cur_state = GPRSERROR;
	gprs_uart_init();
	
	dsys.gprs.cip_mode = CIPMODE_OPAQUE;
	dsys.gprs.rx_sms_seq = 0;
	
	gprs_Uart_ioctl( GPRSUART_SET_TXWAITTIME_MS, UART_TXWAIT_MS);
	gprs_Uart_ioctl( GPRSUART_SET_RXWAITTIME_MS, UART_RXWAIT_MS);
	self->event_cbuf = malloc( sizeof( sCircularBuffer ));
	self->event_cbuf->buf = malloc( EVENT_MAX * sizeof(tElement));
	if( self->event_cbuf->buf == NULL)
	{
		DPRINTF("gprs malloc event buf failed !\n");
		return ERR_MEM_UNAVAILABLE;
	}
	self->operator = COPS_UNKOWN;
	self->event_cbuf->read = 0;
	self->event_cbuf->write = 0;
	self->event_cbuf->size = EVENT_MAX;
	g_GprsMutex_id =  osMutexCreate ( osMutex(GprsMutex));
	if ( g_GprsMutex_id == NULL) {
		DPRINTF("gprs create mutex failed !\n");
		return ERR_MEM_UNAVAILABLE;
	}
	VecBuf_Init( &g_TcpVbm, TCP_data, TCPDATA_LEN, DROP_NEWDATA);
//	TcpRecvData.buf = TCP_data;
//	TcpRecvData.buf_len = TCPDATA_LEN;
//	TcpRecvData.read = 0;
//	TcpRecvData.write = 0;
//	TcpRecvData.free_size = TCPDATA_LEN;
	
	regRxIrq_cb( read_event, (void *)self);
	Gprs_state.sms_msgFromt = SMS_CHRC_SET_ERR;
	Gprs_state.sms_chrcSet = -1;
	return ERR_OK;
}

int lock(  gprs_t *self)
{
	osMutexWait( g_GprsMutex_id, osWaitForever );
	return ERR_OK;
}
int unlock(  gprs_t *self)
{
	osStatus status;
	
	status = osMutexRelease(g_GprsMutex_id);
    if (status != osOK)  {
          // handle failure code
	}
	return ERR_OK;
}



void startup(gprs_t *self)
{
//	char *pp = NULL;
//	int		waitms ;
//	while(1)
	{
//		waitms = 5000;
		GPIO_ResetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
		osDelay(100);
		GPIO_SetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
		osDelay(2000);
		GPIO_ResetBits(Gprs_powerkey.Port, Gprs_powerkey.pin);
	//	osDelay( 2000);

		dsys.gprs.flag_sms_ready = 0;
		dsys.gprs.cur_state = STARTUP;
//		while(waitms)
//		{
//			if( dsys.gprs.cur_state == STARTUP)
//				return;
//			osDelay(1);
//			waitms --;
//		}
	}
	
	return ;
	
}

void shutdown(gprs_t *self)
{
	
	char *pp;
	int count = 0;
//	while(1)
	{
		count ++;
		strcpy( Gprs_cmd_buf, "AT+CPOWD=1\r\n" );		//正常关机
		serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,2000);
//		dsys.gprs.flag_sms_ready = 0;
		pp = strstr((const char*)Gprs_cmd_buf,"NORMAL POWER DOWN");
		if( pp)
			dsys.gprs.flag_sms_ready = 0;
		
	}
}

int	check_simCard( gprs_t *self)
{
	short step = 0;
//	static char successCount = 0;
	short	retry = RETRY_TIMES;
	char *pp = NULL;

	while(1)
	{
		if( dsys.gprs.cur_state == SHUTDOWN)
			goto errOut;
		
		
		
		switch(step)
		{
			
			case 0:
				strcpy( Gprs_cmd_buf, "ATE0\x00D\x00A" );
				SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,5);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");
				if(pp)
				{
					if( dsys.gprs.cur_state < GPRS_OPEN_FINISH)
						dsys.gprs.cur_state = GPRS_OPEN_FINISH;
					step ++;
					DPRINTF(" ATE0 succeed! \t\n");
					retry = RETRY_TIMES ;
					break;
				}
				
				osDelay(1000);
				retry --;
				if( retry == 0) {
					dsys.gprs.cur_state = GPRSERROR;
					DPRINTF(" ATE0 fail \t\n");
					goto errOut;
					
				}
				break;
				
			case 1:
				strcpy( Gprs_cmd_buf, "AT+CPIN?\x00D\x00A" );
				SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,10);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");
				if(pp)
				{
					
					step ++;
					retry = RETRY_TIMES * 2;
					DPRINTF(" AT+CPIN? succeed! \t\n");
					break;
				}
				retry --;
				osDelay(1000);
				if( retry == 0) {
					dsys.gprs.cur_state = GPRSERROR;
					DPRINTF(" AT+CPIN? fail \t\n");
					goto errOut;
				}
				break;
			case 2:
				strcpy( Gprs_cmd_buf, "AT+CREG?\x00D\x00A" );
				SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,10);
				if(((Gprs_cmd_buf[9]=='0')&&
					(Gprs_cmd_buf[11]=='1'))||
				 ((Gprs_cmd_buf[9]=='0')&&
					(Gprs_cmd_buf[11]=='5')))
				{
						DPRINTF(" check_simCard succeed !\r\n");
						if( dsys.gprs.cur_state < INIT_FINISH_OK)
							dsys.gprs.cur_state = INIT_FINISH_OK;
						
						//170720 
						retry = 6000;
						while( dsys.gprs.flag_sms_ready < 2)
						{
							osDelay(3);
							if( retry)
								retry --;
							else
								break;
						}
						if( dsys.gprs.flag_sms_ready < 2)
							goto errOut; 
						osDelay(1000);
						return ERR_OK;
				}
				osDelay(1000);
				retry --;
				if( retry == 0) {
					dsys.gprs.cur_state = GPRSERROR;
					DPRINTF(" AT+CREG? fail %s \r\n", Gprs_cmd_buf );
					goto errOut;
				}
				break;
				
			default:
				step = 0;
				break;
				
		}		//switch
//		osDelay(1000);
	}	//while(1)
	
	errOut:
//	successCount = 0;
	return ERR_FAIL; 
}

int	send_text_sms(  gprs_t *self, char *phnNmbr, char *sms){
//	uint8_t i = 0;
	
	short  sms_len = strlen( sms);
	char	step = 0;
	char	endchar = 0x1A;
	short retry = RETRY_TIMES;
	char *pp = NULL;
	if( sms_len == 0)
		return ERR_OK;
	if( dsys.gprs.flag_sms_ready < 2)
		return ERR_DEV_SICK;
	if( phnNmbr == NULL || sms == NULL)
		return ERR_BAD_PARAMETER;
	if( dsys.gprs.flag_sms_ready == 0)
		return ERR_UNINITIALIZED;
	if( dsys.gprs.cur_state < INIT_FINISH_OK)
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
				SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,100);
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
//				sms[ sms_len] = endchar;		//把字符串的结尾0替换成gprs模块的结束符0x1A
//				sms_len ++;
			
				
				if( UART_SEND( sms, sms_len) == ERR_DEV_TIMEOUT)
					osDelay(1000);
//				else
//					osDelay(100);
				UART_SEND( &endchar, 1);
//				sms[ sms_len] = 0;
				step ++;
				retry = 300 ;			///短信应答的最长延时是60s，接口的接收超时是2s。
				break;
			case 3:
				memset( Gprs_cmd_buf, 0, CMDBUF_LEN);
				UART_RECV( Gprs_cmd_buf, CMDBUF_LEN);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");
				if(pp)
				{
//					osDelay(1000);
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
					Gprs_state.sms_chrcSet = SMS_CHRC_SET_ERR;
					return ERR_FAIL;
				}
				

					
				osDelay(100);
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
	short bufLen = *len;
	
	char *pp = NULL;
	char	*ptarget = out_buf;
	
	short number = 0;
	char  text_begin = 0, text_end = 0;
			
	short legal_phno = 0;
	int	phnoend_offset = 0;
	
	if( check_phoneNO( phnNmbr) == ERR_OK)
		legal_phno = 1;
	//短信为准备好，说明还没有入网
	if( dsys.gprs.flag_sms_ready < 2)
		return ERR_UNINITIALIZED;  
	
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
			
//				+CPMS: <mem1>,<used1>,<total1>,<mem2>,<used2>,<total2>,
//						<mem3>,<used3>,<total3>
				
				if( SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,50) == 0)
				{
					//这情况下，可能是GPRS被关机了
					Gprs_state.sms_msgFromt = SMS_MSG_ERR;
					return ERR_FAIL;
				}
			
				//找到总数
				pp = strstr(Gprs_cmd_buf,",");
				if( pp)
				{
					pp ++;
					
					//没有短信就返回
					number = atoi( pp);	
					if( number == 0)
						return -1;
				}
				else
				{
					return -1;
				}
				pp = strstr(pp,",");
				if( pp)
				{
					pp ++;
				}
				else
				{
					return -1;
				}
				retry --;
				if( retry == 0)
					return ERR_FAIL;
				
//				tmp = strcspn( pp, "0123456789");			
//				pp = Gprs_cmd_buf + tmp;
//				tmp = strcspn( pp, ",");	
				number = atoi( pp);			
				if( number < 1)
					return 0;	
				if( dsys.gprs.rx_sms_seq > number)
					dsys.gprs.rx_sms_seq = 0;
				i = dsys.gprs.rx_sms_seq;		///短信从短信CMIT通知开始读取
				step ++;
				retry = RETRY_TIMES;
				break;
				
			case 2:		///一次读取短信，并从中读取发送方是指定号码的短信
				sprintf( in_buf, "AT+CMGR=%d\x00D\x00A", i);
				
				
				if( SerilTxandRx( in_buf, bufLen, 10) == 0)
				{
					Gprs_state.sms_msgFromt = SMS_MSG_ERR;
					return ERR_FAIL;
				}
			
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
							dsys.gprs.rx_sms_seq = i + 1;
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
					return i;
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
				if( SerilTxandRx( buf, *len, 50) == 0)
				{
					Gprs_state.sms_msgFromt = SMS_MSG_ERR;
					return ERR_FAIL;
				}
			
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
				if( pp || retry)
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
 
 int tcpClose( gprs_t *self, int cnntNum)
 {
	 char *pp;
	 
	 short safeCount = 5;
//	 short i = 0;
	 if( dsys.gprs.cip_mux)
	 {
		 if( Ip_cnnState.cnn_state[ cnntNum] != CNNT_ESTABLISHED)
			 return ERR_OK;
		 
		sprintf( Gprs_cmd_buf, "AT+CIPCLOSE=%d,0\x00D\x00A", cnntNum);		//quick close
		SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,100);
		 
		Ip_cnnState.cnn_state[ cnntNum] = CNNT_DISCONNECT;
	 }
	 else
	 {
		 if( Ip_cnnState.cnn_state[ 0] != CNNT_ESTABLISHED)
			 return ERR_OK;
		 //退出数据模式
		 while(1)
		 {
			 osDelay(1200);
			 sprintf( Gprs_cmd_buf, "+++");		//quick close
			 serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,700);
			 pp = strstr((const char*)Gprs_cmd_buf,"OK");
			 if( pp)
				 break;
			 if( safeCount)
				 safeCount --;
			 else
				 break;
			 
			 
			 
		 }
		sprintf( Gprs_cmd_buf, "AT+CIPCLOSE\x00D\x00A");		//quick close
		SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,100);
		Ip_cnnState.cnn_state[ 0] = CNNT_DISCONNECT;
	 }
	 return ERR_OK;
 }
 
 int tcpip_cnnt( gprs_t *self, int cnnt_num,char *prtl, char *addr, int portnum)
 {
	short step = 0;
	short	retry = RETRY_TIMES;
	int ret = 0;
	char *pp = NULL;

	//短信为准备好，说明还没有入网
	if( dsys.gprs.flag_sms_ready < 2)	//等到SMS 和CALL都可以了才联网
		return ERR_DEV_SICK; 
	if( cnnt_num > IPMUX_NUM)
		return ERR_BAD_PARAMETER;
	if( portnum <= 0)
		return ERR_BAD_PARAMETER;
	
	while(1)
	{
		switch( step)
		{
			case 0:
				ret = prepare_ip( self);
				if( ret != ERR_OK)
				{
					dsys.gprs.cur_state = TCP_IP_ERROR;
					return ERR_DEV_SICK;
				}
				step += 3;			//170720 直接连接，
				break;
			case 1:		//先关闭这个连接
				if( dsys.gprs.cip_mux)
					sprintf( Gprs_cmd_buf, "AT+CIPSTATUS=%d\x00D\x00A", cnnt_num);
				else
				{
					cnnt_num = 0;
					sprintf( Gprs_cmd_buf, "AT+CIPSTATUS\x00D\x00A");
				}
				
				//170719
				//透明模式下要先退出数据模式
//				if( SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,10) == 0)
//				{
//					if( dsys.gprs.cip_mode == CIPMODE_TRSP)
//					{
////						SystemShutdown();
////						self->tcpClose( self, 0);
//					}
//						
//					break;
//				}
				//CONNECTING or CONNECTED
				//在单路模式下，如果前面的配置的中心无法连接就会使CONNECTING状态
				pp = strstr((const char*)Gprs_cmd_buf,"CONNECT");	
				if( pp)
				{
					step ++;
					break;
				}
				pp = strstr((const char*)Gprs_cmd_buf,"ERROR");	
				if( pp)
				{
					dsys.gprs.cur_state = GPRSERROR;
					strcpy( Gprs_cmd_buf, "AT+CIPSHUT\x00D\x00A" );		//Deactivate GPRS PDP context
					serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,1);
					ret = ERR_FAIL;
					goto cntFailed;
				}
				step += 2;
				break;
			case 2:
				if( dsys.gprs.cip_mux)
					sprintf( Gprs_cmd_buf, "AT+CIPCLOSE=%d\x00D\x00A", cnnt_num);
				else
					sprintf( Gprs_cmd_buf, "AT+CIPCLOSE\x00D\x00A");					
				SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,20);
//				pp = strstr((const char*)Gprs_cmd_buf,"OK");	
//				if( pp)
//					step ++;
//				pp = strstr((const char*)Gprs_cmd_buf,"ERROR");	
//				if( pp)
					step ++;
				break;
			case 3:
				if( dsys.gprs.cip_mux)
					sprintf( Gprs_cmd_buf, "AT+CIPSTART=%d,\"%s\",\"%s\",\"%d\"\x00D\x00A", cnnt_num, prtl, addr, portnum);
				else
					sprintf( Gprs_cmd_buf, "AT+CIPSTART=\"%s\",\"%s\",\"%d\"\x00D\x00A", prtl, addr, portnum);
				DPRINTF("  %s ", Gprs_cmd_buf);

				
				
				UART_SEND( Gprs_cmd_buf, strlen( Gprs_cmd_buf));
				
				//当中心地址不正确的时候，GPRS会花花很长时间才能返回错误
				retry = 90;
				step++;
				break;
			case 4:
				memset( Gprs_cmd_buf, 0, CMDBUF_LEN);
				ret = UART_RECV( Gprs_cmd_buf, CMDBUF_LEN);
				if( ret > 0)
				{
					pp = strstr((const char*)Gprs_cmd_buf,"ERROR");	
					if( pp)
					{
						
						dsys.gprs.cur_state = TCP_IP_ERROR;
						ret = ERR_ADDR_ERROR;
						goto cntFailed;
//						return ERR_ADDR_ERROR;
					}
					pp = strstr((const char*)Gprs_cmd_buf,"CONNECT FAIL");
					if( pp)
					{	//服务端地址不正确的时候会出现这个回复，而且等待时间会很长
						ret = ERR_ADDR_ERROR;
						goto cntFailed;
//						return ERR_BAD_PARAMETER;
					}
					
					if( dsys.gprs.cip_mode)
					{
						//透明模式下的成功返回不带OK
						pp = strstr((const char*)Gprs_cmd_buf,"CONNECT");	
						if( pp)
						{
							Ip_cnnState.cnn_state[ cnnt_num] = CNNT_ESTABLISHED;
							ret = ERR_OK;
							osDelay(1000);
							goto exit;
//							osDelay(1000);
//							return ERR_OK;
						}
						
					}
					else
					{
						pp = strstr((const char*)Gprs_cmd_buf,"CONNECT OK");	
						if( pp)
						{
							Ip_cnnState.cnn_state[ cnnt_num] = CNNT_ESTABLISHED;
							ret = ERR_OK;
							osDelay(1000);
							goto exit;
						}
						
						
					}
					
					//服务器关闭连接
					//非正常关机的时候也会出现这个问题
					pp = strstr((const char*)Gprs_cmd_buf,"CLOSED");
					if( pp)
					{	
						
						ret = ERR_BAD_PARAMETER;
						goto cntFailed;
//						return ERR_BAD_PARAMETER;
					}
					
				
				}
				retry --;
				if( retry == 0)
				{
					if( dsys.gprs.cip_mux)
						sprintf( Gprs_cmd_buf, "AT+CIPCLOSE=%d\x00D\x00A", cnnt_num);
					else
						sprintf( Gprs_cmd_buf, "AT+CIPCLOSE\x00D\x00A");
					
//					sprintf( Gprs_cmd_buf, "AT+CIPCLOSE\x00D\x00A");		//quick close
					SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,20);
//					self->tcpClose( self, cnnt_num);
					ret = ERR_DEV_TIMEOUT;
					goto cntFailed;
//					return ERR_DEV_TIMEOUT;
				}
				
				break;
			default:
				break;
		}
		osDelay(1000);		//sundh 170720
	
		
	}	//while
	//sundh 170720
cntFailed:
	
	if( ret == ERR_DEV_TIMEOUT)
	{
		if( dsys.gprs.cip_mode)
		{
			
				SystemShutdown();
			
		}
	}
exit:
		return ret;
		
 }


#define TCPSENDBUF_LEN     256
#define SENDBUF_NUM		2
static short sendDataIdx[SENDBUF_NUM];
//static char sending_cnnt_num = -1;
static short sendBuf = -1;
char  sendTcpdata[SENDBUF_NUM][TCPSENDBUF_LEN];


static void SendBufData(void)
{
	gprs_t	*this_gprs = GprsGetInstance();
	char i = 0, j = 0;
	char timeout = 0;
	char needclean = 0;		//缓存数据太多的时候就清除

	

	this_gprs->lock( this_gprs);
	if( Ringing( ALARM_SENDTCPBUF) == ERR_OK)
		timeout = 1;

	for( i = 0; i < SENDBUF_NUM; i++)
	{
		if( TCPSENDBUF_LEN - sendDataIdx[ i] < 50)
			needclean = 1;
		if( ( timeout + needclean) == 0)
			continue;
		sendBuf = i;
		for( j = 0; j < IPMUX_NUM; j ++)
			this_gprs->sendto_tcp( this_gprs, j, sendTcpdata[ i], sendDataIdx[ i]);
		memset( sendTcpdata[ i], 0 , TCPSENDBUF_LEN);
		needclean = 0;
		sendDataIdx[ i]= 0;
	}
	sendBuf = -1;
	this_gprs->unlock( this_gprs);

	
}
 


void GprsRun( gprs_t *self)
{

	
	{
		SendBufData();
	}	
	
}	
int sendto_tcp_buf( gprs_t *self, char *data, int len)
{
	int wrbuf = 0;
	
	if( len == 0)
		return ERR_OK;

	
	if( dsys.gprs.cip_mode == CIPMODE_TRSP)
	{
		if( UART_SEND( data, len) == ERR_DEV_TIMEOUT)
			osDelay(1000);
		else
			osDelay(5);
		return ERR_OK;
		
	}
//	for( i = 0; i < SENDBUF_NUM; i ++)
//	{
//		if( ( len + sendDataIdx[ 0] ) <  TCPSENDBUF_LEN)
//			continue;
//		memcpy( sendTcpdata[ i] + sendDataIdx[ i], data, len);
//		sendDataIdx[ i] += len;
//		set_alarmclock_ms( ALARM_SENDTCPBUF, 100);
//		return ERR_OK;	
//	}
	
	if( sendBuf == 0)
		wrbuf = 1;

	if( ( len + sendDataIdx[ wrbuf]) >  TCPSENDBUF_LEN)
		return ERR_MEM_UNAVAILABLE;
		
	memcpy( sendTcpdata[ wrbuf]+ sendDataIdx[ wrbuf], data, len);
	sendDataIdx[ wrbuf] += len;
	set_alarmclock_ms( ALARM_SENDTCPBUF, 500);
	return ERR_OK;	
}

 
int sendto_tcp( gprs_t *self, int cnnt_num, char *data, int len)
{
	char *pp;
	int 	ret = 0;
	int retry = 10;
	
	if( dsys.gprs.cip_mode == CIPMODE_TRSP)
		cnnt_num = 0;
	if( len == 0)
		return ERR_OK;
	if( cnnt_num > IPMUX_NUM)
		return ERR_BAD_PARAMETER;
	if( Ip_cnnState.cnn_state[ cnnt_num] != CNNT_ESTABLISHED)
		return ERR_UNINITIALIZED;
	
	if( dsys.gprs.cip_mode == CIPMODE_TRSP)
	{
		if( UART_SEND( data, len) == ERR_DEV_TIMEOUT)
			osDelay(1000);
//		else
//			osDelay(1);
		return ERR_OK;
		
	}



	if( dsys.gprs.cip_mux)
		sprintf( Gprs_cmd_buf, "AT+CIPSEND=%d,%d\x00D\x00A", cnnt_num, len);
	else
		sprintf( Gprs_cmd_buf, "AT+CIPSEND=%d\x00D\x00A", len);
	ret = UART_SEND( Gprs_cmd_buf, strlen( Gprs_cmd_buf));
	if( ret == ERR_DEV_TIMEOUT)
		osDelay(100);
	else
		osDelay(10);
	ret = UART_SEND( data, len);
	if( ret == ERR_DEV_TIMEOUT)
		osDelay(1000);
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
		//花生壳调试时，本地未开启服务器时，会出现连接后马上断开的情况
		pp = strstr((const char*)Gprs_cmd_buf,"CLOS");		
		if( pp)
		{
			Ip_cnnState.cnn_state[ cnnt_num] = CNNT_DISCONNECT;
			return ERR_UNINITIALIZED;
		}
		retry --;
		
		osDelay(20);
		if( retry == 0)
		{
//			Ip_cnnState.cnn_state[ cnnt_num] = CNNT_DISCONNECT;
//			return ERR_UNINITIALIZED;
			return ERR_OK;

		}
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
int Gprs_Event_smsRecv( gprs_t *self, char *buf, int *lsize, char *phno)
{
	
	int i;
	int ret = 0;
	for(i = 0; i < MAX_NUM_SMS; i++)
	{
		if(check_bit((uint8_t *)dsys.gprs.set_sms_recv, i))
		{
			ret = self->read_seq_TextSMS( self, phno, i, buf, lsize);
			if( ret == ERR_OK)
				return i;
			clear_bit((uint8_t *)dsys.gprs.set_sms_recv, i);
			
		}
		
	}
	
//	gprs_event_t *this_event = ( gprs_event_t *)event;
//	int ret = 0;
//	if( CKECK_EVENT( this_event, sms_urc) )
//	{
//		ret = self->read_seq_TextSMS( self, phno, this_event->arg, buf, lsize);
//		if( ret == ERR_OK)
//			return this_event->arg;
//		
//	}
	return ERR_FAIL;
}

	//+RECEIVE,0,6:\0D\0A
	//123456
int Gprs_Event_tcpRecv( gprs_t *self, char *buf, int *len)
{
	int i;
	for(i = 0; i < IPMUX_NUM; i++)
	{
		if(CHK_U8_BIT(dsys.gprs.set_tcp_recv, i))
		{
			*len = VecBuf_read( &g_TcpVbm, buf, *len);
			dsys.gprs.set_tcp_recv = CLR_U8_BIT(dsys.gprs.set_tcp_recv, i);
			return i;
		}
		
	}

//	gprs_event_t *this_event = (gprs_event_t *)event;
//	*len = VecBuf_read( &g_TcpVbm, buf, *len);
//	if( CKECK_EVENT( this_event, tcp_receive) )
//	{
//		return this_event->arg;	
//	}
	
	return ERR_FAIL;
}
int Gprs_Event_tcpclose( gprs_t *self)
{
	int i;
	for(i = 0; i < IPMUX_NUM; i++)
	{
		if(CHK_U8_BIT(dsys.gprs.set_tcp_close, i))
		{
			dsys.gprs.set_tcp_close = CLR_U8_BIT(dsys.gprs.set_tcp_close, i);
			Ip_cnnState.cnn_state[i] = CNNT_DISCONNECT;
			return i;
		}
		
	}
	
//	gprs_event_t *this_event = ( gprs_event_t *)event;
//	if( CKECK_EVENT( this_event, tcp_close) )
//	{
////		self->tcpClose( self, this_event->arg);
//		Ip_cnnState.cnn_state[ this_event->arg] = CNNT_DISCONNECT;
//		
//		return this_event->arg;
//	}
	
	return ERR_FAIL;
}



static int get_seq( char **data)
{
	//+CMTI:"SM",1
	int tmp;
	char *buf = *data;
	tmp = strcspn( buf, "0123456789");	
	*data += tmp;
	return  atoi( buf + tmp);
}

void read_event(void *buf, void *arg, int len)
{
	char *pp;
	gprs_t *cthis;
	int tmp = 0;		
//	gprs_event_t	*event;
	if( arg == NULL)
		return ;
	if(dsys.gprs.flag_cnt)
		return;
	
	cthis = ( gprs_t *)arg;
	
	
	
	
	pp = strstr((const char*)buf,"SMS Ready");
	if( pp)
	{
		dsys.gprs.flag_sms_ready ++;
		return;
		
	}
	
	pp = strstr((const char*)buf,"Call Ready");
	if( pp)
	{
		
		dsys.gprs.flag_sms_ready ++;
		return;
	}	
	
	
	pp = strstr((const char*)buf,"NORMAL POWER DOWN");
	if( pp)
	{
		dsys.gprs.cur_state = SHUTDOWN;
		return;
		
	}
	
	
	//由tcp close指令引起的关闭不作为事件处理
	pp = strstr((const char*)buf,"STATE: TCP CLOSED");
	if( pp)
		return;
	//0,CLOSED
	if( dsys.gprs.cip_mode == CIPMODE_TRSP)
	{
		//透传模式下，如果连接是已经建立的情况下是收不到通知的
		//所以如果在透传模式下收到了电话通知
		//说明连接已经断开了
		pp = strstr((const char*)buf,"RING");
		if( pp)
		{
			
			SystemShutdown();
			
			
			
		}
		pp = strstr((const char*)buf,"CLOSED");
		
		//170717 发现断网之后，会报ERROR，所以对ERROR也要检查
	//不过要在已经建立了连接之后才能这么处理，否则会影响正常的连接过程
	
//	if( ( pp == NULL) && ( Ip_cnnState.cnn_state[ 0] == CNNT_ESTABLISHED) )
//	{
//		pp = strstr((const char*)buf,"\r\nERROR\r\n");
//		if( pp)
//		{	//170718 如果出现了这种情况，重新连接已经不起作用了，得重启才能连上
//			
//			errcount ++;
//			if( errcount > 2)
//			{
//				errcount = 0;
//				SystemShutdown();
//			}
////			return;
//		}
//		
//	}
		
	}
	else
	{
		pp = strstr((const char*)buf,",CLOSED");
		
	}
	
	
	
	if( pp)
	{
		tmp = get_seq(&pp);
		if(tmp > IPMUX_NUM)
			return;
		dsys.gprs.set_tcp_close = SET_U8_BIT(dsys.gprs.set_tcp_close, tmp);
//		event = malloc_event();
//		if( event)
//		{
//			event->type = tcp_close;
//			pp = ( char*)buf;
//			if( dsys.gprs.cip_mode == CIPMODE_TRSP)
//				event->arg = 0;
//			else
//				event->arg = get_seq(&pp);
//			if( CBWrite( cthis->event_cbuf, event) != ERR_OK)
//			{
//				
//				goto CB_WRFAIL;
//				
//			}
//			
//		}
		return;
	}
	
	//+RECEIVE,0,6:\0D\0A
	//123456
	pp = strstr((const char*)buf,"RECEIVE");
	if( pp)
	{
		
		tmp = get_seq(&pp);
		if(tmp > IPMUX_NUM)
			return;
		dsys.gprs.set_tcp_recv = SET_U8_BIT(dsys.gprs.set_tcp_recv, tmp);
		pp = strstr(pp,",");
		tmp = get_seq(&pp); 
		while((*pp != '\0') && (*pp != '\x00A'))
			pp++;
		if(pp[1] == '\0')
			return;
		VecBuf_write(&g_TcpVbm,  pp + 1, tmp);
		
//		event = malloc_event();
//		if( event)
//		{
//			
//			event->type = tcp_receive;
//			event->arg = get_seq(&pp);
//			pp = strstr(pp,",");
//			tmp = get_seq(&pp); 
//			while( *pp != '\x00A')
//				pp++;
//			VecBuf_write( &g_TcpVbm,  pp + 1, tmp);
//			if( CBWrite( cthis->event_cbuf, event) != ERR_OK)
//			{
//				
//				goto CB_WRFAIL;
//				
//			}
//		}
//		
		return;

	}
	pp = strstr((const char*)buf,"CMTI");
	if( pp)
	{
		tmp = get_seq(&pp);
		if(tmp > MAX_NUM_SMS)
			return;
		set_bit((uint8_t *)dsys.gprs.set_sms_recv, tmp);
//		event = malloc_event();
//		if( event)
//		{
//			event->type = sms_urc;
//			event->arg = get_seq(&pp);
//			if( CBWrite( cthis->event_cbuf, event) != ERR_OK)
//			{
//				
//				goto CB_WRFAIL;
//				
//			}
//		}
//		
		return;

		
	}
	
		
	
	if( dsys.gprs.cip_mode == CIPMODE_TRSP)
	{
		if( cthis->get_firstCnt_seq( cthis) < 0)
			return;
		
		tmp = len; 
		VecBuf_write( &g_TcpVbm,  buf, tmp);
		dsys.gprs.set_tcp_recv = SET_U8_BIT(dsys.gprs.set_tcp_recv, 0);
		
//		event = malloc_event();
//		if(event)
//		{
//			
//			event->type = tcp_receive;
//			event->arg = 0;

//			tmp = len; 

//			VecBuf_write( &g_TcpVbm,  buf, tmp);
//			if( CBWrite( cthis->event_cbuf, event) != ERR_OK)
//			{
//				
//				goto CB_WRFAIL;
//				
//			}
//		}
	}
	
	return ;
	
//	CB_WRFAIL:
//		
//		free_event( NULL, event);
	
}

int report_event( gprs_t *self, char *buf, int *lsize)
{
	short	i,j;
	gprs_Uart_ioctl( GPRS_UART_CMD_CLR_RXBLOCK);
	UART_RECV( buf, *lsize);
	gprs_Uart_ioctl( GPRS_UART_CMD_SET_RXBLOCK);
	
	//从各种集合中轮询是否有事件未处理
	if(dsys.gprs.set_tcp_close)
		return 0;
	if(dsys.gprs.set_tcp_recv)
		return 0;
	j = MAX_NUM_SMS / 32;
	for(i = 0; i < j; i++)
	{
		if(dsys.gprs.set_sms_recv[i])
			return 0;
	}
//	return CBRead( self->event_cbuf, event);
	return -1;

}

#define EVENT_NUM   8
//static gprs_event_t *malloc_event()
//{
//	static gprs_event_t *p_event = NULL;
//	int i = 0;
//	
//	if( p_event == NULL)
//	{
//		p_event = malloc( EVENT_NUM * sizeof( gprs_event_t));
//		memset( p_event, 0, EVENT_NUM * sizeof( gprs_event_t));
//		
//	}
//	
//	for( i = 0; i < EVENT_NUM; i ++)
//	{
//		if( p_event[i].used == 0)
//		{
//			p_event[i].used = 1;
//			return p_event + i;
//			
//		}
//		
//	}
//	
//	return NULL;
//	
//	
//		
//		
//	
//}
//void free_event( gprs_t *self, void *event)
//{
//	gprs_event_t *this_event = (gprs_event_t *)event;
////	free(event);
//	this_event->used = 0;
//}

//返回第一个未处于连接状态的序号
int	get_firstDiscnt_seq( gprs_t *self)
{
	static char i = 0; 
	int count = IPMUX_NUM;
	while( count)
	{
		i ++;
		i %= IPMUX_NUM;
		count --;
		if( Ip_cnnState.cnn_state[ i] != CNNT_ESTABLISHED)
			return i;
		
		
	}
//	for(  ; i < IPMUX_NUM; i ++)
//	{
//		if( Ip_cnnState.cnn_state[ i] != CNNT_ESTABLISHED)
//			return i;
//		
//	}
	
	return ERR_FAIL;
	
}
int	get_firstCnt_seq( gprs_t *self)
{
	int i = 0; 
//	char *pp;
	if( dsys.gprs.cip_mode == CIPMODE_TRSP)
	{
//		UART_RECV( Gprs_cmd_buf, CMDBUF_LEN);
//		
//		pp = strstr((const char*)Gprs_cmd_buf,"CLOSED");

//		
//			
//		if( pp)
//			return ERR_FAIL;
		
		if( Ip_cnnState.cnn_state[ 0] == CNNT_ESTABLISHED)
			return 0;

		
	}
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
	if( !check_apn( Dtu_config.apn))
		strcpy(buf, "CMNET, ,");
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
				SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,100);
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
				SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,1000);
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

int buf_test( gprs_t *self, char *buf, int len)
{
	int  i  = 0 ;
//	int	 tcp_buf_len = TCPDATA_LEN;
	int ret = 0;


	//按照全路径法写测试用例
	
	//1 测试写数据长度为0时的操作
	VecBuf_write( &g_TcpVbm, buf, 0);
	
	//2 测试无数据时的读取
	ret = VecBuf_read( &g_TcpVbm, buf, len);
	
	//3 测试写入数据大于缓存的最大限度时的行为
	memset( buf, 1, TCPDATA_LEN );
	VecBuf_write( &g_TcpVbm, buf, TCPDATA_LEN);
	
	//4 测试写入数据长度正好是缓存最大容量时的行为
	memset( buf, 2, TCPDATA_LEN );
	VecBuf_write( &g_TcpVbm, buf, TCPDATA_LEN - VBM_FRAMEHEAD_LEN);
	
	//5 测试缓存数据最大时，读取缓存的行为
	memset( buf, 0, len );
	ret = VecBuf_read( &g_TcpVbm, buf, len);
	
	//6 测试普通长度数据写入功能
	memset( buf, 3, TCPDATA_LEN );
	VecBuf_write( &g_TcpVbm, buf, TCPDATA_LEN/2 );
	
	//7 测试读取数据的缓存不足时，读取操作
	memset( buf, 0, len );
	ret = VecBuf_read( &g_TcpVbm, buf, TCPDATA_LEN/3);
	
	//8 测试普通数据写入后， 正常的读取行为
	memset( buf, 4, TCPDATA_LEN );
	VecBuf_write( &g_TcpVbm, buf, TCPDATA_LEN/2 );
	memset( buf, 0, len );
	ret = VecBuf_read( &g_TcpVbm, buf, TCPDATA_LEN);
	
	//9 写两帧数据，都一次读取时读取缓存不足，测试第二次缓存充足时能否正常读取
	memset( buf, 5, TCPDATA_LEN );
	VecBuf_write( &g_TcpVbm, buf, TCPDATA_LEN/3 );
	memset( buf, 6, TCPDATA_LEN );
	VecBuf_write( &g_TcpVbm, buf, TCPDATA_LEN/3 );
	memset( buf, 0, len );
	ret = VecBuf_read( &g_TcpVbm, buf, TCPDATA_LEN/4);
	memset( buf, 0, len );
	ret = VecBuf_read( &g_TcpVbm, buf, TCPDATA_LEN);
	
	
	//10 测试第二次写数据时缓存不足的行为
	for( i = 1; i < 3; i ++)
	{
		memset( buf, i, TCPDATA_LEN / 2);
		VecBuf_write( &g_TcpVbm, buf, TCPDATA_LEN / 2);
	}
	for( i = 0; i < 2; i ++)
	{
		memset( buf, 0, len);
		ret = VecBuf_read( &g_TcpVbm, buf, len);
	}
	
	//11 测试写字节数不对齐时的写
	
	for( i = 1; i < 15; i += 2)
	{
		memset( buf, i, TCPDATA_LEN );
		VecBuf_write( &g_TcpVbm, buf, i );
	}
	
	//12 测试在写入不对齐长度数据后，写入对齐长度数据
	memset( buf, 12, TCPDATA_LEN );
	VecBuf_write( &g_TcpVbm, buf, 8 );
	
	//13 把数据读取看下是否都正确
	ret = 1;
	while( ret)
	{
		
		ret = VecBuf_read( &g_TcpVbm, buf, len);
	}
	
	return 0;
	
	
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
//				if( ret == tcp_receive)
				{
//					ret = self->deal_tcprecv_event( self, buf,  buf, &len);
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
//				if( ret == tcp_close)
//				{
////					ret = self->deal_tcpclose_event( self, buf, len);
////					if( ret >= 0)
////						self->tcpip_cnnt( self, ret, "TCP", tets_addr, portnum);
//				}
				break;
			default:break;
			
			
		}		//switch
		
		
	}		//while(1)
	
	
	
}

static int SerilTxandRx( char *buf, int bufsize, int count)
{
	int ret = 0;
	
	//先把之前未读取的数据清除掉
	
//	gprs_Uart_ioctl( GPRSUART_SET_RXWAITTIME_MS, 10);
	UART_RECV( (char *)&ret, 4);
//	gprs_Uart_ioctl( GPRSUART_SET_RXWAITTIME_MS, UART_RXWAIT_MS);
	
	UART_SEND( buf, strlen(buf) + 1);
	
	while( count)
	{
		memset( buf, 0, bufsize);
		ret = UART_RECV( buf, bufsize);
		if( ret > 0)
			return ret;
		count --;
			
	}
	
	return 0;

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
				SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,10);
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
	char c = 0;
	while( *apn != '\0' && apn != NULL)
	{
		c = *apn++;
		if( c != ' ' && c != ',')
			return 1;
	}
	
	return 0;
	
}

static int prepare_ip(gprs_t *self)
{
	short retry = RETRY_TIMES;
	short step = 0;
	char *pp = NULL;
	
	if( dsys.gprs.cur_state == TCP_IP_OK)
		return ERR_OK;
	
	while(1)
	{
		switch( step )
		{
			case 0:		//确定运营商
				if( self->operator != COPS_UNKOWN)
				{
					retry = RETRY_TIMES;
					step ++;
					break;
					
				}
				
				strcpy( Gprs_cmd_buf, "AT+COPS?\x00D\x00A" );		
				SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,10);
//				pp = strstr((const char*)Gprs_cmd_buf,"OK");
//				if( !pp)
//				{
//					retry --;
//					if( retry == 0)
//						goto ipExitFail;;
//					osDelay(100);
//					break;
//				}
				
				pp = strstr((const char*)Gprs_cmd_buf,"MOBILE");
				if(pp)
				{
					self->operator = COPS_CHINA_MOBILE;
					retry = RETRY_TIMES;
					step ++;
					break;
					
				}
				pp = strstr((const char*)Gprs_cmd_buf,"UNICOM");
				if(pp)
				{
					self->operator = COPS_CHINA_UNICOM;
					retry = RETRY_TIMES;
					step ++;
					break;
					
				}
				if( retry)
				{
					
					retry --;
					osDelay(100);
				}
				else
				{
					goto ipExitFail;		
//					self->operator = COPS_CHINA_MOBILE;
//					retry = RETRY_TIMES;
//					step ++;
//					break;					
				}
				
				
				break;
				
				
			case 1:
				sprintf( Gprs_cmd_buf, "AT+CIPMUX=%d\x00D\x00A", dsys.gprs.cip_mux);	
//				strcpy( Gprs_cmd_buf, "AT+CIPMUX=1\x00D\x00A");		//设置连接为多连接
				SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,100);
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
					goto ipExitFail;
					
				}
				if( retry)
				{
					
					retry --;
					osDelay(1000);
				}
				else
				{
					goto ipExitFail;
					
					
				}
				break;
				
			
			case 2:
				if( dsys.gprs.cip_mode == CIPMODE_OPAQUE)
				{
					step ++;
					
				}
				else
				{
						
					strcpy( Gprs_cmd_buf, "AT+CIPMODE=1\x00D\x00A" );		//设置连接为透明模式
					SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,100);
					pp = strstr((const char*)Gprs_cmd_buf,"OK");
					if(pp)
					{
						retry = RETRY_TIMES;
						step ++;
						break;
					}
					pp = strstr((const char*)Gprs_cmd_buf,"ERROR");
					if(pp)
					{
						goto ipExitFail;
						
					}
					if( retry)
					{
						
						retry --;
						osDelay(100);
					}
					else
					{
						goto ipExitFail;
						
						
					}
					break;
				}
				
			case 3:
				if( dsys.gprs.cip_mode == CIPMODE_OPAQUE)
				{
					step ++;
					
				}
				else
				{
						
					strcpy( Gprs_cmd_buf, "AT+CIPCCFG=3,2,1024,1\x00D\x00A" );		
					SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,100);
					pp = strstr((const char*)Gprs_cmd_buf,"OK");
					if(pp)
					{
						retry = RETRY_TIMES;
						step ++;
						break;
					}
					pp = strstr((const char*)Gprs_cmd_buf,"ERROR");
					if(pp)
					{
						strcpy( Gprs_cmd_buf, "AT+CIPSHUT\x00D\x00A" );		//Deactivate GPRS PDP context
						SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,1);
						return ERR_FAIL;
						
					}
					if( retry)
					{
						
						retry --;
						osDelay(100);
					}
					else
					{
						goto ipExitFail;
						
						
					}
						
					break;
				}
				
			case 4:
				if( !check_apn( Dtu_config.apn))
				{
					if( self->operator == COPS_CHINA_UNICOM)
						strcpy( Gprs_cmd_buf, "AT+CSTT=\"UNINET\"\x00D\x00A" );		//设置默认gprs接入点
					else
						strcpy( Gprs_cmd_buf, "AT+CSTT=\"CMNET\"\x00D\x00A" );		//设置默认gprs接入点
				}
				else
				{
					sprintf( Gprs_cmd_buf, "AT+CSTT=%s\x00D\x00A", Dtu_config.apn );
					
				}
					
				SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,10);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");
				if(pp)
				{
					retry = RETRY_TIMES;
					step ++;
					break;
				}
				pp = strstr((const char*)Gprs_cmd_buf,"ERROR");
				if(pp)
				{
					goto ipExitFail;
					
				}
				
				if( retry)
				{
					
					retry --;
					osDelay(100);
				}
				else
				{
					goto ipExitFail;
					
					
				}
						
				break;
			case 5:
				strcpy( Gprs_cmd_buf, "AT+CIICR\x00D\x00A" );		//激活移动场景
				SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,10);
				pp = strstr((const char*)Gprs_cmd_buf,"OK");
				if(pp)
				{
					retry = RETRY_TIMES;
					step ++;
					break;
				}
				pp = strstr((const char*)Gprs_cmd_buf,"ERROR");
				if(pp)
				{
					goto ipExitFail;
				}
				
				if( retry)
				{
					
					retry --;
					osDelay(1500);
				}
				else
				{
					goto ipExitFail;
					
					
				}

				break;
			case 6:
				strcpy( Gprs_cmd_buf, "AT+CIFSR\x00D\x00A" );			//获取ip地址
				SerilTxandRx( Gprs_cmd_buf, CMDBUF_LEN,100);
				pp = strstr((const char*)Gprs_cmd_buf,".");
				if(pp)
				{
					dsys.gprs.cur_state = TCP_IP_OK;
					return ERR_OK;
				}
				
				if( retry)
				{
					
					retry --;
					osDelay(1000);
				}
				else
				{
					goto ipExitFail;
					
					
				}

				break;
			case 7:
				
					
				self->set_dns_ip( self, Dtu_config.dns_ip);
				return ERR_OK;
				
			default:
				retry = RETRY_TIMES;
				step = 0;
				break;
		}		//switch
		osDelay(1000);
	}		//while

	ipExitFail:
		self->shutdown( self);			//170720
//		strcpy( Gprs_cmd_buf, "AT+CIPSHUT\x00D\x00A" );		//Deactivate GPRS PDP context
//		serial_cmmn( Gprs_cmd_buf, CMDBUF_LEN,3);
		return ERR_FAIL;
}

//接收的短信的格式是：
//+CMGR: "REC UNREAD","+8613918186089", "","02/01/30,20:40:31+00",This is a test
//
//OK
//返回值是号码结束的地方
static int get_sms_phNO(char *databuf, char *phbuf)
{
	
	char *pp;
	short tmp = 0;
	short count = 0;
	
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
		count ++;
	}
	if( count < 4)
		return -1;
	return tmp;
}


CTOR(gprs_t)
FUNCTION_SETTING(init, Gprs_init);
FUNCTION_SETTING(lock, lock);
FUNCTION_SETTING(unlock, unlock);
FUNCTION_SETTING(startup, startup);
FUNCTION_SETTING(shutdown, shutdown);
FUNCTION_SETTING(check_simCard, check_simCard);
FUNCTION_SETTING(send_text_sms, send_text_sms);
FUNCTION_SETTING(read_phnNmbr_TextSMS, read_phnNmbr_TextSMS);
FUNCTION_SETTING(read_seq_TextSMS, read_seq_TextSMS);

FUNCTION_SETTING(delete_sms, delete_sms);

FUNCTION_SETTING(buf_test, buf_test);
FUNCTION_SETTING(sms_test, sms_test);
FUNCTION_SETTING(sms_test, sms_test);

FUNCTION_SETTING(get_apn, get_apn);

FUNCTION_SETTING(report_event, report_event);
FUNCTION_SETTING(deal_tcpclose_event, Gprs_Event_tcpclose);
FUNCTION_SETTING(deal_tcprecv_event, Gprs_Event_tcpRecv);
FUNCTION_SETTING(deal_smsrecv_event, Gprs_Event_smsRecv);
//FUNCTION_SETTING(free_event, free_event);


FUNCTION_SETTING(get_firstDiscnt_seq, get_firstDiscnt_seq);
FUNCTION_SETTING(get_firstCnt_seq, get_firstCnt_seq);
FUNCTION_SETTING(read_smscAddr, read_smscAddr);
FUNCTION_SETTING(set_smscAddr, set_smscAddr);
FUNCTION_SETTING(set_dns_ip, set_dns_ip);
FUNCTION_SETTING(tcpClose, tcpClose);
FUNCTION_SETTING(tcpip_cnnt, tcpip_cnnt);
FUNCTION_SETTING(sendto_tcp, sendto_tcp);
FUNCTION_SETTING(sendto_tcp_buf, sendto_tcp_buf);

FUNCTION_SETTING(recvform_tcp, recvform_tcp);
FUNCTION_SETTING(tcp_test, tcp_test);
FUNCTION_SETTING(run, GprsRun);

END_CTOR
