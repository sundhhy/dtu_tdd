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

#include "modbusRTU_cli.h"
#define DTUTMP_BUF_LEN		64
char	DtuTempBuf[DTUTMP_BUF_LEN];

static void Debug_485( char *data)
{
	if( Dtu_config.output_mode)
	{
		
		s485_Uart_write(data, strlen(data) );
	}
	
	DPRINTF(" %s \n", data);
	
}

//创建不同工作模式下的环境
StateContextFactory* SCFGetInstance(void)
{
	
	static StateContextFactory *scf = NULL;
	if( scf == NULL)
	{
		scf = StateContextFactory_new();
		
	}
	return scf;
}

StateContext *createContext( int mode)
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
		case MODE_LOCALRTU:
			context =  ( StateContext *)LocalRTUModeContext_new();
			break;
		default:
			context =  NULL;
			break;
	}
	return context;
	
}

CTOR(StateContextFactory)
FUNCTION_SETTING( createContext, createContext);
END_CTOR

//容纳状态的环境

int StateContextInit( StateContext *this, char *buf, int bufLen)
{
	
	this->dataBuf = buf;
	this->bufLen = bufLen;
	return ERR_OK;
}

void setCurState( StateContext *this, WorkState *state)
{
	if( state)
		this->curState = state;
}

int Construct( StateContext *this, Builder *state_builder)
{
	
	
	this->gprsSelfTestState = state_builder->buildGprsSelfTestState( this);
	this->gprsConnectState = state_builder->buildGprsConnectState( this);
	this->gprsEventHandleState = state_builder->buildGprsEventHandleState( this);
	this->gprsDealSMSState = state_builder->buildGprsDealSMSState( this);
	this->gprsSer485ProcessState = state_builder->builderSer485ProcessState( this);
	this->gprsHeatBeatState = state_builder->builderGprsHeatBeatState( this);
	this->gprsCnntManagerState = state_builder->builderGprsCnntManagerState( this);
	
	
	return ERR_OK;
}

ABS_CTOR( StateContext)
FUNCTION_SETTING( init, StateContextInit);
FUNCTION_SETTING( setCurState, setCurState);
FUNCTION_SETTING( construct, Construct);
END_ABS_CTOR





int LRTUInitState( StateContext *this)
{
	Builder *state_builder = ( Builder *)LocalRTUModeBuilder_new();
	this->construct( this, state_builder);
	this->curState = this->gprsSer485ProcessState;
	
	lw_oopc_delete( state_builder);
	return ERR_OK;
}

CTOR(LocalRTUModeContext)
SUPER_CTOR(StateContext);
FUNCTION_SETTING(StateContext.initState, LRTUInitState);
END_CTOR

int SMSInitState( StateContext *this)
{
	
	Builder *state_builder = ( Builder *)SMSModeBuilder_new();
	
	this->construct( this, state_builder);
	this->curState = this->gprsSelfTestState;
	
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
	this->curState = this->gprsSelfTestState;
	
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
	this->curState = this->gprsSelfTestState;
	
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
	
	
	if( this_gprs->check_simCard(this_gprs) == ERR_OK)
	{
		sprintf( this->dataBuf, "detected sim succeed! ...");
		this->print( this, this->dataBuf);
		
	}
	else 
	{
		
		this_gprs->startup(this_gprs);
	}
				
	context->setCurState( context, context->gprsConnectState);	
	return 	ERR_OK;
				
}

CTOR( GprsSelfTestState)
SUPER_CTOR( WorkState);
FUNCTION_SETTING(WorkState.run, GprsSelfTestRun);
END_CTOR

