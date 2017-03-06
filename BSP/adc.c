
#include "adc.h"
#include "hardwareConfig.h"
#include "sdhError.h"
#include <string.h>
#define ADC_BUFLEN	200


unsigned char  ConfigRecFinish;

unsigned long   TimingDelay;
unsigned short  ConfigCheckCnt;
unsigned short  ADC_timcount;                     //ADC触发时间
unsigned short  comout_count; 
unsigned char   BusCheckEN;
unsigned char   HalfSecCnt500mS;
unsigned short  HalfSecCnt5000mS;
unsigned char   Falg_HalfSec;
unsigned char   RunStat=0;                        //LED灯运行状态控制
unsigned char   sec_10;	

unsigned char  collectChannel;                    //当前的采集通道  
unsigned char  RUNorBD;                           //标定或者运行标志(RUNorBD =1表示为标定状态  RUNorBD ==0表示为运行状态)
unsigned char  sampledote0,sampleolddote0;        //通道0采样点
unsigned char  sampledote1,sampleolddote1;        //通道1采样点
unsigned char  sampledote2,sampleolddote2;        //通道2采样点

unsigned char  BD_sampledote,BD_sampleolddote;    //标定当前通道的采样信号点
unsigned short BUSADdelta;                       
unsigned char  BUSADdelataCnt;  
unsigned char  CHannel0finish, CHannel1finish, CHannel2finish;
unsigned short ADCval0 =0, ADCval1 =0,ADCval2=0;
float   AV_BUF0,AV_BUF1,AV_BUF2;
float   rang_f;
float	  ma_tie=1;
unsigned short	sample_V11;
unsigned short	sample_V12;
unsigned short	sample_V13;
unsigned short	sample_HIGH1;
unsigned short	sample_LOW1;
struct  _RemoteRTU_Module_  RTU[3];  
struct  _SampleValue_     SAMPLE[3];
struct  _BDSampleVal_     BDSAMPLE[3];

get_realVAl I_get_val = NULL;
get_rsv I__get_rsv = NULL;
get_alarm I_get_alarm = NULL;
static uint16_t ADCConvertedValue[ADC_BUFLEN];	//采样数据采样200个(滤除50HZ干扰)
static void DMA1_Configuration(void);
static void DealwithCollect(unsigned char channel);
static void init_stm32adc(void *arg);
static void FinishCollect(void);
static void system_para_init(void);

void Regist_get_val( get_realVAl get_val)
{
	
	I_get_val = get_val;
}

void Regist_get_rsv( get_rsv get_rsv)
{
	
	I__get_rsv = get_rsv;
}

void Regist_get_alarm( get_alarm get_alarm)
{
	
	I_get_alarm = get_alarm;
}

void Collect_job(void)
{
	
	DealwithCollect(collectChannel);
	FinishCollect();  
}


void ADC_50ms()
{
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//启动ADC
	ADC_Cmd(ADC1, ENABLE);                    //ADC转换使能
}

void up( char chn)
{
	if( I_get_val)
		I_get_val( CHANNEL0, RTU[chn].value);
	if( I__get_rsv)
		I__get_rsv( CHANNEL0, RTU[chn].RSV);
	if( I_get_alarm)
		I_get_alarm( CHANNEL0, RTU[chn].Alarm);
	
}

void Set_chnType( char chn, uint8_t type)
{
	if( chn > 2) 
		return;
	
	RTU[chn].Type = type;
	
}
void Set_rangH( char chn, uint16_t rang)
{
	if( chn > 2) 
		return;
	
	RTU[chn].RangeH = rang;
}
void Set_rangL( char chn, uint16_t rang)
{
	if( chn > 2) 
		return;
	
	RTU[chn].RangeL = rang;
}
void Set_alarmH( char chn, uint16_t alarm)
{
	if( chn > 2) 
		return;
	
	RTU[chn].HiAlm = alarm;
	
}
void Set_alarmL( char chn, uint16_t alarm)
{
	if( chn > 2) 
		return;
	
	RTU[chn].LowAlm = alarm;
	
}

int create_adc(void)
{
	init_stm32adc(NULL);
	system_para_init();
}


static void init_stm32adc(void *arg)
{   
	ADC_InitTypeDef ADC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;
	

  	// ADC1 configuration -------------------------------
	ADC_StructInit( &ADC_InitStructure);
	
  	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  	ADC_Init(ADC1, &ADC_InitStructure);	               //使用ADC1

    
    //常规转换序列1：通道10,MCP6S21输出，通道数据 
    ADC_RegularChannelConfig(ADC1, ADC_chn, 1, ADC_SampleTime_71Cycles5); 
  
  	//Enable ADC1 DMA 
  	ADC_DMACmd(ADC1, ENABLE);
	// 开启ADC的DMA支持（要实现DMA功能，还需独立配置DMA通道等参数）
  
  	// Enable ADC1 
  	ADC_Cmd(ADC1, ENABLE);

	// 下面是ADC自动校准，开机后需执行一次，保证精度 
  	// Enable ADC1 reset calibaration register    
  	ADC_ResetCalibration(ADC1);
  	// Check the end of ADC1 reset calibration register 
  	while(ADC_GetResetCalibrationStatus(ADC1));

  	// Start ADC1 calibaration 
  	ADC_StartCalibration(ADC1);
  	// Check the end of ADC1 calibration 
  	while(ADC_GetCalibrationStatus(ADC1));
	// ADC自动校准结束---------------
	
	
	
	DMA1_Configuration();                        
	
}


static void DMA1_Configuration( )
{
	DMA_InitTypeDef DMA_InitStructure;
	//DMA1 channel1 configuration ---------------------------------------------	
	DMA_DeInit( DMA_adc.dma_rx_base);
  	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&ADC1->DR);;
  	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)&ADCConvertedValue;
  	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  	DMA_InitStructure.DMA_BufferSize = ADC_BUFLEN;
  	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
  	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  	DMA_Init(DMA1_Channel1, &DMA_InitStructure);	 
  
  	DMA_ITConfig( DMA_adc.dma_rx_base,DMA_IT_TC,ENABLE);
	// Enable DMA1 channel1 
  	DMA_Cmd( DMA_adc.dma_rx_base, ENABLE);			 
}



/********************************************************************************************************
** Function name:        void series_se(unsigned char data)         
** Modified by:          通道切换函数(根据模拟开关选通不同的信号输入)
** Modified date:        2016-11-28
*********************************************************************************************************/
static void series_se(unsigned char data)
{
   switch(data)
   {
     case 0:  
		SelectCLR4051_A;	    //选通信号VX32
		SelectCLR4051_B;
		SelectCLR4051_C;
		break;   	  
	case 1:
		SelectSET4051_A;      //选通信号VX22
		SelectCLR4051_B;
		SelectCLR4051_C;
		break;

	case 2: 
		SelectCLR4051_A;	     //选通VX21信号
		SelectSET4051_B;
		SelectCLR4051_C; 
		break;

	case 3:
		SelectSET4051_A;  	   //选通VX31信号
		SelectSET4051_B;
		SelectCLR4051_C;  
		break;

	case 4:                            //选通VX12信号
		SelectCLR4051_A;
		SelectCLR4051_B;
		SelectSET4051_C;
		break;

	case 5:   
		SelectSET4051_A;       //选通GND信号
		SelectCLR4051_B;
		SelectSET4051_C;
		break;

	case 6: 
		SelectCLR4051_A;       //选通VX11信号
		SelectSET4051_B;
		SelectSET4051_C;
		break;

	case 7:                        //选通VR0基准信号
		SelectSET4051_A;
		SelectSET4051_B;
		SelectSET4051_C;
		break;

	default:                         //选通GND信号
		SelectSET4051_A;       
		SelectCLR4051_B;
		SelectSET4051_C;
		break;
	}	

}

/*********************************************************************************************************
** Function name:        void sw_select(unsigned char channel)        
** Modified by:          选择不同的信号通道channel0-channel2
** Modified date:        2016-11-28
** intput value:         channel为RTU的通道取值范围为 0-2     
*********************************************************************************************************/

