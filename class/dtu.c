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
#include "dtu.h"
#include "system.h"
#include "modbusRTU_cli.h"




char	DtuTempBuf[DTUTMP_BUF_LEN];

static WorkState *StateLine[ STATE_Total];

static void Debug_485( char *data)
{
	if( Dtu_config.output_mode)
	{
		
		s485_Uart_write(data, strlen(data) );
	}
	
	DPRINTF(" %s \n", data);
	
}

//创建不同工作模式下的环境
DtuContextFactory* DCFctGetInstance(void)
{
	
	static DtuContextFactory *dtuFact = NULL;
	if( dtuFact == NULL)
	{
		dtuFact = DtuContextFactory_new();
		
	}
	return dtuFact;
}

StateContext *DtucreateContext( int mode)
{
	StateContext *context;
	switch( mode)
	{
		case MODE_PASSTHROUGH:
			context =  ( StateContext *)PassThroughModeContext_new();
			break;
		case MODE_SMS:
			context =  ( StateContext *)SMSModeContext_new();
			break;
		case MODE_REMOTERTU:
			context =  ( StateContext *)RemoteRTUModeContext_new();
			break;
//		case MODE_LOCALRTU:
//			context =  ( StateContext *)LocalRTUModeContext_new();
//			break;
		default:
			context =  NULL;
			break;
	}
	return context;
	
}

CTOR(DtuContextFactory)
FUNCTION_SETTING( createContext, DtucreateContext);
END_CTOR




//容纳状态的环境

int StateContextInit( StateContext *this, char *buf, int bufLen)
{
	
	this->dataBuf = buf;
	this->bufLen = bufLen;
	return ERR_OK;
}

void setCurState( StateContext *this, int targetState)
{
	if( StateLine[ targetState])
		this->curState = StateLine[ targetState];
}

void switchToSmsMode( StateContext *this)
{
	
	//销毁不必要的工作状态
	if(  StateLine[ STATE_Connect])
	{
		lw_oopc_delete( StateLine[ STATE_Connect]);
		StateLine[ STATE_Connect] = NULL;
	}
	if(  StateLine[ STATE_EventHandle])
	{
		lw_oopc_delete( StateLine[ STATE_EventHandle]);
		StateLine[ STATE_EventHandle] = NULL;
	}
	if(  StateLine[ STATE_HeatBeatHandle])
	{
		lw_oopc_delete( StateLine[ STATE_HeatBeatHandle]);
		StateLine[ STATE_HeatBeatHandle] = NULL;
	}
	if(  StateLine[ STATE_CnntManager])
	{
		lw_oopc_delete( StateLine[ STATE_CnntManager]);
		StateLine[ STATE_CnntManager] = NULL;
	}
		

	//设置当前工作状态为短信处理
	this->curState = StateLine[ STATE_SMSHandle];
	
}

void nextState( StateContext *this, int myState)
{
	int i = 0;
	

	
	
	//切到这个状态之后的第一个存在的状态
	//如果除了本状态，其他状态都不存在，就是保持状态不变
	for( i = 0; i < STATE_Total; i ++)
	{
		
		//初始状态不应该在正常运行中出现
		//所以查询状时跳过初始状态
		myState ++;
		if( myState == STATE_Total)
			myState = 1;
		if( StateLine[ myState] )
		{
			this->curState = StateLine[ myState];
			break;
			
		}
		
	}
		
	
}

int Construct( StateContext *this, Builder *state_builder)
{
	
	
//	this->gprsSelfTestState = state_builder->buildGprsSelfTestState( this);
//	this->gprsConnectState = state_builder->buildGprsConnectState( this);
//	this->gprsEventHandleState = state_builder->buildGprsEventHandleState( this);
//	this->gprsDealSMSState = state_builder->buildGprsDealSMSState( this);
////	this->gprsSer485ProcessState = state_builder->builderSer485ProcessState( this);
//	this->gprsHeatBeatState = state_builder->builderGprsHeatBeatState( this);
//	this->gprsCnntManagerState = state_builder->builderGprsCnntManagerState( this);
//		StateLine[0]  = this->gprsSelfTestState;
//		StateLine[1]  = this->gprsConnectState;
//		StateLine[2]  = this->gprsEventHandleState;
//		StateLine[3]  = this->gprsDealSMSState;
//		StateLine[4]  = this->gprsHeatBeatState;
//		StateLine[5]  = this->gprsCnntManagerState;
		
	StateLine[STATE_SelfTest] = state_builder->buildGprsSelfTestState( this);
	StateLine[STATE_Connect] = state_builder->buildGprsConnectState( this);
	StateLine[STATE_EventHandle] = state_builder->buildGprsEventHandleState( this);
	StateLine[STATE_HeatBeatHandle] = state_builder->builderGprsHeatBeatState( this);
	StateLine[STATE_CnntManager] = state_builder->builderGprsCnntManagerState( this);
	StateLine[STATE_SMSHandle] = state_builder->buildGprsDealSMSState( this);

	this->curState = StateLine[STATE_SelfTest];
	
	return ERR_OK;
}

