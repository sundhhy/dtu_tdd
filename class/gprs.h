#ifndef __GPRS_H__
#define __GPRS_H__
#include "lw_oopc.h"

CLASS(gprs_t)
{
	// public
	void (*startup)( gprs_t *self);
	void (*shutdown)( gprs_t *self);
	
};








#endif
