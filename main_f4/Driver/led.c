#include "led.h"

#include <stdint.h>

#define RCC_AHB1ENR    (*(volatile uint32_t *)0x40023830UL)
#define GPIOC_MODER    (*(volatile uint32_t *)0x40020800UL)
#define GPIOC_OTYPER   (*(volatile uint32_t *)0x40020804UL)
#define GPIOC_OSPEEDR  (*(volatile uint32_t *)0x40020808UL)
#define GPIOC_PUPDR    (*(volatile uint32_t *)0x4002080CUL)
#define GPIOC_ODR      (*(volatile uint32_t *)0x40020814UL)
#define GPIOC_BSRR     (*(volatile uint32_t *)0x40020818UL)

#define RCC_GPIOC_CLOCK_ENABLE  (1UL << 2)
#define LED_PIN                 13UL
#define LED_PIN_MASK            (1UL << LED_PIN)

/* 初始化PC13为低速推挽输出并使低电平有效的板载LED默认熄灭。 */
void LED_Init(void)
{
    RCC_AHB1ENR |= RCC_GPIOC_CLOCK_ENABLE;
    (void)RCC_AHB1ENR;

    GPIOC_BSRR = LED_PIN_MASK;
    GPIOC_MODER &= ~(3UL << (LED_PIN * 2UL));
    GPIOC_MODER |=  (1UL << (LED_PIN * 2UL));
    GPIOC_OTYPER &= ~LED_PIN_MASK;
    GPIOC_OSPEEDR &= ~(3UL << (LED_PIN * 2UL));
    GPIOC_PUPDR &= ~(3UL << (LED_PIN * 2UL));
}

/* 将PC13输出低电平以点亮板载LED。 */
void LED_On(void)
{
    GPIOC_BSRR = LED_PIN_MASK << 16UL;
}

/* 将PC13输出高电平以熄灭板载LED。 */
void LED_Off(void)
{
    GPIOC_BSRR = LED_PIN_MASK;
}

/* 根据PC13当前输出状态切换板载LED的亮灭。 */
void LED_Toggle(void)
{
    if ((GPIOC_ODR & LED_PIN_MASK) != 0UL)
    {
        LED_On();
    }
    else
    {
        LED_Off();
    }
}
