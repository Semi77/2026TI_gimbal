#ifndef __F32C_MOTOR_H
#define __F32C_MOTOR_H

#include "stm32f10x.h"

typedef enum
{
	F32C_MODE_SPEED = 0,
	F32C_MODE_MULTI_POSITION_PLAN = 1,
	F32C_MODE_SINGLE_POSITION_PLAN = 2,
	F32C_MODE_MULTI_POSITION_DIRECT = 3,
	F32C_MODE_SINGLE_POSITION_DIRECT = 4
} F32C_Motor_Mode_t;

typedef struct
{
	u8 id;
	u16 mode;
	s16 target_speed;
	s32 target_position;
	volatile s32 current_position;
	volatile u8 feedback_valid;
} F32C_Motor_t;

extern F32C_Motor_t F32C_Motor1;
extern F32C_Motor_t F32C_Motor2;

void F32C_Motor_Init(F32C_Motor_t *motor, u8 id, u16 mode, s16 speed);
void F32C_Motor_Start(F32C_Motor_t *motor);
void F32C_Motor_Enable(F32C_Motor_t *motor);
void F32C_Motor_Set_Mode(F32C_Motor_t *motor, u16 mode);
void F32C_Motor_Set_Speed(F32C_Motor_t *motor, s16 speed);
void F32C_Motor_Set_Target_Position(F32C_Motor_t *motor, s32 position);
void F32C_Motor_Add_Target_Position(F32C_Motor_t *motor, s32 increment);
void F32C_Motor_Send_Target_Position(F32C_Motor_t *motor);
void F32C_Motor_Request_Position(F32C_Motor_t *motor);
u8 F32C_Motor_Parse_Position_Feedback(F32C_Motor_t *motor, const u8 *data, u8 length);

#endif