ABS_CTOR( StateContext)
FUNCTION_SETTING(init, StateContextInit);
FUNCTION_SETTING(nextState, nextState);
FUNCTION_SETTING(setCurState, setCurState);
FUNCTION_SETTING(construct, Construct);
FUNCTION_SETTING(switchToSmsMode, switchToSmsMode);

END_ABS_CTOR





//int LRTUInitState( StateContext *this)
//{
//	Builder *state_builder = ( Builder *)LocalRTUModeBuilder_new();
//	this->construct( this, state_builder);
//	this->curState = this->gprsSer485ProcessState;
//	
//	lw_oopc_delete( state_builder);
//	return ERR_OK;
//}

//CTOR(LocalRTUModeContext)
//SUPER_CTOR(StateContext);
//FUNCTION_SETTING(StateContext.initState, LRTUInitState);
//END_CTOR

int SMSInitState( StateContext *this)
{
	
	Builder *state_builder = ( Builder *)SMSModeBuilder_new();
	
	this->construct( this, state_builder);
//	this->curState = this->gprsSelfTestState;
	
	lw_oopc_delete( state_builder);
	return ERR_OK;

}


CTOR(SMSModeContext)
SUPER_CTOR(StateContext);
FUNCTION_SETTING(StateContext.initState, SMSInitState);
END_CTOR


int RRTUInitState( StateContext *this)
{
	Builder *state_builder = ( Builder *)RemoteRTUModeBuilder_new();
	
	this->construct( this, state_builder);
//	this->curState = this->gprsSelfTestState;
	
	lw_oopc_delete( state_builder);
	return ERR_OK;
	
}

CTOR(RemoteRTUModeContext)
SUPER_CTOR(StateContext);
FUNCTION_SETTING(StateContext.initState, RRTUInitState);
END_CTOR

int PThrInitState( StateContext *this)
{
	Builder *state_builder = ( Builder *)PassThroughModeBuilder_new();
	
	this->construct( this, state_builder);
//	this->curState = this->gprsSelfTestState;
	
	lw_oopc_delete( state_builder);
	
	return ERR_OK;
	
}

CTOR(PassThroughModeContext)
SUPER_CTOR(StateContext);
FUNCTION_SETTING(StateContext.initState, PThrInitState);
END_CTOR

//工作状态
int WorkStateTestInit( WorkState *this, char *buf, int bufLen)
{
	if(  buf == NULL)
		return ERR_BAD_PARAMETER;
	this->dataBuf = buf;
	this->bufLen = bufLen;
	return ERR_OK;
	
}

void WorkStatePrint( WorkState *this, char *str)
{
	
	Debug_485( str);
}

ABS_CTOR( WorkState)
FUNCTION_SETTING(init, WorkStateTestInit);
FUNCTION_SETTING(print, WorkStatePrint);
END_ABS_CTOR

///-----------------------------------------------------------------------------
int GprsSelfTestRun( WorkState *this, StateContext *context)
{
	gprs_t	*this_gprs = GprsGetInstance();
	
	this->print( this, "[STS] gprs selftest state \r\n");

	Led_level(LED_GPRS_CHECK);
	this_gprs->lock( this_gprs);


	if( this_gprs->check_simCard(this_gprs) == ERR_OK)
	{
		if(this_gprs->get_sim_info(this_gprs) == ERR_OK)
		{
			sprintf( this->dataBuf, "[STS] SGL=%d, PWR=%d mv\r\n", dsys.gprs.signal_strength,  dsys.gprs.voltage_mv);
//			this->print( this, this->dataBuf);
		}
		else
		{
			
			
			sprintf( this->dataBuf, "[STS] get_sim_info sim failed! ...\r\n");
//			this->print( this, this->dataBuf);
			
		}
		
//		sprintf( this->dataBuf, "detected sim succeed! ...");
		this->print( this, this->dataBuf);
		context->nextState( context, STATE_SelfTest);
		
	}
	else 
	{
		this->print( this, "[STS] detected sim failed,restart gprs\r\n");
		this_gprs->startup(this_gprs);
		context->setCurState( context, STATE_SelfTest);	
	}
	
	sprintf(this->dataBuf, "[STS] GSM=%d, GPRS=%d \r\n", dsys.gprs.status_gsm,  dsys.gprs.status_gprs);
	this->print(this, this->dataBuf);
	
	this_gprs->unlock( this_gprs);
//	this->print( this, "gprs selftest state finish\r\n");

//	context->nextState( context, STATE_SelfTest);
				
//	context->setCurState( context, context->gprsConnectState);	
	return 	ERR_OK;
				
}

CTOR( GprsSelfTestState)
SUPER_CTOR( WorkState);
FUNCTION_SETTING(WorkState.run, GprsSelfTestRun);
END_CTOR