///-----------------------------------------------------------------------------
int GprsConnectRun( WorkState *this, StateContext *context)
{
	gprs_t	*this_gprs = GprsGetInstance();
	short cnnt_seq = 0;
	short run = 1;
	int ret = 0;
	
	while( run)
	{
		sprintf( this->dataBuf, "cnnnect DC :%d,%s,%d,%s ...", cnnt_seq,Dtu_config.DateCenter_ip[ cnnt_seq],\
			Dtu_config.DateCenter_port[cnnt_seq],Dtu_config.protocol[cnnt_seq] );
		this->print( this, this->dataBuf);
		
		ret = this_gprs->tcpip_cnnt( this_gprs, cnnt_seq,Dtu_config.protocol[cnnt_seq], Dtu_config.DateCenter_ip[cnnt_seq], Dtu_config.DateCenter_port[cnnt_seq]);
		if( ret == ERR_OK)
		{
			while(1)
			{
				ret = this_gprs->sendto_tcp( this_gprs, cnnt_seq, Dtu_config.registry_package, strlen(Dtu_config.registry_package) );
				if( ret == ERR_OK)
				{
						//启动心跳包的闹钟
						set_alarmclock_s( ALARM_GPRSLINK(cnnt_seq), Dtu_config.hartbeat_timespan_s);
						this->print(this, "succeed !\n");
						if( Dtu_config.multiCent_mode == 0)
							run = 0;
						break;
						
				}
				else if( ret == ERR_UNINITIALIZED )
				{
					this->print( this,"can not send data !\n");
					break;
				}
			}	//while(1)
		}	//if( ret == ERR_OK)
		cnnt_seq ++;
		if( cnnt_seq >= IPMUX_NUM)
		{
			
			cnnt_seq = 0;
			run = 0;
		}
	}	//while( run)				
	context->setCurState( context, context->gprsEventHandleState);	
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
	
	return this_gprs->sendto_tcp( this_gprs, line, data, len);
}

static void SMSConfigSystem_ack( char *data, void *arg)
{
	int source = *(int *)arg;
	gprs_t	*this_gprs = GprsGetInstance();
	
	this_gprs->send_text_sms( this_gprs, Dtu_config.admin_Phone[ source], data);
}



int GprsEventHandleRun( WorkState *this, StateContext *context)
{
	gprs_t	*this_gprs = GprsGetInstance();
	GprsEventHandleState	*self = SUB_PTR( this, WorkState, GprsEventHandleState);
	
	int				lszie = 0;
	int 			i = 0;
	void 			*gprs_event;
	int 			ret = 0;
	int 			smsSource = 0;
	
	while( 1)
	{
			lszie = this->bufLen;
			ret = this_gprs->report_event( this_gprs, &gprs_event, this->dataBuf, &lszie);
			if( ret != ERR_OK)
			{	
				
				break;
			}
			
			ret = this_gprs->deal_tcprecv_event( this_gprs, gprs_event, this->dataBuf,  &lszie);
			if( ret >= 0)
			{
				//接收到数据重置闹钟，避免发送心跳报文
				set_alarmclock_s( ALARM_GPRSLINK(ret), Dtu_config.hartbeat_timespan_s);
				
				self->modbusProcess->process( this->dataBuf, lszie, TcpModbusAckCB	, &ret) ;
				self->forwardSer485->process( this->dataBuf, lszie, NULL, NULL);
				DPRINTF("rx:[%d] %s \n", ret, this->dataBuf);
				this_gprs->free_event( this_gprs, gprs_event);
				continue;
			}
					
			lszie = this->bufLen;
			memset( DtuTempBuf, 0, sizeof( DtuTempBuf));
			
			ret = this_gprs->deal_smsrecv_event( this_gprs, gprs_event, this->dataBuf,  &lszie, DtuTempBuf);			
			
			if( ret > 0)
			{
				for( i = 0; i < ADMIN_PHNOE_NUM; i ++)
				{
					if( compare_phoneNO( DtuTempBuf, Dtu_config.admin_Phone[i]) == 0)
					{
						smsSource =  i;
						self->configSystem->process( this->dataBuf, lszie, ( hookFunc)SMSConfigSystem_ack, &smsSource);
						self->forwardSer485->process( this->dataBuf, lszie, NULL, NULL);
						
					}
						
				}
				this_gprs->delete_sms( this_gprs, ret);
				this_gprs->free_event( this_gprs, gprs_event);
				continue;
			}
					
			ret = this_gprs->deal_tcpclose_event( this_gprs, gprs_event);
			if( ret >= 0)
			{
				sprintf( this->dataBuf, "tcp close : %d ", ret);
				this->print( this, this->dataBuf);
			}
			this_gprs->free_event( this_gprs, gprs_event);
					
	}	//while(1)
				
	context->setCurState( context, context->gprsDealSMSState);	
	return 	ERR_OK;
				
}

