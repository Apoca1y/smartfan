#ifndef __APP_SHARED_H
#define __APP_SHARED_H

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"

/*事件标志组的各个bit位定义*/
#define EVT_TEMP_HIGH		(1 << 0)		//温度超限
#define EVT_NO_PERSON		(1 << 1)		//无人（PIR连续无人达到阈值）
#define EVT_SMOKE			(1 << 2)		//检测到烟雾/可燃气体报警

/*事件标志组，供各任务同步"温度超限"和"无人"两个条件*/
extern EventGroupHandle_t xSecurityEvents;

/*保护下面共享数据的互斥量*/
extern SemaphoreHandle_t xDataMutex;

/*系统运行模式*/
typedef enum
{
	MODE_AUTO = 0,		//自动模式
	MODE_MANUAL = 1		//手动模式
} SysMode_t;

/*风力档位*/
typedef enum
{
	FAN_OFF = 0,
	FAN_LOW = 1,
	FAN_MID = 2,
	FAN_HIGH = 3
} FanLevel_t;

/*全局共享数据，读写前须持有xDataMutex*/
typedef struct
{
	float Temperature;			//当前温度，单位：摄氏度
	float Humidity;				//当前湿度，单位：%RH
	uint16_t GasADC;			//气体传感器ADC原始值（调试显示用）

	SysMode_t Mode;				//运行模式：自动/手动
	FanLevel_t FanLevel;		//当前风力档位
	uint8_t FanStatus;			//风扇开关状态：0=关，1=开

	uint8_t PersonDetected;		//人体检测状态：0=无人，1=有人
	uint8_t SmokeAlarm;			//烟雾报警状态：0=正常，1=报警
	uint8_t SmokeAlarmClearRequest;	//烟雾报警解除请求：由按键任务置1，Task_Safety读取后清除报警并清零本标志

	uint8_t SecurityForceFullSpeed;	//安防联动标志：1=温度超限且无人，要求全速排风
										//由Task_FanControl维护并读取
} AppData_t;

extern AppData_t g_AppData;

/*温度阈值（自动模式下用于判断是否"温度超限"，触发全速排风）*/
#define TEMP_HIGH_THRESHOLD		30.0f
/*温度-档位映射阈值（自动模式，有人在场时使用）*/
#define TEMP_LEVEL1_THRESHOLD	26.0f
#define TEMP_LEVEL2_THRESHOLD	28.0f
#define TEMP_LEVEL3_THRESHOLD	30.0f

/*无人判定的连续时间阈值，单位：ms*/
#define NO_PERSON_HOLD_MS		3000

void AppShared_Init(void);

#endif
