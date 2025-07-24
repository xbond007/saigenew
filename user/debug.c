// debug.c - 延时相关函数实现
// 提供微秒和毫秒级延时功能，基于SysTick定时器
#include "common.h"
#include "debug.h"

// 微秒和毫秒延时系数
static u8 p_us  = 0;
static u16 p_ms = 0;

/*********************************************************************
 * @fn      Delay_Us
 * @brief   微秒级延时函数
 * @param   n - 延时的微秒数
 * @return  无
 * @note    通过配置SysTick定时器实现精确延时
 *********************************************************************/
void Delay_Us(u32 n)
{
    u32 i;

    SysTick->LOAD = n * p_us;         // 设置重装载值，决定延时时间
    SysTick->VAL  = 0x00;             // 清空当前计数值
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; // 使能SysTick

    do {
        i = SysTick->CTRL;            // 读取SysTick控制寄存器
    } while ((i & 0x01) && !(i & (1 << 16))); // 等待计数到0

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; // 关闭SysTick
    SysTick->VAL = 0X00;              // 清空当前计数值
}

/*********************************************************************
 * @fn      Delay_Ms
 * @brief   毫秒级延时函数
 * @param   n - 延时的毫秒数
 * @return  无
 * @note    通过配置SysTick定时器实现精确延时
 *********************************************************************/
void Delay_Ms(u16 n)
{
    u32 i;

    SysTick->LOAD = (u32)n * p_ms;    // 设置重装载值，决定延时时间
    SysTick->VAL  = 0x00;             // 清空当前计数值
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; // 使能SysTick

    do {
        i = SysTick->CTRL;            // 读取SysTick控制寄存器
    } while ((i & 0x01) && !(i & (1 << 16))); // 等待计数到0

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; // 关闭SysTick
    SysTick->VAL = 0X00;              // 清空当前计数值
}
