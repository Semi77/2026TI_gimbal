#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "oled_hardware_i2c.h"
#include "bldc_motor.h"
#include "vision_pid.h"
#include "vofa_firewater.h"
#include <stdlib.h>

// GIMBAL_SAFE_X_X10和GIMBAL_SAFE_Y_X10这两个参数调整初始旋转的位置
#define GIMBAL_SAFE_X_X10       0
#define GIMBAL_SAFE_Y_X10       0
#define GIMBAL_HOME_ACC         35U
#define GIMBAL_HOME_SPEED_RPM   30
#define GIMBAL_TRACK_ACC        100U
#define GIMBAL_TRACK_SPEED_RPM  90
#define GIMBAL_HOME_DELAY_MS    1800U
#define GIMBAL_CONTROL_DELAY_MS 10U
#define GIMBAL_OLED_DIVIDER     10U

int Encoder_cnt, Encoder_pr;

/* 函数功能：初始化STM32基础外设和调试相关系统设置。 */
static void MiniBalance_System_Init(void)
{
	MY_NVIC_PriorityGroupConfig(2);
	delay_init();
	JTAG_Set(JTAG_SWD_DISABLE);
	JTAG_Set(SWD_ENABLE);
}

/* 函数功能：初始化OLED并显示视觉闭环调试界面的启动提示。 */
static void MiniBalance_Display_Init(void)
{
	OLED_I2C_Init();
	OLED_I2C_Clear();
	OLED_I2C_ShowString(0, 0, (const uint8_t *)"VISION GIMBAL", 8);
	OLED_I2C_ShowString(0, 1, (const uint8_t *)"UART WAIT", 8);
}

/* 函数功能：初始化MaixCAM视觉串口和F32C云台控制串口。 */
static void MiniBalance_Comm_Init(void)
{
	Vision_UART_Init(115200);
	VOFA_FireWater_Init(115200);
	uart3_init(115200);
	delay_ms(200);
}

/* 函数功能：将F32C云台切换到多圈直通位置模式并回到安全中心姿态。 */
static void MiniBalance_Gimbal_Home(void)
{
	BLDC_Disable(BLDC_ADDR_X);
	delay_ms(100);
	BLDC_Disable(BLDC_ADDR_Y);
	delay_ms(100);

	BLDC_SetAcc(BLDC_ADDR_X, GIMBAL_HOME_ACC);
	delay_ms(100);
	BLDC_SetAcc(BLDC_ADDR_Y, GIMBAL_HOME_ACC);
	delay_ms(100);

	BLDC_Enable(BLDC_ADDR_X);
	delay_ms(100);
	BLDC_Enable(BLDC_ADDR_Y);
	delay_ms(100);

	BLDC_SetMode(BLDC_ADDR_X, BLDC_MODE_MULTI_DIRECT);
	delay_ms(100);
	BLDC_SetMode(BLDC_ADDR_Y, BLDC_MODE_MULTI_DIRECT);
	delay_ms(100);

	BLDC_SetSpeed(BLDC_ADDR_X, GIMBAL_HOME_SPEED_RPM);
	delay_ms(100);
	BLDC_SetSpeed(BLDC_ADDR_Y, GIMBAL_HOME_SPEED_RPM);
	delay_ms(100);

	BLDC_SetMultiAngle(BLDC_ADDR_X, GIMBAL_SAFE_X_X10);
	delay_ms(2);
	BLDC_SetMultiAngle(BLDC_ADDR_Y, GIMBAL_SAFE_Y_X10);
	delay_ms(GIMBAL_HOME_DELAY_MS);

	BLDC_SetAcc(BLDC_ADDR_X, GIMBAL_TRACK_ACC);
	delay_ms(20);
	BLDC_SetAcc(BLDC_ADDR_Y, GIMBAL_TRACK_ACC);
	delay_ms(20);
	BLDC_SetSpeed(BLDC_ADDR_X, GIMBAL_TRACK_SPEED_RPM);
	delay_ms(20);
	BLDC_SetSpeed(BLDC_ADDR_Y, GIMBAL_TRACK_SPEED_RPM);
	delay_ms(20);
}

/* 函数功能：在OLED指定位置显示带符号的十进制整数。 */
static void MiniBalance_OLED_ShowSigned(uint8_t x, uint8_t y, s16 value, uint8_t len)
{
	if(value < 0)
	{
		OLED_I2C_ShowString(x, y, (const uint8_t *)"-", 8);
		OLED_I2C_ShowNum((uint8_t)(x + 6), y, (u32)abs((int)value), len, 8);
	}
	else
	{
		OLED_I2C_ShowString(x, y, (const uint8_t *)"+", 8);
		OLED_I2C_ShowNum((uint8_t)(x + 6), y, (u32)value, len, 8);
	}
}

/* 函数功能：在OLED上显示MaixCAM目标坐标和相对画面中心的视觉误差。 */
static void MiniBalance_Display_Vision(void)
{
	OLED_I2C_Clear();
	OLED_I2C_ShowString(0, 0, (const uint8_t *)"X:", 8);
	OLED_I2C_ShowNum(18, 0, Vision_X, 4, 8);
	OLED_I2C_ShowString(64, 0, (const uint8_t *)"Y:", 8);
	OLED_I2C_ShowNum(82, 0, Vision_Y, 4, 8);

	OLED_I2C_ShowString(0, 1, (const uint8_t *)"EX:", 8);
	MiniBalance_OLED_ShowSigned(24, 1, Vision_Error_X, 4);
	OLED_I2C_ShowString(0, 2, (const uint8_t *)"EY:", 8);
	MiniBalance_OLED_ShowSigned(24, 2, Vision_Error_Y, 4);

	OLED_I2C_ShowString(0, 3, (const uint8_t *)"DX:", 8);
	MiniBalance_OLED_ShowSigned(24, 3, VisionPID_Delta_X, 3);
	OLED_I2C_ShowString(64, 3, (const uint8_t *)"DY:", 8);
	MiniBalance_OLED_ShowSigned(88, 3, VisionPID_Delta_Y, 3);
}

/* 函数功能：运行MaixCAM视觉数据接收、误差计算和OLED显示主程序。 */
int main(void)
{
	u8 vofa_count = 0;
	u8 oled_count = 0;

	MiniBalance_System_Init();
	MiniBalance_Display_Init();
	MiniBalance_Comm_Init();
	MiniBalance_Gimbal_Home();
	VisionPID_Init();
	Vision_Data_Ready = 0;
	BLDC_SetMultiAngle(BLDC_ADDR_X, GIMBAL_SAFE_X_X10);
	delay_ms(2);
	BLDC_SetMultiAngle(BLDC_ADDR_Y, GIMBAL_SAFE_Y_X10);
	delay_ms(300);

	while(1)
	{
		VisionPID_Update();
		vofa_count++;
		if(vofa_count >= GIMBAL_OLED_DIVIDER)
		{
			vofa_count = 0;
			VOFA_FireWater_Send_PID_Debug(Vision_X, Vision_Y,
				Vision_Error_X, Vision_Error_Y,
				VisionPID_Delta_X, VisionPID_Delta_Y,
				VisionPID_Target_X, VisionPID_Target_Y);
		}
		oled_count++;
		if(oled_count >= GIMBAL_OLED_DIVIDER)
		{
			oled_count = 0;
			MiniBalance_Display_Vision();
		}
		delay_ms(GIMBAL_CONTROL_DELAY_MS);
	}
}