CTOR( GprsEventHandleState)
SUPER_CTOR( WorkState);
FUNCTION_SETTING(WorkState.run, GprsEventHandleRun);
END_CTOR


///-----------------------------------------------------------------------------

int GprsDealSMSRun( WorkState *this, StateContext *context)
{
	gprs_t	*this_gprs = GprsGetInstance();
	GprsDealSMSState	*self = SUB_PTR( this, WorkState, GprsDealSMSState);
	
	int			lszie = 0;
	short 		i = 0;
	int				ret = 0;
	int 			smsSource = 0;
	
	lszie = this->bufLen;
	memset( DtuTempBuf, 0, sizeof( DtuTempBuf));
	
	ret = this_gprs->read_phnNmbr_TextSMS( this_gprs, DtuTempBuf, this->dataBuf,   this->dataBuf, &lszie);				
	if( ret >= 0)
	{
		for( i = 0; i < ADMIN_PHNOE_NUM; i ++)
		{
			if( compare_phoneNO( DtuTempBuf, Dtu_config.admin_Phone[i]) == 0)
			{
				
				smsSource =  i;
				self->configSystem->process( this->dataBuf, lszie, ( hookFunc)SMSConfigSystem_ack, &smsSource);
				self->forwardSer485->process( this->dataBuf, lszie, NULL, NULL);
				
				
				break;
			}
			
		}
		this_gprs->delete_sms( this_gprs, ret);
	}	
	
	context->setCurState( context, context->gprsSer485ProcessState);	
	return 	ERR_OK;
				
}

CTOR( GprsDealSMSState)
SUPER_CTOR( WorkState);
FUNCTION_SETTING(WorkState.run, GprsDealSMSRun);
END_CTOR


///-----------------------------------------------------------------------------
int Ser485ModbusAckCB( char *data, int len, void *arg)
{
	
	return s485_Uart_write( data, len);
}
int Ser485ProcessStateRun( WorkState *this, StateContext *context)
{
	Ser485ProcessState *self = SUB_PTR( this, WorkState, Ser485ProcessState);
	gprs_t	*this_gprs = GprsGetInstance();
	int ret = 0;
	
	ret = s485_Uart_read( this->dataBuf, this->bufLen);
				
				
	if( ret <= 0)
	{
		context->setCurState( context, context->gprsEventHandleState);	
		return ERR_OK;
	}
	self->modbusProcess->process( this->dataBuf, ret, Ser485ModbusAckCB	, NULL) ;
	self->forwardNet->process( this->dataBuf, ret, NULL	, this_gprs) ;
	self->forwardSMS->process( this->dataBuf, ret, NULL	, this_gprs) ;
			
	context->setCurState( context, context->gprsHeatBeatState);	
	return 	ERR_OK;
				
}

CTOR( Ser485ProcessState)
SUPER_CTOR( WorkState);
FUNCTION_SETTING(WorkState.run, GprsSelfTestRun);
END_CTOR
///-----------------------------------------------------------------------------

int GprsHeatBeatRun( WorkState *this, StateContext *context)
{
	gprs_t	*this_gprs = GprsGetInstance();
	int i = 0;
	
	
	for( i = 0; i < IPMUX_NUM; i ++)
	{
		if( Ringing(ALARM_GPRSLINK(i)) == ERR_OK)
		{
			set_alarmclock_s( ALARM_GPRSLINK(i), Dtu_config.hartbeat_timespan_s);
			this_gprs->sendto_tcp( this_gprs, i, Dtu_config.heatbeat_package, strlen( Dtu_config.heatbeat_package));	
		}	
	}

			
	context->setCurState( context, context->gprsCnntManagerState);	
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
	
	
	if( Dtu_config.multiCent_mode == 0)
	{
		cnntNum = this_gprs->get_firstCnt_seq(this_gprs);
		if( cnntNum >= 0)
		{
			context->setCurState( context, context->gprsEventHandleState);	
		}
		else
		{
			
			strcpy( this->dataBuf, "None connnect, reconnect...");
			this->print( this, this->dataBuf);
			context->setCurState( context, context->gprsSelfTestState);	
		}
		return 	ERR_OK;
	}
	else 
	{
		cnntNum = this_gprs->get_firstDiscnt_seq(this_gprs);
		if( cnntNum >= 0)
		{
			sprintf( this->dataBuf, "cnnnect DC :%d,%s,%d,%s ...", cnntNum ,Dtu_config.DateCenter_ip[ cnntNum],\
				Dtu_config.DateCenter_port[ cnntNum],Dtu_config.protocol[ cnntNum] );
			this->print( this, this->dataBuf);
			if( this_gprs->tcpip_cnnt( this_gprs, cnntNum, Dtu_config.protocol[ cnntNum], Dtu_config.DateCenter_ip[cnntNum], Dtu_config.DateCenter_port[cnntNum]) == ERR_OK)
			{
				this->print( this," succeed !\n");
			}
			else
			{
				this->print( this, " failed !\n");
				
			}	
		}
		context->setCurState( context, context->gprsEventHandleState);	
		return 	ERR_OK;			
	}		
}

