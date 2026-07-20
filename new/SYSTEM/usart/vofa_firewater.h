#ifndef __VOFA_FIREWATER_H
#define __VOFA_FIREWATER_H

#include "stm32f10x.h"

void VOFA_FireWater_Init(u32 bound);
void VOFA_FireWater_Send_Char(char data);
void VOFA_FireWater_Send_String(const char *str);
void VOFA_FireWater_Send_S32(s32 value);
void VOFA_FireWater_Send_2S32(const char *prefix, s32 ch0, s32 ch1);
void VOFA_FireWater_Send_4S32(const char *prefix, s32 ch0, s32 ch1, s32 ch2, s32 ch3);
void VOFA_FireWater_Send_6S32(const char *prefix, s32 ch0, s32 ch1, s32 ch2, s32 ch3, s32 ch4, s32 ch5);
void VOFA_FireWater_Send_8S32(const char *prefix, s32 ch0, s32 ch1, s32 ch2, s32 ch3, s32 ch4, s32 ch5, s32 ch6, s32 ch7);
void VOFA_FireWater_Send_PID(s32 error_x, s32 error_y, s32 target_x, s32 target_y, s32 delta_x, s32 delta_y);
void VOFA_FireWater_Send_PID_Debug(s32 vision_x, s32 vision_y, s32 error_x, s32 error_y, s32 delta_x, s32 delta_y, s32 target_x, s32 target_y);

#endif
