#ifndef __GPRS_H__
#define __GPRS_H__
#include "lw_oopc.h"
#include "stdint.h"
#define RETRY_TIMES	5

#define IPMUX_NUM	4		//支持4路连接

//SIM900 ATCMD
#define AT_SET_DNSIP  "AT+CDNSCFG=" 

CLASS(gprs_t)
{
	
	uint32_t	event;
	
	// public
	int ( *init)( gprs_t *self);
	
	void (*startup)( gprs_t *self);
	void (*shutdown)( gprs_t *self);
	
	int	(*check_simCard)( gprs_t *self);
	
	int	(*send_text_sms)(  gprs_t *self, char *phnNmbr, char *sms);
	int	(*read_phnNmbr_TextSMS)(  gprs_t *self, char *phnNmbr, char *in_buf, char *out_buf, int *len);
	int (*delete_sms)( gprs_t *self, int seq);
	
	int (*read_smscAddr)(gprs_t *self, char *addr);
	int (*set_smscAddr)(gprs_t *self, char *addr);
	int (*get_apn)( gprs_t *self, char *buf);
	int (*set_dns_ip)( gprs_t *self, char *dns_ip);
	int (*tcpip_cnnt)( gprs_t *self, int cnnt_num, char *prtl, char *addr, int portnum);
	int (*sendto_tcp)( gprs_t *self, int cnnt_num, char *data, int len);
	int (*recvform_tcp)( gprs_t *self, char *buf, int *lsize);
	int (*deal_smsrecv_event)( gprs_t *self, char *in_buf, char *out_buf, int *lsize, char *phno);
	int (*deal_tcpclose_event)( gprs_t *self, char *data, int len);
	int (*deal_tcprecv_event)( gprs_t *self, char *in_buf, char *out_buf, int *len);
	
	
	int (*sms_test)( gprs_t *self, char *phnNmbr, char *buf, int bufsize);
	int (*tcp_test)( gprs_t *self, char *tets_addr, int portnum, char *buf, int bufsize);
	
	int	(*guard_serial)( gprs_t *self, char *buf, int *lsize);
	int	(*get_firstDiscnt_seq)( gprs_t *self);
	int	(*get_firstCnt_seq)( gprs_t *self);
	

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

#define SET_EVENT( event, flag)  ( event | ( 1 << flag) )
#define CKECK_EVENT( event, flag)  ( event & ( 1 << flag) )
#define CLR_EVENT( event, flag)  ( event & ~( 1 << flag) )

int compare_phoneNO(char *NO1, char *NO2);
int check_phoneNO(char *NO);
int copy_phoneNO(char *dest_NO, char *src_NO);
int check_ip(char *ip);

#endif
