//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//

#include <string.h>

#include "cmsis_os.h"                                           // CMSIS RTOS header file
#include "osObjects.h"                      // RTOS object definitions

#include "gprs.h"
#include "serial485_uart.h"
#include "modbus_master.h"
#include "dtuConfig.h"

#include "KTZ3.h"

/*
定期采集KTZ3设备;
解析传入的命令，并返回数据,暂时只考虑短信方式的数据通信

所用的485串口必须已经初始化过。

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
#define NUM_WR_MSGS		16
//------------------------------------------------------------------------------
// local types
//------------------------------------------------------------------------------

//因为目前是短信服务功能，不大会有连续寄存器的写功能，就不考虑连续写了
typedef struct {
	uint8_t			flag;					//0 空闲 1 些消息有效
	uint8_t			slave_id;
	uint8_t			reg_type;
	uint8_t			addr;
	
	uint16_t		val;
	
	int					msg_src;		//消息源，用于写值处理后返回结果给消息源
}wr_modbus_msg_t;
//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------


static void ktz3_run(Device_server *self);


osThreadId tid_ktz3;                                          // thread id
osThreadDef (ktz3_run, osPriorityNormal, 1, 0);                   // thread object

static wr_modbus_msg_t *p_wr_msgs;
//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
static 	int ktz3_init(Device_server *self);
static int ktz3_data_down(Device_server *self, int data_src, void *data_buf, int data_len);
static int ktz3_deal_write_msg(Device_server *self, uint8_t *buf, int buf_size);
static void ktz3_poll_modbus_dev(Device_server *self, uint8_t *buf, int buf_size);


static int ktz3_get_datatype(char *s);
static int ktz3_get_datavalue(char *s);
static int ktz3_datatype_2_modbus_addr();
static int ktz3_put_modbus_val();
static int ktz3_datatype_2_string(char *buf, int buf_size);

//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//



CTOR(Ktz3)
SUPER_CTOR(Device_server);
FUNCTION_SETTING(Device_server.init, ktz3_init);
FUNCTION_SETTING(Device_server.run, ktz3_run);
FUNCTION_SETTING(Device_server.data_down, ktz3_data_down);
END_CTOR

//=========================================================================//
//                                                                         //
//          P R I V A T E   D E F I N I T I O N S                          //
//                                                                         //
//=========================================================================//
/// \name Private Functions
/// \{
static 	int ktz3_init(Device_server *self)
{
	
	p_wr_msgs = malloc(NUM_WR_MSGS * sizeof(wr_modbus_msg_t));
	
	if(p_wr_msgs == NULL)
		return -1;
	
	memset(p_wr_msgs, 0, NUM_WR_MSGS * sizeof(wr_modbus_msg_t));
	
	tid_ktz3 = osThreadCreate(osThread(ktz3_run), self);
	if (!tid_ktz3) return(-1);

	return(0);
}

static void ktz3_run(Device_server *self)
{
	
	uint8_t  modbus_buf[64];
	while(1)
	{
		threadActive();
		
		ktz3_deal_write_msg(self, modbus_buf, 64);
		ktz3_poll_modbus_dev(self, modbus_buf, 64);
		osDelay(500);
//		osThreadYield();         
		
	}
	
}

static int ktz3_data_down(Device_server *self, int data_src, void *data_buf, int data_len)
{
	//读取目标设备地址
	
	//读取消息类型：读取、写入
	
	//写消息处理,要考虑可能存在多个写值的情况
		//获取一个wr_modbus_msg_t
	
	
		//将访问数据类型转换成Modbus地址和寄存器类型
	
		//提取写值
	
		//将数据源，写地址，寄存器类型，写值存入wr_modbus_msg_t
	
		
	
	
	//读消息处理，要考虑多个数据的读取情况
			//将访问的数据类型的值转换成字符串
	
			//将结果字符串组合成格式，返回给数据源
	
			

	return 0;
	
}

static int ktz3_deal_write_msg(Device_server *self, uint8_t *buf, int buf_size)
{
	
		//对缓存的写消息进行处理
	
}

static void ktz3_poll_modbus_dev(Device_server *self, uint8_t *buf, int buf_size)
{

	int i;
	
	
	//读取设备的寄存器
	for(i = 0; i < NUM_DVS_SLAVE_DEV; i++)
	{
		
		
		
	}
	
	
}

