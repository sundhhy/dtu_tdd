#include "cmsis_os.h"                                           // CMSIS RTOS header file
#include "osObjects.h"                      // RTOS object definitions
#include "gprs.h"
#include "sdhError.h"
#include "dtuConfig.h"
#include "string.h"
#include "debug.h"
#include "TTextConfProt.h"
#include "times.h"
#include "led.h"
#include "modbusRTU_cli.h"
sdhFile *DtuCfg_file;
DtuCfg_t	Dtu_config;


static int get_dtuCfg(DtuCfg_t *conf);


int Init_system_config(void)
{
	get_dtuCfg( &Dtu_config);
	
}

void set_default( DtuCfg_t *conf)
{
	//默认的配置
	int i = 0;
	memset( conf, 0, sizeof( DtuCfg_t));
		 
	conf->ver[0] = DTU_CONFGILE_MAIN_VER;
	conf->ver[1] = DTU_CONFGILE_SUB_VER;
	conf->multiCent_mode = 1;
	conf->hartbeat_timespan_s = 5;
	conf->work_mode = MODE_DTU;
	conf->dtu_id = 1;
	conf->rtu_addr = 1;
	strcpy( conf->sim_NO,"13888888888");
	strcpy( conf->apn," ");
	strcpy( conf->smscAddr," ");
	sprintf( conf->registry_package,"XMYN%09d%s",conf->dtu_id, conf->sim_NO);
	conf->heatbeat_package[0] = '$';
	conf->heatbeat_package[1] = '\0';
	conf->output_mode = 0;
	for( i = 0; i < 3; i ++)
	{
		Dtu_config.chn_type[i] = SIGTYPE_4_20_MA;
		Dtu_config.sign_range[i].rangeH = 0xFFFF;
		Dtu_config.sign_range[i].rangeL = 0;
		Dtu_config.sign_range[i].alarmH = 0xFFFF;
		Dtu_config.sign_range[i].alarmL = 0;
		
		
	}
	memcpy( &conf->the_485cfg, &Conf_S485Usart_default, sizeof( Conf_S485Usart_default));
	
	for( i = 0; i < IPMUX_NUM; i++)
	{
		strcpy( conf->DateCenter_ip[i], DEF_IPADDR);
		strcpy( conf->protocol[i], DEF_PROTOTOCOL);
		conf->DateCenter_port[i] = DEF_PORTNUM;
		
	}
	
}

static int get_dtuCfg(DtuCfg_t *conf)
{
	
	
	DtuCfg_file	= fs_open( DTUCONF_filename);
	DPRINTF(" fs_open  %p \n", DtuCfg_file);
	if( DtuCfg_file)
	{
		//todo: 2017-02-04 21:40:21 此处耗时8s
		fs_read( DtuCfg_file, (uint8_t *)conf, sizeof( DtuCfg_t));
		DPRINTF(" fs_read  done \n");
		if( conf->ver[0] == DTU_CONFGILE_MAIN_VER &&  conf->ver[1] == DTU_CONFGILE_SUB_VER)
		{
			
			return ERR_OK;
		}
	}
	else
	{
		DtuCfg_file	= fs_creator( DTUCONF_filename, sizeof( DtuCfg_t));
		DPRINTF(" fs_creator  %p \n", DtuCfg_file);
			
	}
	
	
	
	set_default( conf);
	
	if( DtuCfg_file)
	{
		fs_lseek( DtuCfg_file, WR_SEEK_SET, 0);
		fs_write( DtuCfg_file, (uint8_t *)conf, sizeof( DtuCfg_t));
		fs_flush();
	}
	
	return ERR_OK;
	
}
