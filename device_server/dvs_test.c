//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//
#include <stdio.h>

#include "cmsis_os.h"                                           // CMSIS RTOS header file
#include "osObjects.h"                      // RTOS object definitions

#include "gprs.h"

#include "dvs_test.h"

/*
实现一个短息回显的测试服务
*/
//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// module global vars
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// global function prototypes
//------------------------------------------------------------------------------

//============================================================================//
//            P R I V A T E   D E F I N I T I O N S                           //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// local types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------
static void test_run(Device_server *self);


osThreadId tid_test;                                          // thread id
osThreadDef (test_run, osPriorityNormal, 1, 0);                   // thread object

//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------

static 	int test_init(Device_server *self);
static int test_data_down(Device_server *self, int data_src, void *data_buf, int data_len);
//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//



CTOR(Dvs_test)
SUPER_CTOR(Device_server);
FUNCTION_SETTING(Device_server.init, test_init);
FUNCTION_SETTING( Device_server.run, test_run);

FUNCTION_SETTING( Device_server.data_down, test_data_down);

END_CTOR

//=========================================================================//
//                                                                         //
//          P R I V A T E   D E F I N I T I O N S                          //
//                                                                         //
//=========================================================================//
/// \name Private Functions
/// \{
static 	int test_init(Device_server *self)
{
	Dvs_test			*cthis = SUB_PTR( self, Device_server, Dvs_test);
	
	
	cthis->first_run = 1;
	
	tid_test = osThreadCreate(osThread(test_run), self);
	if (!tid_test) return(-1);

	
	
	return(0);
}

static void test_run(Device_server *self)
{
	
	char msg[64] = "[DVS Test]send me a txt less than 40B, I will send bakc to you!";
	Dvs_test			*cthis = SUB_PTR( self, Device_server, Dvs_test);

	while(1)
	{
		osDelay(1000);
		
		if(cthis->first_run == 0)
			continue;
		
		//只要有一路发送消息成功即可
		if(GPRS_send_sms(0, msg) == 0)
		{
			cthis->first_run = 0;
			
		}
		else
			continue;
		
		
		GPRS_send_sms(1, msg);
		GPRS_send_sms(2, msg);
		GPRS_send_sms(3, msg);
		
	}
	
	
	
}


static int test_data_down(Device_server *self, int data_src, void *data_buf, int data_len)
{
	char test_buf[64];
	
	sprintf(test_buf, "[DVS Test]send back: %s", data_buf);
	GPRS_send_sms(data_src, test_buf);
	
	return 0;
	
}


