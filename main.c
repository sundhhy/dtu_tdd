/*----------------------------------------------------------------------------
 * CMSIS-RTOS 'main' function template
 *---------------------------------------------------------------------------*/

#define osObjectsPublic                     // define objects in main module
#include "osObjects.h"                      // RTOS object definitions
#include "chipset.h"
#include "gprs.h"
#include "hardwareConfig.h"
#include "debug.h"
#ifdef TDD_GPRS_USART
#include "gprs_uart.h"
#endif
#include "serial485_uart.h"

#include "sdhError.h"
#include "string.h"
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

//#if  defined(TDD_GPRS_USART) ||  defined(TDD_GPRS_SMS ) || defined(TDD_GPRS_TCP ) 
#define TEST_BUF_SIZE 1048
char Test_buf[TEST_BUF_SIZE];
//#endif
/*
 * main: initialize and start the system
 */
int main (void) {
	gprs_t *sim800 = gprs_t_new();
	int i = 0;
//	char c;
  osKernelInitialize ();                    // initialize CMSIS-RTOS

	
	 
  // initialize peripherals here
//	RCC_Configuration();
	
	NVIC_Configuration();
	
	GPIO_Configuration();	
	USART_Configuration();
//	
	printf(" DTU TDD start ...\r\n");
	

  // create 'thread' functions that start executing,
  // example: tid_name = osThreadCreate (osThread(name), NULL);

	osKernelStart ();                         // start thread execution
	
	sim800->init(sim800);
	
#ifdef TDD_GPRS_ONOFF
	while(1)
	{
		
		sim800->startup(sim800);
		osDelay(3000);
		sim800->shutdown(sim800);
		osDelay(3000);
		
	}
#endif
	
#ifdef TDD_GPRS_USART
//	gprs_uart_init();
	sim800->startup(sim800);
	while(1)
	{
		i ++;
		if( gprs_uart_test(Test_buf, TEST_BUF_SIZE) == ERR_OK)
			printf(" gprs uart test  %d sccusseed \r\n", i);
		else
			printf(" gprs uart test  %d fail \r\n", i);
		
		memset( Test_buf, 0, 512);
		osDelay(1000);
		
	}
#endif

#ifdef TDD_GPRS_SMS
	
	while(1)
	{
		if( sim800->startup(sim800) != ERR_OK)
		{
			
			osDelay(1000);
		}
		else if( sim800->check_simCard(sim800) == ERR_OK)
		{	
			
			break;
		}
		else {
			
			osDelay(1000);
		}
		
	}
	
	while(1)
	{
		i ++;
		
		if( sim800->sms_test(sim800, "15858172663", Test_buf, TEST_BUF_SIZE) == ERR_OK)
			printf(" sim800 sms test  %d sccusseed \r\n", i);
		else
			printf(" sim800 sms test  %d fail \r\n", i);
		
		osDelay(1000);
//		printf(" test again Y/N  ? \r\n");
//		c = '\n';
//		while(c == '\n')
//		{
//			c = getchar();
//		}
//		
//		if( c == 'N' || c== 'n')
//			break;
	}


#endif	
	
#ifdef TDD_GPRS_TCP
	while(1)
	{
		if( sim800->startup(sim800) != ERR_OK)
		{
			
			osDelay(1000);
		}
		else if( sim800->check_simCard(sim800) == ERR_OK)
		{	
			
			break;
		}
		else {
			
			osDelay(1000);
		}
		
	}
	
	while(1)
	{
		i ++;
		
		if( sim800->tcp_test(sim800, IPADDR, PORTNUM, Test_buf, TEST_BUF_SIZE) == ERR_OK)
			printf(" sim800 tcp test  %d sccusseed \r\n", i);
		else
			printf(" sim800 tcp test  %d fail \r\n", i);
		
		osDelay(1000);

	}

#endif	
	
	
#ifdef TDD_S485
	s485_uart_init();
	while(1)
	{
		i ++;
		if( s485_uart_test(Test_buf, TEST_BUF_SIZE) == ERR_OK)
			printf(" 485 uart test  %d sccusseed \r\n", i);
		else
			printf(" 485 uart test  %d fail \r\n", i);
		
		memset( Test_buf, 0, 512);
		osDelay(1000);
		
	}
#endif	
	
}
