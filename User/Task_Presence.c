#include "Task_Presence.h"
#include "app_shared.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "PIR.h"

/**
  * 函    数：人体检测任务
  * 参    数：pvParameters 任务参数（未使用）
  * 返 回 值：无
  * 说    明：轮询PIR人体红外传感器，累计"无人"的持续时间。
  *           无人持续达到NO_PERSON_HOLD_MS后，置位事件标志组的EVT_NO_PERSON位，
  *           一旦重新检测到人，立即清除该位。
  *           这里用FreeRTOS的tick计数（xTaskGetTickCount）代替原来裸机版本
  *           TIM3硬件定时器的1ms计数，逻辑更简洁，也不再需要独占一个定时器。
  */
void Task_Presence(void *pvParameters)
{
	TickType_t NoPersonStartTick = 0;
	uint8_t WasNoPerson = 0;		//上一轮循环是否处于"无人"状态

	PIR_Init();

	for (;;)
	{
		uint8_t Detected = PIR_Get();

		if (Detected)
		{
			/*检测到人：立即清除无人标志，重置计时起点*/
			WasNoPerson = 0;
			xEventGroupClearBits(xSecurityEvents, EVT_NO_PERSON);
		}
		else
		{
			if (!WasNoPerson)
			{
				/*刚从"有人"变为"无人"，记录起始时刻*/
				WasNoPerson = 1;
				NoPersonStartTick = xTaskGetTickCount();
			}
			else
			{
				/*持续无人，检查是否已达到判定阈值*/
				if ((xTaskGetTickCount() - NoPersonStartTick) >= pdMS_TO_TICKS(NO_PERSON_HOLD_MS))
				{
					xEventGroupSetBits(xSecurityEvents, EVT_NO_PERSON);
				}
			}
		}

		/*更新共享数据供OLED显示*/
		if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(50)) == pdTRUE)
		{
			g_AppData.PersonDetected = Detected;
			xSemaphoreGive(xDataMutex);
		}

		vTaskDelay(pdMS_TO_TICKS(200));		//200ms轮询一次，人体感应本身响应无需过快
	}
}
