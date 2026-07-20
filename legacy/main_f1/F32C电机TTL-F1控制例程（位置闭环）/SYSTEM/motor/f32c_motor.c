#include "f32c_motor.h"
#include "usart.h"
#include "delay.h"

#define F32C_FRAME_HEADER 0x7A
#define F32C_FRAME_TAIL   0x7B

F32C_Motor_t F32C_Motor1;
F32C_Motor_t F32C_Motor2;

/* 计算F32C协议指定长度数据的异或校验值。 */
static u8 F32C_Motor_Calculate_BCC(const u8 *data, u8 length)
{
	u8 checksum = 0;
	u8 index;

	for(index = 0; index < length; index++)
	{
		checksum ^= data[index];
	}
	return checksum;
}

/* 初始化一个F32C电机对象的编号、模式、速度和位置状态。 */
void F32C_Motor_Init(F32C_Motor_t *motor, u8 id, u16 mode, s16 speed)
{
	if(motor == 0)
	{
		return;
	}
	motor->id = id;
	motor->mode = mode;
	motor->target_speed = speed;
	motor->target_position = 0;
	motor->current_position = 0;
	motor->feedback_valid = 0;
}

/* 按使能、模式和速度的顺序启动一个F32C电机。 */
void F32C_Motor_Start(F32C_Motor_t *motor)
{
	if(motor == 0)
	{
		return;
	}
	F32C_Motor_Enable(motor);
	delay_ms(1);
	F32C_Motor_Set_Mode(motor, motor->mode);
	delay_ms(1);
	F32C_Motor_Set_Speed(motor, motor->target_speed);
	delay_ms(10);
}

/* 向指定编号的F32C电机发送使能指令。 */
void F32C_Motor_Enable(F32C_Motor_t *motor)
{
	u8 frame[5] = {F32C_FRAME_HEADER, 0, 0x06, 0, F32C_FRAME_TAIL};

	if(motor == 0)
	{
		return;
	}
	frame[1] = motor->id;
	frame[3] = F32C_Motor_Calculate_BCC(frame, 3);
	USART2_SEND(frame, sizeof(frame));
}

/* 设置并发送F32C电机的控制模式。 */
void F32C_Motor_Set_Mode(F32C_Motor_t *motor, u16 mode)
{
	u8 frame[7] = {F32C_FRAME_HEADER, 0, 0x00, 0, 0, 0, F32C_FRAME_TAIL};

	if(motor == 0)
	{
		return;
	}
	motor->mode = mode;
	frame[1] = motor->id;
	frame[3] = (u8)(mode >> 8);
	frame[4] = (u8)mode;
	frame[5] = F32C_Motor_Calculate_BCC(frame, 5);
	USART2_SEND(frame, sizeof(frame));
}

/* 设置并发送F32C电机的位置运动速度。 */
void F32C_Motor_Set_Speed(F32C_Motor_t *motor, s16 speed)
{
	u16 speed_data;
	u8 frame[7] = {F32C_FRAME_HEADER, 0, 0x01, 0, 0, 0, F32C_FRAME_TAIL};

	if(motor == 0)
	{
		return;
	}
	motor->target_speed = speed;
	speed_data = (u16)speed;
	frame[1] = motor->id;
	frame[3] = (u8)(speed_data >> 8);
	frame[4] = (u8)speed_data;
	frame[5] = F32C_Motor_Calculate_BCC(frame, 5);
	USART2_SEND(frame, sizeof(frame));
}

/* 修改F32C电机对象中等待发送的绝对目标位置。 */
void F32C_Motor_Set_Target_Position(F32C_Motor_t *motor, s32 position)
{
	if(motor == 0)
	{
		return;
	}
	motor->target_position = position;
}

/* 在F32C电机当前目标位置上增加一个位置增量。 */
void F32C_Motor_Add_Target_Position(F32C_Motor_t *motor, s32 increment)
{
	if(motor == 0)
	{
		return;
	}
	motor->target_position += increment;
}

/* 将F32C电机对象中保存的目标位置发送给驱动器。 */
void F32C_Motor_Send_Target_Position(F32C_Motor_t *motor)
{
	u32 position_data;
	u8 frame[9] = {F32C_FRAME_HEADER, 0, 0x02, 0, 0, 0, 0, 0, F32C_FRAME_TAIL};

	if(motor == 0)
	{
		return;
	}
	position_data = (u32)motor->target_position;
	frame[1] = motor->id;
	frame[3] = (u8)(position_data >> 24);
	frame[4] = (u8)(position_data >> 16);
	frame[5] = (u8)(position_data >> 8);
	frame[6] = (u8)position_data;
	frame[7] = F32C_Motor_Calculate_BCC(frame, 7);
	USART2_SEND(frame, sizeof(frame));
}

/* 请求F32C电机返回当前实时位置。 */
void F32C_Motor_Request_Position(F32C_Motor_t *motor)
{
	u8 frame[6] = {F32C_FRAME_HEADER, 0, 0x0E, 0x01, 0, F32C_FRAME_TAIL};

	if(motor == 0)
	{
		return;
	}
	frame[1] = motor->id;
	frame[4] = F32C_Motor_Calculate_BCC(frame, 4);
	USART2_SEND(frame, sizeof(frame));
}

/* 校验并解析指定F32C电机返回的实时位置数据帧。 */
u8 F32C_Motor_Parse_Position_Feedback(F32C_Motor_t *motor, const u8 *data, u8 length)
{
	u32 position_data;

	if(motor == 0 || data == 0 || length < 9)
	{
		return 0;
	}
	if(data[0] != F32C_FRAME_HEADER || data[8] != F32C_FRAME_TAIL)
	{
		return 0;
	}
	if(data[1] != motor->id || data[2] != 0x01)
	{
		return 0;
	}
	if(data[7] != F32C_Motor_Calculate_BCC(data, 7))
	{
		return 0;
	}

	position_data = ((u32)data[3] << 24)
					| ((u32)data[4] << 16)
					| ((u32)data[5] << 8)
					| (u32)data[6];
	motor->current_position = (s32)position_data;
	motor->feedback_valid = 1;
	return 1;
}