static void sw_select(unsigned char channel)
{
	
	if(channel==0)
	{
		switch(RTU[channel].Type)       //选择channel0
		{

		  case V_5:                     //电压信号输入
			case V_1_5:
			CLR_currentnum0;              //关断Q1 MOS管
			
		   if(sampledote0<=2)           
			{
				 if(sampledote0==0)	          //采样VX11
				 {
					 series_se(VX11);
				 

				 }
				 if(sampledote0==1)            //采样GND
				 {
					 series_se(GND);
				 }
				 if(sampledote0==2)            //采样VR0_A
				 {
					 series_se(VR0_A);  
				 }
			}
			else 
			{
			     sampledote0 = 0;
			}
			
		    break;

			case mA0_10:	          //0-10mA 电流信号  
			case mA4_20:		        //4-20mA 电流信号
			SET_currentnum0;        //打开 Q1 MOS
				if(sampledote0<=3)
				{	
				   
					if(sampledote0==0)	series_se(VX12);  //采样VX12
					if(sampledote0==1)	series_se(VX11);  //采样VX11	
					if(sampledote0==2)	series_se(GND);   //采样GND	
					if(sampledote0==3)	series_se(VR0_A); //采样基准VR0
				}
				else
					sampledote0=0;
			break;

			default :
				break;
		}	
	}	
	
	else if(channel==1)
	{	
		switch(RTU[channel].Type)       //选择channel1
		{

		  case V_5:                     //电压信号输入
			case V_1_5:
			CLR_currentnum1;              //关断Q3 MOS管
			
		   if(sampledote1<=2)           
			{
				 if(sampledote1==0)	          //采样VX21
				 {
					 series_se(VX21);
				 

				 }
				 if(sampledote1==1)           //采样GND
				 {
					 series_se(GND);
				 }
				 if(sampledote1==2)           //采样VR0_A
				 {
					 series_se(VR0_A);  
				 }
			}
			else 
			{
			     sampledote1 = 0;
			}
			
		    break;

			case mA0_10:	          //0-10mA 电流信号  
			case mA4_20:		        //4-20mA 电流信号
				
			SET_currentnum1;        //打开 Q3 MOS管
				if(sampledote1<=3)
				{	
				   
					if(sampledote1==0)	series_se(VX22);  //采样VX22
					if(sampledote1==1)	series_se(VX21);  //采样VX21	
					if(sampledote1==2)	series_se(GND);   //采样GND	
					if(sampledote1==3)	series_se(VR0_A); //采样基准VR0_A
				}
				else
					sampledote1=0;
			break;

			default :
				break;
		}
		
	}
	
	else if(channel==2)
	{
		
		switch(RTU[channel].Type)       //选择channel2
		{

		  case V_5:                     //电压信号输入
			case V_1_5:
			CLR_currentnum2;              //关断Q4 MOS管
			
		   if(sampledote2<=2)           
			{
				 if(sampledote2==0)	          //采样VX31
				 {
					 series_se(VX31);
				 

				 }
				 if(sampledote2==1)           //采样GND
				 {
					 series_se(GND);
				 }
				 if(sampledote2==2)           //采样VR0_A
				 {
					 series_se(VR0_A);  
				 }
			}
			else 
			{
			     sampledote2 = 0;
			}
			
		    break;

			case mA0_10:	          //0-10mA 电流信号  
			case mA4_20:		        //4-20mA 电流信号
				
			SET_currentnum2;        //打开 Q4 MOS管
				if(sampledote2<=3)
				{	
				   
					if(sampledote2==0)	series_se(VX32);  //采样VX32
					if(sampledote2==1)	series_se(VX31);  //采样VX31	
					if(sampledote2==2)	series_se(GND);   //采样GND	
					if(sampledote2==3)	series_se(VR0_A); //采样基准VR0_A
				}
				else
					sampledote2=0;
			break;

			default :
		  break;
		}
		
	}
	
}

/*********************************************************************************************************
** Function name:        void sw_select(unsigned char channel)        
** Modified by:          选择不同的信号通道channel0-channel2
** Modified date:        2016-11-28
** intput value:         channel为RTU的通道取值范围为 0-2     
*********************************************************************************************************/

static void BD_sw_select(unsigned char channel,unsigned signaltype)
{
	
	if(channel==0)
	{
		switch(signaltype)              //选择channel0下对应的输入信号类型
		{

		  case V_5:                     //电压信号输入
			case V_1_5:
			CLR_currentnum0;              //关断Q1 MOS管
			
		   if(BD_sampledote<=2)           
			{
				 if(BD_sampledote==0)	       //采样VX11
				 {
					 series_se(VX11);
				 }
				 if(BD_sampledote==1)        //采样GND
				 {
					 series_se(GND);
				 }
				 if(BD_sampledote==2)        //采样VR0_A
				 {
					 series_se(VR0_A);  
				 }
			}
			else 
			{
			     BD_sampledote = 0;
			}
			
		  break;

			case mA0_10:	          //0-10mA 电流信号  
			case mA4_20:		        //4-20mA 电流信号
			SET_currentnum0;        //打开 Q1 MOS
				if(BD_sampledote<=3)
				{	
				   
					if(BD_sampledote==0)	series_se(VX12);  //采样VX12
					if(BD_sampledote==1)	series_se(VX11);  //采样VX11	
					if(BD_sampledote==2)	series_se(GND);   //采样GND	
					if(BD_sampledote==3)	series_se(VR0_A); //采样基准VR0
				}
				else
					BD_sampledote=0;
			break;

			default :
				break;
		}	
	}	
	
	else if(channel==1)                //选择channel1下输入的信号类型
	{	
		switch(signaltype)            
		{

		  case V_5:                     //电压信号输入
			case V_1_5:
			CLR_currentnum1;              //关断Q3 MOS管
			
		   if(BD_sampledote<=2)           
			{
				 if(BD_sampledote==0)	          //采样VX21
				 {
					 series_se(VX21);
				 

				 }
				 if(BD_sampledote==1)           //采样GND
				 {
					 series_se(GND);
				 }
				 if(BD_sampledote==2)           //采样VR0_A
				 {
					 series_se(VR0_A);  
				 }
			}
			else 
			{
			     BD_sampledote = 0;
			}
			
		  break;

			case mA0_10:	           //0-10mA 电流信号  
			case mA4_20:		         //4-20mA 电流信号
				
			SET_currentnum1;         //打开 Q3 MOS管
			if(BD_sampledote<=3)
			{	
				 
				if(BD_sampledote==0)	series_se(VX22);  //采样VX22
				if(BD_sampledote==1)	series_se(VX21);  //采样VX21	
				if(BD_sampledote==2)	series_se(GND);   //采样GND	
				if(BD_sampledote==3)	series_se(VR0_A); //采样基准VR0_A
			}
			else
				BD_sampledote=0;
			break;

			default :
				break;
		}
		
	}
	
	else if(channel==2)
	{
		
		switch(signaltype)              //选择channel2
		{

		  case V_5:                     //电压信号输入
			case V_1_5:
			CLR_currentnum2;              //关断Q4 MOS管
			
		   if(BD_sampledote<=2)           
			{
				 if(BD_sampledote==0)	          //采样VX31
				 {
					 series_se(VX31);
				 

				 }
				 if(BD_sampledote==1)           //采样GND
				 {
					 series_se(GND);
				 }
				 if(BD_sampledote==2)           //采样VR0_A
				 {
					 series_se(VR0_A);  
				 }
			}
			else 
			{
			     BD_sampledote = 0;
			}
			
		    break;

			case mA0_10:	          //0-10mA 电流信号  
			case mA4_20:		        //4-20mA 电流信号
				
			SET_currentnum2;        //打开 Q4 MOS管
				if(BD_sampledote<=3)
				{	
				   
					if(BD_sampledote==0)	series_se(VX32);  //采样VX32
					if(BD_sampledote==1)	series_se(VX31);  //采样VX31	
					if(BD_sampledote==2)	series_se(GND);   //采样GND	
					if(BD_sampledote==3)	series_se(VR0_A); //采样基准VR0_A
				}
				else
					BD_sampledote=0;
			break;

			default :
		  break;
		}
		
	}
	
}

/*********************************************************************************************************
** Function name:        void Data_Deal(unsigned char ch_temp,unsigned char pred)        
** Modified by:          对采样结果处理函数
** Modified date:        2016-12-01
** intput  value：       ch_temp 为RTU通道(取值范围为0-2)  pred 为针对当前RTU通道,采样的点的位置(取值范围 0-3)
*********************************************************************************************************/

