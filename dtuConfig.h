#ifndef __DTUCONFIG_H__
#define __DTUCONFIG_H__

#include <stdint.h>
#include "gprs.h"
#include "serial485_uart.h"



#define MODE_DTU					0			//
#define MODE_SMS					1			//
#define MODE_REMOTERTU		2
#define MODE_LOCALRTU			3	
#define MODE_END					4

#define DTU_CONFGILE_MAIN_VER		01
#define DTU_CONFGILE_SUB_VER		04

#define DEF_PROTOTOCOL "TCP"
#define DEF_IPADDR "chitic.zicp.net"
#define DEF_PORTNUM 18897
#define	DTUCONF_filename	"dtu.cfg"

#define ADMIN_PHNOE_NUM			4
#define REGPACKAGE_LEN			32
#define HEATBEATPACKAGE_LEN			32

typedef struct {
	
	char	Activestandby_mode;			//主备模式
	char	work_mode;					//工作模式
	uint8_t	ver[2];						//版本
	
	char	DateCenter_ip[IPMUX_NUM][64];
	int		DateCenter_port[IPMUX_NUM];
	char	protocol[IPMUX_NUM][4];
	char	registry_package[32];
	
	char	sim_NO[12];
	char	admin_Phone[ADMIN_PHNOE_NUM][12];
	
	char			output_mode;
	char			resv_1;
	uint16_t	hartbeat_timespan_s;
	char	heatbeat_package[32];
	uint32_t		dtu_id;
	ser_485Cfg	the_485cfg;
	
	
}DtuCfg_t;



#endif
