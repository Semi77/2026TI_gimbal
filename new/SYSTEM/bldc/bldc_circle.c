#include "bldc_circle.h"
#include "bldc_motor.h"
#include "delay.h"
#include "oled_hardware_i2c.h"
#include <math.h>
#include <stdlib.h>

#define BLDC_CIRCLE_CENTER_X_X10       0
#define BLDC_CIRCLE_CENTER_Y_X10       900
#define BLDC_CIRCLE_RADIUS_X10         450
#define BLDC_CIRCLE_POINT_COUNT        240U
#define BLDC_CIRCLE_STEP_DELAY_MS      20U
#define BLDC_CIRCLE_START_DELAY_MS     1200U
#define BLDC_CIRCLE_HOME_DELAY_MS      1500U
#define BLDC_CIRCLE_ACC                35U
#define BLDC_CIRCLE_SPEED_RPM          30
#define BLDC_CIRCLE_PI                 3.14159265358979323846

static u16 bldc_circle_index = 0;
static s32 bldc_circle_target_x = BLDC_CIRCLE_CENTER_X_X10;
static s32 bldc_circle_target_y = BLDC_CIRCLE_CENTER_Y_X10;

/* 函数功能：初始化两个F32C云台电机并移动到圆形轨迹起点。 */
void BLDC_Circle_Init(void)
{
	uart3_init(115200);
	delay_ms(200);

	BLDC_Disable(BLDC_ADDR_X);
	delay_ms(100);
	BLDC_SetAcc(BLDC_ADDR_X, BLDC_CIRCLE_ACC);
	delay_ms(100);
	BLDC_Enable(BLDC_ADDR_X);
	delay_ms(100);
	BLDC_SetMode(BLDC_ADDR_X, BLDC_MODE_MULTI_DIRECT);
	delay_ms(100);
	BLDC_SetSpeed(BLDC_ADDR_X, BLDC_CIRCLE_SPEED_RPM);
	delay_ms(100);

	BLDC_Disable(BLDC_ADDR_Y);
	delay_ms(100);
	BLDC_SetAcc(BLDC_ADDR_Y, BLDC_CIRCLE_ACC);
	delay_ms(100);
	BLDC_Enable(BLDC_ADDR_Y);
	delay_ms(100);
	BLDC_SetMode(BLDC_ADDR_Y, BLDC_MODE_MULTI_DIRECT);
	delay_ms(100);
	BLDC_SetSpeed(BLDC_ADDR_Y, BLDC_CIRCLE_SPEED_RPM);
	delay_ms(100);

	BLDC_SetMultiAngle(BLDC_ADDR_Y, BLDC_CIRCLE_CENTER_Y_X10);
	delay_ms(BLDC_CIRCLE_HOME_DELAY_MS);

	bldc_circle_target_x = BLDC_CIRCLE_CENTER_X_X10 + BLDC_CIRCLE_RADIUS_X10;
	bldc_circle_target_y = BLDC_CIRCLE_CENTER_Y_X10;
	BLDC_SetMultiAngle(BLDC_ADDR_X, bldc_circle_target_x);
	delay_ms(2);
	BLDC_SetMultiAngle(BLDC_ADDR_Y, bldc_circle_target_y);
	delay_ms(BLDC_CIRCLE_START_DELAY_MS);
}

/* 函数功能：计算圆形轨迹的下一个位置点并发送给两个F32C云台电机。 */
void BLDC_Circle_Update(void)
{
	double angle;

	angle = 2.0 * BLDC_CIRCLE_PI * bldc_circle_index / BLDC_CIRCLE_POINT_COUNT;
	bldc_circle_target_x = BLDC_CIRCLE_CENTER_X_X10 + (s32)(BLDC_CIRCLE_RADIUS_X10 * cos(angle));
	bldc_circle_target_y = BLDC_CIRCLE_CENTER_Y_X10 + (s32)(BLDC_CIRCLE_RADIUS_X10 * sin(angle));

	BLDC_SetMultiAngle(BLDC_ADDR_X, bldc_circle_target_x);
	delay_ms(2);
	BLDC_SetMultiAngle(BLDC_ADDR_Y, bldc_circle_target_y);

	bldc_circle_index++;
	if(bldc_circle_index >= BLDC_CIRCLE_POINT_COUNT)
	{
		bldc_circle_index = 0;
	}

	delay_ms(BLDC_CIRCLE_STEP_DELAY_MS);
}

/* 函数功能：在OLED上显示当前圆形轨迹目标和两个电机反馈速度。 */
void BLDC_Circle_Show(void)
{
	OLED_I2C_Clear();
	OLED_I2C_ShowString(0, 0, (const uint8_t *)"CX:", 8);
	if(bldc_circle_target_x < 0)
	{
		OLED_I2C_ShowString(18, 0, (const uint8_t *)"-", 8);
		OLED_I2C_ShowNum(24, 0, (u32)abs((int)bldc_circle_target_x), 4, 8);
	}
	else
	{
		OLED_I2C_ShowString(18, 0, (const uint8_t *)"+", 8);
		OLED_I2C_ShowNum(24, 0, (u32)bldc_circle_target_x, 4, 8);
	}

	OLED_I2C_ShowString(0, 1, (const uint8_t *)"CY:", 8);
	OLED_I2C_ShowNum(24, 1, (u32)bldc_circle_target_y, 4, 8);
	OLED_I2C_ShowString(0, 2, (const uint8_t *)"SX:", 8);
	OLED_I2C_ShowNum(24, 2, (u32)abs((int)BLDC_MotorX.speed), 4, 8);
	OLED_I2C_ShowString(64, 2, (const uint8_t *)"SY:", 8);
	OLED_I2C_ShowNum(88, 2, (u32)abs((int)BLDC_MotorY.speed), 4, 8);
	OLED_I2C_ShowString(0, 3, (const uint8_t *)"MODE:CIRCLE", 8);
}