void Data_Deal(unsigned char ch_temp,unsigned char pred)
{
	unsigned char  i_temp,j_temp,m_temp;
	unsigned long	data_temp;
	data_temp=0;
	
	
	switch(ch_temp)
	{

        //通道0

		case CHANNEL0:
			for(j_temp=0;j_temp<199;j_temp++) 	  //排序算法
			{
				for(m_temp=0;m_temp<199-j_temp;m_temp++)
				{
					if(ADCConvertedValue[m_temp]>ADCConvertedValue[m_temp+1])
					{
						data_temp=ADCConvertedValue[m_temp];
						ADCConvertedValue[m_temp]=ADCConvertedValue[m_temp+1];
						ADCConvertedValue[m_temp+1]=data_temp;
					}
				}
			}
			data_temp=0;
			for(i_temp=88;i_temp<113;i_temp++)
			{
				data_temp=data_temp+ADCConvertedValue[i_temp];
			}
			data_temp=data_temp/25;
			ADCval0=(unsigned short) data_temp;
			
			switch(pred)             //采样点的位置
			{
				case Samplepoint0:		                                                    //若为电流信号则采样VX12
					
					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)  
					{
										
						 SAMPLE[ch_temp].sample_Vpoint_2=(unsigned short) data_temp;
								
				  }

				
					else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)              //若是电压信号,则采样VX11
					{
					  				  
					  SAMPLE[ch_temp].sample_Vpoint_1 = (unsigned short) data_temp; 
				
					}

				break;	 
					 
				case Samplepoint1:                                                          //若为电流信号，则采样VX11 
					
					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)
						
					{
						 SAMPLE[ch_temp].sample_Vpoint_1=(unsigned short) data_temp;
					}
				 
					
			    	else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)	            //如果是电压信号(采样基准GND)
					{
					   SAMPLE[ch_temp].sample_LOW=(unsigned short) data_temp;
					}
				break;

				case Samplepoint2:		                                                     //若为电流信号采样GND
					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)
					{
						SAMPLE[ch_temp].sample_LOW=(unsigned short) data_temp;
					}
					
			    	else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)	             //若为电压信号(采样基准VR0)
					{
					    SAMPLE[ch_temp].sample_HIGH=(unsigned short) data_temp;

					    CHannel0finish = 1;
					}
				   
				break;

				case Samplepoint3:	                                                        //若为电流信号采样VR0
					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)
					{
						SAMPLE[ch_temp].sample_HIGH=(unsigned short) data_temp;
						CHannel0finish = 1;
					}
					else
					{
             ;
					}
				    
				break;

				default :
					break;
			}
			
			break;

      //通道1

			case CHANNEL1:

				for(j_temp=0;j_temp<199;j_temp++) 	  //排序算法
				{
					for(m_temp=0;m_temp<199-j_temp;m_temp++)
					{
						if(ADCConvertedValue[m_temp]>ADCConvertedValue[m_temp+1])
						{
							data_temp=ADCConvertedValue[m_temp];
							ADCConvertedValue[m_temp]=ADCConvertedValue[m_temp+1];
							ADCConvertedValue[m_temp+1]=data_temp;
						}
					}
				}
				data_temp=0;
				for(i_temp=88;i_temp<113;i_temp++)
				{
					data_temp=data_temp+ADCConvertedValue[i_temp];
				}
				data_temp=data_temp/25;
				ADCval1=(unsigned short) data_temp;

				switch(pred)                                                              //采样点的位置
				{
				case Samplepoint0:		                                                    //若为电流信号则采样VX22

					if(RTU[ch_temp].Type== mA0_10||RTU[ch_temp].Type== mA4_20)  
					{

						SAMPLE[ch_temp].sample_Vpoint_2=(unsigned short) data_temp;

					}


					else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)              //若是电压信号,则采样VX21
					{

						SAMPLE[ch_temp].sample_Vpoint_1 = (unsigned short) data_temp; 

					}

					break;	 

				case Samplepoint1:                                                       //若为电流信号，则采样VX21 

					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)

					{
						SAMPLE[ch_temp].sample_Vpoint_1=(unsigned short) data_temp;
					}


					else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)	            //如果是电压信号(采样基准GND)
					{
						SAMPLE[ch_temp].sample_LOW=(unsigned short) data_temp;
					}
					break;

				case Samplepoint2:		                                                     //若为电流信号采样GND
					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)
					{
						SAMPLE[ch_temp].sample_LOW=(unsigned short) data_temp;
					}

					else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)	             //若为电压信号(采样基准VR0)
					{
						SAMPLE[ch_temp].sample_HIGH=(unsigned short) data_temp;

						CHannel1finish = 1;
					}

					break;

				case Samplepoint3:	                                                      //若为电流信号采样VR0
					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)
					{
						SAMPLE[ch_temp].sample_HIGH=(unsigned short) data_temp;
						CHannel1finish = 1;
					}
					else
					{
						;
					}

					break;



				default :
					break;
				}
				break;




				//通道2

			case CHANNEL2:

				for(j_temp=0;j_temp<199;j_temp++) 	  //排序算法
				{
					for(m_temp=0;m_temp<199-j_temp;m_temp++)
					{
						if(ADCConvertedValue[m_temp]>ADCConvertedValue[m_temp+1])
						{
							data_temp=ADCConvertedValue[m_temp];
							ADCConvertedValue[m_temp]=ADCConvertedValue[m_temp+1];
							ADCConvertedValue[m_temp+1]=data_temp;
						}
					}
				}
				data_temp=0;
				for(i_temp=88;i_temp<113;i_temp++)
				{
					data_temp=data_temp+ADCConvertedValue[i_temp];
				}
				data_temp=data_temp/25;
				ADCval2=(unsigned short) data_temp;

				switch(pred)             //采样点的位置
				{
				case Samplepoint0:		                                                    //若为电流信号则采样VX32

					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)  
					{

						SAMPLE[ch_temp].sample_Vpoint_2=(unsigned short) data_temp;

					}


					else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)              //若是电压信号,则采样VX31
					{

						SAMPLE[ch_temp].sample_Vpoint_1 = (unsigned short) data_temp; 

					}

					break;	 

				case Samplepoint1:                                                       //若为电流信号，则采样VX31 

					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)

					{
						SAMPLE[ch_temp].sample_Vpoint_1=(unsigned short) data_temp;
					}


					else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)	            //如果是电压信号(采样基准GND)
					{
						SAMPLE[ch_temp].sample_LOW=(unsigned short) data_temp;
					}
					break;

				case Samplepoint2:		                                                     //若为电流信号采样GND
					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)
					{
						SAMPLE[ch_temp].sample_LOW=(unsigned short) data_temp;
					}

					else if(RTU[ch_temp].Type==V_5||RTU[ch_temp].Type==V_1_5)	             //若为电压信号(采样基准VR0)
					{
						SAMPLE[ch_temp].sample_HIGH=(unsigned short) data_temp;

						CHannel2finish = 1;
					}

					break;

				case Samplepoint3:	                                                      //若为电流信号采样VR0
					if(RTU[ch_temp].Type==mA0_10||RTU[ch_temp].Type==mA4_20)
					{
						SAMPLE[ch_temp].sample_HIGH=(unsigned short) data_temp;
						CHannel2finish = 1;
					}
					else
					{
						;
					}

					break;



				default :
					break;
				}
				break;
	
		default :
			break;
		
	}
	
		
}
//将实际采样到的码值转换成实际电压值或者电流值

