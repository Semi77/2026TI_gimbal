#include "sys.h"
#include "usart.h"
#include "f32c_gimbal.h"

#define VISION_FRAME_LENGTH 11
#define VISION_CENTER_X 224
#define VISION_CENTER_Y 224

u8 Send_Data[12];
RECEIVE_DATA Receive_Data;
volatile u16 Vision_X = 0;
volatile u16 Vision_Y = 0;
volatile s16 Vision_Error_X = 0;
volatile s16 Vision_Error_Y = 0;
volatile u8 Vision_Data_Ready = 0;
u8 usart2_receive_data[10];

static u8 vision_receive_buffer[VISION_FRAME_LENGTH];
static u8 vision_receive_index = 0;

#if 1
#pragma import(__use_no_semihosting)
struct __FILE
{
	int handle;
};

FILE __stdout;

/* 关闭半主机模式退出入口以避免程序停在调试库中。 */
void _sys_exit(int x)
{
	x = x;
}

/* 将printf输出重定向到USART1。 */
int fputc(int ch, FILE *f)
{
	while((USART1->SR & 0X40) == 0);
	USART1->DR = (u8)ch;
	return ch;
}
#endif

/* 判断接收到的字符是否为十进制数字。 */
static u8 Vision_Is_Digit(u8 data)
{
	return (data >= '0' && data <= '9');
}

/* 将视觉协议中的四位ASCII数字转换为整数。 */
static u16 Vision_Parse_4Digits(const u8 *data)
{
	return (u16)((data[0] - '0') * 1000
				+ (data[1] - '0') * 100
				+ (data[2] - '0') * 10
				+ (data[3] - '0'));
}

/* 按“#xxxx|yyyy$”协议解析视觉模块发送的单个字节。 */
static void Vision_Parse_Byte(u8 data)
{
	u16 vision_x_temp;
	u16 vision_y_temp;
	u8 index;

	if(data == '#')
	{
		vision_receive_buffer[0] = data;
		vision_receive_index = 1;
		return;
	}

	if(vision_receive_index == 0)
	{
		return;
	}

	vision_receive_buffer[vision_receive_index++] = data;
	if(vision_receive_index < VISION_FRAME_LENGTH)
	{
		return;
	}
	vision_receive_index = 0;

	if(vision_receive_buffer[0] != '#'
		|| vision_receive_buffer[5] != '|'
		|| vision_receive_buffer[10] != '$')
	{
		return;
	}

	for(index = 1; index <= 4; index++)
	{
		if(!Vision_Is_Digit(vision_receive_buffer[index]))
		{
			return;
		}
	}
	for(index = 6; index <= 9; index++)
	{
		if(!Vision_Is_Digit(vision_receive_buffer[index]))
		{
			return;
		}
	}

	vision_x_temp = Vision_Parse_4Digits(&vision_receive_buffer[1]);
	vision_y_temp = Vision_Parse_4Digits(&vision_receive_buffer[6]);

	if(vision_x_temp > 448 || vision_y_temp > 448)
	{
		return;
	}

	Vision_X = vision_x_temp;
	Vision_Y = vision_y_temp;
	Vision_Error_X = (s16)Vision_X - VISION_CENTER_X;
	Vision_Error_Y = (s16)Vision_Y - VISION_CENTER_Y;
	Vision_Data_Ready = 1;
}

/* 初始化USART1作为视觉数据接收串口。 */
void uart_init(u32 bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART1, ENABLE);
}

/* 初始化用于接收视觉中心点数据的USART1串口实例。 */
void Vision_UART_Init(u32 bound)
{
	uart_init(bound);
}

/* 初始化USART2作为F32C电机通信串口。 */
void uart2_init(u32 bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

	DMA_DeInit(DMA1_Channel6);
	DMA_InitStructure.DMA_BufferSize = 10;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)usart2_receive_data;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART2->DR);
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_Init(DMA1_Channel6, &DMA_InitStructure);
	DMA_ClearFlag(DMA1_FLAG_TC6);
	DMA_Cmd(DMA1_Channel6, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);
	USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);
	USART_Cmd(USART2, ENABLE);
}

/* 通过USART1发送一个字节。 */
void usart1_send(u8 data)
{
	USART1->DR = data;
	while((USART1->SR & 0x40) == 0);
}

/* 通过USART1发送指定长度的数据。 */
void USART1_SEND(u8 *data, u8 len)
{
	u8 index;

	for(index = 0; index < len; index++)
	{
		usart1_send(data[index]);
	}
}

/* 通过USART2发送一个字节。 */
void usart2_send(u8 data)
{
	USART2->DR = data;
	while((USART2->SR & 0x40) == 0);
}

/* 通过USART2发送指定长度的数据。 */
void USART2_SEND(u8 *data, u8 len)
{
	u8 index;

	for(index = 0; index < len; index++)
	{
		usart2_send(data[index]);
	}
}

/* 计算指定长度数据的异或校验值。 */
u8 BCC_Sum1(u8 *usart_data, unsigned char Count_Number)
{
	unsigned char crc_sum = 0;
	unsigned char index;

	for(index = 0; index < Count_Number; index++)
	{
		crc_sum ^= usart_data[index];
	}
	return crc_sum;
}

/* USART1中断用于逐字节接收并解析视觉中心点数据。 */
void USART1_IRQHandler(void)
{
	u8 receive_data;

	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		receive_data = USART_ReceiveData(USART1);
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
		Vision_Parse_Byte(receive_data);
	}
}

/* USART2中断用于接收并分发F32C电机位置反馈数据。 */
void USART2_IRQHandler(void)
{
	if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)
	{
		(void)USART_ReceiveData(USART2);
		DMA_Cmd(DMA1_Channel6, DISABLE);
		F32C_Gimbal_Parse_Position_Feedback(usart2_receive_data, 9);
		DMA_SetCurrDataCounter(DMA1_Channel6, 10);
		DMA_Cmd(DMA1_Channel6, ENABLE);
	}
}
