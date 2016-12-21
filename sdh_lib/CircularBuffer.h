#ifndef _CIRCULARBUFFER_H__
#define _CIRCULARBUFFER_H__
#include "stdint.h"

typedef void* tElement;	
typedef struct {
	tElement	*buf;
	uint32_t	size;		//必须是2的幂
	uint16_t	read;		//当前的读位置
	uint16_t	write;		//当前的写位置
	
}sCircularBuffer;

#define CPU_IRQ_OFF	
#define CPU_IRQ_ON


uint16_t	CBLengthData( sCircularBuffer *cb);
int	CBWrite( sCircularBuffer *cb, tElement data);
int	CBRead( sCircularBuffer *cb, tElement *data);
#endif
