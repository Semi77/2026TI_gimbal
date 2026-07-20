#ifndef __OLED_HARDWARE_I2C_H
#define __OLED_HARDWARE_I2C_H

#include "stm32f10x.h"
#include <stdint.h>

#define OLED_I2C_CMD  0
#define OLED_I2C_DATA 1

#define OLED_I2C_ADDR_7BIT 0x3C
#define OLED_I2C_PORT I2C1
#define OLED_I2C_RCC_APB1 RCC_APB1Periph_I2C1
#define OLED_I2C_GPIO_PORT GPIOB
#define OLED_I2C_GPIO_RCC RCC_APB2Periph_GPIOB
#define OLED_I2C_SCL_PIN GPIO_Pin_6
#define OLED_I2C_SDA_PIN GPIO_Pin_7
#define OLED_I2C_CLOCK_SPEED 400000

/* 初始化STM32F1的I2C接口并启动SSD1306 OLED屏幕。 */
void OLED_I2C_Init(void);

/* 向SSD1306 OLED写入一个命令或数据字节。 */
void OLED_I2C_WR_Byte(uint8_t dat, uint8_t cmd);

/* 释放可能被OLED拉低的I2C数据线。 */
void OLED_I2C_SDA_Unlock(void);

/* 设置OLED显示内容是否反色。 */
void OLED_I2C_ColorTurn(uint8_t enable);

/* 设置OLED显示方向是否旋转180度。 */
void OLED_I2C_DisplayTurn(uint8_t enable);

/* 设置OLED的页地址和列地址。 */
void OLED_I2C_Set_Pos(uint8_t x, uint8_t y);

/* 打开OLED显示。 */
void OLED_I2C_Display_On(void);

/* 关闭OLED显示。 */
void OLED_I2C_Display_Off(void);

/* 清空OLED屏幕内容。 */
void OLED_I2C_Clear(void);

/* 在OLED指定位置显示一个ASCII字符。 */
void OLED_I2C_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t sizey);

/* 计算一个无符号整数的幂值。 */
uint32_t OLED_I2C_Pow(uint8_t m, uint8_t n);

/* 在OLED指定位置显示一个无符号整数。 */
void OLED_I2C_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t sizey);

/* 在OLED指定位置显示字符串。 */
void OLED_I2C_ShowString(uint8_t x, uint8_t y, const uint8_t *chr, uint8_t sizey);

/* 在OLED指定位置显示字库中的汉字。 */
void OLED_I2C_ShowChinese(uint8_t x, uint8_t y, uint8_t no, uint8_t sizey);

/* 在OLED指定区域显示位图。 */
void OLED_I2C_DrawBMP(uint8_t x, uint8_t y, uint8_t sizex, uint8_t sizey, const uint8_t BMP[]);

/* 绘制传感器数据显示界面的固定标签。 */
void OLED_I2C_DrawSensorLabels(void);

/* 更新OLED上传感器数据界面的动态数值。 */
void OLED_I2C_UpdateSensorValues(int32_t yaw, int32_t gyro_z,
	uint8_t gray_digital, const char *state,
	const unsigned short gray_normal[8]);

#endif
