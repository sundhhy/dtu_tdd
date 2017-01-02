#ifndef __ADC_H__
#define __ADC_H__
#include <stdint.h>
#define		Time500mS		       	100
#define     Time5000mS			   	500
#define		Time1000mS		     	100



/*===Ai节点信号类型定义===*/
#define B_o         	 0  /*B型热电偶*/
#define E_o        		 1  /*E型热电偶*/
#define J_o        		 2  /*J型热电偶*/
#define K_o         	 3  /*K型热电偶*/
#define S_o         	 4  /*S型热电偶*/
#define T_o        		 5  /*T型热电偶*/
#define Pt100          6  /*Pt100热电阻*/
#define Cu50           7  /*Cu50热电阻*/
#define mA0_10			   8	/*0~10毫安电流信号*/
#define mA4_20			   9	/*4~20毫安电流信号*/
#define V_5			 	     10	/*0~5伏大信号*/
#define V_1_5			     11	/*1～5伏大信号*/
#define mV_20      	 	 12	/*0~20毫伏电压信号*/
#define mV_100     	 	 13	/*0～100毫伏电压信号*/

//=====================================//
#define VX32           0
#define VX22           1
#define VX21           2
#define VX31           3
#define VX12           4
#define GND            5
#define VX11           6
#define VR0_A          7
//=====================================//

#define CHANNEL0       0
#define CHANNEL1       1
#define CHANNEL2       2
//=====================================//

#define Samplepoint0   0 
#define Samplepoint1   1
#define Samplepoint2   2
#define Samplepoint3   3
//=====================================//


//***********选择采样输入通道切换开关**********//

#define   SelectSET4051_A               GPIO_ResetBits(GPIOB,GPIO_Pin_5)
#define   SelectCLR4051_A               GPIO_SetBits(GPIOB,GPIO_Pin_5) 

#define   SelectSET4051_B               GPIO_ResetBits(GPIOB,GPIO_Pin_6)
#define   SelectCLR4051_B               GPIO_SetBits(GPIOB,GPIO_Pin_6) 

#define   SelectSET4051_C               GPIO_ResetBits(GPIOB,GPIO_Pin_7)
#define   SelectCLR4051_C               GPIO_SetBits(GPIOB,GPIO_Pin_7) 

//---------------------------------------------------------------------//

//***********输入电流采样值时的切换开关*************//
#define   SET_currentnum0         GPIO_SetBits(GPIOC,GPIO_Pin_1) 
#define   CLR_currentnum0 			  GPIO_ResetBits(GPIOC,GPIO_Pin_1)

#define   SET_currentnum1         GPIO_SetBits(GPIOC,GPIO_Pin_2) 
#define   CLR_currentnum1 			  GPIO_ResetBits(GPIOC,GPIO_Pin_2)

#define   SET_currentnum2         GPIO_SetBits(GPIOC,GPIO_Pin_3) 
#define   CLR_currentnum2 			  GPIO_ResetBits(GPIOC,GPIO_Pin_3)


//---------------------------------------------------------------------//


typedef struct{
  	 unsigned short  BD_20mV_HIGH;	       //20mV标定High值	     
	   short           BD_20mV_LOW;				   //20mV标定Low值
	   unsigned short  BD_100_HIGH;	         //100mV标定High值  
	   short           BD_100_LOW;				   //100mV标定Low值	   
	   unsigned short  BD_5_HIGH;		         //5V标定High值	  
	   short           BD_5_LOW;				     //5V标定LOW值		  
	   unsigned short  BD_20mA_HIGH;	       //20mA标定High值	 
	   short           BD_20mA_LOW;			     //20mA标定Low值	   
	   unsigned short  BD_Pt100_HIGH;	       //PT100标定High值
     short           BD_Pt100_LOW;			   //PT100标定Low值
	   unsigned short  BD_Cu50_HIGH;	       //Cu50标定High值
	   short           BD_Cu50_LOW;			     //Cu50标定Low值	
}BD_para;


