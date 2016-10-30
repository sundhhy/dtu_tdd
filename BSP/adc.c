
#include "adc.h"
#include "hardwareConfig.h"
#include "sdhError.h"
#define ADC_BUFLEN	200
static uint16_t ADCConvertedValue[ADC_BUFLEN];	//采样数据采样200个(滤除50HZ干扰)

void adc_init(void)
{   
	ADC_InitTypeDef ADC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;
	// PB1模拟输入(ADC采样口) 
	GPIO_InitStructure.GPIO_Pin = 	ADC_pins.pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; 		//模拟输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; 	//2M时钟速度
	GPIO_Init(ADC_pins.Port, &GPIO_InitStructure);

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
	
	
	
	//DMA1 channel1 configuration ---------------------------------------------
	DMA_Cmd(DMA_adc.dma_rx_base, DISABLE);                           
    DMA_DeInit(DMA_s485_usart.dma_rx_base);                                 
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&ADC_BASE->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ADCConvertedValue;         
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;                     
    DMA_InitStructure.DMA_BufferSize = ADC_BUFLEN;                     
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;        
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                 
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; 
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;         
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                           
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;                 
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                            
    DMA_Init(DMA_adc.dma_rx_base, &DMA_InitStructure);               
    DMA_ClearFlag( DMA_adc.dma_rx_flag);      
	DMA_ITConfig(DMA_adc.dma_rx_base, DMA_IT_TC, ENABLE); 	 // 允许传输完成中断

    DMA_Cmd(DMA_adc.dma_rx_base, ENABLE);                            
	
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


void DMA1_Channel1_IRQHandler(void)
{
  	if(DMA_GetITStatus(DMA1_IT_GL1)!= RESET)
	{
		/* Disable DMA1 channel1 */
  		DMA_Cmd(DMA1_Channel1, DISABLE);

		              		 
		
//		BusCheckEN=1;                            //ADC采样完成标志
		
		
		
		
		DMA_Cmd(DMA_adc.dma_rx_base, DISABLE);       // 关闭DMA
		DMA_ClearFlag( DMA_adc.dma_rx_flag );           // 清除DMA标志
		ADC_SoftwareStartConvCmd(ADC1, DISABLE);  //禁止ADC中断
		ADC_Cmd(ADC_BASE, DISABLE);	     
		DMA_adc.dma_rx_base->CNDTR = ADC_BUFLEN;
		DMA_Cmd( DMA_adc.dma_rx_base, ENABLE);
		
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


#endif


