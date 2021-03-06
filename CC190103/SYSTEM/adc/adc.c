#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "queue.h"

#include "../adc/adc.h"



u16  *ADC_V=NULL;
/**
  * @brief  配置ADC的GPIO
  * @param  无
  * @retval 无
  */
static void Rheostat_ADC_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// 使能 GPIO 时钟
	RCC_AHB1PeriphClockCmd(HIGH_ADC_GPIO_CLK, ENABLE);
		
	// 配置 IO
	GPIO_InitStructure.GPIO_Pin = HIGH_ADC_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;	    
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ; //不上拉不下拉
	GPIO_Init(HIGH_ADC_GPIO_PORT, &GPIO_InitStructure);			
}

/**
  * @brief  配置ADC，DMA传输
  * @param  无
  * @retval 无
  */
static void Rheostat_ADC_Mode_Config(void)
{
  DMA_InitTypeDef DMA_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
	
  // 开启ADC时钟
	RCC_APB2PeriphClockCmd(HIGH_ADC_CLK , ENABLE);
	
	// ------------------DMA Init 结构体参数 初始化--------------------------
  // ADC1使用DMA2，数据流0，通道0，这个是手册固定死的
  // 开启DMA时钟
  RCC_AHB1PeriphClockCmd(HIGH_ADC_DMA_CLK, ENABLE); 
	// 外设基址为：ADC 数据寄存器地址
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&HIGH_ADC_DR_ADDR;	
  // 存储器地址，实际上就是一个内部SRAM的变量	
	DMA_InitStructure.DMA_Memory0BaseAddr = (u32)ADC_V;  
  // 数据传输方向为外设到存储器	
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;	
	// 缓冲区大小为1，缓冲区的大小应该等于存储器的大小
	DMA_InitStructure.DMA_BufferSize = 8;	
	// 外设寄存器只有一个，地址不用递增
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  // 存储器地址固定
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 
  // // 外设数据大小为半字，即两个字节 
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; 
  //	存储器数据大小也为半字，跟外设数据大小相同
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;	
	// 循环传输模式
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  // DMA 传输通道优先级为高，当使用一个DMA通道时，优先级设置不影响
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  // 禁止DMA FIFO	，使用直连模式
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;  
  // FIFO 大小，FIFO模式禁止时，这个不用配置	
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;  
	// 选择 DMA 通道，通道存在于流中
  DMA_InitStructure.DMA_Channel = HIGH_ADC_DMA_CHANNEL; 
  //初始化DMA流，流相当于一个大的管道，管道里面有很多通道
	DMA_Init(HIGH_ADC_DMA_STREAM, &DMA_InitStructure);
	// 使能DMA流
  DMA_Cmd(HIGH_ADC_DMA_STREAM, ENABLE);

  // -------------------ADC Common 结构体 参数 初始化------------------------
	// 独立ADC模式
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
  // 时钟为fpclk x分频	
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
  // 禁止DMA直接访问模式	
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  // 采样时间间隔	
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;  
  ADC_CommonInit(&ADC_CommonInitStructure);
	
  // -------------------ADC Init 结构体 参数 初始化--------------------------
  // ADC 分辨率
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  // 禁止扫描模式，多通道采集才需要	
  ADC_InitStructure.ADC_ScanConvMode = ENABLE; 
  // 连续转换	
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE; 
  //禁止外部边沿触发
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
  //使用软件触发，外部触发不用配置，注释掉即可
  //ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
  //数据右对齐	
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  //转换通道 1个
  ADC_InitStructure.ADC_NbrOfConversion = 8;                                    
  ADC_Init(HIGH_ADC, &ADC_InitStructure);
  //---------------------------------------------------------------------------
	
  // 配置 ADC 通道转换顺序为1，第一个转换，采样时间为3个时钟周期
  ADC_RegularChannelConfig(HIGH_ADC, 0, 1, ADC_SampleTime_3Cycles); 
  ADC_RegularChannelConfig(HIGH_ADC, 1, 2, ADC_SampleTime_3Cycles); 
  ADC_RegularChannelConfig(HIGH_ADC, 2, 3, ADC_SampleTime_3Cycles); 
  ADC_RegularChannelConfig(HIGH_ADC, 3, 4, ADC_SampleTime_3Cycles); 
  ADC_RegularChannelConfig(HIGH_ADC, 4, 5, ADC_SampleTime_3Cycles); 
  ADC_RegularChannelConfig(HIGH_ADC, 5, 6, ADC_SampleTime_3Cycles); 
  ADC_RegularChannelConfig(HIGH_ADC, 6, 7, ADC_SampleTime_3Cycles); 
  ADC_RegularChannelConfig(HIGH_ADC, 7, 8, ADC_SampleTime_3Cycles); 
  // 使能DMA请求 after last transfer (Single-ADC mode)
  ADC_DMARequestAfterLastTransferCmd(HIGH_ADC, ENABLE);
  // 使能ADC DMA
  ADC_DMACmd(HIGH_ADC, ENABLE);
  // 使能ADC
  ADC_Cmd(HIGH_ADC, ENABLE);  
  //开始adc转换，软件触发
  ADC_SoftwareStartConv(HIGH_ADC);
}

/**
  * @brief  ADC1初始化
  * @param  无
  * @retval 无
  */
void ADC8_Init(void)
{

        ADC_V=pvPortMalloc(2*8);  
	Rheostat_ADC_GPIO_Config();
	Rheostat_ADC_Mode_Config();
}

void ADC8(void* PV)
{
ADC8_Init();

vTaskDelete(NULL);//杀掉进程

while(1)
{
vTaskDelay(50/portTICK_PERIOD_MS);//50ms刷新

}


}


/*********************************************END OF FILE**********************/
