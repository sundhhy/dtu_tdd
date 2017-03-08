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
//FUNCTION_SETTING(getInstance, SCFGetInstance);
FUNCTION_SETTING(createContext, createContext);
END_CTOR

//容纳状态的环境
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
FUNCTION_SETTING(setCurState, setCurState);
FUNCTION_SETTING(construct, Construct);
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
	this->curState = this->gprsSer485ProcessState;
	
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
	
	this->curState = this->gprsSer485ProcessState;
	
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
	
	this->curState = this->gprsSer485ProcessState;
	
	lw_oopc_delete( state_builder);
	
	return ERR_OK;
	
}

CTOR(PassThroughModeContext)
SUPER_CTOR(StateContext);
FUNCTION_SETTING(StateContext.initState, PThrInitState);
END_CTOR



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
	WorkState *state = ( WorkState *)Ser485ProcessState_new();
	state->forwardNet = ( BusinessProcess*)EmptyProcess_new();
	state->forwardSMS = ( BusinessProcess*)EmptyProcess_new();
	state->forwardSer485 = ( BusinessProcess*)EmptyProcess_new();
	state->configSystem = ( BusinessProcess*)EmptyProcess_new();
	state->modbusProcess = ( BusinessProcess*)ModbusBusiness_new();
	state->init( state, this->gprs, this->dataBuf, this->bufLen);
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
FUNCTION_SETTING(Builder.builderGprsCnntManagerState, LRTUBuildGprsConnectState);
FUNCTION_SETTING(Builder.buildGprsEventHandleState, LRTUBuildGprsEventHandleState);
FUNCTION_SETTING(Builder.buildGprsDealSMSState, LRTUBuildGprsDealSMSState);
FUNCTION_SETTING(Builder.builderSer485ProcessState, LRTUBuilderSer485ProcessState);
FUNCTION_SETTING(Builder.builderGprsHeatBeatState, LRTUBuilderGprsHeatBeatState);
FUNCTION_SETTING(Builder.builderGprsHeatBeatState, LRTUBuilderGprsCnntManagerState);
END_CTOR


WorkState* SMSModeBuildGprsSelfTestState(StateContext *this )
{
	return NULL;
	
}
WorkState*  SMSModeBuildGprsConnectState(StateContext *this )
{
	return NULL;
	
}
WorkState* SMSModeBuildGprsEventHandleState(StateContext *this )
{
	return NULL;
}
WorkState* SMSModeBuildGprsDealSMSState(StateContext *this )
{
	return NULL;
}
WorkState* SMSModeBuilderSer485ProcessState(StateContext *this )
{
	return NULL;
}
WorkState* SMSModeBuilderGprsHeatBeatState(StateContext *this )
{
	return NULL;
}
WorkState* SMSModeBuilderGprsCnntManagerState(StateContext *this )
{
	return NULL;
}

CTOR( SMSModeBuilder)

FUNCTION_SETTING(Builder.buildGprsSelfTestState, SMSModeBuildGprsSelfTestState);
FUNCTION_SETTING(Builder.builderGprsCnntManagerState, SMSModeBuildGprsConnectState);
FUNCTION_SETTING(Builder.buildGprsEventHandleState, SMSModeBuildGprsEventHandleState);
FUNCTION_SETTING(Builder.buildGprsDealSMSState, SMSModeBuildGprsDealSMSState);
FUNCTION_SETTING(Builder.builderSer485ProcessState, SMSModeBuilderSer485ProcessState);
FUNCTION_SETTING(Builder.builderGprsHeatBeatState, SMSModeBuilderGprsHeatBeatState);
FUNCTION_SETTING(Builder.builderGprsHeatBeatState, SMSModeBuilderGprsCnntManagerState);
END_CTOR

WorkState* RRTUBuildGprsSelfTestState(StateContext *this )
{
	return NULL;
	
}
WorkState*  RRTUBuildGprsConnectState(StateContext *this )
{
	return NULL;
	
}
WorkState* RRTUBuildGprsEventHandleState(StateContext *this )
{
	return NULL;
}
WorkState* RRTUBuildGprsDealSMSState(StateContext *this )
{
	return NULL;
}
WorkState* RRTUBuilderSer485ProcessState(StateContext *this )
{
	return NULL;
}
WorkState* RRTUBuilderGprsHeatBeatState(StateContext *this )
{
	return NULL;
}
WorkState* RRTUBuilderGprsCnntManagerState(StateContext *this )
{
	return NULL;
}

CTOR( RemoteRTUModeBuilder)
FUNCTION_SETTING(Builder.buildGprsSelfTestState, RRTUBuildGprsSelfTestState);
FUNCTION_SETTING(Builder.builderGprsCnntManagerState, RRTUBuildGprsConnectState);
FUNCTION_SETTING(Builder.buildGprsEventHandleState, RRTUBuildGprsEventHandleState);
FUNCTION_SETTING(Builder.buildGprsDealSMSState, RRTUBuildGprsDealSMSState);
FUNCTION_SETTING(Builder.builderSer485ProcessState, RRTUBuilderSer485ProcessState);
FUNCTION_SETTING(Builder.builderGprsHeatBeatState, RRTUBuilderGprsHeatBeatState);
FUNCTION_SETTING(Builder.builderGprsHeatBeatState, RRTUBuilderGprsCnntManagerState);
END_CTOR


