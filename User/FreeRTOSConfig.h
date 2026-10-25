/*
 * FreeRTOSConfig.h
 * 智能安防联动风扇系统 - FreeRTOS配置
 * 基于官方 CORTEX_STM32F103_Keil 示例配置修改
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * 应用相关配置
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION			1
#define configUSE_IDLE_HOOK			0
#define configUSE_TICK_HOOK			0
#define configCPU_CLOCK_HZ				( ( unsigned long ) 72000000 )
#define configTICK_RATE_HZ				( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES			( 6 )
#define configMINIMAL_STACK_SIZE		( ( unsigned short ) 128 )
#define configTOTAL_HEAP_SIZE			( ( size_t ) ( 12 * 1024 ) )	//STM32F103C8共20KB RAM，需给系统栈/全局变量留余量
#define configMAX_TASK_NAME_LEN		( 16 )
#define configUSE_TRACE_FACILITY		0
#define configUSE_16_BIT_TICKS			0
#define configIDLE_SHOULD_YIELD			1
#define configUSE_MUTEXES				1
#define configUSE_RECURSIVE_MUTEXES	0
#define configUSE_COUNTING_SEMAPHORES	0
#define configQUEUE_REGISTRY_SIZE		0
#define configCHECK_FOR_STACK_OVERFLOW	2			//开启栈溢出检测（调试阶段建议开启）
#define configUSE_MALLOC_FAILED_HOOK	1

/*事件标志组，本项目核心学习点：温度超限 AND 无人 同时满足才全速排风*/
#define configUSE_EVENT_GROUPS			1

/*软件定时器（暂未使用，保留关闭以节省资源）*/
#define configUSE_TIMERS				0
#define configTIMER_TASK_PRIORITY		( 3 )
#define configTIMER_QUEUE_LENGTH		5
#define configTIMER_TASK_STACK_DEPTH	( configMINIMAL_STACK_SIZE * 2 )

/* 协程相关，本项目不使用 */
#define configUSE_CO_ROUTINES 			0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

/* API函数包含开关 */
#define INCLUDE_vTaskPrioritySet		1
#define INCLUDE_uxTaskPriorityGet		1
#define INCLUDE_vTaskDelete				1
#define INCLUDE_vTaskCleanUpResources	0
#define INCLUDE_vTaskSuspend			1
#define INCLUDE_vTaskDelayUntil			1
#define INCLUDE_vTaskDelay				1
#define INCLUDE_xTaskGetCurrentTaskHandle	1

/* Cortex-M3 NVIC优先级配置，数值越大实际优先级越低
   保持与官方Keil示例一致，避免中断优先级配置错误导致临界区失效 */
#define configKERNEL_INTERRUPT_PRIORITY 		255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191	/* 等效0xB0，即优先级11 */
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY	15

#endif /* FREERTOS_CONFIG_H */
