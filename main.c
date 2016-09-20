/*----------------------------------------------------------------------------
 * CMSIS-RTOS 'main' function template
 *---------------------------------------------------------------------------*/

#define osObjectsPublic                     // define objects in main module
#include "osObjects.h"                      // RTOS object definitions
#include "chipset.h"
#include "gprs.h"
#include "hardwareConfig.h"
#include "debug.h"
#include "gprs_uart.h"

#include "stm32f10x_usart.h"

#include "stdio.h"

#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
    /* Place your implementation of fputc here */
    /* e.g. write a character to the USART */
    USART_SendData( DEBUG_USART, (uint8_t) ch);

    /* Loop until the end of transmission */
    while (USART_GetFlagStatus( DEBUG_USART, USART_FLAG_TC) == RESET)
    {
    }

    return ch;
}

#ifdef TDD_GPRS_USART

char Test_buf[1024];
#endif
/*
 * main: initialize and start the system
 */
int main (void) {
	gprs_t *sim900 = gprs_t_new();
	int i = 0;
	
  osKernelInitialize ();                    // initialize CMSIS-RTOS

	
	 
  // initialize peripherals here
//	RCC_Configuration();
	
	NVIC_Configuration();
	
	GPIO_Configuration();	
	USART_Configuration();
//	
	printf("DTU TDD start ...\r\n");
	

  // create 'thread' functions that start executing,
  // example: tid_name = osThreadCreate (osThread(name), NULL);

  osKernelStart ();                         // start thread execution

#ifdef TDD_GPRS_ONOFF
	while(1)
	{
		
		sim900->startup(sim900);
		osDelay(3000);
		sim900->shutdown(sim900);
		osDelay(3000);
		
	}
#endif
	
#ifdef TDD_GPRS_USART
	while(1)
	{
		i ++;
		if( gprs_uart_test(Test_buf, 1024))
		{
			DPRINTF(" gprs uart test fail, the size is %d \r\n", i);
			break;
		}
		
	}
	
#endif
}
