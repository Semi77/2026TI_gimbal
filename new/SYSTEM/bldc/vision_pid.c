#include "vision_pid.h"
#include "bldc_motor.h"
#include "usart.h"
#include "delay.h"
#include <stdlib.h>

#define VISION_PID_SCALE             1000
#define VISION_PID_X_DIR             1
#define VISION_PID_Y_DIR             -1
#define VISION_PID_X_MIN_X10         -600
#define VISION_PID_X_MAX_X10         600
#define VISION_PID_Y_MIN_X10         300
#define VISION_PID_Y_MAX_X10         1500
#define VISION_PID_SAFE_X_X10        0
#define VISION_PID_SAFE_Y_X10        1
#define VISION_PID_FILTER_OLD        7
#define VISION_PID_FILTER_NEW        3

static VisionPID_t vision_pid_x;
static VisionPID_t vision_pid_y;
static s16 vision_pid_filter_x = 0;
static s16 vision_pid_filter_y = 0;

s32 VisionPID_Target_X = VISION_PID_SAFE_X_X10;
s32 VisionPID_Target_Y = VISION_PID_SAFE_Y_X10;
s16 VisionPID_Delta_X = 0;
s16 VisionPID_Delta_Y = 0;

/* 函数功能：将有符号32位整数限制在指定范围内。 */
static s32 VisionPID_Limit_S32(s32 value, s32 min_value, s32 max_value)
{
	if(value > max_value)
	{
		return max_value;
	}
	if(value < min_value)
	{
		return min_value;
	}
	return value;
}

/* 函数功能：根据死区规则过滤视觉像素误差。 */
static s16 VisionPID_Apply_DeadZone(s16 error, s16 dead_zone)
{
	if(abs((int)error) <= dead_zone)
	{
		return 0;
	}
	return error;
}

/* 函数功能：对视觉误差做一阶低通滤波以减少目标检测抖动。 */
static s16 VisionPID_Filter_Error(s16 *filter_error, s16 raw_error)
{
	*filter_error = (s16)(((s32)(*filter_error) * VISION_PID_FILTER_OLD
		+ (s32)raw_error * VISION_PID_FILTER_NEW) / (VISION_PID_FILTER_OLD + VISION_PID_FILTER_NEW));
	return *filter_error;
}

/* 函数功能：根据当前视觉误差计算单轴纯P角度增量并更新误差历史。 */
static s16 VisionPID_Calc(VisionPID_t *pid, s16 raw_error)
{
	s32 delta;

	pid->error = VisionPID_Apply_DeadZone(raw_error, pid->dead_zone);
	delta = (s32)pid->kp * pid->error;
	delta /= VISION_PID_SCALE;
	delta = VisionPID_Limit_S32(delta, -pid->max_delta, pid->max_delta);

	pid->prev_error = pid->last_error;
	pid->last_error = pid->error;
	return (s16)delta;
}

/* 函数功能：初始化视觉增量式PID参数和云台目标角度。 */
void VisionPID_Init(void)
{
	vision_pid_x.kp = 350;
	vision_pid_x.ki = 0;
	vision_pid_x.kd = 0;
	vision_pid_x.dead_zone = 6;
	vision_pid_x.max_delta = 35;

	vision_pid_y.kp = 350;
	vision_pid_y.ki = 0;
	vision_pid_y.kd = 0;
	vision_pid_y.dead_zone = 6;
	vision_pid_y.max_delta = 35;

	VisionPID_Reset();
}

/* 函数功能：清零视觉PID历史误差并把云台目标角度复位到安全中心。 */
void VisionPID_Reset(void)
{
	vision_pid_x.error = 0;
	vision_pid_x.last_error = 0;
	vision_pid_x.prev_error = 0;
	vision_pid_y.error = 0;
	vision_pid_y.last_error = 0;
	vision_pid_y.prev_error = 0;
	vision_pid_filter_x = 0;
	vision_pid_filter_y = 0;
	VisionPID_Target_X = VISION_PID_SAFE_X_X10;
	VisionPID_Target_Y = VISION_PID_SAFE_Y_X10;
	VisionPID_Delta_X = 0;
	VisionPID_Delta_Y = 0;
}

/* 函数功能：在收到新视觉坐标后更新PID并发送新的F32C云台目标角度。 */
u8 VisionPID_Update(void)
{
	s16 error_x;
	s16 error_y;

	if(!Vision_Data_Ready)
	{
		return 0;
	}

	error_x = VisionPID_Filter_Error(&vision_pid_filter_x, Vision_Error_X);
	error_y = VisionPID_Filter_Error(&vision_pid_filter_y, Vision_Error_Y);
	Vision_Data_Ready = 0;

	VisionPID_Delta_X = VisionPID_Calc(&vision_pid_x, error_x);
	VisionPID_Delta_Y = VisionPID_Calc(&vision_pid_y, error_y);

	VisionPID_Target_X += (s32)VISION_PID_X_DIR * VisionPID_Delta_X;
	VisionPID_Target_Y += (s32)VISION_PID_Y_DIR * VisionPID_Delta_Y;
	VisionPID_Target_X = VisionPID_Limit_S32(VisionPID_Target_X, VISION_PID_X_MIN_X10, VISION_PID_X_MAX_X10);
	VisionPID_Target_Y = VisionPID_Limit_S32(VisionPID_Target_Y, VISION_PID_Y_MIN_X10, VISION_PID_Y_MAX_X10);

	BLDC_SetMultiAngle(BLDC_ADDR_X, VisionPID_Target_X);
	delay_ms(2);
	BLDC_SetMultiAngle(BLDC_ADDR_Y, VisionPID_Target_Y);
	return 1;
}
