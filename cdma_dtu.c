
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

/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/
#define DTU_BUF_LEN		512
#define HEARTBEAT_ALARMID		0
#define SER485_WORKINGMODE_ALARMID		1			//485从默认模式转换到工作模式的时间
 


void thrd_dtu (void const *argument);                             // thread function
osThreadId tid_ThrdDtu;                                          // thread id
osThreadDef (thrd_dtu, osPriorityNormal, 1, 0);                   // thread object

static uint32_t Heatbeat_count = 0;

static int get_dtuCfg(DtuCfg_t *conf);
static void dtu_conf(void);
gprs_t *SIM800 ;
DtuCfg_t	Dtu_config;
sdhFile *DtuCfg_file;

char	DTU_Buf[DTU_BUF_LEN];
char	Recv_PhnoeNo[16];
int Init_ThrdDtu (void) {
	SIM800 = gprs_t_new();
	SIM800->init(SIM800);
	s485_uart_init( &Conf_S485Usart_default);
	get_dtuCfg( &Dtu_config);
	clean_time2_flags();

	tid_ThrdDtu = osThreadCreate (osThread(thrd_dtu), NULL);
	if (!tid_ThrdDtu) return(-1);

	return(0);
}

void thrd_dtu (void const *argument) {
	short step = 0;
	short cnnt_seq = 0;

	int ret = 0;
	int lszie = 0;
	short i = 0;
	short ser_confmode = 0;
	char *pp;
	set_alarmclock_s( SER485_WORKINGMODE_ALARMID, 3);
	
	//刚上电的时候等待进入配置的模式
	//如果指定时间内不能进入配置模式，就退出等待，进入正常工作模式
	//一旦进入配置模式，就不会退出到工作模式。用户只能通过重启来退出配置模式
	while( 1)
	{
		if( Ringing_s(SER485_WORKINGMODE_ALARMID) == ERR_OK)
		{
			if( ser_confmode == 0)
				break;
			
		}
		
		ret = s485_Uart_read( DTU_Buf, DTU_BUF_LEN);
		if( ret <=  0)
		{
			continue;
		}
			
		if( ser_confmode == 0)
		{
			if( enter_TTCP( DTU_Buf) == ERR_OK)
			{
				get_TTCPVer( DTU_Buf);
				ser_confmode = 1;
				s485_Uart_write(DTU_Buf, strlen(DTU_Buf) );
			}
			
		}
		else
		{
			if( decodeTTCP_begin( DTU_Buf) == ERR_OK)
			{
				dtu_conf();
				decodeTTCP_finish();
				
			}
			
		}
		
		
		
		
	}
	
	//使用用户的配置来重新启动485串口
	s485_uart_init( &Dtu_config.the_485cfg);
	s485_Uart_ioctl(S485_UART_CMD_SET_TXBLOCK);
	s485_Uart_ioctl(S485UART_SET_TXWAITTIME_MS, 2000);
	s485_Uart_ioctl(S485_UART_CMD_CLR_RXBLOCK);
	
	if( Dtu_config.work_mode == MODE_LOCALRTU)		//本地RTU模式不处理短信或者tcp
		step = 2;
	else
	{
		
		while(1)
		{
			if( SIM800->startup(SIM800) != ERR_OK)
			{
				
				osDelay(1000);
			}
			else if( SIM800->check_simCard(SIM800) == ERR_OK)
			{	
				
				break;
			}
			else {
				
				osDelay(1000);
			}
			
		}
		
	}
	while (1) {
		switch( step)
		{
			case 0:
				
				ret = SIM800->tcp_cnnt( SIM800, cnnt_seq, Dtu_config.DateCenter_ip[cnnt_seq], Dtu_config.DateCenter_port[cnnt_seq]);
				if( Dtu_config.Activestandby_mode && ret == ERR_OK)
				{
					step ++;
					break;
				}
				cnnt_seq ++;
				if( cnnt_seq >= IPMUX_NUM)
				{
					step ++;
					cnnt_seq = 0;
				}
				break;
			case 1:
				lszie = DTU_BUF_LEN;
				set_alarmclock_s( HEARTBEAT_ALARMID, Dtu_config.hartbeat_timespan_s);
				ret = SIM800->guard_serial( SIM800, DTU_Buf, &lszie);
				if( ret == tcp_receive)
				{
					ret = SIM800->deal_tcprecv_event( SIM800, DTU_Buf,  DTU_Buf, &lszie);
					if( lszie >= 0)
					{
						
						DPRINTF(" recv : %s \n", DTU_Buf);
						s485_Uart_write(DTU_Buf, lszie);
					}
				
					
				}
				if( ret == tcp_close)
				{
					ret = SIM800->deal_tcpclose_event( SIM800, DTU_Buf, lszie);
				}
				if( ret == sms_urc)
				{
					ret = SIM800->deal_smsrecv_event( SIM800, DTU_Buf, DTU_Buf,  &lszie, Recv_PhnoeNo);
					if( decodeTTCP_begin( DTU_Buf) == ERR_OK)
					{
						dtu_conf();
						decodeTTCP_finish();
						
					}
					else if( Dtu_config.work_mode == MODE_SMS)
					{
						for( i = 0; i < ADMIN_PHNOE_NUM; i ++)
						{
							pp = strstr((const char*)Recv_PhnoeNo, Dtu_config.admin_Phone[i]);
							if( pp)
							{
								s485_Uart_write(DTU_Buf, lszie);
								break;
							}
							
						}
						
						
						
					}
					
					
				}
				step ++;
				break;
			case 2:
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
				if(  Dtu_config.work_mode == MODE_SMS)
				{
					for( i = 0; i < ADMIN_PHNOE_NUM; i ++)
					{
						SIM800->send_text_sms( SIM800, Dtu_config.admin_Phone[i], DTU_Buf);
						
					}
					
					
				}
				
				step++;
			case 3:
				if( Ringing_s(HEARTBEAT_ALARMID) == ERR_OK)
				{
					strcpy( DTU_Buf,  Dtu_config.heatbeat_package);
					Heatbeat_count ++;
					for( i = 0; i < IPMUX_NUM; i ++)
					{
						SIM800->sendto_tcp( SIM800, i, DTU_Buf, ret);
						
					}
					
				}	
				step++;
			case 4:
				if( Dtu_config.Activestandby_mode )
				{
					ret = SIM800->get_firstCnt_seq(SIM800);
					
					step = 0;
					if( ret >= 0)
					{
						step = 1;
					}
					break;
				}
				
			
				ret = SIM800->get_firstDiscnt_seq(SIM800);
				if( ret >= 0)
					SIM800->tcp_cnnt( SIM800, ret, Dtu_config.DateCenter_ip[ret], Dtu_config.DateCenter_port[ret]);
				step = 1;
				break;
			default:
				step = 0;
				break;
			  
	  }
	  
	  
	  osThreadYield ();                                           // suspend thread
	}
}