void Calculate(unsigned char channel)
{
	
	
	float	projectval,bdhigh,bdlow,atemp1,atemp2,tdata1,tdata2,ax1,ax2;
	float	temp_data1,temp_data2,temp_data3,temp_data4,temp_data5,temp_data6;
	switch(channel)
	{
		case CHANNEL0:

			switch(RTU[channel].Type)
			{

				case V_5:		                                    //电压信号0-5V 或者1-5V
				case V_1_5:
		       bdhigh=(float)RTU[channel].BD.BD_5_HIGH/1000;
				   bdlow=(float)RTU[channel].BD.BD_5_LOW/100000;
				   atemp1=(((float)SAMPLE[channel].sample_Vpoint_1)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   tdata2=atemp1*bdhigh+bdlow;                  //获取实际的电压值
				   if(tdata2<0.0005)	tdata2=0;
				   if(AV_BUF0>10.0)     
				   {
					   AV_BUF0=0;	
				   }		
				   if((RTU[channel].Type==V_5)&&(AV_BUF0<0.0005))    //0-5V信号
				   {
						   AV_BUF0=0;	

				   }
				   if((RTU[channel].Type==V_1_5)&&(AV_BUF0<0.0005))  // 1-5V 信号
				   {
						   AV_BUF0=1.0;
				   }				   
				   ax1=tdata2*1000;
				   ax2=AV_BUF0*1000;
				   if(ax1-ax2>=10)
				     AV_BUF0=tdata2;
				   if(ax2-ax1>=10)
				     AV_BUF0=tdata2;	
				   if((ax2-ax1<=0.2)&&(tdata2==0))
					   AV_BUF0=tdata2;
				   if((ax1-ax2<=0.2)&&(tdata2==0))
					   AV_BUF0=tdata2;				   
				     tdata2=tdata2*0.1+AV_BUF0*0.9;    //迟滞滤波 0.9*旧值+0.1新值  
				   if((tdata2<10)&&(tdata2>0.0005))
						 AV_BUF0=tdata2;

				   if(SAMPLE[channel].sample_Vpoint_1<(SAMPLE[channel].sample_LOW-0x200)) //断线这里不好做
				   {
				     tdata2=0;
					   AV_BUF0=0;
					   RTU[channel].Alarm=0x02;         //断线标志
				   }
				   else
				   {
				     RTU[channel].Alarm=0x00;
				   }
				   if(RTU[channel].Type==V_1_5)       //信号为1-5V信号  
				   {
				     if(tdata2<1.0)
					   tdata2=1.0;
						  RTU[channel].Alarm=0x02;         //断线标志
				   }
					 
					 #if 0 
				   tdata2=tdata2/5*30000;
				   if(tdata2>30000) tdata2=30000;
					 #endif
					 
					  projectval = tdata2/5*30000;
					  if(projectval>30000) projectval =30000;
					
					   if(tdata2>5.0)
							 tdata2 =5.0;
					 
					 if(RTU[channel].Type==V_5)                   //0-5V 信号类型
					 {
						 
						   RTU[channel].RSV = 0+(projectval-0)*(65535-0)/(30000-0); //转换成0-65535的工程值
						   RTU[channel].value = tdata2;             //获得实际AV值
						  
						 
           }
					 else if (RTU[channel].Type==V_1_5)           // 1--5V 信号类型
					 {
						 if(tdata2<1.0)
						 {
							  tdata2 = 1.0; 
							  projectval = 6000;
							  
             }
						 else if(projectval>30000)
						 {
							 projectval = 30000; 
							 tdata2 = 5.0;
             }
						 RTU[channel].RSV = 0+(projectval-6000)*(65535-0)/(30000-6000);   //转换成0-65535的工程值
						 RTU[channel].value = tdata2;
						 
           }
					 
					break;

				case mA0_10:                                    //0-10mA
				case mA4_20:                                    //4-20mA
					
		       bdhigh=(float)RTU[channel].BD.BD_20mA_HIGH/100;
				   bdlow=(float)RTU[channel].BD.BD_20mA_LOW;
				   atemp1=(((float)SAMPLE[channel].sample_Vpoint_1)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   atemp2=(((float)SAMPLE[channel].sample_Vpoint_2)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   tdata1=atemp1*((float)RTU[channel].BD.BD_5_HIGH/1000)+((float)RTU[channel].BD.BD_5_LOW/100000);
				   tdata2=atemp2*((float)RTU[channel].BD.BD_5_HIGH/1000)+((float)RTU[channel].BD.BD_5_LOW/100000);
				   rang_f=(tdata1-tdata2)/ma_tie*1000;
				   tdata2=(tdata1-tdata2)/bdhigh*1000;
				   if(tdata2<0.0005)	tdata2=0;
				   if((RTU[channel].Type==mA0_10)&&(AV_BUF0<0.0005))
				   {
						   AV_BUF0=0;	

				   }
				   ax1=tdata2*1000;
				   ax2=AV_BUF0*1000;
				   if(ax1-ax2>=50)
				     AV_BUF0=tdata2;
				   if(ax2-ax1>=50)
				     AV_BUF0=tdata2;			
				   if((ax2-ax1<=0.2)&&(tdata2==0))
					   AV_BUF0=tdata2;
				   if((ax1-ax2<=0.2)&&(tdata2==0))
					   AV_BUF0=tdata2;				   
				    tdata2=tdata2*0.1+AV_BUF0*0.9;
				   if((tdata2<30)&&(tdata2>0.0005))
						AV_BUF0=tdata2;
				   if((RTU[channel].Type==mA4_20)&&(AV_BUF0<3.960))
				   {
				     tdata2=4.0;
					   AV_BUF0=0;
					   RTU[channel].Alarm=0x02; //断线
				   }
				   else
				   {
				     RTU[channel].Alarm=0x00;
				   }
					 #if 0 
				   tdata2=tdata2/20*30000;
				   if(tdata2>30000) tdata2=30000;
					 # endif
					 
					  projectval = tdata2/20*30000;
					  if(projectval>30000) projectval =30000;
					  if(tdata2>20.0)
							 tdata2 =20.0;
					 
					 if(RTU[channel].Type==mA0_10)    //0-10mA 信号类型
					 {
						  if(projectval>15000) 
						 {
							  projectval = 15000; 
							  tdata2 =10.0;
             }
						 if(tdata2>10.0)
						 {
							 tdata2 =10.0;
							 projectval =15000;
						 }
						 
						 RTU[channel].RSV =  0+(projectval-0)*(65535-0)/(15000-0); //转换成0-65535的工程?
						 RTU[channel].value = tdata2;
           }
					 else if (RTU[channel].Type==mA4_20) // 4--20mA 信号类型
					 {
						 if(projectval<6000) 
						 {
							  projectval = 6000; 
							  tdata2 = 4.0;
							 
             }
						 else if(projectval>30000)
						 {
							 projectval = 30000;
               tdata2 =20.0;							 
             }
						 if(tdata2<4.0)
						 {
							 tdata2 =4.0;
							 projectval = 6000;
						 }
						 else if(tdata2>20.0)
						 {
							 projectval = 30000;
						 }
						 RTU[channel].RSV = 0+(projectval-6000)*(65535-0)/(30000-6000);  //转换成0-65535之间的值
						 RTU[channel].value = tdata2;
           }
					 
					break;
				default:
					break;
				   
			}
			break;
			
			case CHANNEL1:
			switch(RTU[channel].Type)
			{
				
				  case V_5:		                                    //电压信号0-5V 或者1-5V
				  case V_1_5:
		       bdhigh=(float)RTU[channel].BD.BD_5_HIGH/1000;
				   bdlow=(float)RTU[channel].BD.BD_5_LOW/100000;
				   atemp1=(((float)SAMPLE[channel].sample_Vpoint_1)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   tdata2=atemp1*bdhigh+bdlow;                    //获取实际的电压值
				   if(tdata2<0.0005)	tdata2=0;
				   if(AV_BUF1>10.0)     
				   {
					   AV_BUF1=0;	
				   }		
				   if((RTU[channel].Type==V_5)&&(AV_BUF1<0.0005))    //0-5V信号
				   {
						   AV_BUF1=0;	
				   }
				   if((RTU[channel].Type==V_1_5)&&(AV_BUF1<0.0005))  // 1-5V 信号
				   {
						   AV_BUF1=1.0;
				   }				   
				   ax1=tdata2*1000;
				   ax2=AV_BUF1*1000;
				   if(ax1-ax2>=10)
				     AV_BUF1=tdata2;
				   if(ax2-ax1>=10)
				     AV_BUF1=tdata2;	
				   if((ax2-ax1<=0.2)&&(tdata2==0))
					   AV_BUF1=tdata2;
				   if((ax1-ax2<=0.2)&&(tdata2==0))
					   AV_BUF1=tdata2;				   
				     tdata2=tdata2*0.1+AV_BUF1*0.9;    //迟滞滤波 0.9*旧值+0.1新值  
				   if((tdata2<10)&&(tdata2>0.0005))
						 AV_BUF1=tdata2;

				   if(SAMPLE[channel].sample_Vpoint_1<(SAMPLE[channel].sample_LOW-0x200)) //断线这里不好做
				   {
				     tdata2=0;
					   AV_BUF1=0;
					   RTU[channel].Alarm=0x02;         //断线标志
				   }
				   else
				   {
				     RTU[channel].Alarm=0x00;
				   }
				   if(RTU[channel].Type==V_1_5)       //信号为1-5V信号  
				   {
				     if(tdata2<1.0)
					   tdata2=1.0;
						  RTU[channel].Alarm=0x02;         //断线标志
				   }
					 
					 #if 0 
				   tdata2=tdata2/5*30000;
				   if(tdata2>30000) tdata2=30000;
					 #endif
					 
					  projectval = tdata2/5*30000;
					  if(projectval>30000) projectval =30000;
					
					   if(tdata2>5.0)
							 tdata2 =5.0;
					 
					 if(RTU[channel].Type==V_5)                   //0-5V 信号类型
					 {
						 
						   RTU[channel].RSV = 0+(projectval-0)*(65535-0)/(30000-0); //转换成0-65535的工程值
						   RTU[channel].value = tdata2;             //获得实际AV值
						  
						 
           }
					 else if (RTU[channel].Type==V_1_5)           // 1--5V 信号类型
					 {
						 if(tdata2<1.0)
						 {
							  tdata2 = 1.0; 
							  projectval = 6000;
							  
             }
						 else if(projectval>30000)
						 {
							 projectval = 30000; 
							 tdata2 = 5.0;
             }
						 RTU[channel].RSV = 0+(projectval-6000)*(65535-0)/(30000-6000);   //转换成0-65535的工程值
						 RTU[channel].value = tdata2;
						 
           }
					 
					break;

				case mA0_10:                                    //0-10mA
				case mA4_20:                                    //4-20mA
					
		       bdhigh=(float)RTU[channel].BD.BD_20mA_HIGH/100;
				   bdlow=(float)RTU[channel].BD.BD_20mA_LOW;
				   atemp1=(((float)SAMPLE[channel].sample_Vpoint_1)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   atemp2=(((float)SAMPLE[channel].sample_Vpoint_2)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   tdata1=atemp1*((float)RTU[channel].BD.BD_5_HIGH/1000)+((float)RTU[channel].BD.BD_5_LOW/100000);
				   tdata2=atemp2*((float)RTU[channel].BD.BD_5_HIGH/1000)+((float)RTU[channel].BD.BD_5_LOW/100000);
				   rang_f=(tdata1-tdata2)/ma_tie*1000;
				   tdata2=(tdata1-tdata2)/bdhigh*1000;
				   if(tdata2<0.0005)	tdata2=0;
				   if((RTU[channel].Type==mA0_10)&&(AV_BUF1<0.0005))
				   {
						   AV_BUF1=0;	

				   }
				   ax1=tdata2*1000;
				   ax2=AV_BUF1*1000;
				   if(ax1-ax2>=50)
				     AV_BUF1=tdata2;
				   if(ax2-ax1>=50)
				     AV_BUF1=tdata2;			
				   if((ax2-ax1<=0.2)&&(tdata2==0))
					   AV_BUF1=tdata2;
				   if((ax1-ax2<=0.2)&&(tdata2==0))
					   AV_BUF1=tdata2;				   
				    tdata2=tdata2*0.1+AV_BUF1*0.9;
				   if((tdata2<30)&&(tdata2>0.0005))
						AV_BUF1=tdata2;
				   if((RTU[channel].Type==mA4_20)&&(AV_BUF1<3.960))
				   {
				     tdata2=4.0;
					   AV_BUF1=1;
					   RTU[channel].Alarm=0x02; //断线
				   }
				   else
				   {
				     RTU[channel].Alarm=0x00;
				   }
					 #if 0 
				   tdata2=tdata2/20*30000;
				   if(tdata2>30000) tdata2=30000;
					 # endif
					 
					  projectval = tdata2/20*30000;
					  if(projectval>30000) projectval =30000;
					  if(tdata2>20.0)
							 tdata2 =20.0;
					 
					 if(RTU[channel].Type==mA0_10)    //0-10mA 信号类型
					 {
						  if(projectval>15000) 
						 {
							  projectval = 15000; 
							  tdata2 =10.0;
             }
						 if(tdata2>10.0)
						 {
							 tdata2 =10.0;
							 projectval =15000;
						 }
						 
						 RTU[channel].RSV =  0+(projectval-0)*(65535-0)/(15000-0); //转换成0-65535的工程?
						 RTU[channel].value = tdata2;
           }
					 else if (RTU[channel].Type==mA4_20) // 4--20mA 信号类型
					 {
						 if(projectval<6000) 
						 {
							  projectval = 6000; 
							  tdata2 = 4.0;
							 
             }
						 else if(projectval>30000)
						 {
							 projectval = 30000;
               tdata2 =20.0;							 
             }
						 if(tdata2<4.0)
						 {
							 tdata2 =4.0;
							 projectval = 6000;
						 }
						 else if(tdata2>20.0)
						 {
							 projectval = 30000;
						 }
						 RTU[channel].RSV = 0+(projectval-6000)*(65535-0)/(30000-6000);  //转换成0-65535之间的值
						 RTU[channel].value = tdata2;
           }
					 
					break;
				default:
					break;
				
			}	
			break;
			
			case CHANNEL2:
		  switch(RTU[channel].Type)
			{
			 	case V_5:		                                    //电压信号0-5V 或者1-5V
				case V_1_5:
		       bdhigh=(float)RTU[channel].BD.BD_5_HIGH/1000;
				   bdlow=(float)RTU[channel].BD.BD_5_LOW/100000;
				   atemp1=(((float)SAMPLE[channel].sample_Vpoint_1)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   tdata2=atemp1*bdhigh+bdlow;                  //获取实际的电压值
				   if(tdata2<0.0005)	tdata2=0;
				   if(AV_BUF2>10.0)     
				   {
					   AV_BUF2=0;	
				   }		
				   if((RTU[channel].Type==V_5)&&(AV_BUF2<0.0005))    //0-5V信号
				   {
						   AV_BUF2=0;	

				   }
				   if((RTU[channel].Type==V_1_5)&&(AV_BUF2<0.0005))  // 1-5V 信号
				   {
						   AV_BUF2=1.0;
				   }				   
				   ax1=tdata2*1000;
				   ax2=AV_BUF2*1000;
				   if(ax1-ax2>=10)
				     AV_BUF2=tdata2;
				   if(ax2-ax1>=10)
				     AV_BUF2=tdata2;	
				   if((ax2-ax1<=0.2)&&(tdata2==0))
					   AV_BUF2=tdata2;
				   if((ax1-ax2<=0.2)&&(tdata2==0))
					   AV_BUF2=tdata2;				   
				     tdata2=tdata2*0.1+AV_BUF2*0.9;    //迟滞滤波 0.9*旧值+0.1新值  
				   if((tdata2<10)&&(tdata2>0.0005))
						 AV_BUF2=tdata2;

				   if(SAMPLE[channel].sample_Vpoint_1<(SAMPLE[channel].sample_LOW-0x200)) //断线这里不好做
				   {
				     tdata2=0;
					   AV_BUF2=0;
					   RTU[channel].Alarm=0x02;         //断线标志
				   }
				   else
				   {
				     RTU[channel].Alarm=0x00;
				   }
				   if(RTU[channel].Type==V_1_5)       //信号为1-5V信号  
				   {
				     if(tdata2<1.0)
					   tdata2=1.0;
						  RTU[channel].Alarm=0x02;         //断线标志
				   }
					 
					 #if 0 
				   tdata2=tdata2/5*30000;
				   if(tdata2>30000) tdata2=30000;
					 #endif
					 
					  projectval = tdata2/5*30000;
					  if(projectval>30000) projectval =30000;
					
					   if(tdata2>5.0)
							 tdata2 =5.0;
					 
					 if(RTU[channel].Type==V_5)                   //0-5V 信号类型
					 {
						 
						   RTU[channel].RSV = 0+(projectval-0)*(65535-0)/(30000-0); //转换成0-65535的工程值
						   RTU[channel].value = tdata2;             //获得实际AV值
						  	 
           }
					 else if (RTU[channel].Type==V_1_5)           // 1--5V 信号类型
					 {
						 if(tdata2<1.0)
						 {
							  tdata2 = 1.0; 
							  projectval = 6000;
							  
             }
						 else if(projectval>30000)
						 {
							 projectval = 30000; 
							 tdata2 = 5.0;
             }
						 RTU[channel].RSV = 0+(projectval-6000)*(65535-0)/(30000-6000);   //转换成0-65535的工程值
						 RTU[channel].value = tdata2;
						 
          }
					 
					break;

				case mA0_10:                                    //0-10mA
				case mA4_20:                                    //4-20mA
					
		       bdhigh=(float)RTU[channel].BD.BD_20mA_HIGH/100;
				   bdlow=(float)RTU[channel].BD.BD_20mA_LOW;
				   atemp1=(((float)SAMPLE[channel].sample_Vpoint_1)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   atemp2=(((float)SAMPLE[channel].sample_Vpoint_2)-(float)SAMPLE[channel].sample_LOW)/((float)SAMPLE[channel].sample_HIGH-(float)SAMPLE[channel].sample_LOW);
				   tdata1=atemp1*((float)RTU[channel].BD.BD_5_HIGH/1000)+((float)RTU[channel].BD.BD_5_LOW/100000);
				   tdata2=atemp2*((float)RTU[channel].BD.BD_5_HIGH/1000)+((float)RTU[channel].BD.BD_5_LOW/100000);
				   rang_f=(tdata1-tdata2)/ma_tie*1000;
				   tdata2=(tdata1-tdata2)/bdhigh*1000;
				   if(tdata2<0.0005)	tdata2=0;
				
				   if((RTU[channel].Type==mA0_10)&&(AV_BUF2<0.0005))
				   {
						   AV_BUF2=0;	
				   }
					 
				   ax1=tdata2*1000;
				   ax2=AV_BUF2*1000;
				   if(ax1-ax2>=50)
				     AV_BUF2=tdata2;
				   if(ax2-ax1>=50)
				     AV_BUF2=tdata2;			
				   if((ax2-ax1<=0.2)&&(tdata2==0))
					   AV_BUF2=tdata2;
				   if((ax1-ax2<=0.2)&&(tdata2==0))
					   AV_BUF2=tdata2;				   
				    tdata2=tdata2*0.1+AV_BUF2*0.9;
				   if((tdata2<30)&&(tdata2>0.0005))
						AV_BUF2=tdata2;
				   if((RTU[channel].Type==mA4_20)&&(AV_BUF2<3.960))
				   {
				     tdata2=4.0;
					   AV_BUF2=0;
					   RTU[channel].Alarm=0x02; //断线
				   }
				   else
				   {
				     RTU[channel].Alarm=0x00;
				   }
					 #if 0 
				   tdata2=tdata2/20*30000;
				   if(tdata2>30000) tdata2=30000;
					 # endif
					 
					  projectval = tdata2/20*30000;
					  if(projectval>30000) projectval =30000;
					  if(tdata2>20.0)
							 tdata2 =20.0;
					 
					 if(RTU[channel].Type==mA0_10)    //0-10mA 信号类型
					 {
						  if(projectval>15000) 
						 {
							  projectval = 15000; 
							  tdata2 =10.0;
             }
						 if(tdata2>10.0)
						 {
							 tdata2 =10.0;
							 projectval =15000;
						 }
						 
						 RTU[channel].RSV =  0+(projectval-0)*(65535-0)/(15000-0); //转换成0-65535的工程?
						 RTU[channel].value = tdata2;
           }
					 else if (RTU[channel].Type==mA4_20) // 4--20mA 信号类型
					 {
						 if(projectval<6000) 
						 {
							  projectval = 6000; 
							  tdata2 = 4.0;
							 
             }
						 else if(projectval>30000)
						 {
							 projectval = 30000;
               tdata2 =20.0;							 
             }
						 if(tdata2<4.0)
						 {
							 tdata2 =4.0;
							 projectval = 6000;
						 }
						 else if(tdata2>20.0)
						 {
							 projectval = 30000;
						 }
						 RTU[channel].RSV = 0+(projectval-6000)*(65535-0)/(30000-6000);  //转换成0-65535之间的值
						 RTU[channel].value = tdata2;
           }
				break;
					 
				default: 
					break;
	 				
			}
			break;
			
			default:
			break;
	}
	
}

static void system_para_init(void)
{

   /**********ADC采样相关变量初始化*********/
	  AV_BUF0 = 0; 
	  AV_BUF1 = 0;
	  AV_BUF2 = 0;
    
   /****************************************/
	  memset(&RTU[0],0,sizeof(RTU[0]));
	  memset(&RTU[1],0,sizeof(RTU[1]));
	  memset(&RTU[2],0,sizeof(RTU[2]));
    sec_10 =0;
	  RUNorBD =0;
	  collectChannel =0;   //当前采集通道初始化为0 
	  BusCheckEN =0;
	  RTU[CHANNEL0].Type = V_5;
	  RTU[CHANNEL1].Type = V_5;
	  RTU[CHANNEL2].Type = V_5;
	  RTU[CHANNEL0].BD.BD_5_HIGH = 5075;  
	  RTU[CHANNEL0].BD.BD_5_LOW = 911; 
	  RTU[CHANNEL1].BD.BD_5_HIGH = 5082;  
	  RTU[CHANNEL1].BD.BD_5_LOW = 788; 	
	  RTU[CHANNEL2].BD.BD_5_HIGH = 5082;  
	  RTU[CHANNEL2].BD.BD_5_LOW = 788; 
	  sampledote0 = 0;
    sampledote1 = 0;
    sampledote2 = 0;
	  sampleolddote0 =0;
	  sampleolddote1 =0;
   	sampleolddote2 =0;
	  CHannel0finish =0;
	  CHannel1finish =0; 
	  CHannel2finish =0;
	
    	
	  
   /****************************************/
	 /****************************************/

}

//处理采样点的切换过程
static void DealwithCollect(unsigned char channel)
{
	if(BusCheckEN==1)
	 {
		  
		  BusCheckEN =0; 
		  switch(channel)
			{
				case CHANNEL0:
				sampleolddote0 = sampledote0;
				sampledote0++;   
				sw_select(CHANNEL0);  
        DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,DISABLE);
				TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);		
				Data_Deal(CHANNEL0,sampleolddote0);
				DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE);
				TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); 
				break;
			  
				case CHANNEL1:
				sampleolddote1 = sampledote1;
				sampledote1++;
				sw_select(CHANNEL1);
				DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,DISABLE);
				TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);	
				Data_Deal(CHANNEL1,sampleolddote1);
				DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE);
				TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
				break;
				
				case CHANNEL2:
				sampleolddote2 = sampledote2;
				sampledote2++;
				sw_select(CHANNEL2);
				DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,DISABLE);
				TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);	
				Data_Deal(CHANNEL2,sampleolddote2);
				DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE);
				TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
				break;
				
				default:
				break;
				
			}
		 	 
	 }
}



