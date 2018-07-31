//------------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------------
#ifndef __INC_system_H__
#define __INC_system_H__
//------------------------------------------------------------------------------
// check for correct compilation options
//------------------------------------------------------------------------------
#include <stdint.h>
#include "stdbool.h"
#include "system_cfg.h"

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

#define SOFTER_VER	304
#define HARD_VER	1
/*
V34 171217:
	这个版本在激活移动场景失败时，并不作为错误处理，而是延时5秒以后开始进行连接。在武康的红旗机械工程表现良好
V302 171215:
增加信号强度和电量检测；无连接时，去除错误地址标记
V300 171211: 
system:增加系统模块，在系统模块中加入：LED闪烁周期控制功能;gprs 的一些状态和事件集合;
gprs: 把事件的存储从队列形式改成集合存储。
dtu: 在gprs的各种操作阶段，使用不同的Led闪烁周期



*/

#define LED_CYCLE_MS		200
#define LED_LV_NORMAL			0
#define LED_FILESYS_ERR			5
//gprs在不同的状态下可以控制LED闪烁频率
#define LED_GPRS_RUN			0	
#define LED_GPRS_CHECK		3			
#define LED_GPRS_CNNTING	1		//联网中
#define LED_GPRS_DISCNNT	2
#define LED_GPRS_ERR			4
#define LED_GPRS_SMS	1


#define	SET_U8_BIT(set, bit) (set | (1 << bit))
#define	CLR_U8_BIT(set, bit) (set & ~(1 << bit))
#define	CHK_U8_BIT(set, bit) (set & (1 << bit))
#define	MAX_NUM_SMS		64
//------------------------------------------------------------------------------
// typedef
//------------------------------------------------------------------------------



typedef struct  {
	struct {
		uint16_t	led_cycle_ms;
		uint16_t	led_count_ms;
	}led;
	struct {
//		uint8_t	flag_cnt;		//正在连接过程中，可能会去主动关闭前面的连接，所以这时关闭事件要过滤掉
		uint8_t	flag_ready;		
		uint8_t	cip_mode;		
		uint8_t	cip_mux;
		uint8_t	ip_status;	
		
		uint8_t	cur_state;
		uint8_t	rx_sms_seq;
		uint8_t	set_tcp_close;		
		uint8_t	set_tcp_recv;
		
		uint8_t		signal_strength;			//0 - 31
		uint8_t		ber;						// 0 - 7
		
		uint8_t		status_gsm;	
		uint8_t		status_gprs;			
		
		uint8_t		bcs;			//0 ME 未充电 1 ME在充电 2 充电完成
		uint8_t		bcl;			//0 - 100 电量 			
		uint16_t	voltage_mv;
		
		uint32_t	set_sms_recv[2];		//最大记录64条		实测最大是50
	}gprs;
	
	char			cfg_change;			//配置信息被修改的标志
	char			res[3];
}dtu_system;
//------------------------------------------------------------------------------
// global variable declarations
//------------------------------------------------------------------------------
extern dtu_system 	dsys;
//------------------------------------------------------------------------------
// function prototypes
//------------------------------------------------------------------------------
extern void Led_level(int lv);
bool check_bit(uint8_t *data, int bit);
void clear_bit(uint8_t *data, int bit);
void set_bit(uint8_t *data, int bit);
int Get_str_data(char *s_data, char* separator, int num, uint8_t	*err);
#endif
