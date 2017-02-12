#ifndef __DTUCONFIG_H__
#define __DTUCONFIG_H__

#include <stdint.h>
#include "gprs.h"
#include "serial485_uart.h"



#define MODE_DTU					0			//
#define MODE_SMS					1			//
#define MODE_REMOTERTU		2
#define MODE_LOCALRTU			3
#define MODE_BEGIN					0
#define MODE_END					4

#define DTU_CONFGILE_MAIN_VER		2
#define DTU_CONFGILE_SUB_VER		1

#define DEF_PROTOTOCOL "TCP"
#define DEF_IPADDR "chitic.zicp.net"
#define DEF_PORTNUM 18897
#define	DTUCONF_filename	"sys.cfg"

#define SIGTYPE_0_5_V			10
#define SIGTYPE_1_5_V			11
#define SIGTYPE_4_20_MA			9

#define PHONENO_LEN				16
#define ADMIN_PHNOE_NUM			4
#define REGPACKAGE_LEN			32
#define HEATBEATPACKAGE_LEN			32
#define IP_LEN		16
#define IP_ADDR_LEN		64



typedef struct {
	uint16_t		rangeH;
	uint16_t		rangeL;
	uint16_t		alarmH;
	uint16_t		alarmL;
}signRange_t;
typedef struct {
	
	char	multiCent_mode;			//多中心模式
	char	work_mode;					//工作模式
	uint8_t	ver[2];						//版本
	
	char	dns_ip[IP_LEN];
	char	smscAddr[16];
	char	apn[64];		//由接入点，用户名，密码 三部分组成
//	char	apn_username[32];
//	char	apn_passwd[16];
	char	DateCenter_ip[IPMUX_NUM][IP_ADDR_LEN];
	int		DateCenter_port[IPMUX_NUM];
	char	protocol[IPMUX_NUM][4];
	char	registry_package[32];
	
	char	sim_NO[PHONENO_LEN];
	char	admin_Phone[ADMIN_PHNOE_NUM][PHONENO_LEN];
	
	char			output_mode;
	char			chn_type[3];
	
	uint32_t	hartbeat_timespan_s;
	char		heatbeat_package[32];
	uint32_t		dtu_id;
	uint32_t		rtu_addr;
	
	
			
	ser_485Cfg	the_485cfg;
	
	signRange_t		sign_range[3];
}DtuCfg_t;

extern DtuCfg_t	Dtu_config;

#endif
