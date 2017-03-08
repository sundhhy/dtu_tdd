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
FUNCTION_SETTING(getInstance, SCFGetInstance);
FUNCTION_SETTING(createContext, createContext);
END_CTOR

//容纳状态的环境
void setCurState( StateContext *this, WorkState *state)
{
	if( state)
		this->curState = state;
}

ABS_CTOR( StateContext)
FUNCTION_SETTING(setCurState, setCurState);
END_ABS_CTOR





int LRTUConstruct( StateContext *this)
{
	Builder *state_builder = ( Builder *)LocalRTUModeBuilder_new();
	
	this->gprsSelfTestState = state_builder->buildGprsSelfTestState();
	this->gprsConnectStatee = state_builder->buildGprsConnectState();
	this->gprsEventHandleState = state_builder->buildGprsEventHandleState();
	this->gprsDealSMSState = state_builder->buildGprsDealSMSState();
	this->gprsSer485ProcessState = state_builder->builderSer485ProcessState();
	this->gprsHeatBeatState = state_builder->builderGprsHeatBeatState();
	this->gprsCnntManagerState = state_builder->builderGprsCnntManagerState();
	
	this->curState = this->gprsSer485ProcessState;
	
	lw_oopc_delete( state_builder);
	return ERR_OK;
}

CTOR(LocalRTUModeContext)
FUNCTION_SETTING(StateContext.construct, LRTUConstruct);
END_CTOR

int SMSConstruct( StateContext *this)
{
	
	Builder *state_builder = ( Builder *)SMSModeBuilder_new();
	
	this->gprsSelfTestState = state_builder->buildGprsSelfTestState();
	this->gprsConnectStatee = state_builder->buildGprsConnectState();
	this->gprsEventHandleState = state_builder->buildGprsEventHandleState();
	this->gprsDealSMSState = state_builder->buildGprsDealSMSState();
	this->gprsSer485ProcessState = state_builder->builderSer485ProcessState();
	this->gprsHeatBeatState = state_builder->builderGprsHeatBeatState();
	this->gprsCnntManagerState = state_builder->builderGprsCnntManagerState();
	
	this->curState = this->gprsSer485ProcessState;
	
	lw_oopc_delete( state_builder);
	return ERR_OK;

}


CTOR(SMSModeContext)
FUNCTION_SETTING(StateContext.construct, SMSConstruct);
END_CTOR


int RRTUConstruct( StateContext *this)
{
	Builder *state_builder = ( Builder *)RemoteRTUModeBuilder_new();
	
	this->gprsSelfTestState = state_builder->buildGprsSelfTestState();
	this->gprsConnectStatee = state_builder->buildGprsConnectState();
	this->gprsEventHandleState = state_builder->buildGprsEventHandleState();
	this->gprsDealSMSState = state_builder->buildGprsDealSMSState();
	this->gprsSer485ProcessState = state_builder->builderSer485ProcessState();
	this->gprsHeatBeatState = state_builder->builderGprsHeatBeatState();
	this->gprsCnntManagerState = state_builder->builderGprsCnntManagerState();
	
	this->curState = this->gprsSer485ProcessState;
	
	lw_oopc_delete( state_builder);
	return ERR_OK;
	
}

CTOR(RemoteRTUModeContext)
FUNCTION_SETTING(StateContext.construct, RRTUConstruct);
END_CTOR

int PThrConstruct( StateContext *this)
{
	Builder *state_builder = ( Builder *)PassThroughModeBuilder_new();
	
	this->gprsSelfTestState = state_builder->buildGprsSelfTestState();
	this->gprsConnectStatee = state_builder->buildGprsConnectState();
	this->gprsEventHandleState = state_builder->buildGprsEventHandleState();
	this->gprsDealSMSState = state_builder->buildGprsDealSMSState();
	this->gprsSer485ProcessState = state_builder->builderSer485ProcessState();
	this->gprsHeatBeatState = state_builder->builderGprsHeatBeatState();
	this->gprsCnntManagerState = state_builder->builderGprsCnntManagerState();
	
	this->curState = this->gprsSer485ProcessState;
	
	lw_oopc_delete( state_builder);
	
	return ERR_OK;
	
}

CTOR(PassThroughModeContext)
SUPER_CTOR(StateContext);
FUNCTION_SETTING(StateContext.construct, PThrConstruct);
END_CTOR


WorkState* LRTUBuildGprsSelfTestState(void )
{
	return NULL;
	
}
WorkState*  LRTUBuildGprsConnectState(void )
{
	return NULL;
	
}
WorkState* LRTUBuildGprsEventHandleState(void )
{
	return NULL;
}
WorkState* LRTUBuildGprsDealSMSState(void )
{
	return NULL;
}
WorkState* LRTUBuilderSer485ProcessState(void )
{
	return NULL;
}
WorkState* LRTUBuilderGprsHeatBeatState(void )
{
	return NULL;
}
WorkState* LRTUBuilderGprsCnntManagerState(void )
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


WorkState* SMSModeBuildGprsSelfTestState(void )
{
	return NULL;
	
}
WorkState*  SMSModeBuildGprsConnectState(void )
{
	return NULL;
	
}
WorkState* SMSModeBuildGprsEventHandleState(void )
{
	return NULL;
}
WorkState* SMSModeBuildGprsDealSMSState(void )
{
	return NULL;
}
WorkState* SMSModeBuilderSer485ProcessState(void )
{
	return NULL;
}
WorkState* SMSModeBuilderGprsHeatBeatState(void )
{
	return NULL;
}
WorkState* SMSModeBuilderGprsCnntManagerState(void )
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

WorkState* RRTUBuildGprsSelfTestState(void )
{
	return NULL;
	
}
WorkState*  RRTUBuildGprsConnectState(void )
{
	return NULL;
	
}
WorkState* RRTUBuildGprsEventHandleState(void )
{
	return NULL;
}
WorkState* RRTUBuildGprsDealSMSState(void )
{
	return NULL;
}
WorkState* RRTUBuilderSer485ProcessState(void )
{
	return NULL;
}
WorkState* RRTUBuilderGprsHeatBeatState(void )
{
	return NULL;
}
WorkState* RRTUBuilderGprsCnntManagerState(void )
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


WorkState* PassThroughModeBuildGprsSelfTestState(void )
{
	return NULL;
	
}
WorkState*  PassThroughModeBuildGprsConnectState(void )
{
	return NULL;
	
}
WorkState* PassThroughModeBuildGprsEventHandleState(void )
{
	return NULL;
}
WorkState* PassThroughModeBuildGprsDealSMSState(void )
{
	return NULL;
}
WorkState* PassThroughModeBuilderSer485ProcessState(void )
{
	return NULL;
}
WorkState* PassThroughModeBuilderGprsHeatBeatState(void )
{
	return NULL;
}
WorkState* PassThroughModeBuilderGprsCnntManagerState(void )
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

