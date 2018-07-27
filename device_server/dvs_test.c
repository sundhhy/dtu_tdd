//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//

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


//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------


static int test_data_down(Device_server *self, int data_src, void *data_buf, int data_len);
//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//



CTOR(Dvs_test)
SUPER_CTOR(Device_server);
//FUNCTION_SETTING( Model.init, MdlChn_init);
//FUNCTION_SETTING( Model.run, MdlChn_run);

//FUNCTION_SETTING( Model.self_check, MdlChn_self_check);
//FUNCTION_SETTING( Model.getMdlData, MdlChn_getData);
//FUNCTION_SETTING( Model.setMdlData, MdlChn_setData);

//FUNCTION_SETTING( Model.to_string, MdlChn_to_string);
//FUNCTION_SETTING( Model.to_percentage, MdlChn_to_percentage);
//FUNCTION_SETTING( Model.modify_str_conf, MdlChn_modify_sconf);

//FUNCTION_SETTING( Model.set_by_string, MdlChn_set_by_string);
FUNCTION_SETTING( Device_server.data_down, test_data_down);

END_CTOR

//=========================================================================//
//                                                                         //
//          P R I V A T E   D E F I N I T I O N S                          //
//                                                                         //
//=========================================================================//
/// \name Private Functions
/// \{
static int test_data_down(Device_server *self, int data_src, void *data_buf, int data_len)
{
	GPRS_send_sms(data_src, data_buf);
	
	return 0;
	
}


