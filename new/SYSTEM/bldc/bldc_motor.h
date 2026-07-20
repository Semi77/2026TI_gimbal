#ifndef __BLDC_MOTOR_H
#define __BLDC_MOTOR_H

#include "sys.h"

#define BLDC_FRAME_HEADER      0x7A
#define BLDC_FRAME_TAIL        0x7B

#define BLDC_ADDR_X            0x01
#define BLDC_ADDR_Y            0x02

#define BLDC_CMD_MODE          0x00
#define BLDC_CMD_SPEED         0x01
#define BLDC_CMD_MULTI_POS     0x02
#define BLDC_CMD_SINGLE_POS    0x03
#define BLDC_CMD_DISABLE       0x05
#define BLDC_CMD_ENABLE        0x06
#define BLDC_CMD_ACC           0x07
#define BLDC_CMD_FEEDBACK      0x0E

#define BLDC_MODE_SPEED        0x0000
#define BLDC_MODE_MULTI_POS    0x0001
#define BLDC_MODE_SINGLE_POS   0x0002
#define BLDC_MODE_MULTI_DIRECT 0x0003
#define BLDC_MODE_SINGLE_DIRECT 0x0004

#define BLDC_FB_SPEED          0x00
#define BLDC_FB_MULTI_ANGLE    0x01
#define BLDC_FB_SINGLE_ANGLE   0x02
#define BLDC_FB_ACC            0x03
#define BLDC_FB_VOLTAGE        0x04

typedef struct
{
	s16 speed;
	s32 multi_angle;
	u16 single_angle;
	s16 acc;
	u16 voltage;
	u8 data_ready;
} BLDC_MotorData_t;

extern volatile BLDC_MotorData_t BLDC_MotorX;
extern volatile BLDC_MotorData_t BLDC_MotorY;

u8 BLDC_Calc_BCC(const u8 *data, u8 len);
void BLDC_SendCmd(u8 addr, u8 cmd, const u8 *data, u8 len);
void BLDC_Enable(u8 addr);
void BLDC_Disable(u8 addr);
void BLDC_SetSpeed(u8 addr, s16 rpm);
void BLDC_SetMode(u8 addr, u16 mode);
void BLDC_SetAcc(u8 addr, u16 acc);
void BLDC_SetMultiAngle(u8 addr, s32 angle_x10);
void BLDC_SetSingleAngle(u8 addr, u16 angle_x10);
void BLDC_ReqFeedback(u8 addr, u8 type);
void BLDC_ParseRxData(u8 rx_byte);

#endif
