#include "stm32f10x.h"                  // Device header

/**
  * 函    数：蜂鸣器初始化
  * 参    数：无
  * 返 回 值：无
  * 说    明：有源蜂鸣器模块接PB12，数字输出（原PIR的引脚，PIR已改接PB11空出）
  *           大多数有源蜂鸣器模块是低电平触发（模块内置PNP管或直接反相驱动），
  *           初始状态先拉高确保上电时不会误响
  */
void Buzzer_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟

	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);						//将PB12引脚初始化为推挽输出

	GPIO_SetBits(GPIOB, GPIO_Pin_12);							//初始拉高，蜂鸣器不响
}

/**
  * 函    数：蜂鸣器开启（响）
  * 参    数：无
  * 返 回 值：无
  */
void Buzzer_On(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_12);						//拉低触发蜂鸣器
}

/**
  * 函    数：蜂鸣器关闭（停）
  * 参    数：无
  * 返 回 值：无
  */
void Buzzer_Off(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_12);							//拉高停止蜂鸣器
}
