#ifndef __GPRS_H__
#define __GPRS_H__
#include "lw_oopc.h"

#define RETRY_TIMES	5

#define IPMUX_NUM	4		//支持4路连接

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
	
	int (*tcp_cnnt)( gprs_t *self, int cnnt_num, char *addr, int portnum);
	int (*sendto_tcp)( gprs_t *self, int cnnt_num, char *data, int len);
	int (*recvform_tcp)( gprs_t *self, char *buf, int *lsize);
	int (*deal_tcpclose_event)( gprs_t *self, char *data, int len);
	int (*deal_tcprecv_event)( gprs_t *self, char *buf, int *lsize);
	
	
	int (*sms_test)( gprs_t *self, char *phnNmbr, char *buf, int bufsize);
	int (*tcp_test)( gprs_t *self, char *tets_addr, int portnum, char *buf, int bufsize);
	
	int	(*guard_serial)( gprs_t *self, char *buf, int *lsize);
	
};



typedef enum {
	SHUTDOWN,
    GPRS_OPEN_FINISH,       /// GPRS 打开成功了
	INIT_FINISH_OK,
    TCP_IP_OK,
    TCP_IP_NO,
}SIM_STATUS ;

typedef enum {
	sms_urc = 1,
	tcp_receive,
	tcp_close,
	
}SIM_Event;



#endif