CTOR(  GprsCnntManagerState)
SUPER_CTOR( WorkState);
FUNCTION_SETTING(WorkState.run, GprsCnntManagerRun);
END_CTOR
///-----------------------------------------------------------------------------

//建造者

WorkState* LRTUBuildGprsSelfTestState( StateContext *this )
{
	return NULL;
	
}
WorkState*  LRTUBuildGprsConnectState( StateContext *this )
{
	return NULL;
	
}
WorkState* LRTUBuildGprsEventHandleState(StateContext *this )
{
	return NULL;
}
WorkState* LRTUBuildGprsDealSMSState(StateContext *this )
{
	return NULL;
}
WorkState* LRTUBuilderSer485ProcessState(StateContext *this )
{
	Ser485ProcessState *concretestate = Ser485ProcessState_new();
	WorkState *state = ( WorkState *)concretestate;
	
	concretestate->forwardNet = GetEmptyProcess();
	concretestate->forwardSMS = GetEmptyProcess();
	concretestate->modbusProcess = GetModbusBusiness();
	
	state->init( state, this->dataBuf, this->bufLen);
	return state;
}
WorkState* LRTUBuilderGprsHeatBeatState(StateContext *this )
{
	return NULL;
}
WorkState* LRTUBuilderGprsCnntManagerState(StateContext *this )
{
	return NULL;
}

CTOR( LocalRTUModeBuilder)

FUNCTION_SETTING(Builder.buildGprsSelfTestState, LRTUBuildGprsSelfTestState);
FUNCTION_SETTING(Builder.buildGprsConnectState, LRTUBuildGprsConnectState);
FUNCTION_SETTING(Builder.buildGprsEventHandleState, LRTUBuildGprsEventHandleState);
FUNCTION_SETTING(Builder.buildGprsDealSMSState, LRTUBuildGprsDealSMSState);
FUNCTION_SETTING(Builder.builderSer485ProcessState, LRTUBuilderSer485ProcessState);
FUNCTION_SETTING(Builder.builderGprsHeatBeatState, LRTUBuilderGprsHeatBeatState);
FUNCTION_SETTING(Builder.builderGprsCnntManagerState, LRTUBuilderGprsCnntManagerState);
END_CTOR


