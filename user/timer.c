// timer.c - 基于SysTick的定时与延时功能实现
// 提供毫秒计时、延时等功能
#include "timer.h"
#include "cw32f003.h"
#include "cw32f003_btim.h"
#include "cw32f003_rcc.h"
void systick_init(void)
{
    // 系统时钟为uint32_t SystemCoreClock = 8000000; 8000000 / 20000 = 400 /  8000000 ~= 50 us
    SysTick_Config(SystemCoreClock / 20000);
}

void BTIM3_Init_100us(void)
{
    BTIM_TimeBaseInitTypeDef btim;

    __RCC_BTIM_CLK_ENABLE(); // 开启 BTIM 时钟

    BTIM_TimeBaseStructInit(&btim);
    btim.BTIM_Prescaler = BTIM_PRS_DIV16; // 8MHz / 16 = 500kHz
    btim.BTIM_Mode      = BTIM_Mode_TIMER;
    btim.BTIM_Period    = 125;  //125 × 2μs = 250μs = 0.25ms
    btim.BTIM_OPMode    = BTIM_OPMode_Repetitive;

    BTIM_TimeBaseInit(CW_BTIM3, &btim);
    NVIC_SetPriority(BTIM3_IRQn, 0); // 更高优先级
    BTIM_ITConfig(CW_BTIM3, BTIM_IT_OV, ENABLE);
    NVIC_EnableIRQ(BTIM3_IRQn);      // 开启中断
    BTIM_Cmd(CW_BTIM3, ENABLE);      // 启动定时器
}

void BTIM2_Init_20ms(void)
{
    BTIM_TimeBaseInitTypeDef btim;
    __RCC_BTIM_CLK_ENABLE();
    BTIM_TimeBaseStructInit(&btim);
    btim.BTIM_Prescaler = BTIM_PRS_DIV16; // 8MHz / 16 = 500kHz
    btim.BTIM_Mode      = BTIM_Mode_TIMER;
    btim.BTIM_Period    = 10000; 
    btim.BTIM_OPMode    = BTIM_OPMode_Repetitive;
    BTIM_TimeBaseInit(CW_BTIM2, &btim);
    BTIM_ITConfig(CW_BTIM2, BTIM_IT_OV, ENABLE);
    
    NVIC_SetPriority(BTIM3_IRQn, 1); // 更高优先级
    NVIC_EnableIRQ(BTIM2_IRQn); // 开启中断
    BTIM_Cmd(CW_BTIM2, ENABLE); // 启动定时器
}
