
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
					else if( Dtu_config.Sms_mode)
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
				ret = s485_Uart_read( DTU_Buf, DTU_BUF_LEN);
				if( ret <= 0)
				{
					step++;
					break;
				}

				for( i = 0; i < IPMUX_NUM; i ++)
				{
					SIM800->sendto_tcp( SIM800, i, DTU_Buf, ret);
					
				}			
				if( Dtu_config.Sms_mode)
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
					sprintf( DTU_Buf, "dtu[%d] heart beat %d \n", Dtu_config.dtu_id, Heatbeat_count);
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
	conf->Sms_mode = 0;
	
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


static void dtu_conf(void)
{
	char *pcmd;
	
	if( get_cmdtype() != CONFCMD_TYPE_ATC)
		return;
	
	while(1)
	{
		pcmd = get_cmd();
		
		if( pcmd == NULL)
			return;
		
		if( strcmp(pcmd ,"SVDM") == 0)
		{
			
			return;
		}
		
		if( strcmp(pcmd ,"SVIP") == 0)
		{
			
			return;
		}
		
		if( strcmp(pcmd ,"DNIP") == 0)
		{
			
			return;
		}
		
		if( strcmp(pcmd ,"ATBT") == 0)
		{
			
			return;
		}
		
		if( strcmp(pcmd ,"PRNT") == 0)
		{
			
			return;
		}
		
		if( strcmp(pcmd ,"SCOM ") == 0)
		{
			
			return;
		}
		
		if( strcmp(pcmd ,"REGT ") == 0)
		{
			
			return;
		}
		
		if( strcmp(pcmd ,"HEAT  ") == 0)
		{
			
			return;
		}
		
		if( strcmp(pcmd ,"VAPN ") == 0)
		{
			
			return;
		}
		
		if( strcmp(pcmd ,"CNUM ") == 0)
		{
			
			return;
		}
	}
	
}
