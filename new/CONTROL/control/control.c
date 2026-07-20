#include "control.h"
#include "f32c_gimbal.h"

/* 定时器中断用于检测按键并触发双轴云台位置测试动作。 */
void TIM1_UP_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
		if(click() == 1)
		{
			F32C_Gimbal_Add_Target_Position(90 * F32C_GIMBAL_POSITION_UNIT_PER_DEGREE,
											90 * F32C_GIMBAL_POSITION_UNIT_PER_DEGREE);
		}
	}
}

