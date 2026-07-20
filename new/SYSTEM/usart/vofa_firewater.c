#include "vofa_firewater.h"

#define VOFA_USART USART2
#define VOFA_USART_RCC RCC_APB1Periph_USART2
#define VOFA_GPIO_RCC RCC_APB2Periph_GPIOA
#define VOFA_GPIO_PORT GPIOA
#define VOFA_TX_PIN GPIO_Pin_2
#define VOFA_RX_PIN GPIO_Pin_3

/* 函数功能：初始化USART2作为VOFA上位机FireWater数据发送串口。 */
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

/* 函数功能：通过USART2向VOFA上位机发送一个字符。 */
void VOFA_FireWater_Send_Char(char data)
{
	USART_SendData(VOFA_USART, (u8)data);
	while(USART_GetFlagStatus(VOFA_USART, USART_FLAG_TXE) == RESET)
	{
	}
}

/* 函数功能：通过USART2向VOFA上位机发送一个字符串。 */
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

/* 函数功能：通过USART2向VOFA上位机发送一个无符号整数。 */
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

/* 函数功能：通过USART2向VOFA上位机发送一个有符号整数。 */
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

/* 函数功能：发送VOFA FireWater数据帧的可选前缀和冒号。 */
static void VOFA_FireWater_Send_Prefix(const char *prefix)
{
	if(prefix == 0 || *prefix == '\0')
	{
		return;
	}
	VOFA_FireWater_Send_String(prefix);
	VOFA_FireWater_Send_Char(':');
}

/* 函数功能：按VOFA FireWater格式发送两个有符号整数通道。 */
void VOFA_FireWater_Send_2S32(const char *prefix, s32 ch0, s32 ch1)
{
	VOFA_FireWater_Send_Prefix(prefix);
	VOFA_FireWater_Send_S32(ch0);
	VOFA_FireWater_Send_Char(',');
	VOFA_FireWater_Send_S32(ch1);
	VOFA_FireWater_Send_Char('\n');
}

/* 函数功能：按VOFA FireWater格式发送四个有符号整数通道。 */
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

/* 函数功能：按VOFA FireWater格式发送六个有符号整数通道。 */
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

/* 函数功能：按VOFA FireWater格式发送八个有符号整数通道。 */
void VOFA_FireWater_Send_8S32(const char *prefix, s32 ch0, s32 ch1, s32 ch2, s32 ch3, s32 ch4, s32 ch5, s32 ch6, s32 ch7)
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
	VOFA_FireWater_Send_Char(',');
	VOFA_FireWater_Send_S32(ch6);
	VOFA_FireWater_Send_Char(',');
	VOFA_FireWater_Send_S32(ch7);
	VOFA_FireWater_Send_Char('\n');
}

/* 函数功能：按VOFA FireWater格式发送基础PID调试数据。 */
void VOFA_FireWater_Send_PID(s32 error_x, s32 error_y, s32 target_x, s32 target_y, s32 delta_x, s32 delta_y)
{
	VOFA_FireWater_Send_6S32("pid", error_x, error_y, target_x, target_y, delta_x, delta_y);
}

/* 函数功能：按VOFA FireWater格式发送视觉PID完整调试数据。 */
void VOFA_FireWater_Send_PID_Debug(s32 vision_x, s32 vision_y, s32 error_x, s32 error_y, s32 delta_x, s32 delta_y, s32 target_x, s32 target_y)
{
	VOFA_FireWater_Send_8S32("vpid", vision_x, vision_y, error_x, error_y, delta_x, delta_y, target_x, target_y);
}
