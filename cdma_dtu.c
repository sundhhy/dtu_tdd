
#include "cmsis_os.h"                                           // CMSIS RTOS header file
#include "osObjects.h"                      // RTOS object definitions
#include "gprs.h"
#include "serial485_uart.h"
#include "sdhError.h"
#include "dtuConfig.h"
#include "string.h"
#include "debug.h"
#include "sw_filesys.h"

/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/
 #define DTU_BUF_LEN		512
void thrd_dtu (void const *argument);                             // thread function
osThreadId tid_ThrdDtu;                                          // thread id
osThreadDef (thrd_dtu, osPriorityNormal, 1, 0);                   // thread object


static int get_dtuCfg(DtuCfg_t *conf);

gprs_t *SIM800 ;
DtuCfg_t	Dtu_config;
sdhFile *DtuCfg_file;

char	DTU_Buf[DTU_BUF_LEN];
int Init_ThrdDtu (void) {
	SIM800 = gprs_t_new();
	SIM800->init(SIM800);
	s485_uart_init();
	s485_Uart_ioctl(S485_UART_CMD_SET_TXBLOCK);
	s485_Uart_ioctl(S485UART_SET_TXWAITTIME_MS, 2000);
	s485_Uart_ioctl(S485_UART_CMD_CLR_RXBLOCK);
	get_dtuCfg( &Dtu_config);
	
	
	
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
	int lszie = DTU_BUF_LEN;
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
				ret = SIM800->guard_serial( SIM800, DTU_Buf, &lszie);
				if( ret == tcp_receive)
				{
					ret = SIM800->deal_tcprecv_event( SIM800, DTU_Buf, &lszie);
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
					
				}
				step ++;
				break;
			case 2:
				ret = s485_Uart_read( DTU_Buf, DTU_BUF_LEN);
				if( ret > 0)
				{
					SIM800->sendto_tcp( SIM800, 0, DTU_Buf, ret);
					SIM800->sendto_tcp( SIM800, 1, DTU_Buf, ret);
					SIM800->sendto_tcp( SIM800, 2, DTU_Buf, ret);
					SIM800->sendto_tcp( SIM800, 3, DTU_Buf, ret);
					
					if( Dtu_config.Sms_mode)
					{
						SIM800->send_text_sms( SIM800, Dtu_config.DC_Phone[0], DTU_Buf);
						SIM800->send_text_sms( SIM800, Dtu_config.DC_Phone[1], DTU_Buf);
						SIM800->send_text_sms( SIM800, Dtu_config.DC_Phone[2], DTU_Buf);
						SIM800->send_text_sms( SIM800, Dtu_config.DC_Phone[3], DTU_Buf);
						
						
					}
					
				}
				step++;
			case 3:
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
		if( conf->ver[0] != 0xff || conf->ver[1] != 0xff)
		{
			
			return ERR_OK;
		}
	}
	else
	{
		DtuCfg_file	= fs_creator( DTUCONF_filename, sizeof( DtuCfg_t));
		
			
	}
	
	
	//Ä¬ÈÏµÄÅäÖÃ
	conf->ver[0] = DTU_CONFGILE_MAIN_VER;
	conf->ver[1] = DTU_CONFGILE_SUB_VER;
	conf->Activestandby_mode = 1;
	conf->Sms_mode = 0;
	for( i = 0; i < IPMUX_NUM; i++)
	{
		strcpy( conf->DateCenter_ip[i], DEF_IPADDR);
		strcpy( conf->protocol[i], DEF_PROTOTOCOL);
		conf->DateCenter_port[i] = DEF_PORTNUM;
		
	}
	
	if( DtuCfg_file)
	{
		fs_write( DtuCfg_file, (uint8_t *)conf, sizeof( DtuCfg_t));
	}
	
	return ERR_OK;
	
}