///-----------------------------------------------------------------------------
//static int cntTry = 5;
int GprsConnectRun( WorkState *this, StateContext *context)
{
	gprs_t	*this_gprs = GprsGetInstance();
	short cnnt_seq = 0;
	short run = 1;
	int ret = 0;
	this->print( this, "[CNN]gprs cnnect state \r\n");
	Led_level(LED_GPRS_CNNTING);
	this_gprs->lock( this_gprs);
	GprsTcpCnnectBeagin();
	while( run)
	{
		sprintf( this->dataBuf, "[CNN] cnnnect DC :%d,%s,%d,%s ...", cnnt_seq,Dtu_config.DateCenter_ip[ cnnt_seq],\
			Dtu_config.DateCenter_port[cnnt_seq],Dtu_config.protocol[cnnt_seq] );
		this->print( this, this->dataBuf);
		
		ret = this_gprs->tcpip_cnnt( this_gprs, cnnt_seq,Dtu_config.protocol[cnnt_seq], \
				Dtu_config.DateCenter_ip[cnnt_seq], Dtu_config.DateCenter_port[cnnt_seq]);
		if( ret == ERR_OK)
		{
			while(1)
			{
				ret = this_gprs->sendto_tcp( this_gprs, cnnt_seq, Dtu_config.registry_package, strlen(Dtu_config.registry_package) );
				if( ret == ERR_OK)
				{
						
						this->print(this, "[CNN] succeed !\n");
						Led_level(LED_GPRS_RUN);
						if( Dtu_config.multiCent_mode == 0)
						{
							//启动心跳包的闹钟
							set_alarmclock_s( ALARM_GPRSLINK(0), Dtu_config.hartbeat_timespan_s);
							run = 0;
						}
						else
						{
							//启动心跳包的闹钟
							set_alarmclock_s( ALARM_GPRSLINK(cnnt_seq), Dtu_config.hartbeat_timespan_s);
						}
						break;
						
				}
				else if( ret == ERR_UNINITIALIZED )
				{
					this->print( this,"[CNN] can not send data !\n");
					break;
				}
				
			}	//while(1)
		}	//if( ret == ERR_OK)
		else if( ret == ERR_DEV_SICK)
		{
			this->print(this, "[CNN] SIM card can not work!\n");
			osDelay(2000);		//170719
			
			context->setCurState(context, STATE_SelfTest);	
			goto exit;
		}
		else if( ret == ERR_ADDR_ERROR )
		{
			//只有多中心模式下才去标记非法地址
			//主备模式下，合法的地址如果多次连接成功-断开 的循环也会返回这个错误，如果标记成错误地址，就再也不能连接了
			if(Dtu_config.multiCent_mode)
				Dtu_config.DateCenter_port[cnnt_seq] = -Dtu_config.DateCenter_port[cnnt_seq];
			Led_level(LED_GPRS_ERR);
			this->print( this,"[CNN] Addr error !\n");
		}
		else
		{
			
			this->print(this, "[CNN] connect failed\n");
		}
		cnnt_seq ++;
		osDelay(2000);		//170719
		if( cnnt_seq >= IPMUX_NUM)
		{
			
			cnnt_seq = 0;
			run = 0;
		}
	}	//while( run)		
	GprsTcpCnnectFinish();	
	this_gprs->unlock( this_gprs);
	
	context->nextState( context, STATE_Connect);
//	context->setCurState( context, context->gprsEventHandleState);	
	exit:
	return 	ERR_OK;
				
}

CTOR( GprsConnectState)
SUPER_CTOR( WorkState);
FUNCTION_SETTING(WorkState.run, GprsConnectRun);
END_CTOR

///-----------------------------------------------------------------------------
int TcpModbusAckCB( char *data, int len, void *arg)
{
	gprs_t	*this_gprs = GprsGetInstance();
	int 		line = *( int *)arg;
	int ret = 0;
	this_gprs->lock( this_gprs);

	ret = this_gprs->sendto_tcp( this_gprs, line, data, len);
	this_gprs->unlock( this_gprs);
	
	return ret;
}

static void SMSConfigSystem_ack( char *data, void *arg)
{
	int source = *(int *)arg;
	gprs_t	*this_gprs = GprsGetInstance();
	
	this_gprs->lock( this_gprs);

	this_gprs->send_text_sms( this_gprs, Dtu_config.admin_Phone[ source], data);
	this_gprs->unlock( this_gprs);
	
}