WorkState* SMSModeBuildGprsSelfTestState(StateContext *this )
{
	
	
	
	WorkState *state = ( WorkState *)GprsSelfTestState_new();
	state->init( state, this->dataBuf, this->bufLen);
	return state;
	
}
WorkState*  SMSModeBuildGprsConnectState(StateContext *this )
{
	
	WorkState *state = ( WorkState *)GprsConnectState_new();
	state->init( state, this->dataBuf, this->bufLen);
	return state;
	
}
WorkState* SMSModeBuildGprsEventHandleState(StateContext *this )
{
	GprsEventHandleState *concretestate = GprsEventHandleState_new();
	WorkState *state = ( WorkState *)concretestate;
	
	concretestate->forwardNet = GetForwardNet();
	concretestate->forwardSMS = GetForwardSMS();
	concretestate->forwardSer485 = GetForwardSer485();
	concretestate->configSystem = GetConfigSystem();
	concretestate->modbusProcess = GetEmptyProcess();
	
	state->init( state, this->dataBuf, this->bufLen);
	return state;
	
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
WorkState* SMSModeBuilderSer485ProcessState(StateContext *this )
{
	Ser485ProcessState *concretestate = Ser485ProcessState_new();
	WorkState *state = ( WorkState *)concretestate;
	
	concretestate->forwardNet = GetForwardNet();
	concretestate->forwardSMS = GetForwardSMS();
	concretestate->modbusProcess = GetModbusBusiness();
	
	state->init( state, this->dataBuf, this->bufLen);
	
	return state;
}
WorkState* SMSModeBuilderGprsHeatBeatState(StateContext *this )
{
	
	WorkState *state = ( WorkState *)GprsHeatBeatState_new();
	state->init( state, this->dataBuf, this->bufLen);
	return state;
}
WorkState* SMSModeBuilderGprsCnntManagerState(StateContext *this )
{
	WorkState *state = ( WorkState *)GprsCnntManagerState_new();
	state->init( state, this->dataBuf, this->bufLen);
	return state;
}

CTOR( SMSModeBuilder)

FUNCTION_SETTING(Builder.buildGprsSelfTestState, SMSModeBuildGprsSelfTestState);
FUNCTION_SETTING(Builder.buildGprsConnectState, SMSModeBuildGprsConnectState);
FUNCTION_SETTING(Builder.buildGprsEventHandleState, SMSModeBuildGprsEventHandleState);
FUNCTION_SETTING(Builder.buildGprsDealSMSState, SMSModeBuildGprsDealSMSState);
FUNCTION_SETTING(Builder.builderSer485ProcessState, SMSModeBuilderSer485ProcessState);
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
	
	concretestate->forwardNet = GetForwardNet();
	concretestate->forwardSMS = GetForwardSMS();
	concretestate->forwardSer485 = GetForwardSer485();
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

WorkState* RRTUBuilderSer485ProcessState(StateContext *this )
{
	
	Ser485ProcessState *concretestate = Ser485ProcessState_new();
	WorkState *state = ( WorkState *)concretestate;
	
	concretestate->forwardNet = GetForwardNet();
	concretestate->forwardSMS = GetForwardSMS();
	concretestate->modbusProcess = GetModbusBusiness();
	return state;
}


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
FUNCTION_SETTING(Builder.builderSer485ProcessState, RRTUBuilderSer485ProcessState);
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
	concretestate->forwardSMS = GetConfigSystem();
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

WorkState* PassThroughModeBuilderSer485ProcessState( StateContext *this )
{
	Ser485ProcessState *concretestate = Ser485ProcessState_new();
	WorkState *state = ( WorkState *)concretestate;
	
	concretestate->forwardNet = GetForwardNet();
	concretestate->forwardSMS = GetForwardSMS();
	concretestate->modbusProcess = GetEmptyProcess();
	return state;
}

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
FUNCTION_SETTING(Builder.builderSer485ProcessState, PassThroughModeBuilderSer485ProcessState);
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
	Config_server( data, configAck, arg);
	return ERR_OK;
}

CTOR( ConfigSystem)
FUNCTION_SETTING( BusinessProcess.process, ConfigSystemProcess);
END_CTOR

int ForwardSer485Process( char *data, int len, hookFunc cb, void *arg)
{
	s485_Uart_write( data, len);
	return ERR_OK;
}

CTOR( ForwardSer485)
FUNCTION_SETTING( BusinessProcess.process, ForwardSer485Process);
END_CTOR

int ForwardNetProcess( char *data, int len, hookFunc cb, void *arg)
{
	gprs_t	*this_gprs = GprsGetInstance();
	int i = 0;
	
	for( i = 0; i < IPMUX_NUM; i ++)
		this_gprs->sendto_tcp( this_gprs, i, data, len);
	
	return ERR_OK;
}

CTOR( ForwardNet)
FUNCTION_SETTING( BusinessProcess.process, ForwardNetProcess);
END_CTOR

int ForwardSMSProcess( char *data, int len, hookFunc cb, void *arg)
{
	gprs_t	*this_gprs = GprsGetInstance();
	short i = 0;
	short count = 0;
	
	for( i = 0; i < ADMIN_PHNOE_NUM; )
	{
		if( this_gprs->send_text_sms( this_gprs, Dtu_config.admin_Phone[i], data) == ERR_FAIL)
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
	
	
	return ERR_OK;
}

CTOR( ForwardSMS)
FUNCTION_SETTING( BusinessProcess.process, ForwardSMSProcess);
END_CTOR
