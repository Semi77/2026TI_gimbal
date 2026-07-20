#include "stm32f10x.h"
#include "sys.h"
#include "usart.h"
#include "f32c_gimbal.h"
#include "oled_hardware_i2c.h"
#include "vofa_firewater.h"

#define GIMBAL_PID_SPEED_RPM 70
#define GIMBAL_PID_X_DIR 1
#define GIMBAL_PID_Y_DIR 1
#define GIMBAL_PID_KP_NUM 1
#define GIMBAL_PID_KP_DEN 1
#define GIMBAL_PID_DEAD_ZONE_PIXEL 5
#define GIMBAL_PID_DELTA_LIMIT 20
#define GIMBAL_PID_TARGET_LIMIT 900
#define GIMBAL_PID_Y_MIN_POSITION 115
#define GIMBAL_PID_Y_MAX_POSITION 2785
#define GIMBAL_PID_Y_START_POSITION ((GIMBAL_PID_Y_MIN_POSITION + GIMBAL_PID_Y_MAX_POSITION) / 2)
#define GIMBAL_PID_LOOP_DELAY_MS 30

int Encoder_cnt, Encoder_pr;

/* 将输入值限制在指定的最小值和最大值之间。 */
static s32 MiniBalance_Limit_S32(s32 value, s32 min_value, s32 max_value)
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

/* 清空OLED上一段固定宽度的字符显示区域。 */
static void MiniBalance_OLED_Clear_Field(uint8_t x, uint8_t y, uint8_t chars)
{
	uint8_t index;

	for(index = 0; index < chars; index++)
	{
		OLED_I2C_ShowChar((uint8_t)(x + index * 6U), y, ' ', 8);
	}
}

/* 在OLED上显示带正负号的有符号整数。 */
static void MiniBalance_OLED_Show_Signed(uint8_t x, uint8_t y, s16 value, uint8_t len)
{
	u16 abs_value;

	if(value < 0)
	{
		OLED_I2C_ShowChar(x, y, '-', 8);
		abs_value = (u16)(-value);
	}
	else
	{
		OLED_I2C_ShowChar(x, y, '+', 8);
		abs_value = (u16)value;
	}
	OLED_I2C_ShowNum((uint8_t)(x + 6U), y, abs_value, len, 8);
}

/* 绘制视觉串口和闭环控制测试界面的固定标签。 */
static void MiniBalance_OLED_Draw_Labels(void)
{
	OLED_I2C_ShowString(0, 0, (const uint8_t *)"X:", 8);
	OLED_I2C_ShowString(0, 1, (const uint8_t *)"Y:", 8);
	OLED_I2C_ShowString(0, 2, (const uint8_t *)"EX:", 8);
	OLED_I2C_ShowString(0, 3, (const uint8_t *)"EY:", 8);
	OLED_I2C_ShowString(0, 5, (const uint8_t *)"M:PID", 8);
}

/* 刷新OLED上的视觉坐标和中心点误差。 */
static void MiniBalance_OLED_Update_Vision(void)
{
	u16 vision_x;
	u16 vision_y;
	s16 error_x;
	s16 error_y;

	vision_x = Vision_X;
	vision_y = Vision_Y;
	error_x = Vision_Error_X;
	error_y = Vision_Error_Y;

	MiniBalance_OLED_Clear_Field(18, 0, 5);
	OLED_I2C_ShowNum(18, 0, vision_x, 4, 8);

	MiniBalance_OLED_Clear_Field(18, 1, 5);
	OLED_I2C_ShowNum(18, 1, vision_y, 4, 8);

	MiniBalance_OLED_Clear_Field(24, 2, 6);
	MiniBalance_OLED_Show_Signed(24, 2, error_x, 4);

	MiniBalance_OLED_Clear_Field(24, 3, 6);
	MiniBalance_OLED_Show_Signed(24, 3, error_y, 4);
}

/* 初始化USART2并按位置模式启动两个F32C云台电机。 */
static void MiniBalance_Gimbal_Init(void)
{
	uart2_init(115200);
	usart2_send(0);
	delay_ms(1500);
	F32C_Gimbal_Init(F32C_GIMBAL_X_ID,
					 F32C_GIMBAL_Y_ID,
					 F32C_MODE_MULTI_POSITION_PLAN,
					 GIMBAL_PID_SPEED_RPM);
	F32C_Gimbal_Start();
	F32C_Gimbal_Set_Target_Position(0, GIMBAL_PID_Y_START_POSITION);
	F32C_Gimbal_Send_Target_Position();
}

/* 根据误差死区过滤中心附近的小抖动。 */
static s16 MiniBalance_PID_Apply_Dead_Zone(s16 error)
{
	if(error > -GIMBAL_PID_DEAD_ZONE_PIXEL && error < GIMBAL_PID_DEAD_ZONE_PIXEL)
	{
		return 0;
	}
	return error;
}

/* 按增量式P控制计算本次需要叠加到电机目标位置上的修正量。 */
static s32 MiniBalance_PID_Calc_Delta(s16 error, s16 direction)
{
	s32 delta;

	error = MiniBalance_PID_Apply_Dead_Zone(error);
	delta = (s32)direction * GIMBAL_PID_KP_NUM * error / GIMBAL_PID_KP_DEN;
	return MiniBalance_Limit_S32(delta, -GIMBAL_PID_DELTA_LIMIT, GIMBAL_PID_DELTA_LIMIT);
}

/* 在收到新视觉数据后更新云台目标位置以把目标拉回画面中心。 */
static void MiniBalance_Gimbal_PID_Update(void)
{
	static s32 target_x = 0;
	static s32 target_y = GIMBAL_PID_Y_START_POSITION;
	s32 delta_x;
	s32 delta_y;

	if(!Vision_Data_Ready)
	{
		return;
	}
	Vision_Data_Ready = 0;

	delta_x = MiniBalance_PID_Calc_Delta(Vision_Error_X, GIMBAL_PID_X_DIR);
	delta_y = MiniBalance_PID_Calc_Delta(Vision_Error_Y, GIMBAL_PID_Y_DIR);

	target_x += delta_x;
	target_y += delta_y;
	target_x = MiniBalance_Limit_S32(target_x, -GIMBAL_PID_TARGET_LIMIT, GIMBAL_PID_TARGET_LIMIT);
	target_y = MiniBalance_Limit_S32(target_y, GIMBAL_PID_Y_MIN_POSITION, GIMBAL_PID_Y_MAX_POSITION);

	F32C_Gimbal_Set_Target_Position(target_x, target_y);
	F32C_Gimbal_Send_Target_Position();
	F32C_Gimbal_Request_Position();
	VOFA_FireWater_Send_PID(Vision_Error_X, Vision_Error_Y, target_x, target_y, delta_x, delta_y);
}

/* 初始化系统资源并运行视觉误差到云台位置的增量式P闭环控制。 */
int main(void)
{
	MY_NVIC_PriorityGroupConfig(2);
	delay_init();
	JTAG_Set(JTAG_SWD_DISABLE);
	JTAG_Set(SWD_ENABLE);

	OLED_I2C_Init();
	OLED_I2C_Clear();
	MiniBalance_OLED_Draw_Labels();

	Vision_UART_Init(115200);
	MiniBalance_Gimbal_Init();
	VOFA_FireWater_Init(115200);

	while(1)
	{
		MiniBalance_OLED_Update_Vision();
		MiniBalance_Gimbal_PID_Update();
		delay_ms(GIMBAL_PID_LOOP_DELAY_MS);
	}
}
