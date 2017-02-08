
#include "cmsis_os.h"                                           // CMSIS RTOS header file
#include "osObjects.h"                      // RTOS object definitions
#include "gprs.h"
#include "sdhError.h"
#include "dtuConfig.h"
#include "string.h"
#include "debug.h"
#include "sw_filesys.h"
#include "TTextConfProt.h"
#include "times.h"
#include "led.h"
/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/
#define DTU_BUF_LEN		256


static int get_dtuCfg(DtuCfg_t *conf);
static void dtu_conf(void);
static void ledcom_uartcb(void *rxbuf, void *arg);

void thrd_dtu (void const *argument);                             // thread function
osThreadId tid_ThrdDtu;                                          // thread id
osThreadDef (thrd_dtu, osPriorityNormal, 1, 0);                   // thread object

gprs_t *SIM800 ;
DtuCfg_t	Dtu_config;
sdhFile *DtuCfg_file;

char	DTU_Buf[DTU_BUF_LEN];
char	Recv_PhnoeNo[16];


#define TTEXTSRC_485 0
#define TTEXTSRC_SMS(n)	( n + 1)
static int TText_source = TTEXTSRC_485;	
static u485RxirqCB T485cb;

int Init_ThrdDtu (void) {
//	int retry = 20;
	SIM800 = gprs_t_new();
	SIM800->init(SIM800);
	T485cb.cb = ledcom_uartcb;
	T485cb.arg = LED_com;
	s485_uart_init( &Conf_S485Usart_default, &T485cb);
	
	//这里尝试启动SIM是为了能够获取默认的短信中心号码
	//todo  :如果只是工作在本地模式并且没有焊SIM900或者没有插入SIM卡的时候，这里就会导致一直死循环，需要处理掉这个问题
//	while(retry)
//	{
//		SIM800->startup(SIM800);
//		if( SIM800->check_simCard(SIM800) == ERR_OK)
//		{	
//			
//			break;
//		}
//		else {
//			retry --;
//			osDelay(1000);
//		}
//		
//	}
	DPRINTF(" get_dtuCfg\n");
	get_dtuCfg( &Dtu_config);
	DPRINTF(" get_dtuCfg done\n");
	clean_time2_flags();

	tid_ThrdDtu = osThreadCreate (osThread(thrd_dtu), NULL);
	if (!tid_ThrdDtu) return(-1);

	return(0);
}


static void prnt_485( char *data)
{
	if( Dtu_config.output_mode)
	{
		
		s485_Uart_write(data, strlen(data) );
	}
	
	DPRINTF(" %s \n", data);
	
}

static void ledcom_uartcb(void *rxbuf, void *arg)
{
	stm32LED	*led_com = ( stm32LED	*)arg;
	
	led_com->turnon(led_com);
}

