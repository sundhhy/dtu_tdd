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

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

#define SOFTER_VER	300
#define HARD_VER	1
/*
V300 171211: 
system:增加系统模块，在系统模块中加入：LED闪烁周期控制功能;gprs 的一些状态和事件集合;
gprs: 把事件的存储从队列形式改成集合存储。
dtu: 在gprs的各种操作阶段，使用不同的Led闪烁周期



*/

#define LED_CYCLE_MS		200
#define LED_LV_NORMAL			0
//gprs在不同的状态下可以控制LED闪烁频率
#define LED_GPRS_RUN			0	
#define LED_GPRS_CHECK		3			
#define LED_GPRS_CNNTING	1		//联网中
#define LED_GPRS_DISCNNT	2
#define LED_GPRS_ERR			4


#define	SET_U8_BIT(set, bit) (set | (1 << bit))
#define	CLR_U8_BIT(set, bit) (set & ~(1 << bit))
#define	CHK_U8_BIT(set, bit) (set & (1 << bit))
#define	MAX_NUM_SMS		256
//------------------------------------------------------------------------------
// typedef
//------------------------------------------------------------------------------



typedef struct  {
	struct {
		uint16_t	led_cycle_ms;
		uint16_t	led_count_ms;
	}led;
	struct {
		uint8_t	flag_cnt;		//正在连接过程中，可能会去主动关闭前面的连接，所以这时关闭事件要过滤掉
		uint8_t	flag_sms_ready;
		uint8_t	cip_mode;
		uint8_t	cip_mux;
		uint8_t	cur_state;
		uint8_t	rx_sms_seq;
		uint8_t	set_tcp_close;
		uint8_t	set_tcp_recv;
		uint32_t	set_sms_recv[8];		//最大记录256条
	}gprs;
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
#endif