//判断处理完一个通道的数据采集
static void FinishCollect(void)
{
	if((CHannel0finish==1)||(CHannel1finish==1)||(CHannel2finish==1))
	{
		 if(CHannel0finish==1)            //通道0采样完成？
		 {
			  CHannel0finish =0;
			  DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,DISABLE);
			  TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE); 
			  sampleolddote0 =0;
			  sampledote0 =0;
			  Calculate(CHANNEL0);          //处理并转换通道0
			 up( CHANNEL0);
			  if(collectChannel==0)         //采集完当前的输入通道,切换到下一个通道
				{
					collectChannel =1;
					sw_select(CHANNEL1);        //准备采样通道1
				}
				DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE);
				TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); 
		 }
		 else if(CHannel1finish ==1)         //通道1 采样完成?
		 {
			   CHannel1finish =0;
			   DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,DISABLE);
			   TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE); 
			   sampleolddote1 =0;
			   sampledote1 =0;
			   Calculate(CHANNEL1);          //处理并转换通道1
			 up( CHANNEL1);
			   if(collectChannel==1)         //采集完当前的输入通道,切换到下一个通道
				 {
					 collectChannel =2;
					 sw_select(CHANNEL2);        //准备采集通道2 
				 }
				 
				DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE);
				TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); 
		 }
		 
		 else if(CHannel2finish==1)          //通道2 采样完成?
		 {
			 CHannel2finish=0;
			 DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,DISABLE);
			 TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
			sampleolddote2 =0;
			 sampledote2 =0;	
			Calculate(CHANNEL2);           //处理并转换通道2
			 up( CHANNEL2);
			 if(collectChannel==2)
			 {
				 collectChannel =0;
				 sw_select(CHANNEL0);         //准备采样通道0
			 }
			 DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE);
			 TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
       			 
		 }
		 
	}
				 
				
}

