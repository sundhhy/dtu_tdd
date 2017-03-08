#ifndef __DTU_H__
#define __DTU_H__
#include "lw_oopc.h"
#include "stdint.h"
#include "gprs.h"

typedef int ( *hookFunc)( char *data, int len, void *arg);
ABS_CLASS( StateContext);
INTERFACE( BusinessProcess);
ABS_CLASS( WorkState);
INTERFACE( Builder);



CLASS( StateContextFactory)
{
//	StateContextFactory* (*getInstance)(void);
	StateContext *( *createContext)( int mode);
	
};
StateContextFactory* SCFGetInstance(void);
ABS_CLASS( StateContext)
{
//	Builder *stateBuilder;
	
	gprs_t *gprs;
	char 	*dataBuf;
	int 	bufLen;
	
	WorkState *gprsSelfTestState;
	WorkState *gprsConnectState;
	WorkState *gprsEventHandleState;
	WorkState *gprsDealSMSState;
	WorkState *gprsSer485ProcessState;
	WorkState *gprsHeatBeatState;
	WorkState *gprsCnntManagerState;
	WorkState *curState;
	
	int (*init)( StateContext *this, gprs_t *myGprs, char *buf, int bufLen);
	void ( *setCurState)( StateContext *this, WorkState *state);
	int ( *construct)( StateContext *this , Builder *state_builder);
	
	//abs
	int (*initState)( StateContext *this);
};

//state

ABS_CLASS( WorkState)
{
	gprs_t *gprs;
	char 	*dataBuf;
	int 	bufLen;
	BusinessProcess *modbusProcess;
	BusinessProcess *configSystem;
	BusinessProcess *forwardSer485;
	BusinessProcess *forwardSMS;
	BusinessProcess *forwardNet;


	//abs
	int (*run)( WorkState *this, StateContext *context);
	
	//impl
	int ( *init)( WorkState *this, gprs_t *myGprs, char *buf, int bufLen);
	void (*print)( WorkState *this, char *str);
};

INTERFACE( Builder)
{
	WorkState* ( *buildGprsSelfTestState)(StateContext *this );
	WorkState* ( *buildGprsConnectState)(StateContext *this );
	WorkState* ( *buildGprsEventHandleState)(StateContext *this );
	WorkState* ( *buildGprsDealSMSState)(StateContext *this );
	WorkState* ( *builderSer485ProcessState)(StateContext *this );
	WorkState* ( *builderGprsHeatBeatState)(StateContext *this );
	WorkState* ( *builderGprsCnntManagerState)(StateContext *this );
};


CLASS( GprsSelfTestState)
{
	
	IMPLEMENTS( WorkState);
	
	
	
};

CLASS( GprsConnect_State)
{
	IMPLEMENTS( WorkState);
	
};

CLASS( GprsEventHandleState)
{
	IMPLEMENTS( WorkState);
	
};

CLASS( GprsDealSMSState)
{
	IMPLEMENTS( WorkState);
	
};
CLASS( Ser485ProcessState)
{
	IMPLEMENTS( WorkState);
	
};

CLASS( GprsHeatBeatState)
{
	IMPLEMENTS( WorkState);
	
};

CLASS( GprsCnntManagerState)
{
	IMPLEMENTS( WorkState);
	
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




CLASS( LocalRTUModeBuilder)
{
	IMPLEMENTS( Builder);
};
CLASS( SMSModeBuilder)
{
	IMPLEMENTS( Builder);
};
CLASS( RemoteRTUModeBuilder)
{
	IMPLEMENTS( Builder);
};
CLASS( PassThroughModeBuilder)
{
	IMPLEMENTS( Builder);
};

//业务处理

INTERFACE( BusinessProcess)
{
	
//	int (*forwardSer485)( BusinessProcess *this);
//	int (*forwardNet)( BusinessProcess *this);
//	int (*forwardSMS)( BusinessProcess *this);
//	int (*modbusProcess)( BusinessProcess *this);
//	int (*configSystem)( BusinessProcess *this);
	int ( *process)(  char *data, int len, hookFunc cb, void *arg);

};

CLASS( EmptyProcess)
{
	IMPLEMENTS( BusinessProcess);
	
};

CLASS( ForwardSer485)
{
	IMPLEMENTS( BusinessProcess);
	
};

CLASS( ForwardNet)
{
	IMPLEMENTS( BusinessProcess);
	
	
};
CLASS( ForwardSMS)
{
	IMPLEMENTS( BusinessProcess);
	
	
};

CLASS( ModbusBusiness)
{
	IMPLEMENTS( BusinessProcess);
	
};

CLASS( ConfigSystem)
{
	
	IMPLEMENTS( BusinessProcess);
};




#endif