int GprsEventHandleRun( WorkState *this, StateContext *context)
{
	gprs_t	*this_gprs = GprsGetInstance();
	GprsEventHandleState	*self = SUB_PTR( this, WorkState, GprsEventHandleState);
	
	int				lszie = 0;
	short 			i = 0;
	short			safeCount = 0;
	char 			*pp;
	int 			ret = 0;
	int				smsSeq = 0;
	int 			smsSource = 0;
//	this->print( this, "gprs event handle state \r\n");

	
	if(dsys.gprs.cur_state == SHUTDOWN)
	{
		this->print(this, "[EHA] found SIM card shutdown!\n");
		osDelay(2000);		//170719
		
		context->setCurState(context, STATE_SelfTest);	
		goto evenExit;
	}
		
	while( 1)
	{
		if( safeCount > EVENT_MAX )
			break;
		safeCount ++;

		lszie = this->bufLen;
		ret = this_gprs->report_event( this_gprs,  this->dataBuf, &lszie);
		if( ret != ERR_OK)
		{	
			
			break;
		}
		this_gprs->lock( this_gprs);		
		ret = this_gprs->deal_tcprecv_event(this_gprs, this->dataBuf,  &lszie);
		this_gprs->unlock( this_gprs);
		if( ret >= 0)
		{
			//接收到数据重置闹钟，避免发送心跳报文
			set_alarmclock_s( ALARM_GPRSLINK(ret), Dtu_config.hartbeat_timespan_s);
			
			//在线模式下，需要使用短信配置系统之前，要下发SMSMODE来进入短信模式
			pp = strstr((const char*)this->dataBuf,"SMSMODE");
			if( pp)
			{
				this->print( this, "[EHA] switch to SMSMODE\r\n");
				this_gprs->tcpClose( this_gprs, ret);
				context->switchToSmsMode( context);
//				Dtu_config.work_mode = MODE_SMS;
	//					context->setCurState( context, context->gprsDealSMSState);	
				goto evenExit;
			}
			
			self->modbusProcess->process( this->dataBuf, lszie, TcpModbusAckCB	, &ret) ;
			self->forwardSer485->process( this->dataBuf, lszie, NULL, NULL);
			DPRINTF("[EHA] rx:[%d] %s \n", ret, this->dataBuf);
//			this_gprs->free_event( this_gprs, gprs_event);
			continue;
		}
				
		lszie = this->bufLen;
		memset( DtuTempBuf, 0, sizeof( DtuTempBuf));
		
		this_gprs->lock( this_gprs);	
		smsSeq = this_gprs->deal_smsrecv_event( this_gprs, this->dataBuf,  &lszie, DtuTempBuf);			
		this_gprs->unlock( this_gprs);
		if( smsSeq > 0)
		{
			for( i = 0; i < ADMIN_PHNOE_NUM; i ++)
			{
				if( compare_phoneNO( DtuTempBuf, Dtu_config.admin_Phone[i]) == 0)
				{
					smsSource =  i;
					ret = self->configSystem->process( this->dataBuf, lszie, ( hookFunc)SMSConfigSystem_ack, &smsSource);
					if( ret != ERR_OK)
						self->forwardSer485->process( this->dataBuf, lszie, NULL, NULL);
					
				}
					
			}
			this_gprs->delete_sms( this_gprs, smsSeq);
//			this_gprs->free_event( this_gprs, gprs_event);
			continue;
		}
				
		ret = this_gprs->deal_tcpclose_event( this_gprs);
		if( ret >= 0)
		{	
			Led_level(LED_GPRS_DISCNNT);
			sprintf( this->dataBuf, "[EHA] tcp close : %d ", ret);
			this->print( this, this->dataBuf);
			osDelay(10000);
		}
//		this_gprs->free_event( this_gprs, gprs_event);
					
	}	//while(1)
				
	context->nextState( context, STATE_EventHandle);
	
	evenExit:
	return 	ERR_OK;
				
}

CTOR( GprsEventHandleState)
SUPER_CTOR( WorkState);
FUNCTION_SETTING(WorkState.run, GprsEventHandleRun);
END_CTOR





///-----------------------------------------------------------------------------
int Ser485ModbusAckCB( char *data, int len, void *arg)
{
	
	return s485_Uart_write( data, len);
}

int GprsHeatBeatRun( WorkState *this, StateContext *context)
{
	gprs_t	*this_gprs = GprsGetInstance();
	int i = 0;
//	this->print( this, "gprs heatbeat state \r\n");

	
	for( i = 0; i < IPMUX_NUM; i ++)
	{
		if( Ringing(ALARM_GPRSLINK(i)) == ERR_OK)
		{
			set_alarmclock_s( ALARM_GPRSLINK(i), Dtu_config.hartbeat_timespan_s);
			this_gprs->lock( this_gprs);			
			this_gprs->sendto_tcp( this_gprs, i, Dtu_config.heatbeat_package, strlen( Dtu_config.heatbeat_package));	
			this_gprs->unlock( this_gprs);
		
		}	
	}

			
//	context->setCurState( context, context->gprsCnntManagerState);	
	context->nextState( context, STATE_HeatBeatHandle);
	return 	ERR_OK;
				
}

CTOR( GprsHeatBeatState)
SUPER_CTOR( WorkState);
FUNCTION_SETTING(WorkState.run, GprsHeatBeatRun);
END_CTOR
///-----------------------------------------------------------------------------


///-----------------------------------------------------------------------------