void thrd_dtu (void const *argument) {
	short step = 0;
	short cnnt_seq = 0;
	int ret = 0;
	int lszie = 0;
	short i = 0;
	char ser_confmode = 0;
	char count = 0;
//	char *pp;
	void *gprs_event;
	int retry = 20;
	
	//刚上电的时候等待进入配置的模式
	//如果指定时间内不能进入配置模式，就退出等待，进入正常工作模式
	//一旦进入配置模式，就不会退出到工作模式。用户只能通过重启来退出配置模式
//	strcpy( DTU_Buf, " wait for signal ...");
//	s485_Uart_write(DTU_Buf, strlen(DTU_Buf) );
//	osDelay(10);
	
	set_alarmclock_s( ALARM_CHGWORKINGMODE, 3);
	while( 1)
	{
		if( Ringing(ALARM_CHGWORKINGMODE) == ERR_OK)
		{
			if( ser_confmode == 0)
				break;
			
		}
		osDelay(10);
		
		ret = s485_Uart_read( DTU_Buf, DTU_BUF_LEN);
		if( ret <=  0)
		{
			continue;
		}
			
		if( ser_confmode)
		{
			if( decodeTTCP_begin( DTU_Buf) == ERR_OK)
			{
				TText_source = TTEXTSRC_485;
				dtu_conf();
				decodeTTCP_finish();
				
			}
		}
		else
		{
			if( enter_TTCP( DTU_Buf) == ERR_OK)
			{
				get_TTCPVer( DTU_Buf);
				ser_confmode = 1;
				while( s485_Uart_write(DTU_Buf, strlen(DTU_Buf) ) != ERR_OK)
					;
				osDelay(10);
			}
			
		}
		
		
		
		
	}
	
	sprintf(DTU_Buf, "starting up gprs ...");
	prnt_485( DTU_Buf);
	while(retry)
	{
		SIM800->startup(SIM800);
		if( SIM800->check_simCard(SIM800) == ERR_OK)
		{	
			sprintf(DTU_Buf, "succeed !\n");
			break;
		}
		else {
			retry --;
			osDelay(1000);
		}
		
	}	
	
	
	//使用用户的配置来重新启动485串口
	s485_uart_init( &Dtu_config.the_485cfg, NULL);
	s485_Uart_ioctl(S485_UART_CMD_SET_RXBLOCK);
	s485_Uart_ioctl(S485UART_SET_RXWAITTIME_MS, 200);
	s485_Uart_ioctl(S485_UART_CMD_SET_TXBLOCK);
	s485_Uart_ioctl(S485UART_SET_TXWAITTIME_MS, 2000);
	
	
	
	


	sprintf(DTU_Buf, "Begain DTU thread ...");
	prnt_485( DTU_Buf);
	

	while (1) {
//		LED_run->blink( LED_run);
		switch( step)
		{
			
			case 0:
				if( SIM800->check_simCard(SIM800) == ERR_OK)
				{
					sprintf(DTU_Buf, "detected sim succeed! ...");
					prnt_485( DTU_Buf);
					step += 2;
					
				}
				else 
				{
					step ++;
					
				}
				
				if( Dtu_config.work_mode == MODE_LOCALRTU)
				{
					step = 4 ;
				
				}
				break;
			case 1:
				SIM800->startup(SIM800);
				step = 0;
				break;
			case 2:
				
				
				sprintf(DTU_Buf, "cnnnect DC :%d,%s,%d,%s ...", cnnt_seq,Dtu_config.DateCenter_ip[ cnnt_seq],\
								Dtu_config.DateCenter_port[cnnt_seq],Dtu_config.protocol[cnnt_seq] );
				prnt_485( DTU_Buf);
				ret = SIM800->tcpip_cnnt( SIM800, cnnt_seq,Dtu_config.protocol[cnnt_seq], Dtu_config.DateCenter_ip[cnnt_seq], Dtu_config.DateCenter_port[cnnt_seq]);
				
				if( ret == ERR_OK)
				{
					while(1)
					{
						ret = SIM800->sendto_tcp( SIM800, cnnt_seq, Dtu_config.registry_package, strlen(Dtu_config.registry_package) );
						if( ret == ERR_OK)
						{
							//启动心跳包的闹钟
							set_alarmclock_s( ALARM_GPRSLINK(cnnt_seq), Dtu_config.hartbeat_timespan_s);
							prnt_485("succeed !\n");
							if( Dtu_config.multiCent_mode == 0)
								step ++;
							break;
							
						}
						
						
						if( ret == ERR_UNINITIALIZED )
						{
							prnt_485("can not send data !\n");
							break;
						}
					}
					
					
				}
				else
				{
					prnt_485(" failed !\n");
				}
				cnnt_seq ++;
				if( cnnt_seq >= IPMUX_NUM)
				{
					step ++;
					cnnt_seq = 0;
				}
				break;
			case 3:
				while(1)
				{
					lszie = DTU_BUF_LEN;
					ret = SIM800->report_event( SIM800, &gprs_event, DTU_Buf, &lszie);
					if( ret != ERR_OK)
					{	
						step ++;
						break;
					}
					ret = SIM800->deal_tcprecv_event( SIM800, gprs_event, DTU_Buf,  &lszie);
					if( ret >= 0)
					{
						//接收到数据就将闹钟的起始时间设置为当前时间
						set_alarmclock_s( ALARM_GPRSLINK(ret), Dtu_config.hartbeat_timespan_s);
						s485_Uart_write( DTU_Buf, lszie);
//						prnt_485("recv: ");
//						prnt_485(DTU_Buf);
						DPRINTF("rx:[%d] %s \n", ret, DTU_Buf);
						SIM800->free_event( SIM800, gprs_event);
						continue;
					}
					
					lszie = DTU_BUF_LEN;
					memset( Recv_PhnoeNo, 0, sizeof( Recv_PhnoeNo));
					ret = SIM800->deal_smsrecv_event( SIM800, gprs_event, DTU_Buf,  &lszie, Recv_PhnoeNo);				
					if( ret > 0)
					{
						for( i = 0; i < ADMIN_PHNOE_NUM; i ++)
						{
							if( compare_phoneNO( Recv_PhnoeNo, Dtu_config.admin_Phone[i]) == 0)
							{
								TText_source = TTEXTSRC_SMS( i);
								if( decodeTTCP_begin( DTU_Buf) == ERR_OK)
								{
									dtu_conf();
									decodeTTCP_finish();
									
								}
								else if( Dtu_config.work_mode == MODE_SMS)
									s485_Uart_write(DTU_Buf, lszie);
								break;
							}
							
						}
						SIM800->delete_sms( SIM800, ret);
						SIM800->free_event( SIM800, gprs_event);
						continue;
					}
					
					SIM800->deal_tcpclose_event( SIM800, gprs_event);
					SIM800->free_event( SIM800, gprs_event);
				
				}	//while(1)
				
				break;
			case 4:
				//MODE_LOCALRTU 之下不需要执行其他步骤，只需要读取485的数据并处理即可
				ret = s485_Uart_read( DTU_Buf, DTU_BUF_LEN);
				
				
				if( ret <= 0)
				{
					if( Dtu_config.work_mode != MODE_LOCALRTU)
						step++;
					break;
				}
				
				if( Dtu_config.work_mode == MODE_LOCALRTU)
					break;

				for( i = 0; i < IPMUX_NUM; i ++)
				{
					SIM800->sendto_tcp( SIM800, i, DTU_Buf, ret);
					
				}		
				///发生了tcpip数据之后不能立即发送短信，所以延迟一会再发短信
				if(  Dtu_config.work_mode == MODE_SMS)	
				{
					osDelay(2000);
				}					
				if(  Dtu_config.work_mode == MODE_SMS)
				{
					for( i = 0; i < ADMIN_PHNOE_NUM; )
					{
						if( SIM800->send_text_sms( SIM800, Dtu_config.admin_Phone[i], DTU_Buf) == ERR_FAIL)
						{
							count ++;
							if( count > 3)	//重试3次
							{
								i ++;
								count = 0;
							}
							
						}
						else
						{
							i ++;
							count = 0;
						}
//						osDelay(2000);
					}
					
					
				}
				
				step++;
			case 5:
				for( i = 0; i < IPMUX_NUM; i ++)
				{
					if( Ringing(ALARM_GPRSLINK(i)) == ERR_OK)
					{
						set_alarmclock_s( ALARM_GPRSLINK(i), Dtu_config.hartbeat_timespan_s);
						
						SIM800->sendto_tcp( SIM800, i, Dtu_config.heatbeat_package, strlen( Dtu_config.heatbeat_package));
							
						
						
					}	
				}
				step++;
				break;
			case 6:
				if( Dtu_config.multiCent_mode == 0)
				{
					ret = SIM800->get_firstCnt_seq(SIM800);
					
					step = 0;
					if( ret >= 0)
					{
						step = 3;
					}
					else
					{
						
						strcpy(DTU_Buf, "None connnect, reconnect...");
						prnt_485( DTU_Buf);
					}
					break;
				}
				else 
				{
					
					ret = SIM800->get_firstDiscnt_seq(SIM800);
					if( ret >= 0)
					{
						sprintf(DTU_Buf, "cnnnect DC :%d,%s,%d,%s ...", ret,Dtu_config.DateCenter_ip[ ret],\
									Dtu_config.DateCenter_port[ret],Dtu_config.protocol[ret] );
						prnt_485( DTU_Buf);
						if( SIM800->tcpip_cnnt( SIM800, ret, Dtu_config.protocol[cnnt_seq], Dtu_config.DateCenter_ip[ret], Dtu_config.DateCenter_port[ret]) == ERR_OK)
						{
							prnt_485(" succeed !\n");
						
						}
						else
						{
							prnt_485(" failed !\n");
						}	
					}
					step = 3;
					
				}
			
				
				break;
			default:
				step = 0;
				break;
			  
	  }
	  
	  
	  osThreadYield ();                                           // suspend thread
	}
}