int adc_start( void *base, int chn)
{
	
	return ERR_OK;
}

//读取原始采样数据，返回值是数据的长度
//当返回值<0的时候表示失败
int adc_getRawData( void *base, int chn, void *out_data)
{
	
	return 0;
}

void digit_filtering( void *in_data, int in_len, void *out_data, int *out_len)
{
	
	
}

//计算实时值,将采样码值转换成电压值
int calc_voltage(void *in_data, int in_len, void *out_val)
{
	
	
}

//将电压值转换成工程值
int calc_engiVal( void *voltage, void *engival)
{
	
}
int adc_test(void *buf, int size)
{
	int len = 0;
	int vlt = 0;
	float engi = 00;
	while(1)
	{
		//一次数据采集的完整的处理过程
		adc_start( ADC_BASE, ADC_chn);
		len = adc_getRawData( ADC_BASE, ADC_chn, buf);
		digit_filtering( buf, len, buf, &len);
		calc_voltage( buf, len, &vlt);
		calc_engiVal( &vlt, &engi);
		
	
		
		
		
	}
	return ERR_OK;
}

//DMA1通道1（ADC）中断
//接受完64个数据进中断
void DMA1_Channel1_IRQHandler(void)
{
  	if(DMA_GetITStatus(DMA1_IT_GL1)!= RESET)
	{
		/* Disable DMA1 channel1 */
  		DMA_Cmd(DMA1_Channel1, DISABLE);

		ADC_SoftwareStartConvCmd(ADC1, DISABLE);  //禁止ADC中断

		ADC_Cmd(ADC1, DISABLE);	                   		 
		//常规转换序列1：通道10,MCP6S21输出，通道数据 
    	//ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5); 
		BusCheckEN=1;                            //ADC采样完成标志
		// dote0++;
		// if(dote0>3)
		//  dote0 = 0;
		//  sw_select(0);
		DMA1_Configuration();
		DMA_ClearFlag(DMA1_IT_GL1);
	}

}




