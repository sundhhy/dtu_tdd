
#include "cmsis_os.h"                                           // CMSIS RTOS header file
#include "sdhError.h"
#include "dtuConfig.h"
#include "string.h"
#include "debug.h"
/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/
 
void Thread_adc (void const *argument);                             // thread function
osThreadId tid_Thread_RTU;                                          // thread id
osThreadDef (Thread_adc, osPriorityNormal, 1, 0);                   // thread object

int Init_Thread_adc (void) {

  tid_Thread_RTU = osThreadCreate (osThread(Thread_adc), NULL);
  if (!tid_Thread_RTU) return(-1);
  
  return(0);
}

void Thread_adc (void const *argument) {

  while (1) {
    ; // Insert thread code here...
    osThreadYield ();                                           // suspend thread
  }
}
