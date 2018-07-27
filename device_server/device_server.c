//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//


#include "device_server.h"

#include "KTZ3.h"
#include "dvs_test.h"

/*


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
CLASS(Dvs_none)
{
	
	EXTENDS(Device_server);
	
	
};
//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------
static Ktz3 *p_ktz3;
static Dvs_test *p_test;
static Dvs_none *p_none;

//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
static 	int dvs_init(Device_server *self);
static int none_data_down(Device_server *self, int data_src, void *data_buf, int data_len);


//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//

Device_server *DVS_get_dev_service(int dvs_type)
{
	
	switch(dvs_type)
	{
		case DVS_KTZ3:
			if(p_ktz3 == NULL)
				p_ktz3 = Ktz3_new();
			
			return SUPER_PTR(p_ktz3, Device_server);
		case DVS_TEST:

			if(p_test == NULL)
					p_test = Dvs_test_new();
			
			return SUPER_PTR(p_test, Device_server);
		case DVS_NONE:

			if(p_none == NULL)
					p_none = Dvs_none_new();
			
			return SUPER_PTR(p_none, Device_server);
		
	}
	
	
	return NULL;
	
	
}

ABS_CTOR(Device_server)
FUNCTION_SETTING(init, dvs_init);

END_ABS_CTOR




CTOR(Dvs_none)
SUPER_CTOR(Device_server);
FUNCTION_SETTING( Device_server.data_down, none_data_down);
END_CTOR

//=========================================================================//
//                                                                         //
//          P R I V A T E   D E F I N I T I O N S                          //
//                                                                         //
//=========================================================================//
/// \name Private Functions
/// \{

static 	int dvs_init(Device_server *self)
{
	return 0;
}


static int none_data_down(Device_server *self, int data_src, void *data_buf, int data_len)
{
	
	return 0;
	
}



