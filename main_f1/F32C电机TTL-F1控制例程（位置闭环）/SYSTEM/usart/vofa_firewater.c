#include "vofa_firewater.h"

#define VOFA_USART USART3
#define VOFA_USART_RCC RCC_APB1Periph_USART3
#define VOFA_GPIO_RCC RCC_APB2Periph_GPIOB
#define VOFA_GPIO_PORT GPIOB
#define VOFA_TX_PIN GPIO_Pin_10
#define VOFA_RX_PIN GPIO_Pin_11

/* 初始化USART3作为VOFA++ FireWater数据发送串口。 */
void VOFA_FireWater_Init(u32 bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	RCC_APB2PeriphClockCmd(VOFA_GPIO_RCC, ENABLE);
	RCC_APB1PeriphClockCmd(VOFA_USART_RCC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = VOFA_TX_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(VOFA_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = VOFA_RX_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(VOFA_GPIO_PORT, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(VOFA_USART, &USART_InitStructure);
	USART_Cmd(VOFA_USART, ENABLE);
}

/* 通过USART3向VOFA++发送一个字符。 */
void VOFA_FireWater_Send_Char(char data)
{
	USART_SendData(VOFA_USART, (u8)data);
	while(USART_GetFlagStatus(VOFA_USART, USART_FLAG_TXE) == RESET)
	{
	}
}

/* 通过USART3向VOFA++发送一个字符串。 */
void VOFA_FireWater_Send_String(const char *str)
{
	if(str == 0)
	{
		return;
	}
	while(*str != '\0')
	{
		VOFA_FireWater_Send_Char(*str++);
	}
}

/* 通过USART3向VOFA++发送一个无符号整数。 */
static void VOFA_FireWater_Send_U32(u32 value)
{
	char buffer[10];
	u8 index = 0;

	if(value == 0)
	{
		VOFA_FireWater_Send_Char('0');
		return;
	}

	while(value > 0 && index < sizeof(buffer))
	{
		buffer[index++] = (char)('0' + value % 10);
		value /= 10;
	}
	while(index > 0)
	{
		VOFA_FireWater_Send_Char(buffer[--index]);
	}
}

/* 通过USART3向VOFA++发送一个有符号整数。 */
void VOFA_FireWater_Send_S32(s32 value)
{
	if(value < 0)
	{
		VOFA_FireWater_Send_Char('-');
		VOFA_FireWater_Send_U32((u32)(-value));
	}
	else
	{
		VOFA_FireWater_Send_U32((u32)value);
	}
}

/* 发送FireWater数据帧的可选前缀和冒号。 */
static void VOFA_FireWater_Send_Prefix(const char *prefix)
{
	if(prefix == 0 || *prefix == '\0')
	{
		return;
	}
	VOFA_FireWater_Send_String(prefix);
	VOFA_FireWater_Send_Char(':');
}

/* 按FireWater格式向VOFA++发送两个通道数据。 */
void VOFA_FireWater_Send_2S32(const char *prefix, s32 ch0, s32 ch1)
{
	VOFA_FireWater_Send_Prefix(prefix);
	VOFA_FireWater_Send_S32(ch0);
	VOFA_FireWater_Send_Char(',');
	VOFA_FireWater_Send_S32(ch1);
	VOFA_FireWater_Send_Char('\n');
}

/* 按FireWater格式向VOFA++发送四个通道数据。 */
void VOFA_FireWater_Send_4S32(const char *prefix, s32 ch0, s32 ch1, s32 ch2, s32 ch3)
{
	VOFA_FireWater_Send_Prefix(prefix);
	VOFA_FireWater_Send_S32(ch0);
	VOFA_FireWater_Send_Char(',');
	VOFA_FireWater_Send_S32(ch1);
	VOFA_FireWater_Send_Char(',');
	VOFA_FireWater_Send_S32(ch2);
	VOFA_FireWater_Send_Char(',');
	VOFA_FireWater_Send_S32(ch3);
	VOFA_FireWater_Send_Char('\n');
}

/* 按FireWater格式向VOFA++发送六个通道数据。 */
void VOFA_FireWater_Send_6S32(const char *prefix, s32 ch0, s32 ch1, s32 ch2, s32 ch3, s32 ch4, s32 ch5)
{
	VOFA_FireWater_Send_Prefix(prefix);
	VOFA_FireWater_Send_S32(ch0);
	VOFA_FireWater_Send_Char(',');
	VOFA_FireWater_Send_S32(ch1);
	VOFA_FireWater_Send_Char(',');
	VOFA_FireWater_Send_S32(ch2);
	VOFA_FireWater_Send_Char(',');
	VOFA_FireWater_Send_S32(ch3);
	VOFA_FireWater_Send_Char(',');
	VOFA_FireWater_Send_S32(ch4);
	VOFA_FireWater_Send_Char(',');
	VOFA_FireWater_Send_S32(ch5);
	VOFA_FireWater_Send_Char('\n');
}

/* 按FireWater格式向VOFA++发送PID调试数据。 */
void VOFA_FireWater_Send_PID(s32 error_x, s32 error_y, s32 target_x, s32 target_y, s32 delta_x, s32 delta_y)
{
	VOFA_FireWater_Send_6S32("pid", error_x, error_y, target_x, target_y, delta_x, delta_y);
}
