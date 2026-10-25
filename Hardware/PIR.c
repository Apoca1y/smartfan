#include "stm32f10x.h"                  // Device header

/**
  * 函    数：人体红外传感器初始化
  * 参    数：无
  * 返 回 值：无
  * 说    明：HC-SR501人体红外模块接PB11，数字输出
  *           复用原TCRT5000的引脚，但极性相反：
  *           HC-SR501检测到人体移动时输出高电平，无人时输出低电平
  *           因此使用下拉输入，避免悬空误触发
  */
void PIR_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟

	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;				//下拉输入
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);						//将PB11引脚初始化为下拉输入
}

/**
  * 函    数：读取人体红外传感器状态
  * 参    数：无
  * 返 回 值：1=检测到人体移动，0=未检测到
  * 说    明：HC-SR501检测到人体时输出高电平，无人时输出低电平
  */
uint8_t PIR_Get(void)
{
	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 1)		//读取PB11状态，高电平表示检测到人
	{
		return 1;		//检测到人
	}
	else
	{
		return 0;		//未检测到人
	}
}
