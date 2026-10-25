#include "stm32f10x.h"

/**
  * 说    明：引入FreeRTOS后，SysTick被系统调度器独占（见Start/startup_stm32f10x_md.s
  *           中SysTick_Handler已映射为xPortSysTickHandler），因此本文件改用
  *           Cortex-M3自带的DWT周期计数器实现忙等延时，不与FreeRTOS抢占SysTick。
  *           软件I2C（OLED）等需要精确us级延时的场合仍可安全调用Delay_us。
  *           此版本core_cm3.h（V3.5.0）未提供DWT寄存器定义，故在此直接用地址访问。
  */
#define DEMCR_ADDR		(*(volatile uint32_t *)0xE000EDFCUL)	//CoreDebug->DEMCR
#define DWT_CTRL_ADDR	(*(volatile uint32_t *)0xE0001000UL)	//DWT->CTRL
#define DWT_CYCCNT_ADDR	(*(volatile uint32_t *)0xE0001004UL)	//DWT->CYCCNT
#define DEMCR_TRCENA	(1UL << 24)
#define DWT_CTRL_CYCCNTENA	(1UL << 0)

/**
  * @brief  DWT周期计数器初始化，须在调用Delay_us/Delay_ms之前调用一次
  * @param  无
  * @retval 无
  */
void Delay_Init(void)
{
	DEMCR_ADDR |= DEMCR_TRCENA;			//使能跟踪功能
	DWT_CYCCNT_ADDR = 0;					//清零计数器
	DWT_CTRL_ADDR |= DWT_CTRL_CYCCNTENA;	//使能CYCCNT计数
}

/**
  * @brief  微秒级延时
  * @param  xus 延时时长，范围：0~4294967295（受限于CYCCNT 32位溢出周期，72MHz下约59秒溢出一次）
  * @retval 无
  */
void Delay_us(uint32_t xus)
{
	uint32_t StartTick = DWT_CYCCNT_ADDR;
	uint32_t Ticks = xus * (SystemCoreClock / 1000000);	//SystemCoreClock通常为72000000

	while ((DWT_CYCCNT_ADDR - StartTick) < Ticks);			//利用无符号数溢出特性，兼容CYCCNT回绕
}

/**
  * @brief  毫秒级延时
  * @param  xms 延时时长，单位：ms
  * @retval 无
  */
void Delay_ms(uint32_t xms)
{
	while (xms--)
	{
		Delay_us(1000);
	}
}

/**
  * @brief  秒级延时
  * @param  xs 延时时长，单位：s
  * @retval 无
  */
void Delay_s(uint32_t xs)
{
	while (xs--)
	{
		Delay_ms(1000);
	}
}
