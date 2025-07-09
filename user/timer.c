#include "timer.h"

static volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void)
{
    ms_ticks++;
}

void timer_init(void)
{
    SysTick_Config(SystemCoreClock / 1000);
}

uint32_t timer_ms(void)
{
    return ms_ticks;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = timer_ms();
    while ((timer_ms() - start) < ms) {
    }
}
