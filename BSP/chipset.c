/**
  ******************************************************************************
  * @file    chipset.c
  * @author  sundh
  * @version V0.1.1
  * @date    2016-7-31
  * @brief   配置外设的时钟和中断
  *   
  ******************************************************************************
  * @attention
  *
  *
  * <h2><center>&copy; sundh </center></h2>
  ******************************************************************************
	---------------------------------------------------------------------------- 

	  Change History :                                                           

	  <Date>      <Version>  <Author>       | <Description>                    

	---------------------------------------------------------------------------- 

	  2016/07/31 | 0.1.1   	| sundh     		| Create file                      

	---------------------------------------------------------------------------- 

  */
#include <stdio.h>
#include "string.h"
#include "chipset.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_flash.h"
#include "hardwareConfig.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_iwdg.h"

#include "misc.h" /* High level functions for NVIC and SysTick (add-on to CMSIS functions) */
static vu32 TimingDelay;




/*! system dog*/
//void IWDG_Configuration(void)
//{

//    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);   /* 写入0x5555,用于允许狗狗寄存器写入功能 */
//    IWDG_SetPrescaler(IWDG_Prescaler_256);          /* 狗狗时钟分频,40K/256=156HZ(6.4ms)*/
//    IWDG_SetReload(781);                            /* 喂狗时间 5s/6.4MS=781 .注意不能大于0xfff*/
//    IWDG_ReloadCounter();                           /* 喂狗*/
//    IWDG_Enable();                                  /* 使能狗狗*/
//}


/** 
* @brief 系统时钟配置
*
*
* @param void   
*
* @return void
*
*/
void RCC_Configuration(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 /*| RCC_APB1Periph_TIM3*/,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOC  | RCC_APB2Periph_GPIOD  | RCC_APB2Periph_GPIOE, ENABLE);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);  //Enable UART4 clocks
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div8);	     //2014.5.18 
	// ADC Periph clock enable 

  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);   //2014.5.18
	
	
}

void IWDG_Configuration(void)
{
	
	
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);   /* 写入0x5555,允许向狗的寄存器写入 */
    IWDG_SetPrescaler(IWDG_Prescaler_256);          /* 时钟分频,40K/256=156HZ(6.4ms)*/
    IWDG_SetReload(468);                            /* 喂狗时间 3s/6.4MS=468 不得超过0xfff*/
    IWDG_ReloadCounter();                           /* 喂狗*/
    IWDG_Enable();                                  /* 使能狗*/
	
}

/*! System NVIC Configuration */
void NVIC_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

#ifdef VECT_TAB_RAM
    /* Set the Vector Table base location at 0x20000000 */
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else /* VECT_TAB_FLASH */
    /* Set the Vector Table base location at 0x08000000 */
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
#endif

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);//设置优先级分组形式1，即抢占级占一位，优先级占3位

    /* Enable the USART1 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);


    /* Enable the USART2 Interrupt*/
    NVIC_InitStructure.NVIC_IRQChannel=USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=0;
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Enable the USART3 Interrupt*/
    NVIC_InitStructure.NVIC_IRQChannel=USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=0;
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
    NVIC_Init(&NVIC_InitStructure);



/* Enable the DMA Interrupt */

    NVIC_InitStructure.NVIC_IRQChannel = DMA_gprs_usart.dma_tx_irq;   // 发送DMA配置
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;     // 优先级配置
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = DMA_s485_usart.dma_tx_irq;   // 发送DMA配置
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;     // 优先级配置
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = DMA_s485_usart.dma_rx_irq;    
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;     
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = DMA_adc.dma_rx_irq;   // ADC接收
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;     // 优先级配置
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = W25Q_Spi.irq;			//spi中断
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1 ;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
	
	
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}



///*! GPIO Configuration */
void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    // Configure the all GPIO port pins in Analog Input Mode(Floating input
    // trigger OFF), this will reduce the power consumption and increase the
    // device immunity against EMI/EMC
    // Enables or disables the High Speed APB(APB2) peripheral clock

    

	
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    //LED   PinLED_run
	
    GPIO_InitStructure.GPIO_Pin = PinLED_run.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(PinLED_run.Port, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = PinLED_com.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(PinLED_com.Port, &GPIO_InitStructure);
	
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOC, &GPIO_InitStructure);

	//ADC pins
	
	GPIO_InitStructure.GPIO_Pin = ADC_pins_4051A1.pin|ADC_pins_4051B1.pin|ADC_pins_4051C1.pin;		 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;  //2M时钟速度
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = ADC_pins_control0.pin | ADC_pins_control1.pin | ADC_pins_control2.pin;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; //2M时钟速度
	GPIO_Init(GPIOC, &GPIO_InitStructure);
