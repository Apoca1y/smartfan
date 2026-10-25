#include "Task_Safety.h"
#include "app_shared.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "Gas.h"
#include "Motor.h"

/**
  * 函    数：安防安全任务（最高优先级）
  * 参    数：pvParameters 任务参数（未使用）
  * 返 回 值：无
  * 说    明：本任务优先级设为系统最高，专门监测烟雾/可燃气体。
  *           一旦确认报警，立即执行Motor_SetLevel(0)让电机停转（防止鼓风助燃），
  *           不经过Task_FanControl的正常调度路径，把响应延迟降到最低。
  *           置位EVT_SMOKE后，Task_FanControl和Task_Display会分别读取
  *           该状态来锁定电机、切换OLED报警闪烁画面。
  *           烟雾报警解除采用手动确认策略（不自动恢复），因为可燃气体泄漏后
  *           空间中残留浓度消散需要时间，自动恢复风扇运转有再次点燃风险；
  *           需要用户按K1或K2确认现场安全后才允许恢复。
  */
void Task_Safety(void *pvParameters)
{
	uint8_t SmokeLatched = 0;		//烟雾报警锁存标志，报警后必须手动确认才能复位

	Gas_Init();

	for (;;)
	{
		uint16_t CurrentADC = Gas_GetADCValue();
		uint8_t SmokeNow = Gas_CheckSmoke();

		if (SmokeNow && !SmokeLatched)
		{
			/*首次检测到报警：立即停转电机，锁存报警状态*/
			Motor_SetLevel(FAN_OFF);
			SmokeLatched = 1;
			xEventGroupSetBits(xSecurityEvents, EVT_SMOKE);
		}

		if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(50)) == pdTRUE)
		{
			g_AppData.GasADC = CurrentADC;

			/*手动确认解除：按键任务确认现场安全后置位ClearRequest，
			  这里读取到该请求时才真正解除报警锁存*/
			if (SmokeLatched && g_AppData.SmokeAlarmClearRequest)
			{
				SmokeLatched = 0;
				g_AppData.SmokeAlarmClearRequest = 0;
				xEventGroupClearBits(xSecurityEvents, EVT_SMOKE);
			}

			g_AppData.SmokeAlarm = SmokeLatched;
			xSemaphoreGive(xDataMutex);
		}

		vTaskDelay(pdMS_TO_TICKS(100));		//100ms周期，保证报警响应足够快
	}
}
