/**
* @file 		Ping_PongBuf.c
* @brief		实现双RAM
* @details		1. 当ping和pong都没被外设使用的时候，优先使用ping。 
*				2. 当ping和pong都被装载了数据时，数据都没被应用程序取走的时候，吧pong的缓存内容放弃掉
* @author		author
* @date		date
* @version	A001
* @par Copyright (c): 
* 		XXX??
* @par History:         
*	version: author, date, desc\n
*/
#include "Ping_PongBuf.h"
#include <string.h>
void  init_pingponfbuf( PPBuf_t *ppbuf)
{
	ppbuf->ping_status = PPBUF_STATUS_IDLE;
	ppbuf->pong_status = PPBUF_STATUS_IDLE;
	ppbuf->loading_buf = BUF_NONE;
	ppbuf->playload_buf = BUF_NONE;
}
//
void switch_receivebuf( PPBuf_t *ppbuf, char **buf, short *len)
{
	if( ppbuf->ping_status == PPBUF_STATUS_IDLE)
	{
		if( ppbuf->loading_buf == BUF_PONG)	//上一次装载的缓存是PONG，说明已经有数据存入了
		{
			ppbuf->playload_buf = BUF_PONG;
		}
		*buf = ppbuf->ping_buf;
		*len = ppbuf->ping_len;
		ppbuf->loading_buf = BUF_PING;
		ppbuf->ping_status = PPBUF_STATUS_LOADING;
		
	}
	else if( ppbuf->pong_status == PPBUF_STATUS_IDLE)
	{
		if( ppbuf->loading_buf == BUF_PING)	//上一次装载的缓存是PING，说明已经有数据存入了
		{
			ppbuf->playload_buf = BUF_PING;
		}
		*buf = ppbuf->pong_buf;
		*len = ppbuf->pong_len;
		ppbuf->loading_buf = BUF_PONG;
		ppbuf->pong_status = PPBUF_STATUS_LOADING;
		
	}
	else		//没有空闲的缓冲区，就不切换
	{
		//返回上一次使用的缓存
		if( ppbuf->loading_buf == BUF_PING)
		{
		
			*buf = ppbuf->ping_buf;
			*len = ppbuf->ping_len;
		}
		else if( ppbuf->loading_buf == BUF_PONG)
		{
			*buf = ppbuf->pong_buf;
			*len = ppbuf->pong_len;
			
		}
			
		
	}
}



char *get_playloadbuf( PPBuf_t *ppbuf)
{
	
	if( ppbuf->playload_buf == BUF_PING)
	{
		ppbuf->ping_status = PPBUF_STATUS_IDLE;
		return ppbuf->ping_buf;
		
	}
	else if( ppbuf->playload_buf == BUF_PONG)
	{
		ppbuf->pong_status = PPBUF_STATUS_IDLE;
		return ppbuf->pong_buf;
		
	}
	else
	{
		
		return ppbuf->ping_buf;
	}
}
void free_playloadbuf( PPBuf_t *ppbuf)
{
//	if( ppbuf->playload_buf == BUF_PING)
//	{
//		memset( ppbuf->ping_buf, 0, ppbuf->ping_len);
//	}
//	else if( ppbuf->playload_buf == BUF_PONG)
//	{
//		memset( ppbuf->pong_buf, 0, ppbuf->pong_len);
//		
//	}
//	ppbuf->playload_buf = BUF_NONE;
	
}
//当ping和pong的装载标志都为1的时候
//ping缓存的内容未被读取
//pong缓存作为接收缓存一直在被外设数据覆盖
int get_loadbuflen( PPBuf_t *ppbuf)
{
//	if( ppbuf->playload_buf == BUF_PING)
//	{
		return ppbuf->ping_len;
//	}
//	else
//	{
//		return ppbuf->pong_len;
//	}
	
}
