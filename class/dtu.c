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


void setCurState( StateContext *this, WorkState *state)
{
	
	this->curState = state;
}

ABS_CTOR( StateContext)
FUNCTION_SETTING(setCurState, setCurState);
END_ABS_CTOR

int LRTUConstruct( StateContext *this)
{
	
	return ERR_OK;
}

CTOR(LocalRTUModeContext)
FUNCTION_SETTING(StateContext.construct, LRTUConstruct);
END_CTOR

int SMSConstruct( StateContext *this)
{
	
	return ERR_OK;

}

CTOR(SMSModeContext)
FUNCTION_SETTING(StateContext.construct, SMSConstruct);
END_CTOR


int RRTUConstruct( StateContext *this)
{
	
	return ERR_OK;
	
}

CTOR(RemoteRTUModeContext)
FUNCTION_SETTING(StateContext.construct, RRTUConstruct);
END_CTOR

int PThrConstruct( StateContext *this)
{
	
	return ERR_OK;
	
}

CTOR(PassThroughModeContext)
FUNCTION_SETTING(StateContext.construct, PThrConstruct);
END_CTOR


