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
#define CONFIG_BUF_LEN  512
sdhFile *DtuCfg_file;
DtuCfg_t	Dtu_config;
static other_ack g_other_ack = NULL;
static	void *g_ack_arg = NULL;

static int get_dtuCfg(DtuCfg_t *conf);
static void dtu_conf(char *data);


int Init_system_config(void)
{
	get_dtuCfg( &Dtu_config);
	return ERR_OK;

}


void Config_job(void)
{
	char *p_buf = malloc( CONFIG_BUF_LEN);
	int ret = 0;
	memset( p_buf, 0, CONFIG_BUF_LEN);
	
	while(1)
	{
		osDelay(10);
		ret = s485_Uart_read( p_buf, CONFIG_BUF_LEN);
		if( ret <=  0)
		{
			continue;
		}
		
		if( enter_TTCP( p_buf) == ERR_OK)
		{
			get_TTCPVer( p_buf);
			while( s485_Uart_write(p_buf, strlen(p_buf) ) != ERR_OK)
					;

		}
		
		if( decodeTTCP_begin( p_buf) == ERR_OK)
		{
//			TText_source = TTEXTSRC_485;
			dtu_conf(p_buf);
			decodeTTCP_finish();
			
		}
		
	}
	
}

int Config_server( char *data, other_ack ack, void *arg)
{
	
	if( decodeTTCP_begin( data) == ERR_OK)
	{
		g_other_ack = ack;
		g_ack_arg = arg;
		dtu_conf(data);
		decodeTTCP_finish();
		return 0;
	}
	g_other_ack = NULL;
	return -1;
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
	conf->work_mode = MODE_PASSTHROUGH;
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
		Dtu_config.chn_type[i] = SIGTYPE_0_5_V;
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
		conf->DateCenter_port[i] = DEF_PORTNUM+ i;
		
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




static void ack_str( char *str)
{
//	if( TText_source == TTEXTSRC_485)
//	{
//		s485_Uart_write(str, strlen(str));
//	}
//	else
//	{
//		SIM800->send_text_sms( SIM800, Dtu_config.admin_Phone[TText_source - 1], str);
//		
//	}
	if( g_other_ack)
	{
		g_other_ack( str, g_ack_arg);
	}
	else
	{
		s485_Uart_write(str, strlen(str));
	}
}

static void dtu_conf(char *data)
{
	char *pcmd;
	char *parg;
	int 	i_data = 0;
	short		i = 0, j = 0;
	char		tmpbuf[8];
	char		com_Wordbits[4] = { '8', '9', 0, 0};
	char		com_stopbit[4] = { '1', '2',0,0};
	char		parity = 0;
	if( get_cmdtype() != CONFCMD_TYPE_ATC)
		return;
	
	pcmd = get_cmd();
	if( pcmd == NULL)
			return;
	
	while(1)
	{
		parg = get_firstarg();
		if( strcmp(pcmd ,"MODE") == 0)
		{
			i_data = MODE_END;
			if( parg)
				i_data = atoi( parg);
			if( *parg == '?')
			{
				sprintf( data, "%d", Dtu_config.work_mode);
				ack_str( data);
			}
			else if( i_data < MODE_END && i_data >= MODE_BEGIN)
			{
				Dtu_config.work_mode = i_data;
				strcpy( data, "OK");
				ack_str( data);
			}
			else
			{
				strcpy( data, "ERROR");
				ack_str( data);
			}
			goto exit;
		}
		
		else if( strcmp(pcmd ,"CONN") == 0)
		{
			i_data = 2;
			if( parg)
				i_data = atoi( parg);
			
			if( *parg == '?')
			{
				sprintf( data, "%d", Dtu_config.multiCent_mode);
				ack_str( data);
			}
			else if( i_data == 0 || i_data == 1)
			{
				Dtu_config.multiCent_mode = i_data;
				strcpy( data, "OK");
				ack_str( data);
			}
			else
			{
				strcpy( data, "ERROR");
				ack_str( data);
			}
			goto exit;
		}
		
		else if( strcmp(pcmd ,"SVDM") == 0 || strcmp(pcmd ,"SVIP") == 0)
		{
			if( parg == NULL)
			{
				
				goto exit;
			}
			if( parg[0] == '?')
			{
				memset( data, 0, CONFIG_BUF_LEN);
				for( i = 0; i < IPMUX_NUM; i++)
				{
					sprintf(tmpbuf, "%d,", i);
					strcat(data, tmpbuf);
					strcat(data, Dtu_config.DateCenter_ip[ i]);
					strcat(data, ",");
					if( Dtu_config.DateCenter_port[i])
						sprintf(tmpbuf, "%d,", Dtu_config.DateCenter_port[i]);
					else
						sprintf(tmpbuf, " ,");
					strcat(data, tmpbuf);
					strcat(data, Dtu_config.protocol[i]);
					strcat(data, ",");
					
				}
				
				ack_str(data);
				goto exit;
			}
			//N, ip, port, protocol
			
			switch( i)
			{
				case 0:
					i_data = atoi(parg);
					if( i_data < 0 || i_data >= IPMUX_NUM)
					{
						strcpy( data, "ERROR");
						ack_str( data);
						goto exit;
					}
					
					i ++;
					break;
				case 1:
					strcpy( Dtu_config.DateCenter_ip[ i_data], parg);
					i ++;
					break;
				case 2:
					Dtu_config.DateCenter_port[ i_data] = atoi(parg);
					i++;
					break;
				case 3:
					strcpy( Dtu_config.protocol[ i_data], parg);
					
					strcpy( data, "OK");
					ack_str( data);
					
					goto exit;
				default:
					strcpy( data, "ERROR");
					ack_str( data);
					goto exit;
			}
				
				
		}
		else if( strcmp(pcmd ,"DNIP") == 0)
		{
			if( parg[0] == '?')
			{
				if( check_ip( Dtu_config.dns_ip) == ERR_OK)
					sprintf( data, "%s",Dtu_config.dns_ip);
				else
					strcpy( data, "  ");
				
				ack_str( data);
			}
			else
			{
				if( check_ip( parg) != ERR_OK)
				{
					strcpy( data, "ERROR");
					ack_str( data);
				}
				else
				{
					strcpy( Dtu_config.dns_ip, parg);
					strcpy( data, "OK");
					ack_str( data);
				}
				
			}
			goto exit;
		}
		
		else if( strcmp(pcmd ,"ATBT") == 0)
		{
				
			if( parg == NULL)
			{
				strcpy( data, "OK");
				ack_str( data);
				goto exit;
			}
			if( parg[0] == '?')
			{
				i_data = 0;								
				sprintf( data, "%09d,%d,%s", Dtu_config.dtu_id, Dtu_config.hartbeat_timespan_s, Dtu_config.sim_NO );
				for( i = 0; i < ADMIN_PHNOE_NUM; i ++)
				{
					
					if( check_phoneNO( Dtu_config.admin_Phone[i]) == ERR_OK)
					{
						strcat( data,",");
						strcat( data, Dtu_config.admin_Phone[i]);
						
					}
					
				}
				ack_str( data);
				goto exit;
				
			}
			
			switch(i)
			{
				case 0:
					i_data = atoi( parg);
					Dtu_config.dtu_id = i_data;
					memset( Dtu_config.admin_Phone[0], 0, sizeof(Dtu_config.admin_Phone));
					i++;
					break;
				case 1:
					i_data = atoi( parg);
					Dtu_config.hartbeat_timespan_s = i_data;
					i++;
					break;
				case 2:
					if( check_phoneNO(parg) != ERR_OK)
					{
						strcpy( data, "ERROR");
						ack_str( data);
						goto exit;
					}
					copy_phoneNO( Dtu_config.sim_NO, parg);
					
					
					i++;
					i_data = 0;
					break;
				case 3:
					if( check_phoneNO(parg) != ERR_OK)
					{
						strcpy( data, "ERROR");
						ack_str( data);
						goto exit;
					}
					copy_phoneNO( Dtu_config.admin_Phone[ i_data], parg);
					i_data ++;
					
					if( i_data >= ADMIN_PHNOE_NUM)
					{
						strcpy( data, "OK");
						ack_str( data);
						goto exit;
					}
					break;
				default:
					strcpy( data, "ERROR");
					ack_str( data);
					goto exit;
					
			}		//switch
			
		}
		
		else if( strcmp(pcmd ,"PRNT") == 0)
		{
			if( parg[0] == '?')
			{
				if( Dtu_config.output_mode)
				{
					ack_str( "YES");
					
				}
				else
				{
					ack_str( "NO");
				}
				goto exit;
				
			}
			
			
			if( strcmp(parg ,"YES") == 0)
			{
				Dtu_config.output_mode = 1;
				strcpy( data, "OK");
				ack_str( data);
			}
			else if( strcmp(parg ,"NO") == 0)
			{
				Dtu_config.output_mode = 0;
				ack_str( "OK");
			}
			else
			{
				strcpy( data, "ERROR");
				ack_str( data);
			}
			goto exit;
		}
		
		else if( strcmp(pcmd ,"SCOM") == 0)
		{
			if( parg == NULL)
			{
				strcpy( data, "OK");
				ack_str( data);
				goto exit;
			}
			
			if( parg[0] == '?')
			{
				
				//stm32中，校验位要占据一个数据位
				if( Dtu_config.the_485cfg.USART_Parity == USART_Parity_No)
				{
					parity = 'N';
					i_data = 0;
				}
				else if( Dtu_config.the_485cfg.USART_Parity == USART_Parity_Odd)
				{
					parity = 'O';
					i_data = 1;
					
				}
				else if( Dtu_config.the_485cfg.USART_Parity == USART_Parity_Even)
				{
					parity = 'E';
					i_data = 1;
				}
				
				
				
				sprintf( data, "%d,%c,%c,%c", Dtu_config.the_485cfg.USART_BaudRate, \
												 com_Wordbits[Dtu_config.the_485cfg.USART_WordLength/0x1000] - i_data, \
												 parity, \
												 com_stopbit[Dtu_config.the_485cfg.USART_StopBits/0x2000]	);
	
				
				ack_str(data);
				
				goto exit;
				
			}
			
			switch(i)
			{
				case 0:
					i_data = atoi( parg);
					Dtu_config.the_485cfg.USART_BaudRate = i_data;
					i++;
					break;
				case 1:
					//stm32中，校验位要占据一个数据位
					if( parg[0] == '7')
					{
						Dtu_config.the_485cfg.USART_WordLength = USART_WordLength_8b;
					}
					else if( parg[0] == '8')
						Dtu_config.the_485cfg.USART_WordLength = USART_WordLength_9b;
					else
					{
						strcpy( data, "ERROR");
						ack_str( data);
						goto exit;
					}
					i++;
					break;
				case 2:

					if( parg[0] == 'N')
					{
						
						//stm32的数据位与校验位是算到一起的
						//所以用户如果配置成8位数据位，无校验；其实是就是指数据位是8位
						//stm32在无校验的情况下，只允许配置成8位数据位
						Dtu_config.the_485cfg.USART_WordLength = USART_WordLength_8b;
						Dtu_config.the_485cfg.USART_Parity = USART_Parity_No;
					}
					else if( parg[0] == 'E')
					{
						Dtu_config.the_485cfg.USART_Parity = USART_Parity_Even;
					}
					else if( parg[0] == 'O')
					{
						Dtu_config.the_485cfg.USART_Parity = USART_Parity_Odd;
					}
					else
					{
						strcpy( data, "ERROR");
						ack_str( data);
						goto exit;
					}
						
					i++;
					break;
				case 3:
					if( parg[0] == '1')
						Dtu_config.the_485cfg.USART_StopBits = USART_StopBits_1;
					else if( parg[0] == '2')
						Dtu_config.the_485cfg.USART_StopBits = USART_StopBits_2;
					else
					{
						strcpy( data, "ERROR");
						ack_str( data);
						goto exit;
					}
					i++;
					break;
				default:
					strcpy( data, "ERROR");
					ack_str( data);
					goto exit;
					
			}		//switch
			
		}
		
		else if( strcmp(pcmd ,"REGT") == 0)
		{
			if( *parg == '?')
			{
				if( strlen( Dtu_config.registry_package))
					sprintf( data, "%s", Dtu_config.registry_package);
				else
					sprintf( data," ");
				ack_str( data);
			}
			else
			{
				i =  strlen(parg);
				if( i > 29)
					i = 29;
				memset(  Dtu_config.registry_package, 0, sizeof( Dtu_config.registry_package));		
				memcpy( Dtu_config.registry_package, parg, i );
				strcat( Dtu_config.registry_package, "\r\n");
				strcpy( data, "OK");
				ack_str( data);
			}
			
			goto exit;
		}
		
		else if( strcmp(pcmd ,"HEAT") == 0)
		{
			if( *parg == '?')
			{
				
				if( strlen( Dtu_config.heatbeat_package))
					sprintf( data, "%s", Dtu_config.heatbeat_package);
				else
					sprintf( data," ");
				ack_str( data);
			}
			else
			{
				i =  strlen(parg);
				if( i > 31)
					i = 31;
				memset( Dtu_config.heatbeat_package, 0, sizeof( Dtu_config.heatbeat_package));		
				memcpy( Dtu_config.heatbeat_package, parg, i );
				strcpy( data, "OK");
				ack_str( data);
			}
			
			goto exit;
		}
		
		else if( strncmp(pcmd ,"VAPN",4) == 0)
		{
			if( parg == NULL)
			{
				strcpy( data, "OK");
				ack_str( data);
				goto exit;
			}
			if( parg[0] == '?')
			{
				get_apn( NULL, data);
//				strcat( data, ", ");
//				strcat( data, Dtu_config.apn_username);
//				strcat( data, ", ");
//				strcat( data, Dtu_config.apn_passwd);
				ack_str( data);
				return;	
				
				
			}
			else
			{
				if( strlen( parg) > 20)
				{
					strcpy( data, "ERROR");
					ack_str( data);
					goto exit;
				}
					
				if( i == 0)
				{
					strcpy( Dtu_config.apn, parg);
					
				}
				else if( i <= 2)
				{
					strcat( Dtu_config.apn, ",");
					strcat( Dtu_config.apn, parg);
				}
				i ++;
				
			}
			
			
		}
		
		else if( strcmp(pcmd ,"CNUM") == 0)
		{
			if( *parg == '?')
			{
//				SIM800->read_smscAddr( SIM800, Dtu_config.smscAddr);
				if( check_phoneNO( Dtu_config.smscAddr) == ERR_OK)
					ack_str( Dtu_config.smscAddr);
				else
					ack_str( " ");
			}
			else
			{
				if( check_phoneNO( parg) == ERR_OK)
				{
					strcpy( Dtu_config.smscAddr, parg);
					
				}
				else
				{
					memset( Dtu_config.smscAddr, 0, sizeof( Dtu_config.smscAddr));
				}
				
				strcpy( data, "OK");
				ack_str( data);
				
			}
			
			goto exit;
		}
		else if( strcmp(pcmd ,"RTUA") == 0)
		{
			if( *parg == '?')
			{
				sprintf( data, "%d", Dtu_config.rtu_addr);
				ack_str( data);
			}
			else
			{
				i_data = atoi( parg);
				if( i_data > 0)
				{
					Dtu_config.rtu_addr = i_data;
					strcpy( data, "OK");
					ack_str( data);
				}
				else
				{
					strcpy( data, "ERROR");
					ack_str( data);
				}
			}
			
			goto exit;
		}
		else if( strcmp(pcmd ,"SIGN") == 0)
		{
			
			if( *parg == '?')
			{
				sprintf( data, "%d,%d,%d", Dtu_config.chn_type[0],Dtu_config.chn_type[1],Dtu_config.chn_type[2] );
				ack_str( data);
				goto exit;
			}
			else
			{
				switch(i)
				{
					case 0:
						j = atoi( parg) - 1;
						if( j >2 || j < 0)
						{
							strcpy( data, "ERROR");
							ack_str( data);
							goto exit;
						}
						i++;
						break;
					case 1:
						i_data = atoi( parg);
						Dtu_config.chn_type[j] = i_data;
						strcpy( data, "OK");
						ack_str( data);
						goto exit;
					default:
						strcpy( data, "ERROR");
						ack_str( data);
						goto exit;
						
				}		//switch
				
				
			}
		}
		else if( strcmp(pcmd ,"RANG") == 0)
		{
			
			if( *parg == '?')
			{
				memset( data, 0 ,sizeof( CONFIG_BUF_LEN));
				
				for( i = 0; i < 3; i ++)
				{
					sprintf(tmpbuf, "%d,", i +1);
					strcat( data, tmpbuf);
					sprintf(tmpbuf, "%d,", Dtu_config.chn_type[i]);
					strcat( data, tmpbuf);
					sprintf(tmpbuf, "%d,", Dtu_config.sign_range[i].rangeH);
					strcat( data, tmpbuf);
					sprintf(tmpbuf, "%d,", Dtu_config.sign_range[i].rangeL);
					strcat( data, tmpbuf);
					sprintf(tmpbuf, "%d,", Dtu_config.sign_range[i].alarmH);
					strcat( data, tmpbuf);
					sprintf(tmpbuf, "%d,", Dtu_config.sign_range[i].alarmL);
					strcat( data, tmpbuf);
					
				}
				data[ strlen( data) -1] = '\0';		//去除最后一个逗号
				ack_str( data);
				goto exit;
			}
			else
			{
				switch(i)
				{
					case 0:
						j = atoi( parg) - 1;
						if( j >2 || j < 0)
						{
							strcpy( data, "ERROR");
							ack_str( data);
							goto exit;
						}
						i++;
						break;
					case 1:
						i_data = atoi( parg);
						Dtu_config.chn_type[j] = i_data;
						i++;
						break;
					case 2:
						i_data = atoi( parg);
						Dtu_config.sign_range[j].rangeH = i_data;
						i++;
						break;
					case 3:
						i_data = atoi( parg);
						Dtu_config.sign_range[j].rangeL = i_data;
						i++;
						break;
					case 4:
						i_data = atoi( parg);
						Dtu_config.sign_range[j].alarmH = i_data;
						i++;
						break;
					case 5:
						i_data = atoi( parg);
						Dtu_config.sign_range[j].alarmL = i_data;
						i++;
						strcpy( data, "OK");
						ack_str( data);
						goto exit;
					default:
						strcpy( data, "ERROR");
						ack_str( data);
						goto exit;
						
				}		//switch
				
				
			}
		}
		else if( strcmp(pcmd ,"FACT") == 0)
		{
			set_default(&Dtu_config);
			strcpy( data, "OK");
			ack_str( data);
			goto exit;
		}
		else if( strcmp(pcmd ,"RESET") == 0)
		{
			
			strcpy( data, "OK");
			ack_str( data);
			
			LED_run->destory(LED_run);
			fs_flush();
			if( g_shutdow)
				g_shutdow();
			os_reboot();			
			
			goto exit;
		}
		if( parg == NULL)
		{
			strcpy( data, "ERROR");
			ack_str( data);
			goto exit;
		}

		
	}

	exit:	
	fs_lseek( DtuCfg_file, WR_SEEK_SET, 0);
	fs_write( DtuCfg_file, (uint8_t *)&Dtu_config, sizeof( DtuCfg_t));
	fs_flush();
}