WorkState* PassThroughModeBuildGprsSelfTestState(StateContext *this )
{
	return NULL;
	
}
WorkState*  PassThroughModeBuildGprsConnectState(StateContext *this )
{
	return NULL;
	
}
WorkState* PassThroughModeBuildGprsEventHandleState(StateContext *this )
{
	return NULL;
}
WorkState* PassThroughModeBuildGprsDealSMSState(StateContext *this )
{
	return NULL;
}
WorkState* PassThroughModeBuilderSer485ProcessState(StateContext *this )
{
	return NULL;
}
WorkState* PassThroughModeBuilderGprsHeatBeatState(StateContext *this )
{
	return NULL;
}
WorkState* PassThroughModeBuilderGprsCnntManagerState(StateContext *this )
{
	return NULL;
}

CTOR( PassThroughModeBuilder)
FUNCTION_SETTING(Builder.buildGprsSelfTestState, PassThroughModeBuildGprsSelfTestState);
FUNCTION_SETTING(Builder.builderGprsCnntManagerState, PassThroughModeBuildGprsConnectState);
FUNCTION_SETTING(Builder.buildGprsEventHandleState, PassThroughModeBuildGprsEventHandleState);
FUNCTION_SETTING(Builder.buildGprsDealSMSState, PassThroughModeBuildGprsDealSMSState);
FUNCTION_SETTING(Builder.builderSer485ProcessState, PassThroughModeBuilderSer485ProcessState);
FUNCTION_SETTING(Builder.builderGprsHeatBeatState, PassThroughModeBuilderGprsHeatBeatState);
FUNCTION_SETTING(Builder.builderGprsHeatBeatState, PassThroughModeBuilderGprsCnntManagerState);
END_CTOR


//工作状态

static void Debug_485( char *data)
{
	if( Dtu_config.output_mode)
	{
		
		s485_Uart_write(data, strlen(data) );
	}
	
	DPRINTF(" %s \n", data);
	
}

int WorkStateTestInit( WorkState *this, gprs_t *myGprs, char *buf, int bufLen)
{
	if( myGprs == NULL || buf == NULL)
		return ERR_BAD_PARAMETER;
	this->gprs = myGprs;
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






int GprsSelfTestRun( WorkState *this, StateContext *context)
{
	gprs_t	*this_gprs = this->gprs;
	
	
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

int Ser485ModbusAckCB( char *data, int len, void *arg)
{
	
	return s485_Uart_write( data, len);
}
int Ser485ProcessStateRun( WorkState *this, StateContext *context)
{
	GprsSelfTestState *self = SUB_PTR( this, WorkState, GprsSelfTestState);
	gprs_t	*this_gprs = this->gprs;
	int ret = 0;
	
	ret = s485_Uart_read( this->dataBuf, this->bufLen);
				
				
	if( ret <= 0)
	{
		context->setCurState( context, context->gprsEventHandleState);	
		return ERR_OK;
	}
	this->modbusProcess->process( this->dataBuf, ret, Ser485ModbusAckCB	, NULL) ;
	this->forwardNet->process( this->dataBuf, ret, NULL	, this_gprs) ;
	this->forwardSMS->process( this->dataBuf, ret, NULL	, this_gprs) ;
			
	context->setCurState( context, context->gprsHeatBeatState);	
	return 	ERR_OK;
				
}

CTOR( Ser485ProcessState)
SUPER_CTOR( WorkState);
FUNCTION_SETTING(WorkState.run, GprsSelfTestRun);
END_CTOR



//业务处理

int Emptyprocess( char *data, int len, hookFunc cb, void *cbArg)
{
	return ERR_UNKOWN;
}

CTOR( EmptyProcess)
FUNCTION_SETTING( BusinessProcess.process, Emptyprocess);
END_CTOR

int ModbusBusinessProcess( char *data, int len, hookFunc cb, void *cbArg)
{
	return ERR_OK;
}

CTOR( ModbusBusiness)
FUNCTION_SETTING( BusinessProcess.process, ModbusBusinessProcess);
END_CTOR

int ConfigSystemProcess( char *data, int len, hookFunc cb, void *cbArg)
{
	return ERR_OK;
}

CTOR( ConfigSystem)
FUNCTION_SETTING( BusinessProcess.process, ConfigSystemProcess);
END_CTOR

int ForwardSer485Process( char *data, int len, hookFunc cb, void *cbArg)
{
	return ERR_OK;
}

CTOR( ForwardSer485)
FUNCTION_SETTING( BusinessProcess.process, ForwardSer485Process);
END_CTOR

int ForwardNetProcess( char *data, int len, hookFunc cb, void *cbArg)
{
	return ERR_OK;
}

CTOR( ForwardNet)
FUNCTION_SETTING( BusinessProcess.process, ForwardNetProcess);
END_CTOR

int ForwardSMSProcess( char *data, int len, hookFunc cb, void *cbArg)
{
	return ERR_OK;
}

CTOR( ForwardSMS)
FUNCTION_SETTING( BusinessProcess.process, ForwardSMSProcess);
END_CTOR