int  GprsCnntManagerRun( WorkState *this, StateContext *context)
{
	gprs_t	*this_gprs = GprsGetInstance();
	int cnntNum = 0;	
	int	safecount = 0;
	int	i;
//	this->print( this, "gprs cnnt manager state \r\n");

	//连接无法建立时，处理下短信
	//这样可以让用户通过短信配置系统
	cnntNum = this_gprs->get_firstCnt_seq(this_gprs);
	if( cnntNum < 0)
	{
		strcpy( this->dataBuf, "[CMN] None connnect, reconnect...");
		this->print( this, this->dataBuf);
		if(this_gprs->get_sim_info(this_gprs) != ERR_OK)
		{
			sprintf( this->dataBuf, "[CMN] get_sim_info sim failed! ...");
			this->print( this, this->dataBuf);
		}
		else
		{
			if(dsys.gprs.signal_strength < 10)
			{
				sprintf( this->dataBuf, "[CMN] bad signal %d !!!", dsys.gprs.signal_strength );
				this->print( this, this->dataBuf);
			}
			
			if(dsys.gprs.voltage_mv < 3000)
			{
				sprintf( this->dataBuf, "[CMN] low power  %d mv !!!", dsys.gprs.voltage_mv );
				this->print( this, this->dataBuf);
			}
		}
		//一个连接也没有的话，就把错误地址标识给去除掉
		for(i = 0; i < IPMUX_NUM; i++)
		{
			if(Dtu_config.DateCenter_port[i] < 0)
				Dtu_config.DateCenter_port[i] = -Dtu_config.DateCenter_port[i];
		}
		
		context->setCurState( context, STATE_SMSHandle );	
		return ERR_OK;
	}
	
	//多中心的话，对未连接上的链路进行链接
	if( Dtu_config.multiCent_mode)
	{
		
		this_gprs->lock( this_gprs);
		
		while( 1)
		{
			cnntNum = this_gprs->get_firstDiscnt_seq(this_gprs);
			if( cnntNum >= 0)
			{
//				sprintf( this->dataBuf, "cnnnect DC :%d,%s,%d,%s ...", cnntNum ,Dtu_config.DateCenter_ip[ cnntNum],\
//					Dtu_config.DateCenter_port[ cnntNum],Dtu_config.protocol[ cnntNum] );
//				this->print( this, this->dataBuf);
				if( this_gprs->tcpip_cnnt( this_gprs, cnntNum, Dtu_config.protocol[ cnntNum], Dtu_config.DateCenter_ip[cnntNum], Dtu_config.DateCenter_port[cnntNum]) == ERR_OK)
				{
					this_gprs->sendto_tcp( this_gprs, cnntNum, Dtu_config.registry_package, strlen(Dtu_config.registry_package) );
//					this->print( this," succeed !\n");
				}
//				else
//				{
//					this->print( this, " failed !\n");
//					
//				}
			}
			safecount ++;
			if( safecount > IPMUX_NUM)
				break;
		}

		this_gprs->unlock( this_gprs);		
	}		
	context->setCurState( context, STATE_EventHandle );	
	return 	ERR_OK;		
}

CTOR(  GprsCnntManagerState)
SUPER_CTOR( WorkState);
FUNCTION_SETTING(WorkState.run, GprsCnntManagerRun);
END_CTOR
///-----------------------------------------------------------------------------

int GprsDealSMSRun( WorkState *this, StateContext *context)
{
	gprs_t	*this_gprs = GprsGetInstance();
	GprsDealSMSState	*self = SUB_PTR( this, WorkState, GprsDealSMSState);
	static	uint8_t skip = 0;		//为了避免每次都无意义的查询
	
	int			lszie = 0;
	short 		i = 0;
	int				smsSeq = 0;
	int				ret = 0;
	int 			smsSource = 0;
	
	memset( DtuTempBuf, 0, sizeof( DtuTempBuf));
	

	Led_level(LED_GPRS_SMS);
	context->nextState( context, STATE_SMSHandle);
	
	if( skip)
	{
		skip --;
		return 	ERR_OK;
	}
	this->print( this, "[SMS] gprs deal sms state \r\n");
	skip = 255;
	this_gprs->lock( this_gprs);
	while( 1)
	{
		lszie = this->bufLen;
		smsSeq = this_gprs->read_phnNmbr_TextSMS( this_gprs, DtuTempBuf, this->dataBuf,   this->dataBuf, &lszie);				
		if( smsSeq >= 0)
		{
			//有收到短信下回就不跳过
			skip = 0;
			for( i = 0; i < ADMIN_PHNOE_NUM; i ++)
			{
				if( compare_phoneNO( DtuTempBuf, Dtu_config.admin_Phone[i]) == 0)
				{
					
					
					smsSource =  i;
					ret = self->configSystem->process( this->dataBuf, lszie, ( hookFunc)SMSConfigSystem_ack, &smsSource);
					if( ret != ERR_OK)
						self->forwardSer485->process( this->dataBuf, lszie, NULL, NULL);
					
					break;
				}
				
			}
			this_gprs->delete_sms( this_gprs, smsSeq);
		}	
		else
		{
			if( ret == ERR_UNINITIALIZED)
				context->setCurState( context,  STATE_SelfTest);	
			else 
				context->setCurState( context,  STATE_Connect);	
			break;
		}
	}	
	this_gprs->unlock( this_gprs);

	return 	ERR_OK;
				
}

CTOR( GprsDealSMSState)
SUPER_CTOR( WorkState);
FUNCTION_SETTING(WorkState.run, GprsDealSMSRun);
END_CTOR

