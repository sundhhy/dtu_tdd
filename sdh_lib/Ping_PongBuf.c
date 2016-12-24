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
	
	ppbuf->loading_buf = BUF_NONE;
	ppbuf->playload_buf = BUF_NONE;
}

void switch_receivebuf( PPBuf_t *ppbuf, char **buf, short *len)
{
	if( ppbuf->loading_buf == BUF_NONE)
	{
		*buf = ppbuf->ping_buf;
		*len = ppbuf->ping_len;
		ppbuf->loading_buf = BUF_PING;
		ppbuf->playload_buf = BUF_NONE;
	}
	else if( ppbuf->loading_buf == BUF_PING)
	{
		//另一个缓存的数据被取走了才切换过去
		if( ppbuf->playload_buf != BUF_PONG)
		{
			*buf = ppbuf->pong_buf;
			*len = ppbuf->pong_len;
			ppbuf->loading_buf = BUF_PONG;
			//将此缓存作为数据载荷缓存
			ppbuf->playload_buf = BUF_PING;
		}
		
	}
	else if( ppbuf->loading_buf == BUF_PONG)
	{
		//另一个缓存的数据被取走了才切换过去
		if( ppbuf->playload_buf != BUF_PING)
		{
			*buf = ppbuf->ping_buf;
			*len = ppbuf->ping_len;
			ppbuf->loading_buf = BUF_PING;
			
			ppbuf->playload_buf = BUF_PONG;
		}
			
	}
	else
	{
		*buf = ppbuf->ping_buf;
		*len = ppbuf->ping_len;
		ppbuf->loading_buf = BUF_PING;
		ppbuf->playload_buf = BUF_NONE;
	}
	
}



char *get_playloadbuf( PPBuf_t *ppbuf)
{
	
	if( ppbuf->playload_buf == BUF_PING)
	{
		return ppbuf->ping_buf;
		
	}
	else
	{
		return ppbuf->pong_buf;
		
	}
	ppbuf->playload_buf = BUF_NONE;
}
void free_playloadbuf( PPBuf_t *ppbuf)
{
	if( ppbuf->playload_buf == BUF_PING)
	{
		memset( ppbuf->ping_buf, 0, ppbuf->ping_len);
	}
	else
	{
		memset( ppbuf->ping_buf, 0, ppbuf->pong_len);
		
	}
	ppbuf->playload_buf = BUF_NONE;
	
}
//当ping和pong的装载标志都为1的时候
//ping缓存的内容未被读取
//pong缓存作为接收缓存一直在被外设数据覆盖
int get_loadbuflen( PPBuf_t *ppbuf)
{
	if( ppbuf->playload_buf == BUF_PING)
	{
		return ppbuf->ping_len;
	}
	else
	{
		return ppbuf->pong_len;
	}
	
}