#if 0
void Data_Deal(unsigned char ch_temp,unsigned char pred)
{
	unsigned char  i_temp,j_temp,m_temp;
	unsigned int	data_temp;
	data_temp=0;
	switch(ch_temp)
	{
		case 0:
			for(j_temp=0;j_temp<200;j_temp++) 	  //排序算法
			{
				for(m_temp=0;m_temp<200-j_temp;m_temp++)
				{
					if(ADCConvertedValue[m_temp]>ADCConvertedValue[m_temp+1])
					{
						data_temp=ADCConvertedValue[m_temp];
						ADCConvertedValue[m_temp]=ADCConvertedValue[m_temp+1];
						ADCConvertedValue[m_temp+1]=data_temp;
					}
				}
			}
			data_temp=0;
			for(i_temp=88;i_temp<113;i_temp++)
			{
				data_temp=data_temp+ADCConvertedValue[i_temp];
			}
			data_temp=data_temp/25;
			test_data0=(unsigned short) data_temp;
			
			switch(pred)
			{
				case 0:		       //采样VX12(实际信号low)
					
					if(RTU.Type==8||RTU.Type==9)
					{
					  	if(signal_select==3)	  //输入采样信号20mA的时候，对应的200R电阻下端的电压值
						{
						            BD20mA_tvh2_currentval = (unsigned short) data_temp;
									if(BD20mA_tvh2_currentval>=BD20mA_tvh2_lastval)
									{
										BUSADError20mA_tvh2 = BD20mA_tvh2_currentval- BD20mA_tvh2_lastval;
									} 
									else
									{
										BUSADError20mA_tvh2 = BD20mA_tvh2_lastval - BD20mA_tvh2_currentval; 
									}	
									if(BUSADError20mA_tvh2>10)
									{
										 BDADCnt20mA_tvh2 = 0;  
									}	
		                            else
		                            {  
											 BDADCnt20mA_tvh2++;
									   if(BDADCnt20mA_tvh2>100)
									   {
									      BDADCnt20mA_tvh2 = 0;
									      BD20mA_tvh2 = BD20mA_tvh2_currentval;
										  BD_restart = 3;
		                               }	
		 								 
								    }
								
						BD20mA_tvh2_lastval = BD20mA_tvh2_currentval;      
					   }
					
						else if( signal_select==4)     //输入电流0-20mA基准是的 0mA
						{
								 BD20mA_tl = 0; 
                                 BD_finish = 2;							
						}
						else
						{
								sample_V12=(unsigned short) data_temp;
						}
					
						
				}

				
					else if(RTU.Type==10||RTU.Type==11)          //如果是电压信号 采样值VX11
					{
					  
                       if(signal_select==1)	                     //BD状态时采样到外部输入5V 基准信号
					  {
					        BD5V_th_currentval =  (unsigned short) data_temp;
							if(BD5V_th_currentval>=BD5V_th_lastval)
							{
							  	  BUSADError_5Vth =BD5V_th_currentval-BD5V_th_lastval;
							}
							else 
							{
							      BUSADError_5Vth =BD5V_th_lastval-BD5V_th_currentval;
							}

	
							if(BUSADError_5Vth>10)
							{
							   BDADCnt_5Vth = 0;
							}
							else 
							{
							     BDADCnt_5Vth++;
								 if(BDADCnt_5Vth>100)
								 {
								    BDADCnt_5Vth = 0;
								    BD5V_th = BD5V_th_currentval;
									  BD_restart = 2;                        //采样完0-5V 信号的  5V 基准之后，进入键盘读取采样0V的状态 
									 
								 }
							}
                         
						BD5V_th_lastval = BD5V_th_currentval;    

					  }

					  else if(signal_select==2)                 //BD状态时采样到0-5V外部输入 0V
					  {
						     BD5V_tl_currentval =  (unsigned short) data_temp;
							if(BD5V_tl_currentval>=BD5V_tl_lastval)
							{
							  	  BUSADError_5Vtl =BD5V_tl_currentval-BD5V_tl_lastval;
							}
							else 
							{
							      BUSADError_5Vtl = 	BD5V_tl_lastval - BD5V_tl_currentval;
							}


							if(BUSADError_5Vtl>10)
							{
							   BDADCnt_5Vtl = 0;
							}
							else 
							{
							    BDADCnt_5Vtl++;
								 if(BDADCnt_5Vtl>100)
								 {
								    BDADCnt_5Vtl = 0;
								    BD5V_tl = BD5V_tl_currentval;
									  BD_finish = 1;
									 
								 }
							}
                         
						  BD5V_tl_lastval = BD5V_tl_currentval;    

					  }

					  else
					  {
					    sample_V11 = (unsigned short) data_temp; 
					  }

					}

				break;	          //采样VX11(实际信号high)
				case 1:
					if(RTU.Type==8||RTU.Type==9)
					{
						if(signal_select==3)       //输入采样信号20mA的时候，对应的200R电阻上端的电压值
						{
							       BD20mA_tvh1_currentval = (unsigned short) data_temp;
									if(BD20mA_tvh1_currentval>=BD20mA_tvh1_lastval)
									{
										BUSADError20mA_tvh1 = BD20mA_tvh1_currentval- BD20mA_tvh1_lastval;
									} 
									else
									{
										BUSADError20mA_tvh1 = BD20mA_tvh1_lastval - BD20mA_tvh1_currentval; 
									}	
									if(BUSADError20mA_tvh1>10)
									{
										 BDADCnt20mA_tvh1 = 0;  
									}	
		                             else
		                            {  
											 BDADCnt20mA_tvh1++;
											 if(BDADCnt20mA_tvh1>100)
											 {
											      BDADCnt20mA_tvh1 = 0;
											      BD20mA_tvh1 = BD20mA_tvh1_currentval;
												  BD_restart = 4;
				                             }	
		 								 
								    }
								
						               BD20mA_tvh1_lastval = BD20mA_tvh1_currentval;   
                      }
					else if( signal_select==4)         //输入电流0-20mA基准是的 0mA
					{
						 BD20mA_tl = 0; 
                         BD_finish = 2;									
					}
					else
					{
						 sample_V11=(unsigned short) data_temp;
					}


					}
				 
			    	else if(RTU.Type==10||RTU.Type==11)	      //如果是电压信号(采样基准GND)
					{
					   sample_LOW1=(unsigned short) data_temp;
					}
				break;

				case 2:		 //采样GND
					if(RTU.Type==8||RTU.Type==9)
					{
						sample_LOW1=(unsigned short) data_temp;
					}
					
			    	else if(RTU.Type==10||RTU.Type==11)	     //如果是电压信号(采样基准VR0)
					{
					    sample_HIGH1=(unsigned short) data_temp;
					    ch0_over = 1;
					}
				   
				break;

				case 3:	 //采样VR0
					if(RTU.Type==8||RTU.Type==9)
					{
						sample_HIGH1=(unsigned short) data_temp;
						ch0_over=1;
					}
					else
					{

					}
					 /*
					else if(RTU.Type==10||RTU.Type==11)	     //如果是电压信号(采样基准VR0)
					{
					    sample_NC=(unsigned short) data_temp;
					    ch0_over = 1;
					}
					*/
				    
				break;

				   /*
				case 4:
				      	if(RTU.Type==8||RTU.Type==9)
					{
						sample_NC=(unsigned short) data_temp;
						ch0_over=1;
					}

					else 
					{

					}

				  break;
				 */

				default :
					break;
			}
			break;
		
		
		default :
			break;
		
	}
	
	if( (BD_restart ==3)||(BD_restart == 4))   //采样完0-20mA信号的20mA  进入采样0mA (置位相应的标志)
	{
		   BD_restart =5;  
    }
		
}

