#ifndef __DTUCONFIG_H__
#define __DTUCONFIG_H__

#include <stdint.h>
#include "gprs.h"



#define DEF_PROTOTOCOL "TCP"
#define DEF_IPADDR "chitic.zicp.net"
#define DEF_PORTNUM 18897

typedef struct {
	char	DateCenter_ip[IPMUX_NUM][64];
	int		DateCenter_port[IPMUX_NUM];
	char	protocol[IPMUX_NUM][4];
	
	char	Activestandby_mode;			//主备模式
	char	Sms_mode;					//短信模式
	char	resv[2];
}DtuCfg_t;



#endif