static int get_dtuCfg(DtuCfg_t *conf)
{
	int i = 0;
	
	DtuCfg_file	= fs_open( DTUCONF_filename);

	if( DtuCfg_file)
	{
		fs_read( DtuCfg_file, (uint8_t *)conf, sizeof( DtuCfg_t));
		if( conf->ver[0] == DTU_CONFGILE_MAIN_VER &&  conf->ver[1] == DTU_CONFGILE_SUB_VER)
		{
			
			return ERR_OK;
		}
	}
	else
	{
		DtuCfg_file	= fs_creator( DTUCONF_filename, sizeof( DtuCfg_t));
		
			
	}
	
	
	
	//默认的配置
	
	
		 
	conf->ver[0] = DTU_CONFGILE_MAIN_VER;
	conf->ver[1] = DTU_CONFGILE_SUB_VER;
	conf->Activestandby_mode = 1;
	conf->hartbeat_timespan_s = 5;
	conf->work_mode = 0;
	conf->dtu_id = MODE_DTU;
	strcpy( conf->sim_NO,"13888888888");
	SIM800->read_simPhonenum( SIM800, DTU_Buf);
	sprintf( conf->registry_package,"XMYN%09d%s",conf->dtu_id, DTU_Buf);
	conf->heatbeat_package[0] = '$';
	conf->heatbeat_package[1] = '\0';
	conf->output_mode = 0;
	memcpy( &conf->the_485cfg, &Conf_S485Usart_default, sizeof( Conf_S485Usart_default));
	for( i = 0; i < IPMUX_NUM; i++)
	{
		strcpy( conf->DateCenter_ip[i], DEF_IPADDR);
		strcpy( conf->protocol[i], DEF_PROTOTOCOL);
		conf->DateCenter_port[i] = DEF_PORTNUM;
		
	}
	
	if( DtuCfg_file)
	{
		fs_write( DtuCfg_file, (uint8_t *)conf, sizeof( DtuCfg_t));
		fs_flush();
	}
	
	return ERR_OK;
	
}


void ack_str( char *str)
{
	
	s485_Uart_write(str, strlen(str));
}

static int check_phoneNO(char *NO)
{
	int j = 0;
	while(NO[j] != '\0')
	{
		if( NO[j] < '0' && NO[j] > '9')
			break;
		j ++;
		if( j == 11)
			return ERR_OK;
	}
	
	return ERR_BAD_PARAMETER;
	
}

static int copy_phoneNO(char *dest_NO, char *src_NO)
{
	int j = 0;
	while(src_NO[j] != '\0')
	{
		if( src_NO[j] >= '0' && src_NO[j] <= '9')
		{
			dest_NO[j] = src_NO[j];
		}
		else
			break;
		j ++;
		if( j == 11)
			return ERR_OK;
	}
	
	return ERR_BAD_PARAMETER;
	
}


