#include "Driver/led.h"

/* 完成LED初始化后持续运行以保持PC13的点亮状态。 */
int main(void)
{
    LED_Init();
    LED_On();

    while (1)
    {
    }
}
