#ifndef __RTU_H__
#define __RTU_H__
#include "lw_oopc.h"
#include "stdint.h"
#include "dtu.h"

CLASS( RtuInstance)
{
	char 						*dataBuf;
	BusinessProcess *modbusProcess;
	BusinessProcess *forwardSMS;
	BusinessProcess *forwardNet;
	
	int ( *init)( RtuInstance *self, int workmode);
	void ( *run)( RtuInstance *self);
	
	
};



void Init_rtu(void);


#endif
