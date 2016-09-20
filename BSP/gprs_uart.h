#ifndef __GPRS_UART_H__
#define __GPRS_UART_H__
#include "stdint.h"




/**
 * @brief gprs串口发送功能
 * 
 * @param data 发送数据的内存地址
 * @param size 发送数据的长度
 * @return ERR_OK 成功
 * @return 
 */
int gprs_Uart_write(char *data, uint16_t size);

/**
 * @brief gprs串口接收程序
 * 
 * @param data 
 * @param size 
 * @return ERR_OK 成功
 * @return 
 */
int gprs_Uart_read(char *data, uint16_t size);

/**
 * @brief 控制串口的行为
 * 
 * @param data 
 * @param size 
 * @return ERR_OK 成功
 * @return 
 */
void gprs_Uart_ioctl(int cmd, ...);

/**
 * @brief 测试gprs uart的收发功能
 * 
 * @param size 测试数据的长度
 * @return 0xFF if error, else return 0
 */
int gprs_uart_test(char *buf, int size);

#define GPRS_UART_CMD_SET_TXBLOCK	1
#define GPRS_UART_CMD_CLR_TXBLOCK	2
#define GPRS_UART_CMD_SET_RXBLOCK	3
#define GPRS_UART_CMD_CLR_RXBLOCK	4
#define GPRS_UART_CMD_SET_TXWAITTIME	5
#define GPRS_UART_CMD_SET_RXWAITTIME	6

#endif