void Calculate(unsigned char ch_temp)
{
	float	bdhigh,bdlow,atemp1,atemp2,tdata1,tdata2,ax1,ax2;
	
	float	temp_data1,temp_data2,temp_data3,temp_data4,temp_data5,temp_data6;
	switch(ch_temp)
	{
		case 0:

			switch(RTU.Type)
			{

				case 10:		                //电压信号0-5V 或者1-5V
				case 11:
		       bdhigh=(float)RTU.BD.BD_5_HIGH/1000;
				   bdlow=(float)RTU.BD.BD_5_LOW/100000;
				   atemp1=(((float)sample_V11)-(float)sample_LOW1)/((float)sample_HIGH1-(float)sample_LOW1);
				   tdata2=atemp1*bdhigh+bdlow;
				   if(tdata2<0.0005)	tdata2=0;
				   if(AV_BUF0>10)
				   {
					   AV_BUF0=0;	
				   }		
				   if((RTU.Type==V_5)&&(AV_BUF0<0.0005))
				   {
						   AV_BUF0=0;	

				   }
				   if((RTU.Type==V_1_5)&&(AV_BUF0<0.0005))
				   {
						   AV_BUF0=1.0;
				   }				   
				   ax1=tdata2*1000;
				   ax2=AV_BUF0*1000;
				   if(ax1-ax2>=10)
				     AV_BUF0=tdata2;
				   if(ax2-ax1>=10)
				     AV_BUF0=tdata2;	
				   if((ax2-ax1<=0.2)&&(tdata2==0))
					   AV_BUF0=tdata2;
				   if((ax1-ax2<=0.2)&&(tdata2==0))
					   AV_BUF0=tdata2;				   
				   tdata2=tdata2*0.1+AV_BUF0*0.9;
				   
				   if((tdata2<10)&&(tdata2>0.0005))
						AV_BUF0=tdata2;

				   if(sample_V11<(sample_LOW1-0x200))
				   {
				     tdata2=0;
					 AV_BUF0=0;
					 RTU.Alarm=0x02; //断线
				   }
				   else
				   {
				     RTU.Alarm=0x00;
				   }
				   if(RTU.Type==V_1_5)
				   {
				     if(tdata2<1.0)
					   tdata2=1;
				   }
				   tdata2=tdata2/5*30000;
				   if(tdata2>30000) tdata2=30000;
					 
					 if(RTU.Type==10)        //0-5V 信号类型
					 {
						 RTU.AV = tdata2*65535/30000;     //转换成工程值
           }
					 else if (RTU.Type==11) // 1--5V 信号类型
					 {
						 if(tdata2<6000) 
						 {
							  tdata2 = 6000; 
             }
						 else if(tdata2>30000)
						 {
							 tdata2 = 30000; 
             }
						 RTU.AV = ((tdata2-6000) *65535/24000 );  //转换成工程值
           }
				  // RTU.AV=tdata2;      //2014.11.27 llj
					break;

				case 8:  //0-10mA
				case 9:  //4-20mA
		       bdhigh=(float)RTU.BD.BD_20mA_HIGH/100;
				   bdlow=(float)RTU.BD.BD_20mA_LOW;
				   atemp1=(((float)sample_V11)-(float)sample_LOW1)/((float)sample_HIGH1-(float)sample_LOW1);
				   atemp2=(((float)sample_V12)-(float)sample_LOW1)/((float)sample_HIGH1-(float)sample_LOW1);
				   tdata1=atemp1*((float)RTU.BD.BD_5_HIGH/1000)+((float)RTU.BD.BD_5_LOW/100000);
				   tdata2=atemp2*((float)RTU.BD.BD_5_HIGH/1000)+((float)RTU.BD.BD_5_LOW/100000);
				   rang_f=(tdata1-tdata2)/ma_tie*1000;
				   tdata2=(tdata1-tdata2)/bdhigh*1000;
				   if(tdata2<0.0005)	tdata2=0;
				   if((RTU.Type==8)&&(AV_BUF0<0.0005))
				   {
						   AV_BUF0=0;	

				   }

				   ax1=tdata2*1000;
				   ax2=AV_BUF0*1000;
				   if(ax1-ax2>=50)
				     AV_BUF0=tdata2;
				   if(ax2-ax1>=50)
				     AV_BUF0=tdata2;			
				   if((ax2-ax1<=0.2)&&(tdata2==0))
					   AV_BUF0=tdata2;
				   if((ax1-ax2<=0.2)&&(tdata2==0))
					   AV_BUF0=tdata2;				   
				   tdata2=tdata2*0.1+AV_BUF0*0.9;
				   
				   if((tdata2<30)&&(tdata2>0.0005))
						AV_BUF0=tdata2;

				   if((RTU.Type==mA4_20)&&(AV_BUF0<3.960))
				   {
				     tdata2=4;
					 AV_BUF0=0;
					 RTU.Alarm=0x02; //断线
				   }
				   else
				   {
				     RTU.Alarm=0x00;
				   }
				   tdata2=tdata2/20*30000;
				   if(tdata2>30000) tdata2=30000;
					 
					 if(RTU.Type==8)        //0-10mA 信号类型
					 {
						  if(tdata2>15000) 
						 {
							  tdata2 = 15000; 
             }
						 RTU.AV = tdata2*65535/15000;     //转换成工程值
           }
					 else if (RTU.Type==9) // 4--20mA 信号类型
					 {
						 if(tdata2<6000) 
						 {
							  tdata2 = 6000; 
             }
						 else if(tdata2>30000)
						 {
							 tdata2 = 30000; 
                         }
						 RTU.AV = ((tdata2-6000) *65535/24000 );  //转换成工程值
           }
					 
			//	   RTU.AV=tdata2;
					break;
				default:
					break;
				   
			}
			break;

			default :
			break;
	}
	
}



//DMA1通道1（ADC）中断
//接受完64个数据进中断
void DMA1_Channel1_IRQHandler(void)
{
  	if(DMA_GetITStatus(DMA1_IT_GL1)!= RESET)
	{
		/* Disable DMA1 channel1 */
  		DMA_Cmd(DMA1_Channel1, DISABLE);

		ADC_SoftwareStartConvCmd(ADC1, DISABLE);  //禁止ADC中断

		ADC_Cmd(ADC1, DISABLE);	                   		 
		//常规转换序列1：通道10,MCP6S21输出，通道数据 
    	//ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5); 
		BusCheckEN=1;                            //ADC采样完成标志
		// dote0++;
		// if(dote0>3)
		//  dote0 = 0;
		//  sw_select(0);
		DMA1_Configuration();
		DMA_ClearFlag(DMA1_IT_GL1);
	}

}
#endif


