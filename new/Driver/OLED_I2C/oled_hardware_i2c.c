#include "oled_hardware_i2c.h"
#include "oledfont.h"
#include "delay.h"
#include <stddef.h>

#define OLED_I2C_TIMEOUT_COUNT 50000U
#define OLED_I2C_FONT_8       8U

/* 等待指定I2C事件出现并在超时后返回失败。 */
static uint8_t OLED_I2C_Wait_Event(uint32_t event)
{
	uint32_t timeout = OLED_I2C_TIMEOUT_COUNT;

	while(I2C_CheckEvent(OLED_I2C_PORT, event) != SUCCESS)
	{
		if(timeout-- == 0U)
		{
			return 0;
		}
	}
	return 1;
}

/* 等待I2C总线空闲并在超时后返回失败。 */
static uint8_t OLED_I2C_Wait_Idle(void)
{
	uint32_t timeout = OLED_I2C_TIMEOUT_COUNT;

	while(I2C_GetFlagStatus(OLED_I2C_PORT, I2C_FLAG_BUSY) == SET)
	{
		if(timeout-- == 0U)
		{
			return 0;
		}
	}
	return 1;
}

/* 使用STM32F1硬件I2C向OLED发送两个字节。 */
static uint8_t OLED_I2C_Write_Two_Bytes(uint8_t control, uint8_t data)
{
	if(!OLED_I2C_Wait_Idle())
	{
		OLED_I2C_SDA_Unlock();
	}

	I2C_GenerateSTART(OLED_I2C_PORT, ENABLE);
	if(!OLED_I2C_Wait_Event(I2C_EVENT_MASTER_MODE_SELECT))
	{
		OLED_I2C_SDA_Unlock();
		return 0;
	}

	I2C_Send7bitAddress(OLED_I2C_PORT, (uint8_t)(OLED_I2C_ADDR_7BIT << 1), I2C_Direction_Transmitter);
	if(!OLED_I2C_Wait_Event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
	{
		OLED_I2C_SDA_Unlock();
		return 0;
	}

	I2C_SendData(OLED_I2C_PORT, control);
	if(!OLED_I2C_Wait_Event(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
	{
		OLED_I2C_SDA_Unlock();
		return 0;
	}

	I2C_SendData(OLED_I2C_PORT, data);
	if(!OLED_I2C_Wait_Event(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
	{
		OLED_I2C_SDA_Unlock();
		return 0;
	}

	I2C_GenerateSTOP(OLED_I2C_PORT, ENABLE);
	return 1;
}

/* 初始化STM32F1的I2C GPIO和I2C1外设。 */
static void OLED_I2C_Bus_Init(void)
{
	GPIO_InitTypeDef gpio_init;
	I2C_InitTypeDef i2c_init;

	RCC_APB2PeriphClockCmd(OLED_I2C_GPIO_RCC, ENABLE);
	RCC_APB1PeriphClockCmd(OLED_I2C_RCC_APB1, ENABLE);

	gpio_init.GPIO_Pin = OLED_I2C_SCL_PIN | OLED_I2C_SDA_PIN;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_init.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(OLED_I2C_GPIO_PORT, &gpio_init);

	I2C_DeInit(OLED_I2C_PORT);
	I2C_StructInit(&i2c_init);
	i2c_init.I2C_Mode = I2C_Mode_I2C;
	i2c_init.I2C_DutyCycle = I2C_DutyCycle_2;
	i2c_init.I2C_OwnAddress1 = 0x00;
	i2c_init.I2C_Ack = I2C_Ack_Enable;
	i2c_init.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	i2c_init.I2C_ClockSpeed = OLED_I2C_CLOCK_SPEED;
	I2C_Init(OLED_I2C_PORT, &i2c_init);
	I2C_Cmd(OLED_I2C_PORT, ENABLE);
}

/* 释放可能被OLED拉低的I2C数据线。 */
void OLED_I2C_SDA_Unlock(void)
{
	GPIO_InitTypeDef gpio_init;
	uint8_t index;

	I2C_Cmd(OLED_I2C_PORT, DISABLE);
	I2C_DeInit(OLED_I2C_PORT);

	gpio_init.GPIO_Pin = OLED_I2C_SCL_PIN | OLED_I2C_SDA_PIN;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_init.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(OLED_I2C_GPIO_PORT, &gpio_init);

	GPIO_SetBits(OLED_I2C_GPIO_PORT, OLED_I2C_SCL_PIN | OLED_I2C_SDA_PIN);
	delay_ms(1);

	for(index = 0; index < 9U; index++)
	{
		GPIO_ResetBits(OLED_I2C_GPIO_PORT, OLED_I2C_SCL_PIN);
		delay_us(5);
		GPIO_SetBits(OLED_I2C_GPIO_PORT, OLED_I2C_SCL_PIN);
		delay_us(5);
		if(GPIO_ReadInputDataBit(OLED_I2C_GPIO_PORT, OLED_I2C_SDA_PIN) == Bit_SET)
		{
			break;
		}
	}

	GPIO_ResetBits(OLED_I2C_GPIO_PORT, OLED_I2C_SDA_PIN);
	delay_us(5);
	GPIO_SetBits(OLED_I2C_GPIO_PORT, OLED_I2C_SCL_PIN);
	delay_us(5);
	GPIO_SetBits(OLED_I2C_GPIO_PORT, OLED_I2C_SDA_PIN);
	delay_us(5);

	OLED_I2C_Bus_Init();
}

/* 向SSD1306 OLED写入一个命令或数据字节。 */
void OLED_I2C_WR_Byte(uint8_t dat, uint8_t cmd)
{
	uint8_t control = (cmd == OLED_I2C_DATA) ? 0x40U : 0x00U;

	OLED_I2C_Write_Two_Bytes(control, dat);
}

/* 设置OLED显示内容是否反色。 */
void OLED_I2C_ColorTurn(uint8_t enable)
{
	if(enable == 0U)
	{
		OLED_I2C_WR_Byte(0xA6, OLED_I2C_CMD);
	}
	else
	{
		OLED_I2C_WR_Byte(0xA7, OLED_I2C_CMD);
	}
}

/* 设置OLED显示方向是否旋转180度。 */
void OLED_I2C_DisplayTurn(uint8_t enable)
{
	if(enable == 0U)
	{
		OLED_I2C_WR_Byte(0xC8, OLED_I2C_CMD);
		OLED_I2C_WR_Byte(0xA1, OLED_I2C_CMD);
	}
	else
	{
		OLED_I2C_WR_Byte(0xC0, OLED_I2C_CMD);
		OLED_I2C_WR_Byte(0xA0, OLED_I2C_CMD);
	}
}

/* 设置OLED的页地址和列地址。 */
void OLED_I2C_Set_Pos(uint8_t x, uint8_t y)
{
	OLED_I2C_WR_Byte((uint8_t)(0xB0U + y), OLED_I2C_CMD);
	OLED_I2C_WR_Byte((uint8_t)(((x & 0xF0U) >> 4) | 0x10U), OLED_I2C_CMD);
	OLED_I2C_WR_Byte((uint8_t)(x & 0x0FU), OLED_I2C_CMD);
}

/* 打开OLED显示。 */
void OLED_I2C_Display_On(void)
{
	OLED_I2C_WR_Byte(0x8D, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0x14, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0xAF, OLED_I2C_CMD);
}

/* 关闭OLED显示。 */
void OLED_I2C_Display_Off(void)
{
	OLED_I2C_WR_Byte(0x8D, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0x10, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0xAE, OLED_I2C_CMD);
}

/* 清空OLED屏幕内容。 */
void OLED_I2C_Clear(void)
{
	uint8_t page;
	uint8_t column;

	for(page = 0; page < 8U; page++)
	{
		OLED_I2C_WR_Byte((uint8_t)(0xB0U + page), OLED_I2C_CMD);
		OLED_I2C_WR_Byte(0x00, OLED_I2C_CMD);
		OLED_I2C_WR_Byte(0x10, OLED_I2C_CMD);
		for(column = 0; column < 128U; column++)
		{
			OLED_I2C_WR_Byte(0x00, OLED_I2C_DATA);
		}
	}
}

/* 在OLED指定位置显示一个ASCII字符。 */
void OLED_I2C_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t sizey)
{
	uint8_t c;
	uint8_t sizex;
	uint16_t i;
	uint16_t size1;

	if(chr < ' ' || chr > '~')
	{
		chr = ' ';
	}

	c = (uint8_t)(chr - ' ');
	sizex = (uint8_t)(sizey / 2U);
	if(sizey == 8U)
	{
		size1 = 6U;
	}
	else
	{
		size1 = (uint16_t)((sizey / 8U + ((sizey % 8U) ? 1U : 0U)) * (sizey / 2U));
	}

	OLED_I2C_Set_Pos(x, y);
	for(i = 0; i < size1; i++)
	{
		if((i % sizex) == 0U && sizey != 8U)
		{
			OLED_I2C_Set_Pos(x, y++);
		}
		if(sizey == 8U)
		{
			OLED_I2C_WR_Byte(asc2_0806[c][i], OLED_I2C_DATA);
		}
		else if(sizey == 16U)
		{
			OLED_I2C_WR_Byte(asc2_1608[c][i], OLED_I2C_DATA);
		}
		else
		{
			return;
		}
	}
}

/* 计算一个无符号整数的幂值。 */
uint32_t OLED_I2C_Pow(uint8_t m, uint8_t n)
{
	uint32_t result = 1U;

	while(n--)
	{
		result *= m;
	}
	return result;
}

/* 在OLED指定位置显示一个无符号整数。 */
void OLED_I2C_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t sizey)
{
	uint8_t t;
	uint8_t temp;
	uint8_t m = 0U;
	uint8_t enshow = 0U;

	if(sizey == 8U)
	{
		m = 2U;
	}
	for(t = 0; t < len; t++)
	{
		temp = (uint8_t)((num / OLED_I2C_Pow(10U, (uint8_t)(len - t - 1U))) % 10U);
		if(enshow == 0U && t < (len - 1U))
		{
			if(temp == 0U)
			{
				OLED_I2C_ShowChar((uint8_t)(x + (sizey / 2U + m) * t), y, ' ', sizey);
				continue;
			}
			enshow = 1U;
		}
		OLED_I2C_ShowChar((uint8_t)(x + (sizey / 2U + m) * t), y, (uint8_t)(temp + '0'), sizey);
	}
}

/* 在OLED指定位置显示字符串。 */
void OLED_I2C_ShowString(uint8_t x, uint8_t y, const uint8_t *chr, uint8_t sizey)
{
	uint8_t index = 0U;

	if(chr == NULL)
	{
		return;
	}
	while(chr[index] != '\0')
	{
		OLED_I2C_ShowChar(x, y, chr[index++], sizey);
		if(sizey == 8U)
		{
			x = (uint8_t)(x + 6U);
		}
		else
		{
			x = (uint8_t)(x + sizey / 2U);
		}
	}
}

/* 显示带符号整数并自动添加正负号。 */
static void OLED_I2C_ShowSignedNum(uint8_t x, uint8_t y, int32_t value, uint8_t len)
{
	if(value < 0)
	{
		OLED_I2C_ShowChar(x, y, '-', OLED_I2C_FONT_8);
		value = -value;
	}
	else
	{
		OLED_I2C_ShowChar(x, y, '+', OLED_I2C_FONT_8);
	}
	OLED_I2C_ShowNum((uint8_t)(x + 6U), y, (uint32_t)value, len, OLED_I2C_FONT_8);
}

/* 用空格清除OLED上的指定字符区域。 */
static void OLED_I2C_ClearField(uint8_t x, uint8_t y, uint8_t chars)
{
	uint8_t index;

	for(index = 0; index < chars; index++)
	{
		OLED_I2C_ShowChar((uint8_t)(x + index * 6U), y, ' ', OLED_I2C_FONT_8);
	}
}

/* 将一个字节按二进制形式显示到OLED上。 */
static void OLED_I2C_ShowDigitalByte(uint8_t x, uint8_t y, uint8_t value)
{
	uint8_t index;
	uint8_t bit;

	for(index = 0; index < 8U; index++)
	{
		bit = (uint8_t)((value >> index) & 0x01U);
		OLED_I2C_ShowChar((uint8_t)(x + index * 6U), y, (uint8_t)('0' + bit), OLED_I2C_FONT_8);
	}
}

/* 绘制传感器数据显示界面的固定标签。 */
void OLED_I2C_DrawSensorLabels(void)
{
	OLED_I2C_ShowString(0, 0, (const uint8_t *)"Y:", OLED_I2C_FONT_8);
	OLED_I2C_ShowString(60, 0, (const uint8_t *)"GZ:", OLED_I2C_FONT_8);
	OLED_I2C_ShowString(0, 1, (const uint8_t *)"D:", OLED_I2C_FONT_8);
	OLED_I2C_ShowString(0, 2, (const uint8_t *)"STATE:", OLED_I2C_FONT_8);
	OLED_I2C_ShowString(0, 6, (const uint8_t *)"N0:", OLED_I2C_FONT_8);
	OLED_I2C_ShowString(60, 6, (const uint8_t *)"N7:", OLED_I2C_FONT_8);
}

/* 更新OLED上传感器数据界面的动态数值。 */
void OLED_I2C_UpdateSensorValues(int32_t yaw, int32_t gyro_z,
	uint8_t gray_digital, const char *state,
	const unsigned short gray_normal[8])
{
	OLED_I2C_ClearField(12, 0, 4);
	OLED_I2C_ShowSignedNum(12, 0, yaw, 3);

	OLED_I2C_ClearField(78, 0, 4);
	OLED_I2C_ShowSignedNum(78, 0, gyro_z, 3);

	OLED_I2C_ClearField(12, 1, 8);
	OLED_I2C_ShowDigitalByte(12, 1, gray_digital);

	OLED_I2C_ClearField(42, 2, 10);
	OLED_I2C_ShowString(42, 2, (const uint8_t *)state, OLED_I2C_FONT_8);

	if(gray_normal != NULL)
	{
		OLED_I2C_ClearField(18, 6, 4);
		OLED_I2C_ShowNum(18, 6, gray_normal[0], 4, OLED_I2C_FONT_8);
		OLED_I2C_ClearField(78, 6, 4);
		OLED_I2C_ShowNum(78, 6, gray_normal[7], 4, OLED_I2C_FONT_8);
	}
}

/* 在OLED指定位置显示字库中的汉字。 */
void OLED_I2C_ShowChinese(uint8_t x, uint8_t y, uint8_t no, uint8_t sizey)
{
	uint16_t i;
	uint16_t size1;

	size1 = (uint16_t)((sizey / 8U + ((sizey % 8U) ? 1U : 0U)) * sizey);
	for(i = 0; i < size1; i++)
	{
		if((i % sizey) == 0U)
		{
			OLED_I2C_Set_Pos(x, y++);
		}
		if(sizey == 16U)
		{
			OLED_I2C_WR_Byte(Hzk[no][i], OLED_I2C_DATA);
		}
		else
		{
			return;
		}
	}
}

/* 在OLED指定区域显示位图。 */
void OLED_I2C_DrawBMP(uint8_t x, uint8_t y, uint8_t sizex, uint8_t sizey, const uint8_t BMP[])
{
	uint16_t j = 0U;
	uint8_t i;
	uint8_t m;
	uint8_t page_count;

	if(BMP == NULL)
	{
		return;
	}
	page_count = (uint8_t)(sizey / 8U + ((sizey % 8U) ? 1U : 0U));
	for(i = 0; i < page_count; i++)
	{
		OLED_I2C_Set_Pos(x, (uint8_t)(i + y));
		for(m = 0; m < sizex; m++)
		{
			OLED_I2C_WR_Byte(BMP[j++], OLED_I2C_DATA);
		}
	}
}

/* 初始化STM32F1的I2C接口并启动SSD1306 OLED屏幕。 */
void OLED_I2C_Init(void)
{
	OLED_I2C_Bus_Init();
	if(GPIO_ReadInputDataBit(OLED_I2C_GPIO_PORT, OLED_I2C_SDA_PIN) == Bit_RESET)
	{
		OLED_I2C_SDA_Unlock();
	}

	delay_ms(200);
	OLED_I2C_WR_Byte(0xAE, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0x00, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0x10, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0x40, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0x81, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0xCF, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0xA1, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0xC8, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0xA6, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0xA8, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0x3F, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0xD3, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0x00, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0xD5, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0x80, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0xD9, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0xF1, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0xDA, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0x12, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0xDB, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0x40, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0x20, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0x02, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0x8D, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0x14, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0xA4, OLED_I2C_CMD);
	OLED_I2C_WR_Byte(0xA6, OLED_I2C_CMD);
	OLED_I2C_Clear();
	OLED_I2C_WR_Byte(0xAF, OLED_I2C_CMD);
}
