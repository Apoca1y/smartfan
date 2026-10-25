#include "stm32f10x.h"                  // Device header
#include "PWM.h"

/**
  * 函    数：直流电机初始化（TB6612驱动）
  * 参    数：无
  * 返 回 值：无
  * 说    明：TB6612使用PA1(AIN1)、PA2(AIN2)控制方向，PWM(PA0)控制速度
  */
void Motor_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟

	/*GPIO初始化：PA1和PA2作为方向控制引脚*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA1和PA2引脚初始化为推挽输出

	/*初始状态：电机停止*/
	GPIO_ResetBits(GPIOA, GPIO_Pin_1);
	GPIO_ResetBits(GPIOA, GPIO_Pin_2);

	PWM_Init();													//初始化直流电机的底层PWM（PA0, TIM2_CH1）
}

/**
  * 函    数：直流电机设置档位
  * 参    数：Level 风力档位，0=关闭，1=低速，2=中速，3=高速
  * 返 回 值：无
  */
void Motor_SetLevel(uint8_t Level)
{
	if (Level == 0)							//档位0：关闭
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_1);	//AIN1 = 0
		GPIO_ResetBits(GPIOA, GPIO_Pin_2);	//AIN2 = 0
		PWM_SetCompare1(0);					//PWM = 0
	}
	else if (Level == 1)					//档位1：低速（PWM 20%）
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_1);	//AIN1 = 1
		GPIO_ResetBits(GPIOA, GPIO_Pin_2);	//AIN2 = 0，设置正转
		PWM_SetCompare1(20);				//PWM占空比20%
	}
	else if (Level == 2)					//档位2：中速（PWM 30%）
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_1);	//AIN1 = 1
		GPIO_ResetBits(GPIOA, GPIO_Pin_2);	//AIN2 = 0，设置正转
		PWM_SetCompare1(30);				//PWM占空比30%
	}
	else if (Level == 3)					//档位3：高速（PWM 40%）
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_1);	//AIN1 = 1
		GPIO_ResetBits(GPIOA, GPIO_Pin_2);	//AIN2 = 0，设置正转
		PWM_SetCompare1(40);				//PWM占空比40%
	}
}
