#ifndef __DTUCONFIG_H__
#define __DTUCONFIG_H__

#include <stdint.h>
#include "gprs.h"
#include "serial485_uart.h"


#define DTU_CONFGILE_MAIN_VER		01
#define DTU_CONFGILE_SUB_VER		01

#define DEF_PROTOTOCOL "TCP"
#define DEF_IPADDR "chitic.zicp.net"
#define DEF_PORTNUM 18897
#define	DTUCONF_filename	"dtu.cfg"

typedef struct {
	
	char	Activestandby_mode;			//主备模式
	char	Sms_mode;					//短信模式
	uint8_t	ver[2];						//版本
	
	char	DateCenter_ip[IPMUX_NUM][64];
	int		DateCenter_port[IPMUX_NUM];
	char	protocol[IPMUX_NUM][4];
	
	char	DC_Phone[4][12];
	
	short	hartbeat_timespan_s;
	
	ser_485Cfg	the_485cfg;
	
	
}DtuCfg_t;



#endif