static void set_default( DtuCfg_t *conf)
{
	//默认的配置
	int i = 0;
	memset( conf, 0, sizeof( DtuCfg_t));
		 
	conf->ver[0] = DTU_CONFGILE_MAIN_VER;
	conf->ver[1] = DTU_CONFGILE_SUB_VER;
	conf->multiCent_mode = 1;
	conf->hartbeat_timespan_s = 5;
	conf->work_mode = MODE_DTU;
	conf->dtu_id = 1;
	conf->rtu_addr = 1;
	strcpy( conf->sim_NO,"13888888888");
	strcpy( conf->apn," ");
	strcpy( conf->smscAddr," ");
	sprintf( conf->registry_package,"XMYN%09d%s",conf->dtu_id, conf->sim_NO);
	conf->heatbeat_package[0] = '$';
	conf->heatbeat_package[1] = '\0';
	conf->output_mode = 0;
	conf->chn_type[0] = SIGTYPE_4_20_MA;
	conf->chn_type[1] = SIGTYPE_4_20_MA;
	conf->chn_type[2] = SIGTYPE_4_20_MA;
	memcpy( &conf->the_485cfg, &Conf_S485Usart_default, sizeof( Conf_S485Usart_default));
	
	for( i = 0; i < IPMUX_NUM; i++)
	{
		strcpy( conf->DateCenter_ip[i], DEF_IPADDR);
		strcpy( conf->protocol[i], DEF_PROTOTOCOL);
		conf->DateCenter_port[i] = DEF_PORTNUM;
		
	}
	
}

