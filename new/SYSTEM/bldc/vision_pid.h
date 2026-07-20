#ifndef __VISION_PID_H
#define __VISION_PID_H

#include "sys.h"

typedef struct
{
	s16 error;
	s16 last_error;
	s16 prev_error;
	s16 kp;
	s16 ki;
	s16 kd;
	s16 dead_zone;
	s16 max_delta;
} VisionPID_t;

extern s32 VisionPID_Target_X;
extern s32 VisionPID_Target_Y;
extern s16 VisionPID_Delta_X;
extern s16 VisionPID_Delta_Y;

void VisionPID_Init(void);
void VisionPID_Reset(void);
u8 VisionPID_Update(void);

#endif
