#include "Task_FanControl.h"
#include "app_shared.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"
#include "Motor.h"
#include "Key.h"

/**
  * 函    数：根据温度自动调节风扇档位（有人在场时使用，自动模式）
  * 参    数：Temp 当前温度
  * 返 回 值：建议的风力档位
  */
static FanLevel_t AutoAdjustLevel(float Temp)
{
	if (Temp < TEMP_LEVEL1_THRESHOLD)
	{
		return FAN_OFF;
	}
	else if (Temp < TEMP_LEVEL2_THRESHOLD)
	{
		return FAN_LOW;
	}
	else if (Temp < TEMP_LEVEL3_THRESHOLD)
	{
		return FAN_MID;
	}
	else
	{
		return FAN_HIGH;
	}
}

/**
  * 函    数：风扇控制核心任务
  * 参    数：pvParameters 任务参数（未使用）
  * 返 回 值：无
  * 说    明：本任务是FreeRTOS事件标志组学习的核心演示点：
  *           使用xEventGroupWaitBits等待"温度超限(EVT_TEMP_HIGH)"和
  *           "无人(EVT_NO_PERSON)"两个条件同时满足（xWaitForAllBits=pdTRUE），
  *           一旦两者都成立就立即被唤醒进入"全速排风"分支；即使条件不满足，
  *           也会在超时后被唤醒以处理按键、维持基础换气等常规工作，
  *           因此本次调用同时承担了原main.c中"主循环节拍"的角色。
  *
  *           决策优先级（从高到低）：
  *           1. 烟雾/可燃气体报警（EVT_SMOKE）：无条件停转，此优先级最高
  *           2. 手动模式：保持用户通过K2设定的档位
  *           3. 自动模式 + 有人在场：按原有温度分档（OFF/LOW/MID/HIGH）
  *           4. 自动模式 + 无人 + 温度超限：全速排风降温
  *           5. 自动模式 + 无人 + 温度未超限：低速换气（基础工况）
  */
void Task_FanControl(void *pvParameters)
{
	uint8_t KeyNum;

	for (;;)
	{
		/*等待"温度超限"和"无人"两个事件位，最长阻塞500ms。
		  xClearOnExit=pdFALSE：不清除标志位，因为Task_Sensor/Task_Presence
		  才是这两个位的维护者，本任务只读取不清除。
		  xWaitForAllBits=pdTRUE：要求两个条件同时满足才提前返回。
		  超时到达（500ms）后即使条件不满足，也会返回一次，用于处理按键、
		  维持基础换气等常规逻辑，相当于原main.c的主循环节拍。*/
		xEventGroupWaitBits(xSecurityEvents,
							EVT_TEMP_HIGH | EVT_NO_PERSON,
							pdFALSE,
							pdTRUE,
							pdMS_TO_TICKS(500));

		/*按键扫描：K1切换模式（或在报警时用作"确认解除"），K2手动模式下调节档位*/
		KeyNum = Key_GetNum();

		if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
		{
			uint8_t Smoke = g_AppData.SmokeAlarm;

			if (Smoke)
			{
				/*报警状态下，K1用作"确认现场安全，解除报警"操作*/
				if (KeyNum == 1)
				{
					g_AppData.SmokeAlarmClearRequest = 1;
				}
				/*报警期间电机保持停转，由Task_Safety已直接执行，这里不再重复调用，
				  避免在报警瞬间和解除瞬间出现竞争写*/
			}
			else
			{
				/*正常按键逻辑*/
				if (KeyNum == 1)
				{
					g_AppData.Mode = (g_AppData.Mode == MODE_AUTO) ? MODE_MANUAL : MODE_AUTO;
				}
				else if (KeyNum == 2 && g_AppData.Mode == MODE_MANUAL)
				{
					g_AppData.FanLevel = (FanLevel_t)((g_AppData.FanLevel + 1) % 4);	//OFF→LOW→MID→HIGH→OFF循环
				}

				/*根据模式和事件位决策目标档位*/
				FanLevel_t TargetLevel;
				uint8_t ForceFullSpeed = 0;

				if (g_AppData.Mode == MODE_MANUAL)
				{
					TargetLevel = g_AppData.FanLevel;		//手动模式：保持用户设定档位
				}
				else if (g_AppData.PersonDetected)
				{
					TargetLevel = AutoAdjustLevel(g_AppData.Temperature);	//有人：按原有温度分档
				}
				else if (g_AppData.Temperature >= TEMP_HIGH_THRESHOLD)
				{
					TargetLevel = FAN_HIGH;			//无人且温度超限：全速排风
					ForceFullSpeed = 1;
				}
				else
				{
					TargetLevel = FAN_LOW;				//无人且温度正常：低速换气（基础工况）
				}

				g_AppData.FanLevel = TargetLevel;
				g_AppData.FanStatus = (TargetLevel != FAN_OFF) ? 1 : 0;
				g_AppData.SecurityForceFullSpeed = ForceFullSpeed;

				Motor_SetLevel((uint8_t)TargetLevel);
			}

			xSemaphoreGive(xDataMutex);
		}
	}
}