static void dtu_conf(void)
{
	char *pcmd;
	char *parg;
	int 	i_data = 0;
	short		i = 0, j = 0;
	char	com_Wordbits[4] = { '8', '9', '0', 0};
	char	com_Paritybit[4] = { 'N', 'E', 'O', 0};
	char	com_stopbit[4] = { '1', '2',0,0};
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
			else if( i_data < MODE_END)
			{
				Dtu_config.work_mode = i_data;
				ack_str( "OK");
			}
			else
			{
				ack_str( "ERROR");
			}
			goto exit;
		}
		
		if( strcmp(pcmd ,"CONN") == 0)
		{
			i_data = 2;
			if( parg)
				i_data = atoi( parg);
			
			if( *parg == '?')
			{
				sprintf( DTU_Buf, "%d", Dtu_config.Activestandby_mode);
				ack_str( DTU_Buf);
			}
			else if( i_data < 2)
			{
				Dtu_config.Activestandby_mode = i_data;
				ack_str( "OK");
			}
			else
			{
				ack_str( "ERROR");
			}
			goto exit;
		}
		
		if( strcmp(pcmd ,"SVDM") == 0)
		{
			
			goto exit;
		}
		
		if( strcmp(pcmd ,"SVIP") == 0)
		{
			
			goto exit;
		}
		
		if( strcmp(pcmd ,"DNIP") == 0)
		{
			if( SIM800->set_dns_ip( SIM800, parg) == ERR_OK)
				ack_str( "OK");
			else
				ack_str( "ERROR");
			goto exit;
		}
		
		if( strcmp(pcmd ,"ATBT") == 0)
		{
				
			if( parg == NULL)
			{
				ack_str( "ERROR");
				goto exit;
			}
			if( parg[0] == '?')
			{
				i_data = 0;
				for( i = 0; j < ADMIN_PHNOE_NUM; i ++)
				{
					
					if( check_phoneNO( Dtu_config.admin_Phone[i]) == ERR_OK)
					{
						i_data ++;
						
					}
					
				}
				
				sprintf( DTU_Buf, "%d,%d,%s,%d", Dtu_config.dtu_id, Dtu_config.hartbeat_timespan_s,\
																		Dtu_config.sim_NO, i_data );
				for( i = 0; j < ADMIN_PHNOE_NUM; i ++)
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
					i++;
					break;
				case 1:
					i_data = atoi( parg);
					Dtu_config.hartbeat_timespan_s = i_data;
					i++;
					break;
				case 2:
					i_data = atoi( parg);
					Dtu_config.hartbeat_timespan_s = i_data;
					i++;
					break;
				case 3:
					if( check_phoneNO(parg) != ERR_OK)
					{
						ack_str( "ERROR");
						goto exit;
					}
					copy_phoneNO( Dtu_config.sim_NO, parg);
					
					i++;
					break;
				case 4:
					j = ADMIN_PHNOE_NUM +1;
					j = atoi( parg);
					if( j > ADMIN_PHNOE_NUM)
					{
						ack_str( "ERROR");
						goto exit;
						
					}
					i++;
					i_data = 0;
					break;
				case 5:
					if( check_phoneNO(parg) != ERR_OK)
					{
						ack_str( "ERROR");
						goto exit;
					}
					copy_phoneNO( Dtu_config.admin_Phone[ i_data], parg);
					i_data ++;
					j --;
					if( j)
					{
						break;
					}
					else
					{
						ack_str( "ERROR");
						goto exit;
					}
				default:
					ack_str( "ERROR");
					goto exit;
					
			}		//switch
			
		}
		
		if( strcmp(pcmd ,"PRNT") == 0)
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
				ack_str( "OK");
			}
			else if( strcmp(parg ,"NO") == 0)
			{
				Dtu_config.output_mode = 0;
				ack_str( "OK");
			}
			else
			{
				ack_str( "ERROR");
			}
			goto exit;
		}
		
		if( strcmp(pcmd ,"SCOM ") == 0)
		{
			if( parg[0] == '?')
			{
				
				sprintf( DTU_Buf, "%d,%c,%c,%c", Dtu_config.the_485cfg.USART_BaudRate, \
																				 com_Wordbits[Dtu_config.the_485cfg.USART_WordLength/0x1000], \
																					com_Paritybit[Dtu_config.the_485cfg.USART_Parity/0x300], \
																					com_stopbit[Dtu_config.the_485cfg.USART_StopBits/0x2000]	);
				for( i = 0; j < ADMIN_PHNOE_NUM; i ++)
				{
					
					if( check_phoneNO( Dtu_config.admin_Phone[i]) == ERR_OK)
					{
						strcat( DTU_Buf,",");
						strcat( DTU_Buf, Dtu_config.admin_Phone[i]);
						
					}
					
				}
				
				
				
				goto exit;
				
			}
			
			goto exit;
		}
		
		if( strcmp(pcmd ,"REGT ") == 0)
		{
			
			goto exit;
		}
		
		if( strcmp(pcmd ,"HEAT  ") == 0)
		{
			
			goto exit;
		}
		
		if( strcmp(pcmd ,"VAPN ") == 0)
		{
			
			goto exit;
		}
		
		if( strcmp(pcmd ,"CNUM ") == 0)
		{
			
			goto exit;
		}
	}

	exit:	
	fs_write( DtuCfg_file, (uint8_t *)&Dtu_config, sizeof( DtuCfg_t));
	
}