#ifdef USE_STM3210E_EVAL
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF | RCC_APB2Periph_GPIOG, ENABLE);

    GPIO_Init(GPIOF, &GPIO_InitStructure);
    GPIO_Init(GPIOG, &GPIO_InitStructure);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF | RCC_APB2Periph_GPIOG, DISABLE);
#endif
}

///*! adc Configuration */
void adc_Configuration(void)
{
	
	GPIO_InitTypeDef GPIO_InitStructure;
	
	/*******PB5 PB6 PB7 用于切换模拟开关(输出)*********/
     GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;		 
     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //推挽输出
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;  //2M时钟速度
	 GPIO_Init(GPIOB, &GPIO_InitStructure);
   /**********************************************/	
	
	
	
	/*************PC1 PC2 PC3 设置电流 电压MOS****************/
     GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //推挽输出
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;  //2M时钟速度
	 GPIO_Init(GPIOC, &GPIO_InitStructure);
	 /*************************************/
}

///*! USART Configuration */
void USART_Configuration(void)
{
    USART_InitTypeDef USART_InitStructure;

    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_4;        //tx
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;                   //rx
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;                   //tx
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;                  //rx
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;                   //tx
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;                  //rx
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);


    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_11|GPIO_Pin_5|GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);


    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl =USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    /* Configure USART1 basic and asynchronous paramters */
    USART_Init(USART1, &USART_InitStructure);
    USART_Init(USART2, &USART_InitStructure);
    USART_Init(USART3, &USART_InitStructure);

//    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
//    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
//    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    /* Enable USART1 */
    USART_Cmd(USART1, ENABLE);
    USART_Cmd(USART2, ENABLE);
    USART_Cmd(USART3, ENABLE);
}


//设置10ms产生一次中断
void Init_TIM2(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseSturcture;			 //定义TIM结构体变量
    TIM_DeInit(TIM2);										 //复位时钟TIM2

    TIM_TimeBaseSturcture.TIM_Period = 10000;				  //定时器周期
    TIM_TimeBaseSturcture.TIM_Prescaler = 71;				  //72000000/72=1000000
    TIM_TimeBaseSturcture.TIM_ClockDivision = 0x00;				//TIM_CKD_DIV1    TIM2时钟分频
    TIM_TimeBaseSturcture.TIM_CounterMode = TIM_CounterMode_Up; //計數方式	   

    TIM_TimeBaseInit(TIM2,&TIM_TimeBaseSturcture);
																//初始化
    TIM_ClearFlag(TIM2,TIM_FLAG_Update);						//清除標誌
    TIM_ITConfig(TIM2, TIM_IT_Update,ENABLE);
    TIM_Cmd(TIM2, ENABLE);										//使能
}




void w25q_init_cs(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	//w25q cs
	GPIO_InitStructure.GPIO_Pin = W25Q_csPin.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(W25Q_csPin.Port, &GPIO_InitStructure);
}

void w25q_init_spi(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef spi_conf;
	

	///spi gpio init
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5   | GPIO_Pin_7 ;        //spi_sck spi_mosi
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;                   //spi_miso
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);




	SPI_StructInit( &spi_conf);
	/* Initialize the SPI_Direction member */
	spi_conf.SPI_Mode = SPI_Mode_Master;
	/* Initialize the SPI_CPOL member */
	spi_conf.SPI_CPOL = SPI_CPOL_High;
	/* Initialize the SPI_CPHA member */
	spi_conf.SPI_CPHA = SPI_CPHA_2Edge;
	/* Initialize the SPI_NSS member */
	spi_conf.SPI_NSS = SPI_NSS_Soft;
	
	/* Initialize the SPI_BaudRatePrescaler member */
	spi_conf.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	
	
	W25Q_Spi.config = &spi_conf;
	spi_init( &W25Q_Spi);
	spi_ioctl( &W25Q_Spi, CMD_SET_RXBLOCK);
	spi_ioctl( &W25Q_Spi, SET_RXWAITTIME_MS, 2000);
	spi_ioctl( &W25Q_Spi, CMD_SET_TXBLOCK);
	spi_ioctl( &W25Q_Spi, SET_TXWAITTIME_MS, 1000);
}
