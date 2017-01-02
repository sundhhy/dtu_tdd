#ifndef __DEBUG_H__
#define __DEBUG_H__
#include <stdio.h>
#define DEBUG_LEVEL		0
#define DEBUG_SWITCH_ON
#ifdef DEBUG_SWITCH_ON
	#define DPRINTF(format, arg...) printf( format,##arg)
#else 
	#define DPRINTF(format, arg...) 
#endif



#ifdef DEBUG_SWITCH_ONfff
/** print debug message only if debug message type is enabled...
 *  AND is of correct type AND is at least LWIP_DBG_LEVEL
 */
#define LEVEL_DEBUGF(debug, message) do { \
                               if (debug >= DEBUG_LEVEL) { \
                                 printf(message); \
                                
                               } \
                             } while(0)

#else  /* LWIP_DEBUG */
#define LEVEL_DEBUGF(debug, message) 
#endif /* LWIP_DEBUG */

							 
/**
 * @brief 打印等级定义
 */	

#define 	DDG_EMERG 	0
#define		DDG_ALERT 	1
#define 	DDG_ERR		2
#define		DDG_WARNING	3
#define 	DDG_NOTICE	5
#define		DDG_INFO	6
#define		DDG_DEBUG	7
							 


/** TDD开发中的测试内容.
 *
 */
//#define TDD_GPRS_ONOFF
//#define TDD_GPRS_USART
//#define TDD_GPRS_SMS
//#define TDD_GPRS_TCP

#define PROTOTOCOL "TCP"
#define IPADDR "chitic.zicp.net"
#define PORTNUM 19350


//#define TDD_S485
//#define TDD_FILESYS_TEST

#define TDD_ADC
//#define TDD_TIMER
#endif
