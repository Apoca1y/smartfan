#include "app_shared.h"
#include "semphr.h"

EventGroupHandle_t xSecurityEvents = NULL;
SemaphoreHandle_t xDataMutex = NULL;

AppData_t g_AppData = {
	.Temperature = 27.0f,
	.Humidity = 50.0f,
	.GasADC = 0,
	.Mode = MODE_AUTO,
	.FanLevel = FAN_OFF,
	.FanStatus = 0,
	.PersonDetected = 0,
	.SmokeAlarm = 0,
	.SmokeAlarmClearRequest = 0,
	.SecurityForceFullSpeed = 0
};

/**
  * 函    数：共享资源初始化（事件标志组+互斥量）
  * 参    数：无
  * 返 回 值：无
  * 说    明：须在创建任何任务之前调用
  */
void AppShared_Init(void)
{
	xSecurityEvents = xEventGroupCreate();
	xDataMutex = xSemaphoreCreateMutex();

	/*创建失败通常意味着堆内存不足，此处进入死循环以便调试观察，
	  实际产品中可考虑触发系统复位或看门狗*/
	if (xSecurityEvents == NULL || xDataMutex == NULL)
	{
		while (1);
	}
}
