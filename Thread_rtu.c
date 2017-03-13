
#include "cmsis_os.h"                                           // CMSIS RTOS header file
#include "sdhError.h"
#include "dtuConfig.h"
#include "string.h"
#include "debug.h"
#include "rtu.h"
/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/
 
void Thread_rtu (void const *argument);                             // thread function
osThreadId tid_Thread_RTU;                                          // thread id
osThreadDef (Thread_rtu, osPriorityNormal, 1, 0);                   // thread object


RtuInstance *g_p_myRtu;
int Init_Thread_rtu (void) {

	int ret = 0;

	g_p_myRtu = RtuInstance_new();
	ret = g_p_myRtu->init( g_p_myRtu, Dtu_config.work_mode);
	if( ret != ERR_OK)
	{
		lw_oopc_delete( g_p_myRtu);
		return ERR_OK;
		
	}

	s485_uart_init( &Dtu_config.the_485cfg, NULL);
	s485_Uart_ioctl(S485_UART_CMD_SET_RXBLOCK);
	s485_Uart_ioctl(S485UART_SET_RXWAITTIME_MS, 200);
	s485_Uart_ioctl(S485_UART_CMD_SET_TXBLOCK);
	s485_Uart_ioctl(S485UART_SET_TXWAITTIME_MS, 200);
	
	tid_Thread_RTU = osThreadCreate (osThread(Thread_rtu), NULL);

	if (!tid_Thread_RTU) return(-1);
  
  return(0);
}

void Thread_rtu (void const *argument) {
  while (1) {    
		g_p_myRtu->run( g_p_myRtu);	
		osThreadYield ();                                           // suspend thread
  }
}