extern unsigned short ConfigCheckCnt;
extern unsigned short ADC_timcount;        //ADC触发时间
extern unsigned short comout_count;
extern unsigned char  HalfSecCnt500mS;
extern unsigned short HalfSecCnt5000mS;
extern unsigned char  Falg_HalfSec;
extern unsigned char  RunStat;
extern unsigned char  sec_10;
extern unsigned char  BusCheckEN;		
extern unsigned char  collectChannel;                    //当前的采集通道
extern unsigned char  RUNorBD;                           //标定或者运行标志(RUNorBD =1表示为标定状态  RUNorBD ==0表示为运行状态)
extern unsigned char  sampledote0,sampleolddote0;        //通道0采样点
extern unsigned char  sampledote1,sampleolddote1;        //通道1采样点
extern unsigned char  sampledote2,sampleolddote2;        //通道2采样点

extern unsigned char  BD_sampledote,BD_sampleolddote;    //标定当前通道的采样信号点
extern unsigned short BUSADdelta; 
extern unsigned char  BUSADdelataCnt;                     
extern unsigned char  CHannel0finish, CHannel1finish, CHannel2finish; //一次信号采样完成标志
extern unsigned short ADCval0,ADCval1,ADCval2;
extern float AV_BUF0,AV_BUF1,AV_BUF2;
extern float rang_f;
extern float ma_tie;
extern unsigned short ADCConvertedValue[200];


struct _RemoteRTU_Module_
{
	uint8_t		Type;			 //节点类型			 	
	uint8_t		Status;		 //节点状态			          
	uint8_t		Alarm;		 //节点报警			 
	uint8_t    none;      //保留字节  
	float value;     //实际采样的电流值或者电压值
	uint16_t   RSV;       //0-65535的工程转换值
	uint16_t		AV;			   //当前工程值	
	uint16_t   AlarmDead; //报警回差
	uint16_t   RangeH;    //量程上限
	uint16_t   RangeL;    //量程下限
	uint16_t   HiAlm;     //报警上限
	uint16_t   LowAlm;    //报警下限		
	uint16_t   DeadBard;  //前后两次采样值差量
	uint32_t   Nouse;     //字节对其

	BD_para		BD;			
	uint8_t    SPACE[2];  //字节对其用	
	uint8_t		LRC_NUM[2];				
};


struct _SampleValue_
{
	
 unsigned short	sample_Vpoint_1;
 unsigned short	sample_Vpoint_2;
 unsigned short	sample_Vpoint_3;
 unsigned short	sample_HIGH;
 unsigned short	sample_LOW;
 unsigned short sample_NC;
};


struct _BDSampleVal_
{		  
	
	
 unsigned short BD5V_th;
 unsigned short BD5V_tl;
 unsigned short BD5V_th_currentval;
 unsigned short BD5V_tl_currentval;
 unsigned short BD5V_th_lastval;
 unsigned short BD5V_tl_lastval; 
 unsigned char  BD5V_thDelta;
 unsigned char  BD5V_tlDelta;
 unsigned char  BD5V_thDeltaCnt;
 unsigned char  BD5V_tlDeltaCnt;

	
 unsigned short BD20mA_tvh2;
 unsigned short BD20mA_tvh1;
 unsigned short BD20mA_tvl;
 unsigned char  BD20mA_tvh2Delta;
 unsigned char  BD20mA_tvh1Delta;
 unsigned char  BD20mA_tvh2DeltaCnt;
 unsigned char  BD20mA_tvh1DeltaCnt;

 unsigned char  BDSignalHorL;            //1:表示输入的信号是标定信号0-20mA的上限(20mA)  2:表示输入的信号标定信号0-20mA是标定信号的下限(0mA)                                          
                                         //3:表示输入的信号是标定信号0-5V的上限(5V)      4:表示输入的信号标定信号0-5V是标定信号的下限(0V)
 unsigned char  SignalCollectFinish;     //标定信号是否已经采样结束
 unsigned short BD20mA_tvh2_currentval;
 unsigned short BD20mA_tvh2_lastval;
 unsigned short BD20mA_tvh1_currentval;
 unsigned short BD20mA_tvh1_lastval;

 unsigned short BD20mA_tl_currentval;
 unsigned short BD20mA_tl_lastval;
 unsigned short SampleLOW;
 unsigned short SampleHIGH;
	
};

extern struct  _RemoteRTU_Module_  RTU[3]; 
extern struct  _SampleValue_     SAMPLE[3];
extern struct  _BDSampleVal_     BDSAMPLE[3];


/*****************单通道——PI结构体定义******************/
typedef enum { FAILED = 0, PASSED = !FAILED} TestStatus;

void init_stm32adc(void *arg);
int adc_test(void *buf, int size);


#endif
