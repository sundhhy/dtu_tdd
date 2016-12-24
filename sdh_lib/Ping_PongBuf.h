#ifndef _PINGPONGBUF_H_
#define _PINGPONGBUF_H_
#include <stdint.h>
#define BUF_NONE	0
#define BUF_PING	1
#define BUF_PONG	2


typedef struct
{

	char 	*ping_buf;
	char 	*pong_buf;
	short 	ping_len;
	short 	pong_len;
	
	
	
	uint8_t	loading_buf;		//外设接收中的buf
	uint8_t	playload_buf;		//数据已经结束完成的buf
	uint8_t	resv[2];
	
}PPBuf_t;
void  init_pingponfbuf( PPBuf_t *ppbuf);
void switch_receivebuf( PPBuf_t *ppbuf, char **buf, short *len);
char *get_playloadbuf( PPBuf_t *ppbuf);
int get_loadbuflen( PPBuf_t *ppbuf);
void free_playloadbuf( PPBuf_t *ppbuf);
#endif
