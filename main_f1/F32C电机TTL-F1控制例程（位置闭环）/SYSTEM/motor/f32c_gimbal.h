#ifndef __F32C_GIMBAL_H
#define __F32C_GIMBAL_H

#include "stm32f10x.h"
#include "f32c_motor.h"

#define F32C_GIMBAL_X_ID 1
#define F32C_GIMBAL_Y_ID 2
#define F32C_GIMBAL_DEFAULT_SPEED 50
#define F32C_GIMBAL_POSITION_UNIT_PER_DEGREE 10

#define F32C_Gimbal_X F32C_Motor1
#define F32C_Gimbal_Y F32C_Motor2

/* 初始化双轴云台的电机ID、运动模式和默认速度。 */
void F32C_Gimbal_Init(u8 x_id, u8 y_id, u16 mode, s16 speed);

/* 按照协议顺序启动双轴云台的两个电机。 */
void F32C_Gimbal_Start(void);

/* 同时设置双轴云台两个电机的运动模式。 */
void F32C_Gimbal_Set_Mode(u16 mode);

/* 同时设置双轴云台两个电机的位置运动速度。 */
void F32C_Gimbal_Set_Speed(s16 speed);

/* 设置双轴云台两个电机的绝对目标位置。 */
void F32C_Gimbal_Set_Target_Position(s32 x_position, s32 y_position);

/* 在当前目标位置基础上给双轴云台增加位置增量。 */
void F32C_Gimbal_Add_Target_Position(s32 x_increment, s32 y_increment);

/* 将双轴云台保存的目标位置发送给两个电机。 */
void F32C_Gimbal_Send_Target_Position(void);

/* 请求双轴云台两个电机返回当前实时位置。 */
void F32C_Gimbal_Request_Position(void);

/* 解析串口收到的电机位置反馈并自动分发到对应轴。 */
u8 F32C_Gimbal_Parse_Position_Feedback(const u8 *data, u8 length);

/* 读取X轴电机最近一次反馈的当前位置。 */
s32 F32C_Gimbal_Get_X_Position(void);

/* 读取Y轴电机最近一次反馈的当前位置。 */
s32 F32C_Gimbal_Get_Y_Position(void);

#endif
