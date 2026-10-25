#include "Task_Cloud.h"
#include "app_shared.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "ESP8266.h"
#include "Motor.h"
#include <string.h>
#include <stdio.h>

/*云平台配置*/
#define WIFI_SSID "Ciallo"				//你的WiFi名称
#define WIFI_PASSWORD "aaaa6666"		//WiFi密码
#define BEMFA_UID "bcaa27b45f69472f8d2e619c8624f0ce"
#define BEMFA_TOPIC "fan"				//巴法云主题

/**
  * 函    数：应用云端指令到本地状态（假定已持有xDataMutex）
  * 参    数：Level 目标档位，-1表示不修改档位仅按需求处理开关
  * 返 回 值：无
  */
static void ApplyCloudLevel(FanLevel_t Level)
{
	/*烟雾报警期间，云端指令不允许恢复电机运转，安全优先级最高*/
	if (g_AppData.SmokeAlarm)
	{
		return;
	}

	g_AppData.Mode = MODE_MANUAL;
	g_AppData.FanLevel = Level;
	g_AppData.FanStatus = (Level != FAN_OFF) ? 1 : 0;
	Motor_SetLevel((uint8_t)Level);
}

/**
  * 函    数：处理云端下发的控制指令
  * 参    数：无
  * 返 回 值：无
  */
static void ProcessCloudCommand(void)
{
	if (strstr(ESP8266_RxBuffer, "low") != NULL)
	{
		if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
		{
			ApplyCloudLevel(FAN_LOW);
			xSemaphoreGive(xDataMutex);
		}
		ESP8266_ClearBuffer();
	}
	else if (strstr(ESP8266_RxBuffer, "mid") != NULL)
	{
		if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
		{
			ApplyCloudLevel(FAN_MID);
			xSemaphoreGive(xDataMutex);
		}
		ESP8266_ClearBuffer();
	}
	else if (strstr(ESP8266_RxBuffer, "high") != NULL)
	{
		if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
		{
			ApplyCloudLevel(FAN_HIGH);
			xSemaphoreGive(xDataMutex);
		}
		ESP8266_ClearBuffer();
	}
	else if (strstr(ESP8266_RxBuffer, "off") != NULL)
	{
		if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
		{
			ApplyCloudLevel(FAN_OFF);
			xSemaphoreGive(xDataMutex);
		}
		ESP8266_ClearBuffer();
	}
	else if (strstr(ESP8266_RxBuffer, "auto") != NULL)
	{
		if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
		{
			if (!g_AppData.SmokeAlarm)
			{
				g_AppData.Mode = MODE_AUTO;		//切回自动模式，具体档位交由Task_FanControl下一轮循环重新计算
			}
			xSemaphoreGive(xDataMutex);
		}
		ESP8266_ClearBuffer();
	}
	else if (strstr(ESP8266_RxBuffer, "on") != NULL)
	{
		if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
		{
			if (!g_AppData.SmokeAlarm)
			{
				FanLevel_t Level = (g_AppData.FanLevel == FAN_OFF) ? FAN_LOW : g_AppData.FanLevel;
				ApplyCloudLevel(Level);
			}
			xSemaphoreGive(xDataMutex);
		}
		ESP8266_ClearBuffer();
	}
}

/**
  * 函    数：云端上传/指令处理任务
  * 参    数：pvParameters 任务参数（未使用）
  * 返 回 值：无
  * 说    明：迁移自原main.c中的ESP8266相关逻辑。初始化、连接WiFi和巴法云都是
  *           秒级耗时操作，全部改用vTaskDelay等待，不会阻塞其他任务运行。
  */
void Task_Cloud(void *pvParameters)
{
	AppData_t LocalData;
	char DataBuf[64];

	ESP8266_Init();

	ESP8266_ConnectWiFi(WIFI_SSID, WIFI_PASSWORD);
	ESP8266_ConnectBemfa(BEMFA_UID, BEMFA_TOPIC);

	for (;;)
	{
		ProcessCloudCommand();

		if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
		{
			LocalData = g_AppData;
			xSemaphoreGive(xDataMutex);

			sprintf(DataBuf, "Temp:%.1fC,Level:%d,Status:%d,Alarm:%d",
				LocalData.Temperature, LocalData.FanLevel, LocalData.FanStatus, LocalData.SmokeAlarm);
			ESP8266_SendToCloud(BEMFA_TOPIC, DataBuf);
		}

		vTaskDelay(pdMS_TO_TICKS(10000));		//每10秒上传一次，与原逻辑保持一致
	}
}