static int get_dtuCfg(DtuCfg_t *conf)
{
	
	
	DtuCfg_file	= fs_open( DTUCONF_filename);
	DPRINTF(" fs_open  %p \n", DtuCfg_file);
	if( DtuCfg_file)
	{
		//todo: 2017-02-04 21:40:21 此处耗时8s
		fs_read( DtuCfg_file, (uint8_t *)conf, sizeof( DtuCfg_t));
		DPRINTF(" fs_read  done \n");
		if( conf->ver[0] == DTU_CONFGILE_MAIN_VER &&  conf->ver[1] == DTU_CONFGILE_SUB_VER)
		{
			
			return ERR_OK;
		}
	}
	else
	{
		DtuCfg_file	= fs_creator( DTUCONF_filename, sizeof( DtuCfg_t));
		DPRINTF(" fs_creator  %p \n", DtuCfg_file);
			
	}
	
	
	
	set_default( conf);
	
	if( DtuCfg_file)
	{
		fs_lseek( DtuCfg_file, WR_SEEK_SET, 0);
		fs_write( DtuCfg_file, (uint8_t *)conf, sizeof( DtuCfg_t));
		fs_flush();
	}
	
	return ERR_OK;
	
}


void ack_str( char *str)
{
	if( TText_source == TTEXTSRC_485)
	{
		s485_Uart_write(str, strlen(str));
	}
	else
	{
		SIM800->send_text_sms( SIM800, Dtu_config.admin_Phone[TText_source - 1], str);
		
	}
}





