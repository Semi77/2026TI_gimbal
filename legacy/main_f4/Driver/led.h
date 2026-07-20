#ifndef DRIVER_LED_H
#define DRIVER_LED_H

/* 初始化PC13板载LED并使其默认熄灭。 */
void LED_Init(void);

/* 点亮PC13板载LED。 */
void LED_On(void);

/* 熄灭PC13板载LED。 */
void LED_Off(void);

/* 翻转PC13板载LED的亮灭状态。 */
void LED_Toggle(void);

#endif