WorkState* SMSModeBuildGprsSelfTestState(StateContext *this )
{
	
	
	
	WorkState *state = ( WorkState *)GprsSelfTestState_new();
	state->init( state, this->dataBuf, this->bufLen);
	return state;
	
}
WorkState*  SMSModeBuildGprsConnectState(StateContext *this )
{
	
//	WorkState *state = ( WorkState *)GprsConnectState_new();
//	state->init( state, this->dataBuf, this->bufLen);
//	return state;
	return NULL;
	
}
WorkState* SMSModeBuildGprsEventHandleState(StateContext *this )
{
	GprsEventHandleState *concretestate = GprsEventHandleState_new();
	WorkState *state = ( WorkState *)concretestate;
	
	concretestate->forwardNet = GetEmptyProcess();
	concretestate->forwardSMS = GetEmptyProcess();
	concretestate->forwardSer485 = GetForwardSer485();
	concretestate->configSystem = GetConfigSystem();
	concretestate->modbusProcess = GetEmptyProcess();
	
	state->init( state, this->dataBuf, this->bufLen);
	return state;
	
//	return NULL;
	
}
WorkState* SMSModeBuildGprsDealSMSState(StateContext *this )
{
	
	GprsDealSMSState *concretestate = GprsDealSMSState_new();
	WorkState *state = ( WorkState *)concretestate;
	
	concretestate->forwardSer485 = GetForwardSer485();
	concretestate->configSystem = GetConfigSystem();
	
	state->init( state, this->dataBuf, this->bufLen);
	
	
	return state;
}
//WorkState* SMSModeBuilderSer485ProcessState(StateContext *this )
//{
//	Ser485ProcessState *concretestate = Ser485ProcessState_new();
//	WorkState *state = ( WorkState *)concretestate;
//	
//	concretestate->forwardNet = GetForwardNet();
//	concretestate->forwardSMS = GetForwardSMS();
//	concretestate->modbusProcess = GetModbusBusiness();
//	
//	state->init( state, this->dataBuf, this->bufLen);
//	
//	return state;
//}
WorkState* SMSModeBuilderGprsHeatBeatState(StateContext *this )
{
	
//	WorkState *state = ( WorkState *)GprsHeatBeatState_new();
//	state->init( state, this->dataBuf, this->bufLen);
//	return state;
	
	return NULL;
}
WorkState* SMSModeBuilderGprsCnntManagerState(StateContext *this )
{
//	WorkState *state = ( WorkState *)GprsCnntManagerState_new();
//	state->init( state, this->dataBuf, this->bufLen);
//	return state;
	
	return NULL;
}

CTOR( SMSModeBuilder)

FUNCTION_SETTING(Builder.buildGprsSelfTestState, SMSModeBuildGprsSelfTestState);
FUNCTION_SETTING(Builder.buildGprsConnectState, SMSModeBuildGprsConnectState);
FUNCTION_SETTING(Builder.buildGprsEventHandleState, SMSModeBuildGprsEventHandleState);
FUNCTION_SETTING(Builder.buildGprsDealSMSState, SMSModeBuildGprsDealSMSState);
//FUNCTION_SETTING(Builder.builderSer485ProcessState, SMSModeBuilderSer485ProcessState);
FUNCTION_SETTING(Builder.builderGprsHeatBeatState, SMSModeBuilderGprsHeatBeatState);
FUNCTION_SETTING(Builder.builderGprsCnntManagerState, SMSModeBuilderGprsCnntManagerState);
END_CTOR

WorkState* RRTUBuildGprsSelfTestState(StateContext *this )
{
	WorkState *state = ( WorkState *)GprsSelfTestState_new();
	state->init( state, this->dataBuf, this->bufLen);
	return state;
	
}
WorkState*  RRTUBuildGprsConnectState(StateContext *this )
{
	WorkState *state = ( WorkState *)GprsConnectState_new();
	state->init( state, this->dataBuf, this->bufLen);
	return state;
	
}
WorkState* RRTUBuildGprsEventHandleState(StateContext *this )
{
	GprsEventHandleState *concretestate = GprsEventHandleState_new();
	WorkState *state = ( WorkState *)concretestate;
	
//	concretestate->forwardNet = GetForwardNet();
//	concretestate->forwardSMS = GetForwardSMS();
//	concretestate->forwardSer485 = GetForwardSer485();
//	concretestate->configSystem = GetConfigSystem();
//	concretestate->modbusProcess = GetModbusBusiness();
	
	
	concretestate->forwardNet = GetEmptyProcess();
	concretestate->forwardSMS = GetEmptyProcess();
	concretestate->forwardSer485 = GetEmptyProcess();
	concretestate->configSystem = GetConfigSystem();
	concretestate->modbusProcess = GetModbusBusiness();
	
	state->init( state, this->dataBuf, this->bufLen);
	return state;
}
WorkState* RRTUBuildGprsDealSMSState(StateContext *this )
{
	GprsDealSMSState *concretestate = GprsDealSMSState_new();
	WorkState *state = ( WorkState *)concretestate;
	
	concretestate->forwardSer485 = GetEmptyProcess();
	concretestate->configSystem = GetConfigSystem();

	state->init( state, this->dataBuf, this->bufLen);
	
	return state;
}