static void dtu_conf(void)
{
	char *pcmd;
	char *parg;
	int 	i_data = 0;
	short		i = 0, j = 0;
	char		tmpbuf[8];
	char		com_Wordbits[4] = { '8', '9', 0, 0};
	char		com_Paritybit[4] = { 'N', 'E', 'O', 0};
	char		com_stopbit[4] = { '1', '2',0,0};
	if( get_cmdtype() != CONFCMD_TYPE_ATC)
		return;
	
	pcmd = get_cmd();
	if( pcmd == NULL)
			return;
	
	while(1)
	{
		parg = get_firstarg();
		if( strcmp(pcmd ,"MODE") == 0)
		{
			i_data = MODE_END;
			if( parg)
				i_data = atoi( parg);
			if( *parg == '?')
			{
				sprintf( DTU_Buf, "%d", Dtu_config.work_mode);
				ack_str( DTU_Buf);
			}
			else if( i_data < MODE_END && i_data >= MODE_BEGIN)
			{
				Dtu_config.work_mode = i_data;
				strcpy( DTU_Buf, "OK");
				ack_str( DTU_Buf);
			}
			else
			{
				strcpy( DTU_Buf, "ERROR");
				ack_str( DTU_Buf);
			}
			goto exit;
		}
		
		else if( strcmp(pcmd ,"CONN") == 0)
		{
			i_data = 2;
			if( parg)
				i_data = atoi( parg);
			
			if( *parg == '?')
			{
				sprintf( DTU_Buf, "%d", Dtu_config.multiCent_mode);
				ack_str( DTU_Buf);
			}
			else if( i_data == 0 || i_data == 1)
			{
				Dtu_config.multiCent_mode = i_data;
				strcpy( DTU_Buf, "OK");
				ack_str( DTU_Buf);
			}
			else
			{
				strcpy( DTU_Buf, "ERROR");
				ack_str( DTU_Buf);
			}
			goto exit;
		}
		
		else if( strcmp(pcmd ,"SVDM") == 0 || strcmp(pcmd ,"SVIP") == 0)
		{
			if( parg == NULL)
			{
				
				goto exit;
			}
			if( parg[0] == '?')
			{
				memset( DTU_Buf, 0, DTU_BUF_LEN);
				for( i = 0; i < IPMUX_NUM; i++)
				{
					sprintf(tmpbuf, "%d,", i);
					strcat(DTU_Buf, tmpbuf);
					strcat(DTU_Buf, Dtu_config.DateCenter_ip[ i]);
					strcat(DTU_Buf, ",");
					if( Dtu_config.DateCenter_port[i])
						sprintf(tmpbuf, "%d,", Dtu_config.DateCenter_port[i]);
					else
						sprintf(tmpbuf, " ,");
					strcat(DTU_Buf, tmpbuf);
					strcat(DTU_Buf, Dtu_config.protocol[i]);
					strcat(DTU_Buf, ",");
					
				}
				
				ack_str(DTU_Buf);
				goto exit;
			}
			//N, ip, port, protocol
			
			switch( i)
			{
				case 0:
					i_data = atoi(parg);
					if( i_data < 0 || i_data >= IPMUX_NUM)
					{
						strcpy( DTU_Buf, "ERROR");
						ack_str( DTU_Buf);
						goto exit;
					}
					
					i ++;
					break;
				case 1:
					strcpy( Dtu_config.DateCenter_ip[ i_data], parg);
					i ++;
					break;
				case 2:
					Dtu_config.DateCenter_port[ i_data] = atoi(parg);
					i++;
					break;
				case 3:
//					if( strcmp(parg ,"TCP") == 0 || strcmp(parg ,"UDP") == 0)
//					{
//						strcpy( Dtu_config.protocol[ i_data], parg);
//						
//					}
//					else
//					{
//						strcpy( Dtu_config.protocol[ i_data], " ");
//						
//					}
					strcpy( Dtu_config.protocol[ i_data], parg);
					
					strcpy( DTU_Buf, "OK");
					ack_str( DTU_Buf);
					
					goto exit;
				default:
					strcpy( DTU_Buf, "ERROR");
					ack_str( DTU_Buf);
					goto exit;
			}
				
				
		}
		else if( strcmp(pcmd ,"DNIP") == 0)
		{
			if( parg[0] == '?')
			{
				if( check_ip( Dtu_config.dns_ip) == ERR_OK)
					sprintf( DTU_Buf, "%s",Dtu_config.dns_ip);
				else
					strcpy( DTU_Buf, "  ");
				
				ack_str( DTU_Buf);
			}
			else
			{
				if( check_ip( parg) != ERR_OK)
				{
					strcpy( DTU_Buf, "ERROR");
					ack_str( DTU_Buf);
				}
				else
				{
					strcpy( Dtu_config.dns_ip, parg);
					strcpy( DTU_Buf, "OK");
					ack_str( DTU_Buf);
				}
				
			}
			goto exit;
		}
		
		else if( strcmp(pcmd ,"ATBT") == 0)
		{
				
			if( parg == NULL)
			{
				strcpy( DTU_Buf, "OK");
				ack_str( DTU_Buf);
				goto exit;
			}
			if( parg[0] == '?')
			{
				i_data = 0;								
				sprintf( DTU_Buf, "%09d,%d,%s", Dtu_config.dtu_id, Dtu_config.hartbeat_timespan_s, Dtu_config.sim_NO );
				for( i = 0; i < ADMIN_PHNOE_NUM; i ++)
				{
					
					if( check_phoneNO( Dtu_config.admin_Phone[i]) == ERR_OK)
					{
						strcat( DTU_Buf,",");
						strcat( DTU_Buf, Dtu_config.admin_Phone[i]);
						
					}
					
				}
				ack_str( DTU_Buf);
				goto exit;
				
			}
			
			switch(i)
			{
				case 0:
					i_data = atoi( parg);
					Dtu_config.dtu_id = i_data;
					memset( Dtu_config.admin_Phone[0], 0, sizeof(Dtu_config.admin_Phone));
					i++;
					break;
				case 1:
					i_data = atoi( parg);
					Dtu_config.hartbeat_timespan_s = i_data;
					i++;
					break;
				case 2:
					if( check_phoneNO(parg) != ERR_OK)
					{
						strcpy( DTU_Buf, "ERROR");
						ack_str( DTU_Buf);
						goto exit;
					}
					copy_phoneNO( Dtu_config.sim_NO, parg);
					
					
					i++;
					i_data = 0;
					break;
				case 3:
					if( check_phoneNO(parg) != ERR_OK)
					{
						strcpy( DTU_Buf, "ERROR");
						ack_str( DTU_Buf);
						goto exit;
					}
					copy_phoneNO( Dtu_config.admin_Phone[ i_data], parg);
					i_data ++;
					
					if( i_data >= ADMIN_PHNOE_NUM)
					{
						strcpy( DTU_Buf, "OK");
						ack_str( DTU_Buf);
						goto exit;
					}
					break;
				default:
					strcpy( DTU_Buf, "ERROR");
					ack_str( DTU_Buf);
					goto exit;
					
			}		//switch
			
		}
		
		else if( strcmp(pcmd ,"PRNT") == 0)
		{
			if( parg[0] == '?')
			{
				if( Dtu_config.output_mode)
				{
					ack_str( "YES");
					
				}
				else
				{
					ack_str( "NO");
				}
				goto exit;
				
			}
			
			
			if( strcmp(parg ,"YES") == 0)
			{
				Dtu_config.output_mode = 1;
				strcpy( DTU_Buf, "OK");
				ack_str( DTU_Buf);
			}
			else if( strcmp(parg ,"NO") == 0)
			{
				Dtu_config.output_mode = 0;
				ack_str( "OK");
			}
			else
			{
				strcpy( DTU_Buf, "ERROR");
				ack_str( DTU_Buf);
			}
			goto exit;
		}
		
		else if( strcmp(pcmd ,"SCOM") == 0)
		{
			if( parg == NULL)
			{
				strcpy( DTU_Buf, "OK");
				ack_str( DTU_Buf);
				goto exit;
			}
			
			if( parg[0] == '?')
			{
				
				sprintf( DTU_Buf, "%d,%c,%c,%c", Dtu_config.the_485cfg.USART_BaudRate, \
												 com_Wordbits[Dtu_config.the_485cfg.USART_WordLength/0x1000], \
												 com_Paritybit[Dtu_config.the_485cfg.USART_Parity/0x300], \
												 com_stopbit[Dtu_config.the_485cfg.USART_StopBits/0x2000]	);
	
				
				ack_str(DTU_Buf);
				
				goto exit;
				
			}
			
			switch(i)
			{
				case 0:
					i_data = atoi( parg);
					Dtu_config.the_485cfg.USART_BaudRate = i_data;
					i++;
					break;
				case 1:
					if( parg[0] == '8')
						Dtu_config.the_485cfg.USART_WordLength = USART_WordLength_8b;
					else if( parg[0] == '9')
						Dtu_config.the_485cfg.USART_WordLength = USART_WordLength_9b;
					else
					{
						strcpy( DTU_Buf, "ERROR");
						ack_str( DTU_Buf);
						goto exit;
					}
					i++;
					break;
				case 2:
					if( parg[0] == 'N')
						Dtu_config.the_485cfg.USART_Parity = USART_Parity_No;
					else if( parg[0] == 'E')
						Dtu_config.the_485cfg.USART_Parity = USART_Parity_Even;
					else if( parg[0] == 'O')
						Dtu_config.the_485cfg.USART_Parity = USART_Parity_Odd;
					else
					{
						strcpy( DTU_Buf, "ERROR");
						ack_str( DTU_Buf);
						goto exit;
					}
						
					i++;
					break;
				case 3:
					if( parg[0] == '1')
						Dtu_config.the_485cfg.USART_StopBits = USART_StopBits_1;
					else if( parg[0] == '2')
						Dtu_config.the_485cfg.USART_StopBits = USART_StopBits_2;
					else
					{
						strcpy( DTU_Buf, "ERROR");
						ack_str( DTU_Buf);
						goto exit;
					}
					i++;
					break;
				default:
					strcpy( DTU_Buf, "ERROR");
					ack_str( DTU_Buf);
					goto exit;
					
			}		//switch
			
		}
		
		else if( strcmp(pcmd ,"REGT") == 0)
		{
			if( *parg == '?')
			{
				sprintf( DTU_Buf, "%s", Dtu_config.registry_package);
				ack_str( DTU_Buf);
			}
			else
			{
				i =  strlen(parg);
				if( i > 31)
					i = 31;
						
				memcpy( Dtu_config.registry_package, parg, i );
				Dtu_config.registry_package[31] = 0;
				strcpy( DTU_Buf, "OK");
				ack_str( DTU_Buf);
			}
			
			goto exit;
		}
		
		else if( strcmp(pcmd ,"HEAT") == 0)
		{
			if( *parg == '?')
			{
				sprintf( DTU_Buf, "%s", Dtu_config.heatbeat_package);
				ack_str( DTU_Buf);
			}
			else
			{
				i =  strlen(parg);
				if( i > 31)
					i = 31;
						
				memcpy( Dtu_config.heatbeat_package, parg, i );
				Dtu_config.heatbeat_package[31] = 0;
				strcpy( DTU_Buf, "OK");
				ack_str( DTU_Buf);
			}
			
			goto exit;
		}
		
		else if( strncmp(pcmd ,"VAPN",4) == 0)
		{
			if( parg == NULL)
			{
				strcpy( DTU_Buf, "OK");
				ack_str( DTU_Buf);
				goto exit;
			}
			if( parg[0] == '?')
			{
				SIM800->get_apn( SIM800, DTU_Buf);
//				strcat( DTU_Buf, ", ");
//				strcat( DTU_Buf, Dtu_config.apn_username);
//				strcat( DTU_Buf, ", ");
//				strcat( DTU_Buf, Dtu_config.apn_passwd);
				ack_str( DTU_Buf);
				return;	
				
				
			}
			else
			{
				if( i == 0)
				{
					strcpy( Dtu_config.apn, parg);
					
				}
				else if( i <= 2)
				{
					strcat( Dtu_config.apn, ",");
					strcpy( Dtu_config.apn, parg);
				}
				i ++;
				
			}
			
			
		}
		
		else if( strcmp(pcmd ,"CNUM") == 0)
		{
			if( *parg == '?')
			{
//				SIM800->read_smscAddr( SIM800, Dtu_config.smscAddr);
				if( check_phoneNO( Dtu_config.smscAddr) == ERR_OK)
					ack_str( Dtu_config.smscAddr);
				else
					ack_str( " ");
			}
			else
			{
				if( check_phoneNO( parg) == ERR_OK)
				{
					strcpy( Dtu_config.smscAddr, parg);
					
				}
				else
				{
					memset( Dtu_config.smscAddr, 0, sizeof( Dtu_config.smscAddr));
				}
				
				strcpy( DTU_Buf, "OK");
				ack_str( DTU_Buf);
				
			}
			
			goto exit;
		}
		else if( strcmp(pcmd ,"RTUA") == 0)
		{
			if( *parg == '?')
			{
				sprintf( DTU_Buf, "%d", Dtu_config.rtu_addr);
				ack_str( DTU_Buf);
			}
			else
			{
				i_data = atoi( parg);
				if( i_data > 0)
				{
					Dtu_config.rtu_addr = i_data;
					strcpy( DTU_Buf, "OK");
					ack_str( DTU_Buf);
				}
				else
				{
					strcpy( DTU_Buf, "ERROR");
					ack_str( DTU_Buf);
				}
			}
			
			goto exit;
		}
		else if( strcmp(pcmd ,"SIGN") == 0)
		{
			
			if( *parg == '?')
			{
				sprintf( DTU_Buf, "%d,%d,%d", Dtu_config.chn_type[0],Dtu_config.chn_type[1],Dtu_config.chn_type[2] );
				ack_str( DTU_Buf);
				goto exit;
			}
			else
			{
				switch(i)
				{
					case 0:
						j = atoi( parg) - 1;
						if( j >2 || j < 0)
						{
							strcpy( DTU_Buf, "ERROR");
							ack_str( DTU_Buf);
							goto exit;
						}
						i++;
						break;
					case 1:
						i_data = atoi( parg);
						Dtu_config.chn_type[j] = i_data;
						strcpy( DTU_Buf, "OK");
						ack_str( DTU_Buf);
						goto exit;
					default:
						strcpy( DTU_Buf, "ERROR");
						ack_str( DTU_Buf);
						goto exit;
						
				}		//switch
				
				
			}
		}
		else if( strcmp(pcmd ,"FACT") == 0)
		{
			set_default(&Dtu_config);
			strcpy( DTU_Buf, "OK");
			ack_str( DTU_Buf);
			goto exit;
		}
		else if( strcmp(pcmd ,"RESET") == 0)
		{
			
			strcpy( DTU_Buf, "OK");
			ack_str( DTU_Buf);
			
			LED_run->destory(LED_run);
			fs_flush();
			SIM800->shutdown(SIM800);
			os_reboot();
			
			goto exit;
		}
		if( parg == NULL)
		{
			strcpy( DTU_Buf, "ERROR");
			ack_str( DTU_Buf);
			goto exit;
		}

		
	}

	exit:	
	fs_lseek( DtuCfg_file, WR_SEEK_SET, 0);
	fs_write( DtuCfg_file, (uint8_t *)&Dtu_config, sizeof( DtuCfg_t));
	fs_flush();
	
}
