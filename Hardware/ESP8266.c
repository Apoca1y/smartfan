#include "stm32f10x.h"
#include "ESP8266.h"
#include "Delay.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

/*说明：本文件中原本用Delay_ms实现的等待，均改为vTaskDelay。
  AT指令交互耗时可达数秒（尤其WiFi连接），使用vTaskDelay可以在等待期间让出CPU给
  其他任务运行，而不会像忙等一样独占处理器，更符合FreeRTOS多任务的使用方式。*/

char ESP8266_RxBuffer[256];				// 接收缓冲区
volatile uint16_t ESP8266_RxIndex = 0;	// 接收索引

/**
  * 函数：ESP8266 初始化（使用 USART1, PA9=TX, PA10=RX）
  * 参数：无
  * 返回值：无
  */
void ESP8266_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/*GPIO 配置：PA9(TX1), PA10(RX1)*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;			// 复用推挽输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;			// 上拉输入
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*USART 配置*/
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART1, &USART_InitStructure);

	/*使能接收中断*/
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	/*NVIC 配置*/
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 12;	//须>=11(configMAX_SYSCALL_INTERRUPT_PRIORITY对应值)，FreeRTOS才能安全管理
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			//Group 4下无子优先级，填0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_Cmd(USART1, ENABLE);
}

/**
  * 函数：发送字符串
  * 参数：str 要发送的字符串
  * 返回值：无
  */
void ESP8266_SendString(char *str)
{
	while (*str)
	{
		while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
		USART_SendData(USART1, *str++);
	}
}

/**
  * 函数：清空接收缓冲区
  * 参数：无
  * 返回值：无
  */
void ESP8266_ClearBuffer(void)
{
	ESP8266_RxIndex = 0;
	memset(ESP8266_RxBuffer, 0, sizeof(ESP8266_RxBuffer));
}

/**
  * 函数：发送 AT 指令并等待响应
  * 参数：cmd 指令字符串，reply 期望的响应，timeout 超时时间(10ms单位)
  * 返回值：无
  */
void ESP8266_SendCommand(char *cmd, char *reply, uint16_t timeout)
{
	ESP8266_ClearBuffer();

	ESP8266_SendString(cmd);
	ESP8266_SendString("\r\n");

	// 等待响应
	while (timeout--)
	{
		vTaskDelay(pdMS_TO_TICKS(10));
		if (strstr(ESP8266_RxBuffer, reply) != NULL)
		{
			return;
		}
	}
}

/**
  * 函数：连接 WiFi
  * 参数：ssid WiFi名称，password WiFi密码
  * 返回值：1=成功，0=失败
  */
uint8_t ESP8266_ConnectWiFi(char *ssid, char *password)
{
	char cmd[128];

	// 测试 AT
	ESP8266_SendCommand("AT", "OK", 100);
	vTaskDelay(pdMS_TO_TICKS(500));

	// 设置为 Station 模式
	ESP8266_SendCommand("AT+CWMODE=1", "OK", 100);
	vTaskDelay(pdMS_TO_TICKS(500));

	// 连接 WiFi
	sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", ssid, password);
	ESP8266_SendCommand(cmd, "OK", 2000);  // WiFi 连接较慢，等待 20 秒
	vTaskDelay(pdMS_TO_TICKS(2000));

	// 查询 IP
	ESP8266_SendCommand("AT+CIFSR", "OK", 100);

	return 1;
}

/**
  * 函数：连接巴法云 MQTT 服务器
  * 参数：uid 巴法云私钥，topic 主题名称
  * 返回值：无
  */
void ESP8266_ConnectBemfa(char *uid, char *topic)
{
	char cmd[128];
	char payload[128];

	// 设置单路连接模式
	ESP8266_SendCommand("AT+CIPMODE=0", "OK", 100);
	vTaskDelay(pdMS_TO_TICKS(500));

	// 连接巴法云 TCP 服务器
	ESP8266_SendCommand("AT+CIPSTART=\"TCP\",\"bemfa.com\",8344", "OK", 500);
	vTaskDelay(pdMS_TO_TICKS(3000));

	// 订阅主题（身份验证）
	sprintf(payload, "cmd=1&uid=%s&topic=%s\r\n", uid, topic);

	sprintf(cmd, "AT+CIPSEND=%d", strlen(payload));
	ESP8266_SendCommand(cmd, ">", 100);
	vTaskDelay(pdMS_TO_TICKS(100));
	ESP8266_SendString(payload);
	vTaskDelay(pdMS_TO_TICKS(1000));
}

/**
  * 函数：发送数据到云端
  * 参数：topic 主题名称，data 数据内容
  * 返回值：无
  */
void ESP8266_SendToCloud(char *topic, char *data)
{
	char cmd[64];
	char payload[128];

	sprintf(payload, "cmd=2&topic=%s&msg=%s\r\n", topic, data);

	sprintf(cmd, "AT+CIPSEND=%d", strlen(payload));
	ESP8266_SendCommand(cmd, ">", 100);
	vTaskDelay(pdMS_TO_TICKS(100));
	ESP8266_SendString(payload);
	vTaskDelay(pdMS_TO_TICKS(500));
}

/**
  * 函数：USART1 中断服务函数
  * 参数：无
  * 返回值：无
  */
void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
	{
		uint8_t data = USART_ReceiveData(USART1);

		if (ESP8266_RxIndex < sizeof(ESP8266_RxBuffer) - 1)
		{
			ESP8266_RxBuffer[ESP8266_RxIndex++] = data;
			ESP8266_RxBuffer[ESP8266_RxIndex] = '\0';  // 确保字符串结尾
		}

		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}
