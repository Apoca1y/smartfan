#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_shared.h"
#include "Motor.h"
#include "Key.h"

#include "Task_Safety.h"
#include "Task_FanControl.h"
#include "Task_Presence.h"
#include "Task_Sensor.h"
#include "Task_Display.h"
#include "Task_Cloud.h"

/*任务优先级，数值越大优先级越高（configMAX_PRIORITIES = 6，取值范围0~5）*/
#define PRIO_SAFETY			5		//最高：烟雾/可燃气体报警，响应必须最快
#define PRIO_FANCONTROL		4		//核心控制逻辑，事件标志组联动判断
#define PRIO_PRESENCE		3		//人体检测，维护无人计时
#define PRIO_SENSOR			3		//温湿度采集
#define PRIO_DISPLAY		2		//OLED显示，包括报警闪烁
#define PRIO_CLOUD			1		//云端通信，含秒级阻塞等待，优先级最低

/*各任务栈大小，单位：字（word），实际字节数 = 值 * 4*/
#define STACK_SAFETY		128
#define STACK_FANCONTROL	160
#define STACK_PRESENCE		96
#define STACK_SENSOR		128
#define STACK_DISPLAY		160
#define STACK_CLOUD			256		//sprintf等库函数栈开销较大，且ESP8266命令缓冲较多局部变量

/**
  * 函    数：主函数
  * 参    数：无
  * 返 回 值：无
  * 说    明：仅负责底层初始化、共享资源初始化、创建各任务、启动调度器。
  *           电机(Motor)和按键(Key)的初始化放在这里而不是任务内部，因为
  *           Motor_SetLevel会被Task_Safety(优先级最高)、Task_FanControl、Task_Cloud
  *           三个任务调用，必须保证调度器启动前GPIO就已配置完成，
  *           否则高优先级任务抢先调用时引脚还处于复位后的浮空输入状态，写寄存器无效。
  *           其余外设(PIR/AHT20/Gas/OLED/Buzzer/ESP8266)只被各自单一任务使用，
  *           初始化放在对应任务内部，便于把初始化和运行逻辑放在一起维护。
  */
int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);	//FreeRTOS要求：4位全部用作抢占优先级，无子优先级

	Delay_Init();			//初始化DWT周期计数器，供Delay_us/Delay_ms使用（不占用SysTick）

	Motor_Init();			//初始化TB6612方向引脚(PA1/PA2)和PWM(PA0)
	Key_Init();				//初始化按键PB14/PB13为上拉输入

	AppShared_Init();		//创建事件标志组和互斥量，必须在创建任务之前完成

	xTaskCreate(Task_Safety,     "Safety",  STACK_SAFETY,     NULL, PRIO_SAFETY,     NULL);
	xTaskCreate(Task_FanControl, "FanCtrl", STACK_FANCONTROL, NULL, PRIO_FANCONTROL, NULL);
	xTaskCreate(Task_Presence,   "Presence",STACK_PRESENCE,   NULL, PRIO_PRESENCE,   NULL);
	xTaskCreate(Task_Sensor,     "Sensor",  STACK_SENSOR,     NULL, PRIO_SENSOR,     NULL);
	xTaskCreate(Task_Display,    "Display", STACK_DISPLAY,    NULL, PRIO_DISPLAY,    NULL);
	xTaskCreate(Task_Cloud,      "Cloud",   STACK_CLOUD,      NULL, PRIO_CLOUD,      NULL);

	vTaskStartScheduler();	//启动FreeRTOS调度器，正常情况下不会返回

	/*若代码运行到这里，说明堆内存不足导致空闲任务/调度器启动失败*/
	while (1);
}

/**
  * 函    数：FreeRTOS内存分配失败钩子函数
  * 参    数：无
  * 返 回 值：无
  * 说    明：configUSE_MALLOC_FAILED_HOOK为1时必须提供此函数。
  *           进入死循环便于调试时通过调试器观察，实际产品建议记录日志后复位。
  */
void vApplicationMallocFailedHook(void)
{
	while (1);
}

/**
  * 函    数：FreeRTOS栈溢出检测钩子函数
  * 参    数：xTask 溢出的任务句柄，pcTaskName 溢出的任务名称
  * 返 回 值：无
  * 说    明：configCHECK_FOR_STACK_OVERFLOW为2时必须提供此函数。
  */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
	(void)xTask;
	(void)pcTaskName;
	while (1);
}
