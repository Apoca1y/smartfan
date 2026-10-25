#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "FreeRTOS.h"
#include "task.h"

/*AHT20温湿度传感器，7位I2C地址0x38*/
#define AHT20_ADDRESS		0x38

/*AHT20命令*/
#define AHT20_CMD_INIT		0xBE		//初始化（校准）命令
#define AHT20_CMD_MEASURE	0xAC		//触发测量命令
#define AHT20_CMD_SOFTRESET	0xBA		//软复位命令

#define AHT20_STATUS_BUSY	0x80		//状态字节：忙标志位
#define AHT20_STATUS_CAL	0x08		//状态字节：校准使能标志位

/*I2C超时计数上限，防止总线异常时程序卡死*/
#define I2C_TIMEOUT			10000

static uint8_t AHT20_WaitEvent(uint32_t I2C_EVENT);

/**
  * 函    数：等待I2C事件，带超时保护
  * 参    数：I2C_EVENT 期望的事件
  * 返 回 值：0=成功，1=超时（总线异常）
  */
static uint8_t AHT20_WaitEvent(uint32_t I2C_EVENT)
{
	uint32_t Timeout = I2C_TIMEOUT;
	while (I2C_CheckEvent(I2C1, I2C_EVENT) == ERROR)
	{
		if ((Timeout--) == 0)
		{
			return 1;		//超时，总线异常
		}
	}
	return 0;
}

/**
  * 函    数：AHT20初始化
  * 参    数：无
  * 返 回 值：无
  * 说    明：使用硬件I2C1，SCL接PB6，SDA接PB7
  */
void AHT20_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);		//开启I2C1的时钟

	/*GPIO初始化：PB6(SCL)、PB7(SDA)，复用开漏输出*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/*I2C初始化*/
	I2C_InitTypeDef I2C_InitStructure;
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_OwnAddress1 = 0x00;					//本机地址（作为从机时使用，此处无意义）
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed = 100000;					//标准模式，100kHz
	I2C_Init(I2C1, &I2C_InitStructure);

	/*I2C使能*/
	I2C_Cmd(I2C1, ENABLE);

	vTaskDelay(pdMS_TO_TICKS(40));				//上电后等待40ms，AHT20需要时间完成内部初始化

	/*发送初始化（校准）命令*/
	I2C_GenerateSTART(I2C1, ENABLE);
	if (AHT20_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT)) return;

	I2C_Send7bitAddress(I2C1, AHT20_ADDRESS << 1, I2C_Direction_Transmitter);
	if (AHT20_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) return;

	I2C_SendData(I2C1, AHT20_CMD_INIT);
	if (AHT20_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) return;

	I2C_SendData(I2C1, 0x08);
	if (AHT20_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) return;

	I2C_SendData(I2C1, 0x00);
	if (AHT20_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) return;

	I2C_GenerateSTOP(I2C1, ENABLE);

	vTaskDelay(pdMS_TO_TICKS(10));				//等待校准命令执行完成
}

/**
  * 函    数：读取AHT20温湿度数据
  * 参    数：Temperature 温度输出指针，单位：摄氏度
  *           Humidity 湿度输出指针，单位：%RH
  * 返 回 值：0=读取成功，1=读取失败（总线异常或超时）
  * 说    明：该函数会阻塞约80ms等待AHT20完成测量，请勿在高频任务中频繁调用
  */
uint8_t AHT20_ReadData(float *Temperature, float *Humidity)
{
	uint8_t RxData[6];
	uint8_t i;
	uint32_t RawHumidity, RawTemperature;

	/*触发测量*/
	I2C_GenerateSTART(I2C1, ENABLE);
	if (AHT20_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT)) return 1;

	I2C_Send7bitAddress(I2C1, AHT20_ADDRESS << 1, I2C_Direction_Transmitter);
	if (AHT20_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) return 1;

	I2C_SendData(I2C1, AHT20_CMD_MEASURE);
	if (AHT20_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) return 1;

	I2C_SendData(I2C1, 0x33);
	if (AHT20_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) return 1;

	I2C_SendData(I2C1, 0x00);
	if (AHT20_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) return 1;

	I2C_GenerateSTOP(I2C1, ENABLE);

	vTaskDelay(pdMS_TO_TICKS(80));				//等待AHT20完成一次测量转换

	/*读取6字节数据：状态字节 + 2.5字节湿度 + 2.5字节温度*/
	I2C_GenerateSTART(I2C1, ENABLE);
	if (AHT20_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT)) return 1;

	I2C_Send7bitAddress(I2C1, AHT20_ADDRESS << 1, I2C_Direction_Receiver);
	if (AHT20_WaitEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) return 1;

	for (i = 0; i < 6; i++)
	{
		if (i == 5)
		{
			I2C_AcknowledgeConfig(I2C1, DISABLE);		//最后一个字节不应答
			I2C_GenerateSTOP(I2C1, ENABLE);
		}
		if (AHT20_WaitEvent(I2C_EVENT_MASTER_BYTE_RECEIVED)) return 1;
		RxData[i] = I2C_ReceiveData(I2C1);
	}

	I2C_AcknowledgeConfig(I2C1, ENABLE);			//恢复应答使能，供下次通信使用

	/*校验状态字节，忙标志或未校准都视为数据无效*/
	if ((RxData[0] & AHT20_STATUS_BUSY) || !(RxData[0] & AHT20_STATUS_CAL))
	{
		return 1;
	}

	/*解析湿度：20位原始数据*/
	RawHumidity = ((uint32_t)RxData[1] << 12) | ((uint32_t)RxData[2] << 4) | (RxData[3] >> 4);
	*Humidity = (float)RawHumidity * 100.0f / 1048576.0f;		//1048576 = 2^20

	/*解析温度：20位原始数据*/
	RawTemperature = (((uint32_t)RxData[3] & 0x0F) << 16) | ((uint32_t)RxData[4] << 8) | RxData[5];
	*Temperature = (float)RawTemperature * 200.0f / 1048576.0f - 50.0f;

	return 0;
}
