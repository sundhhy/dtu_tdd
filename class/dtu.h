#ifndef __DTU_H__
#define __DTU_H__
#include "lw_oopc.h"
#include "stdint.h"
#include "gprs.h"



INTERFACE( WorkState)
{
	
	void (*run)( WorkState *this);
};

INTERFACE( Builder)
{
	WorkState* ( *buildGprsSelfTestState)(void );
	WorkState* ( *buildGprsConnectState)(void );
	WorkState* ( *buildGprsEventHandleState)(void );
	WorkState* ( *buildGprsDealSMSState)(void );
	WorkState* ( *builderSer485ProcessState)(void );
	WorkState* ( *builderGprsHeatBeatState)(void );
	WorkState* ( *builderGprsCnntManagerState)(void );
};


ABS_CLASS( StateContext)
{
	Builder *stateBuilder;
	gprs_t *gprs;
	WorkState *gprsSelfTestState;
	WorkState *gprsConnectStatee;
	WorkState *gprsEventHandleState;
	WorkState *gprsDealSMSState;
	WorkState *gprsSer485ProcessState;
	WorkState *gprsHeatBeatState;
	WorkState *gprsCnntManagerState;
	WorkState *curState;


	void ( *setCurState)( StateContext *this, WorkState *state);
	int ( *construct)( StateContext *this);
};

CLASS(LocalRTUModeContext)
{
	EXTENDS( StateContext);
	
};
CLASS(SMSModeContext)
{
	EXTENDS( StateContext);
	
};
CLASS(RemoteRTUModeContext)
{
	EXTENDS( StateContext);
	
};
CLASS(PassThroughModeContext)
{
	EXTENDS( StateContext);
	
};


CLASS( StateContextFactory)
{
	StateContextFactory* (*getInstance)(void);
	StateContext *( *createContext)( int mode);
	
};


#endif