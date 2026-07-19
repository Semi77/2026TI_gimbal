#ifndef __VOFA_FIREWATER_H
#define __VOFA_FIREWATER_H

#include "stm32f10x.h"

/* 初始化USART3作为VOFA++ FireWater数据发送串口。 */
void VOFA_FireWater_Init(u32 bound);

/* 通过USART3向VOFA++发送一个字符。 */
void VOFA_FireWater_Send_Char(char data);

/* 通过USART3向VOFA++发送一个字符串。 */
void VOFA_FireWater_Send_String(const char *str);

/* 通过USART3向VOFA++发送一个有符号整数。 */
void VOFA_FireWater_Send_S32(s32 value);

/* 按FireWater格式向VOFA++发送两个通道数据。 */
void VOFA_FireWater_Send_2S32(const char *prefix, s32 ch0, s32 ch1);

/* 按FireWater格式向VOFA++发送四个通道数据。 */
void VOFA_FireWater_Send_4S32(const char *prefix, s32 ch0, s32 ch1, s32 ch2, s32 ch3);

/* 按FireWater格式向VOFA++发送六个通道数据。 */
void VOFA_FireWater_Send_6S32(const char *prefix, s32 ch0, s32 ch1, s32 ch2, s32 ch3, s32 ch4, s32 ch5);

/* 按FireWater格式向VOFA++发送PID调试数据。 */
void VOFA_FireWater_Send_PID(s32 error_x, s32 error_y, s32 target_x, s32 target_y, s32 delta_x, s32 delta_y);

#endif