//WorkState* RRTUBuilderSer485ProcessState(StateContext *this )
//{
//	
//	Ser485ProcessState *concretestate = Ser485ProcessState_new();
//	WorkState *state = ( WorkState *)concretestate;
//	
//	concretestate->forwardNet = GetForwardNet();
//	concretestate->forwardSMS = GetForwardSMS();
//	concretestate->modbusProcess = GetModbusBusiness();
//	return state;
//}


WorkState* RRTUBuilderGprsHeatBeatState(StateContext *this )
{
	
	WorkState *state = ( WorkState *)GprsHeatBeatState_new();
	state->init( state, this->dataBuf, this->bufLen);
	return state;
}

WorkState* RRTUBuilderGprsCnntManagerState(StateContext *this )
{
	WorkState *state = ( WorkState *)GprsCnntManagerState_new();
	state->init( state, this->dataBuf, this->bufLen);
	return state;
}
CTOR( RemoteRTUModeBuilder)
FUNCTION_SETTING(Builder.buildGprsSelfTestState, RRTUBuildGprsSelfTestState);
FUNCTION_SETTING(Builder.buildGprsConnectState, RRTUBuildGprsConnectState);
FUNCTION_SETTING(Builder.buildGprsEventHandleState, RRTUBuildGprsEventHandleState);
FUNCTION_SETTING(Builder.buildGprsDealSMSState, RRTUBuildGprsDealSMSState);
//FUNCTION_SETTING(Builder.builderSer485ProcessState, RRTUBuilderSer485ProcessState);
FUNCTION_SETTING(Builder.builderGprsHeatBeatState, RRTUBuilderGprsHeatBeatState);
FUNCTION_SETTING(Builder.builderGprsCnntManagerState, RRTUBuilderGprsCnntManagerState);
END_CTOR


WorkState* PassThroughModeBuildGprsSelfTestState(StateContext *this )
{
	WorkState *state = ( WorkState *)GprsSelfTestState_new();
	state->init( state, this->dataBuf, this->bufLen);
	return state;
	
}
WorkState*  PassThroughModeBuildGprsConnectState(StateContext *this )
{
	WorkState *state = ( WorkState *)GprsConnectState_new();
	state->init( state, this->dataBuf, this->bufLen);
	return state;
	
}
WorkState* PassThroughModeBuildGprsEventHandleState(StateContext *this )
{
	GprsEventHandleState *concretestate = GprsEventHandleState_new();
	WorkState *state = ( WorkState *)concretestate;
	
	concretestate->forwardNet = GetForwardNet();
	concretestate->forwardSMS = GetEmptyProcess();
	concretestate->forwardSer485 = GetForwardSer485();
	concretestate->configSystem = GetConfigSystem();
	concretestate->modbusProcess = GetEmptyProcess();
	
	state->init( state, this->dataBuf, this->bufLen);
	return state;
}
WorkState* PassThroughModeBuildGprsDealSMSState(StateContext *this )
{
	
	GprsDealSMSState *concretestate = GprsDealSMSState_new();
	WorkState *state = ( WorkState *)concretestate;
	
	concretestate->forwardSer485 = GetEmptyProcess();
	concretestate->configSystem = GetConfigSystem();

	state->init( state, this->dataBuf, this->bufLen);
	
	
	return state;
}

//WorkState* PassThroughModeBuilderSer485ProcessState( StateContext *this )
//{
//	Ser485ProcessState *concretestate = Ser485ProcessState_new();
//	WorkState *state = ( WorkState *)concretestate;
//	
//	concretestate->forwardNet = GetForwardNet();
//	concretestate->forwardSMS = GetForwardSMS();
//	concretestate->modbusProcess = GetEmptyProcess();
//	return state;
//}

WorkState* PassThroughModeBuilderGprsHeatBeatState(StateContext *this )
{
	
	WorkState *state = ( WorkState *)GprsHeatBeatState_new();
	state->init( state, this->dataBuf, this->bufLen);
	return state;
}
WorkState* PassThroughModeBuilderGprsCnntManagerState(StateContext *this )
{
	WorkState *state = ( WorkState *)GprsCnntManagerState_new();
	state->init( state, this->dataBuf, this->bufLen);
	return state;
}







CTOR( PassThroughModeBuilder)
FUNCTION_SETTING(Builder.buildGprsSelfTestState, PassThroughModeBuildGprsSelfTestState);
FUNCTION_SETTING(Builder.buildGprsConnectState, PassThroughModeBuildGprsConnectState);
FUNCTION_SETTING(Builder.buildGprsEventHandleState, PassThroughModeBuildGprsEventHandleState);
FUNCTION_SETTING(Builder.buildGprsDealSMSState, PassThroughModeBuildGprsDealSMSState);
//FUNCTION_SETTING(Builder.builderSer485ProcessState, PassThroughModeBuilderSer485ProcessState);
FUNCTION_SETTING(Builder.builderGprsHeatBeatState, PassThroughModeBuilderGprsHeatBeatState);
FUNCTION_SETTING(Builder.builderGprsCnntManagerState, PassThroughModeBuilderGprsCnntManagerState);
END_CTOR





