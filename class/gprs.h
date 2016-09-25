#ifndef __GPRS_H__
#define __GPRS_H__
#include "lw_oopc.h"

CLASS(gprs_t)
{
	// public
	int ( *init)( gprs_t *self);
	
	int (*startup)( gprs_t *self);
	void (*shutdown)( gprs_t *self);
	
	int	(*check_simCard)( gprs_t *self);
	
	int	(*send_text_sms)(  gprs_t *self, char *phnNmbr, char *sms);
	int	(*read_phnNmbr_TextSMS)(  gprs_t *self, char *phnNmbr, char *buf, int *len);
	int (*delete_sms)( gprs_t *self, int seq);
	
	
	int (*sms_test)( gprs_t *self, char *phnNmbr, char *buf, int bufsize);
};



typedef enum {
			SHUTDOWN,
//    INIT0,
//    CGMM,
//    ECHO0,
//    SIMCARD,
//    RST,
//    CREG,
//    INIT_FINISH_OK,

//    GPRS_JH_S,
//    GPRS_DK_S ,
//    GPRS_QD_S ,
    GPRS_OPEN_FINISH,       /// GPRS 打开成功了
		INIT_FINISH_OK,
//    TCPIP_IO_MODE,
//    TCPIP_BJ_ADDR,
//    TCPIP_CONNECT,
//    TCP_IP_OK,
//    TCP_IP_NO,
//    TCP_IP_CONNECTING,

//    GPRS_DEF_PDP_S,         /// 定义PDP场景
//    GPRS_ACT_PDP_S,         /// 激活场景
//    GPRS_ACT_PDP_S_RET,     /// 激活反馈
//    GPRS_CMNET_APN_S,       /// 接入模式

//    TRANSPARENT_MODE_START,
//    TRANSPARENT_MODE_DEF,
//    TRANSPARENT_MODE_ACT,
//    TRANSPARENT_MODE_IOMODE,
//    TRANSPARENT_MODE_TYPE,
//    TRANSPARENT_MODE_CONFIG,
//    TRANSPARENT_MODE_CONNECT,
//    TRANSPARENT_MODE_CONNECT_RET,
//    TM_OK,
//    TM_NO,

//    TCPIP_CLOSE_START,
//    TCPIP_CLOSE_DEF,
//    TCPIP_CLOSE_ETCPIP,
//    TCP_IP_CLOSE_OK,
//    TCP_IP_CLOSE_NO,


//    GSM_SEND_TEXT_S,
//    GSM_MSG_MODE_S,
//    GSM_CHAR_MODE_S,
//    GSM_TEXT_MODE_FINISH,

//    SMS_READ_MESSAGE_START_S,
//    SMS_READ_CONTENT_S,
//    READ_SMS_FINISH,

//    INIT_GPS_FINISH_OK,

//    SIM900_LOC_START,
//    SIM900_GPRS_S,
//    SIM900_APN_S,
//    SIM900_PBR1_S,
//    SIM900_PBR2_S,
//    SIM900_LOC_S,
//    SIM900_LOC_G_S,
//    SIM900_LOC_FINISH


}SIM_STATUS ;




#endif
