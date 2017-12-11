
#include "cmsis_os.h"                                           // CMSIS RTOS header file
#include "osObjects.h"                      // RTOS object definitions
#include "gprs.h"
#include "sdhError.h"
#include "dtuConfig.h"
#include "string.h"
#include "debug.h"
#include "TTextConfProt.h"
#include "times.h"
#include "led.h"
#include "dtu.h"

#include "modbusRTU_cli.h"
/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/
#define DTU_BUF_LEN		256
#define TMP_BUF_LEN		64


#define NEW_CODE			1



void thrd_dtu (void const *argument);                             // thread function
osThreadId tid_ThrdDtu;                                          // thread id
osThreadDef (thrd_dtu, osPriorityNormal, 1, 0);                   // thread object

gprs_t *SIM800 ;

char	DTU_Buf[DTU_BUF_LEN];

#define TTEXTSRC_485 0
#define TTEXTSRC_SMS(n)	( n + 1)

#ifndef NEW_CODE
static void Config_ack( char *data, void *arg);

static int TText_source = TTEXTSRC_485;	
#endif


StateContext *MyContext;

static void prnt_485( char *data)
{
	if( Dtu_config.output_mode)
	{
		
		s485_Uart_write(data, strlen(data) );
	}
	
	DPRINTF(" %s \n", data);
	
}

static void GprsShutdown()
{
	gprs_t *sim800 ;
	sim800 = GprsGetInstance();
	
	sim800->tcpClose( sim800, 0);
	//在短信重起的时候，如果在这里关闭gprs，就无法删除掉这条重启指令了
	//导致永远都要重启.所以不能在这里关闭gprs
//	sim800->shutdown( sim800);
}

int Init_ThrdDtu (void) {
	
#ifdef NEW_CODE	
	gprs_t *sim800 ;
	DtuContextFactory* factory = DCFctGetInstance();
	
	MyContext = factory->createContext( Dtu_config.work_mode);
	
	if( MyContext == NULL)
		return 0;
	
	prnt_485("gprs threat startup ! \r\n");
	if( NEED_GPRS( Dtu_config.work_mode)) 
	{
		sim800 = GprsGetInstance();
		if( Dtu_config.multiCent_mode == 0)
		{
			
			Grps_SetCipmode( CIPMODE_TRSP);
			Grps_SetCipmux(0);
		}
		sim800->startup(sim800);
	}
	
	MyContext->init( MyContext, DTU_Buf, DTU_BUF_LEN);
	MyContext->initState( MyContext);
	g_shutdow = GprsShutdown;
#else	

#endif	

	tid_ThrdDtu = osThreadCreate (osThread(thrd_dtu), NULL);
	if (!tid_ThrdDtu) return(-1);

	return(0);
}





void thrd_dtu (void const *argument) {
	
	
#ifdef NEW_CODE		
	while(1)
	{
		threadActive();
		MyContext->curState->run( MyContext->curState, MyContext);
//		osDelay(10);
		osThreadYield();         
		
	}
#else

	
#endif
}