//业务处理

BusinessProcess *GetEmptyProcess(void)
{
	static EmptyProcess *singleton = NULL;
	if( singleton == NULL)
	{
		singleton = EmptyProcess_new();
		
	}
	return ( BusinessProcess *)singleton;
	
}

BusinessProcess *GetModbusBusiness(void)
{
	static ModbusBusiness *singleton = NULL;
	if( singleton == NULL)
	{
		singleton = ModbusBusiness_new();
		
	}
	return ( BusinessProcess *)singleton;
	
}

BusinessProcess *GetConfigSystem(void)
{
	static ConfigSystem *singleton = NULL;
	if( singleton == NULL)
	{
		singleton = ConfigSystem_new();
		
	}
	return ( BusinessProcess *)singleton;
	
}

BusinessProcess *GetForwardSer485(void)
{
	static ForwardSer485 *singleton = NULL;
	if( singleton == NULL)
	{
		singleton = ForwardSer485_new();
		
	}
	return ( BusinessProcess *)singleton;
	
}

BusinessProcess *GetForwardNet(void)
{
	static ForwardNet *singleton = NULL;
	if( singleton == NULL)
	{
		singleton = ForwardNet_new();
		
	}
	return ( BusinessProcess *)singleton;
	
}

BusinessProcess *GetForwardSMS(void)
{
	static ForwardSMS *singleton = NULL;
	if( singleton == NULL)
	{
		singleton = ForwardSMS_new();
		
	}
	return ( BusinessProcess *)singleton;
	
}

// 具体业务实现
int Emptyprocess( char *data, int len, hookFunc cb, void *arg)
{
	return ERR_UNKOWN;
}

CTOR( EmptyProcess)
FUNCTION_SETTING( BusinessProcess.process, Emptyprocess);
END_CTOR

int ModbusBusinessProcess( char *data, int len, hookFunc cb, void *arg)
{
	int lszie = 0;
	if( modbusRTU_getID( (uint8_t *)data) != Dtu_config.rtu_addr)
				return ERR_OK;
	lszie = modbusRTU_data( (uint8_t *)data, len, (uint8_t *)DtuTempBuf, sizeof( DtuTempBuf));
	
	if( cb)
		cb( DtuTempBuf, lszie, arg);
	return ERR_OK;
}

CTOR( ModbusBusiness)
FUNCTION_SETTING( BusinessProcess.process, ModbusBusinessProcess);
END_CTOR




int ConfigSystemProcess( char *data, int len, hookFunc cb, void *arg)
{
	other_ack configAck = ( other_ack)cb;
	return Config_server( data, configAck, arg);
}

CTOR( ConfigSystem)
FUNCTION_SETTING( BusinessProcess.process, ConfigSystemProcess);
END_CTOR

int ForwardSer485Process( char *data, int len, hookFunc cb, void *arg)
{
	int ret = 0;
	
	ret = s485_Uart_write( data, len);
	if( ret == ERR_DEV_TIMEOUT)
		osDelay(100);
	else
		osDelay(10);
	return ERR_OK;
}

CTOR( ForwardSer485)
FUNCTION_SETTING( BusinessProcess.process, ForwardSer485Process);
END_CTOR

int ForwardNetProcess( char *data, int len, hookFunc cb, void *arg)
{
	gprs_t	*this_gprs = GprsGetInstance();
	int i = 0;
//	this_gprs->lock( this_gprs);
	
	if( Dtu_config.multiCent_mode == 0)
	{
		//启动心跳包的闹钟
		set_alarmclock_s( ALARM_GPRSLINK(0), Dtu_config.hartbeat_timespan_s);
		
	}
	else
	{
		//启动心跳包的闹钟
		for( i = 0; i < IPMUX_NUM; i ++)
			set_alarmclock_s( ALARM_GPRSLINK(i), Dtu_config.hartbeat_timespan_s);
	}
	
	

	this_gprs->sendto_tcp_buf( this_gprs, data, len);
//	this_gprs->unlock( this_gprs);
	
	return ERR_OK;
}

CTOR( ForwardNet)
FUNCTION_SETTING( BusinessProcess.process, ForwardNetProcess);
END_CTOR

int ForwardSMSProcess( char *data, int len, hookFunc cb, void *arg)
{
	gprs_t	*this_gprs = GprsGetInstance();
	int	ret;
	short i = 0;
	short count = 0;
	
	this_gprs->lock( this_gprs);
	for( i = 0; i < ADMIN_PHNOE_NUM; )
	{
		ret = this_gprs->send_text_sms( this_gprs, Dtu_config.admin_Phone[i], data);
		if( ret == ERR_FAIL)
		{
			count ++;
			if( count > 3)	//重试3次
			{
				i ++;
				count = 0;
			}
			
		}
		
		else 
		{
			i ++;
			count = 0;
		}
	}
	this_gprs->unlock( this_gprs);
	
	return ERR_OK;
}

CTOR( ForwardSMS)
FUNCTION_SETTING( BusinessProcess.process, ForwardSMSProcess);
END_CTOR
