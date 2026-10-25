#include "Task_Sensor.h"
#include "app_shared.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "AHT20.h"

/**
  * 函    数：温湿度采集任务
  * 参    数：pvParameters 任务参数（未使用）
  * 返 回 值：无
  * 说    明：定期读取AHT20温湿度，更新共享数据，并维护事件标志组的EVT_TEMP_HIGH位。
  *           AHT20单次读取内部有约80ms的阻塞等待（等待传感器完成AD转换），
  *           因此该任务的调用周期不宜太短，这里选择2秒一次。
  */
void Task_Sensor(void *pvParameters)
{
	float Temperature, Humidity;
	uint8_t ReadOk;

	AHT20_Init();

	for (;;)
	{
		ReadOk = (AHT20_ReadData(&Temperature, &Humidity) == 0);

		if (ReadOk)
		{
			if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(50)) == pdTRUE)
			{
				g_AppData.Temperature = Temperature;
				g_AppData.Humidity = Humidity;
				xSemaphoreGive(xDataMutex);
			}

			if (Temperature >= TEMP_HIGH_THRESHOLD)
			{
				xEventGroupSetBits(xSecurityEvents, EVT_TEMP_HIGH);
			}
			else
			{
				xEventGroupClearBits(xSecurityEvents, EVT_TEMP_HIGH);
			}
		}
		/*读取失败（总线异常）时保留上一次的温度值和事件标志位不变，
		  避免瞬时通信错误导致误判"温度骤降"或状态抖动*/

		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}
