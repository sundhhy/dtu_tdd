/**
* @file 		circularbuffer.c
* @brief		实现环形缓存的算法.
* @details		1. 环形缓存的长度
* @author		author
* @date		date
* @version	A001
* @par Copyright (c): 
* 		XXX??
* @par History:         
*	version: author, date, desc\n
*/
#include "CircularBuffer.h"
#include "sdhError.h"
/**
 * @brief 返回环形缓存中存储数据的长度
 *
 * @details 缓存.
 * 
 * @param[in]	inArgName input argument description.
 * @param[out]	outArgName output argument description. 
 * @retval	OK	??
 * @retval	ERROR	?? 
 */
uint16_t	CBLengthData( sCircularBuffer *cb)
{
	return ( ( cb->write - cb->read) & ( cb->size - 1));
}

/**
 * @brief 像环形缓存写一个数据.
 *
 * @details This is a detail description.
 * 
 * @param[in]	inArgName input argument description.
 * @param[out]	outArgName output argument description. 
 * @retval	OK	??
 * @retval	ERROR	?? 
 */
int	CBWrite( sCircularBuffer *cb, tElement data)
{
	if( CBLengthData( cb) == ( cb->size - 1))	return ERR_MEM_UNAVAILABLE;
	cb->buf[ cb->write] = data;
	CPU_IRQ_OFF;
	cb->write = ( cb->write + 1) & ( cb->size - 1);  //必须是原子的
	CPU_IRQ_ON;
	return ERR_OK;
}

/**
 * @brief 从环形缓存读取一个数据.
 *
 * @details This is a detail description.
 * 
 * @param[in]	inArgName input argument description.
 * @param[out]	outArgName output argument description. 
 * @retval	OK	??
 * @retval	ERROR	?? 
 */
int	CBRead( sCircularBuffer *cb, tElement *data)
{
	if( CBLengthData( cb) == 0)	return ERR_RES_UNAVAILABLE;
	*data = cb->buf[ cb->read];
	CPU_IRQ_OFF;
	cb->read = ( cb->read + 1) & ( cb->size - 1);  //必须是原子的
	CPU_IRQ_ON;
	return ERR_OK;
}
