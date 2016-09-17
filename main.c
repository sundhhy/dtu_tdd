/*----------------------------------------------------------------------------
 * CMSIS-RTOS 'main' function template
 *---------------------------------------------------------------------------*/

#define osObjectsPublic                     // define objects in main module
#include "osObjects.h"                      // RTOS object definitions
#include "chipset.h"
#include "gprs.h"
/*
 * main: initialize and start the system
 */
int main (void) {
	gprs_t *sim900 = gprs_t_new();;
	
  osKernelInitialize ();                    // initialize CMSIS-RTOS

	
	 
  // initialize peripherals here
//	RCC_Configuration();
	
	NVIC_Configuration();
	
	GPIO_Configuration();
	

  // create 'thread' functions that start executing,
  // example: tid_name = osThreadCreate (osThread(name), NULL);

  osKernelStart ();                         // start thread execution

	sim900->startup(sim900);
	while(1)
	{
		
		 sim900->startup(sim900);
		osDelay(1000);
		
	}
}
