#include "bldc_motor.h"
#include "usart.h"
#include <string.h>

volatile BLDC_MotorData_t BLDC_MotorX = {0};
volatile BLDC_MotorData_t BLDC_MotorY = {0};

static u8 bldc_rx_buf[10];
static u8 bldc_rx_index = 0;
static u8 bldc_rx_state = 0;

/* 函数功能：计算F32C通信帧从帧头到数据区的异或校验值。 */
u8 BLDC_Calc_BCC(const u8 *data, u8 len)
{
	u8 bcc = 0;
	u8 index;

	for(index = 0; index < len; index++)
	{
		bcc ^= data[index];
	}
	return bcc;
}

/* 函数功能：按照F32C串口协议组帧并通过USART3发送一条命令。 */
void BLDC_SendCmd(u8 addr, u8 cmd, const u8 *data, u8 len)
{
	u8 tx_buf[20];
	u8 index = 0;

	tx_buf[index++] = BLDC_FRAME_HEADER;
	tx_buf[index++] = addr;
	tx_buf[index++] = cmd;

	if(data != 0 && len > 0)
	{
		memcpy(&tx_buf[index], data, len);
		index += len;
	}

	tx_buf[index] = BLDC_Calc_BCC(tx_buf, index);
	index++;
	tx_buf[index++] = BLDC_FRAME_TAIL;
	USART3_SEND(tx_buf, index);
}

/* 函数功能：使能指定地址的F32C电机。 */
void BLDC_Enable(u8 addr)
{
	BLDC_SendCmd(addr, BLDC_CMD_ENABLE, 0, 0);
}

/* 函数功能：失能指定地址的F32C电机。 */
void BLDC_Disable(u8 addr)
{
	BLDC_SendCmd(addr, BLDC_CMD_DISABLE, 0, 0);
}

/* 函数功能：设置指定地址F32C电机的速度目标值。 */
void BLDC_SetSpeed(u8 addr, s16 rpm)
{
	u8 data[2];

	data[0] = (u8)((rpm >> 8) & 0xFF);
	data[1] = (u8)(rpm & 0xFF);
	BLDC_SendCmd(addr, BLDC_CMD_SPEED, data, 2);
}

/* 函数功能：设置指定地址F32C电机的运行模式。 */
void BLDC_SetMode(u8 addr, u16 mode)
{
	u8 data[2];

	data[0] = (u8)((mode >> 8) & 0xFF);
	data[1] = (u8)(mode & 0xFF);
	BLDC_SendCmd(addr, BLDC_CMD_MODE, data, 2);
}

/* 函数功能：设置指定地址F32C电机的位置或速度加速度。 */
void BLDC_SetAcc(u8 addr, u16 acc)
{
	u8 data[2];

	data[0] = (u8)((acc >> 8) & 0xFF);
	data[1] = (u8)(acc & 0xFF);
	BLDC_SendCmd(addr, BLDC_CMD_ACC, data, 2);
}

/* 函数功能：设置指定地址F32C电机的多圈位置目标值。 */
void BLDC_SetMultiAngle(u8 addr, s32 angle_x10)
{
	u8 data[4];

	data[0] = (u8)((angle_x10 >> 24) & 0xFF);
	data[1] = (u8)((angle_x10 >> 16) & 0xFF);
	data[2] = (u8)((angle_x10 >> 8) & 0xFF);
	data[3] = (u8)(angle_x10 & 0xFF);
	BLDC_SendCmd(addr, BLDC_CMD_MULTI_POS, data, 4);
}

/* 函数功能：设置指定地址F32C电机的单圈位置目标值。 */
void BLDC_SetSingleAngle(u8 addr, u16 angle_x10)
{
	u8 data[2];

	if(angle_x10 > 3599)
	{
		angle_x10 = 3599;
	}
	data[0] = (u8)((angle_x10 >> 8) & 0xFF);
	data[1] = (u8)(angle_x10 & 0xFF);
	BLDC_SendCmd(addr, BLDC_CMD_SINGLE_POS, data, 2);
}

/* 函数功能：请求指定地址F32C电机返回某一种反馈数据。 */
void BLDC_ReqFeedback(u8 addr, u8 type)
{
	u8 data[1];

	data[0] = type;
	BLDC_SendCmd(addr, BLDC_CMD_FEEDBACK, data, 1);
}

/* 函数功能：将通过USART3收到的单字节反馈数据解析到对应电机状态中。 */
void BLDC_ParseRxData(u8 rx_byte)
{
	switch(bldc_rx_state)
	{
		case 0:
			if(rx_byte == BLDC_FRAME_HEADER)
			{
				bldc_rx_buf[0] = rx_byte;
				bldc_rx_index = 1;
				bldc_rx_state = 1;
			}
			break;

		case 1:
			if(rx_byte == BLDC_ADDR_X || rx_byte == BLDC_ADDR_Y)
			{
				bldc_rx_buf[bldc_rx_index++] = rx_byte;
				bldc_rx_state = 2;
			}
			else
			{
				bldc_rx_state = 0;
			}
			break;

		case 2:
			if(rx_byte <= BLDC_FB_VOLTAGE)
			{
				bldc_rx_buf[bldc_rx_index++] = rx_byte;
				bldc_rx_state = 3;
			}
			else
			{
				bldc_rx_state = 0;
			}
			break;

		case 3:
			bldc_rx_buf[bldc_rx_index++] = rx_byte;
			if(bldc_rx_index >= 7)
			{
				bldc_rx_state = 4;
			}
			break;

		case 4:
			bldc_rx_buf[bldc_rx_index++] = rx_byte;
			bldc_rx_state = 5;
			break;

		case 5:
			if(rx_byte == BLDC_FRAME_TAIL && BLDC_Calc_BCC(bldc_rx_buf, 7) == bldc_rx_buf[7])
			{
				u8 addr = bldc_rx_buf[1];
				u8 type = bldc_rx_buf[2];
				s32 value = ((s32)bldc_rx_buf[3] << 24)
							| ((s32)bldc_rx_buf[4] << 16)
							| ((s32)bldc_rx_buf[5] << 8)
							| bldc_rx_buf[6];
				volatile BLDC_MotorData_t *motor = (addr == BLDC_ADDR_X) ? &BLDC_MotorX : &BLDC_MotorY;

				if(type == BLDC_FB_SPEED)
				{
					motor->speed = (s16)(value & 0xFFFF);
				}
				else if(type == BLDC_FB_MULTI_ANGLE)
				{
					motor->multi_angle = value;
				}
				else if(type == BLDC_FB_SINGLE_ANGLE)
				{
					motor->single_angle = (u16)(value & 0xFFFF);
				}
				else if(type == BLDC_FB_ACC)
				{
					motor->acc = (s16)(value & 0xFFFF);
				}
				else if(type == BLDC_FB_VOLTAGE)
				{
					motor->voltage = (u16)(value & 0xFFFF);
				}
				motor->data_ready = 1;
			}
			bldc_rx_state = 0;
			bldc_rx_index = 0;
			break;

		default:
			bldc_rx_state = 0;
			bldc_rx_index = 0;
			break;
	}
}
