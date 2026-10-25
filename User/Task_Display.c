#include "Task_Display.h"
#include "app_shared.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "OLED.h"
#include "Buzzer.h"

/**
  * 函    数：显示报警画面（烟雾/可燃气体检测到）
  * 参    数：Visible 本次是否显示文字内容（用于闪烁效果，交替传入1/0）
  * 返 回 值：无
  */
static void ShowAlarmScreen(uint8_t Visible)
{
	if (Visible)
	{
		OLED_ShowString(1, 1, "! GAS ALARM !  ");
		OLED_ShowString(2, 1, "Fan STOPPED     ");
		OLED_ShowString(3, 1, "Check for smoke ");
		OLED_ShowString(4, 1, "Press K1 to ack ");
	}
	else
	{
		OLED_Clear();
	}
}

/**
  * 函    数：显示正常运行画面
  * 参    数：无
  * 返 回 值：无
  */
static void ShowNormalScreen(const AppData_t *Data)
{
	float Temp = Data->Temperature;

	OLED_ShowString(1, 1, "T:");
	OLED_ShowNum(1, 3, (uint16_t)Temp, 2);
	OLED_ShowString(1, 5, ".");
	OLED_ShowNum(1, 6, (uint16_t)((Temp - (uint16_t)Temp) * 10), 1);
	OLED_ShowString(1, 7, "C H:");
	OLED_ShowNum(1, 11, (uint16_t)Data->Humidity, 2);
	OLED_ShowString(1, 13, "% ");

	OLED_ShowString(2, 1, "Mode:");
	OLED_ShowString(2, 6, Data->Mode == MODE_AUTO ? "Auto " : "Manu ");
	OLED_ShowString(2, 12, Data->PersonDetected ? "Person" : "Empty ");

	OLED_ShowString(3, 1, "Fan :");
	OLED_ShowString(3, 6, Data->FanStatus ? "ON  " : "OFF ");
	if (Data->SecurityForceFullSpeed)
	{
		OLED_ShowString(3, 11, "SEC!");	//安防联动全速排风提示
	}
	else
	{
		OLED_ShowString(3, 11, "    ");
	}

	OLED_ShowString(4, 1, "Level:");
	switch (Data->FanLevel)
	{
		case FAN_OFF:  OLED_ShowString(4, 7, "OFF "); break;
		case FAN_LOW:  OLED_ShowString(4, 7, "LOW "); break;
		case FAN_MID:  OLED_ShowString(4, 7, "MID "); break;
		case FAN_HIGH: OLED_ShowString(4, 7, "HIGH"); break;
	}
	OLED_ShowString(4, 12, "G:");
	OLED_ShowNum(4, 14, Data->GasADC, 4);
}

/**
  * 函    数：OLED显示任务
  * 参    数：pvParameters 任务参数（未使用）
  * 返 回 值：无
  * 说    明：检测到烟雾报警时切换为闪烁报警画面（500ms周期交替显示/清屏），
  *           同步驱动蜂鸣器"响-停"，形成声光同步报警；否则正常显示温湿度、
  *           模式、风扇状态等信息，且确保蜂鸣器保持关闭。
  */
void Task_Display(void *pvParameters)
{
	uint8_t BlinkVisible = 0;
	AppData_t LocalData;

	OLED_Init();
	Buzzer_Init();
	OLED_ShowString(1, 1, "Smart Fan Guard ");
	OLED_ShowString(2, 1, "System Starting ");
	vTaskDelay(pdMS_TO_TICKS(1000));
	OLED_Clear();

	for (;;)
	{
		if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
		{
			LocalData = g_AppData;		//拷贝一份本地副本，尽量缩短持锁时间
			xSemaphoreGive(xDataMutex);

			if (LocalData.SmokeAlarm)
			{
				BlinkVisible = !BlinkVisible;
				ShowAlarmScreen(BlinkVisible);
				if (BlinkVisible)
				{
					Buzzer_On();	//OLED文字显示时蜂鸣器同步响
				}
				else
				{
					Buzzer_Off();	//OLED清屏时蜂鸣器同步停，形成声光同步的"响-停"节奏
				}
			}
			else
			{
				Buzzer_Off();		//报警解除后确保蜂鸣器停止
				ShowNormalScreen(&LocalData);
			}
		}

		vTaskDelay(pdMS_TO_TICKS(500));		//报警闪烁周期约1秒（500ms显示+500ms清屏）
	}
}
