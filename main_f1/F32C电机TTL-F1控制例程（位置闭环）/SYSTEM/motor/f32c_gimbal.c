#include "f32c_gimbal.h"
#include "delay.h"

/* 初始化双轴云台的电机ID、运动模式和默认速度。 */
void F32C_Gimbal_Init(u8 x_id, u8 y_id, u16 mode, s16 speed)
{
	F32C_Motor_Init(&F32C_Gimbal_X, x_id, mode, speed);
	F32C_Motor_Init(&F32C_Gimbal_Y, y_id, mode, speed);
}

/* 按照协议顺序启动双轴云台的两个电机。 */
void F32C_Gimbal_Start(void)
{
	F32C_Motor_Start(&F32C_Gimbal_X);
	delay_ms(1);
	F32C_Motor_Start(&F32C_Gimbal_Y);
	delay_ms(10);
}

/* 同时设置双轴云台两个电机的运动模式。 */
void F32C_Gimbal_Set_Mode(u16 mode)
{
	F32C_Motor_Set_Mode(&F32C_Gimbal_X, mode);
	delay_ms(1);
	F32C_Motor_Set_Mode(&F32C_Gimbal_Y, mode);
}

/* 同时设置双轴云台两个电机的位置运动速度。 */
void F32C_Gimbal_Set_Speed(s16 speed)
{
	F32C_Motor_Set_Speed(&F32C_Gimbal_X, speed);
	delay_ms(1);
	F32C_Motor_Set_Speed(&F32C_Gimbal_Y, speed);
}

/* 设置双轴云台两个电机的绝对目标位置。 */
void F32C_Gimbal_Set_Target_Position(s32 x_position, s32 y_position)
{
	F32C_Motor_Set_Target_Position(&F32C_Gimbal_X, x_position);
	F32C_Motor_Set_Target_Position(&F32C_Gimbal_Y, y_position);
}

/* 在当前目标位置基础上给双轴云台增加位置增量。 */
void F32C_Gimbal_Add_Target_Position(s32 x_increment, s32 y_increment)
{
	F32C_Motor_Add_Target_Position(&F32C_Gimbal_X, x_increment);
	F32C_Motor_Add_Target_Position(&F32C_Gimbal_Y, y_increment);
}

/* 将双轴云台保存的目标位置发送给两个电机。 */
void F32C_Gimbal_Send_Target_Position(void)
{
	F32C_Motor_Send_Target_Position(&F32C_Gimbal_X);
	delay_ms(10);
	F32C_Motor_Send_Target_Position(&F32C_Gimbal_Y);
	delay_ms(10);
}

/* 请求双轴云台两个电机返回当前实时位置。 */
void F32C_Gimbal_Request_Position(void)
{
	F32C_Motor_Request_Position(&F32C_Gimbal_X);
	delay_ms(10);
	F32C_Motor_Request_Position(&F32C_Gimbal_Y);
	delay_ms(10);
}

/* 解析串口收到的电机位置反馈并自动分发到对应轴。 */
u8 F32C_Gimbal_Parse_Position_Feedback(const u8 *data, u8 length)
{
	if(F32C_Motor_Parse_Position_Feedback(&F32C_Gimbal_X, data, length))
	{
		return 1;
	}
	if(F32C_Motor_Parse_Position_Feedback(&F32C_Gimbal_Y, data, length))
	{
		return 1;
	}
	return 0;
}

/* 读取X轴电机最近一次反馈的当前位置。 */
s32 F32C_Gimbal_Get_X_Position(void)
{
	return F32C_Gimbal_X.current_position;
}

/* 读取Y轴电机最近一次反馈的当前位置。 */
s32 F32C_Gimbal_Get_Y_Position(void)
{
	return F32C_Gimbal_Y.current_position;
}
