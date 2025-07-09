#include "timer.h"
#include "common.h"
uint32_t timetimes;
extern uint8_t rx_nub, time_out;
SystemTimer system_timer;

void timerinit()
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_TimeBaseInitTypeDef TimStructInit;
    NVIC_InitTypeDef TIMStrctInit;
    TimStructInit.TIM_ClockDivision = 0;
    TimStructInit.TIM_Prescaler     = 72;
    TimStructInit.TIM_Period        = 50;
    TimStructInit.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TimStructInit);
    TIM_Cmd(TIM3, ENABLE);
    TIMStrctInit.NVIC_IRQChannel                   = TIM3_IRQn;
    TIMStrctInit.NVIC_IRQChannelSubPriority        = 1;
    TIMStrctInit.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_Init(&TIMStrctInit);
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
}
volatile uint32_t SystemTicks   = 0; // 定时器计数器
volatile uint32_t SystemTickBak = 0;
/*******************************************************
 * Function name : clock_time
 * Description   : 获取当前时钟计数器
 * Parameter     : void
 * Return        : void
 ********************************************************/
uint32_t clock_time(void)
{
    // uint32_t cpu_stat = enter_critical();
    SystemTickBak = SystemTicks;
    // exit_critical(cpu_stat);
    return SystemTickBak;
}

/*******************************************************
 * Function name : Clock_Time_Add
 * Description   : 计数器递增
 * Parameter     : void
 * Return        : void
 ********************************************************/
void Clock_Time_Add(void)
{
    // uint32_t cpu_stat = enter_critical();
    SystemTicks++;
    //	exit_critical(cpu_stat);
}
/*---------------------------------------------------------------------------*/
/**
 * Set a timer.
 *
 * This function is used to set a timer for a time sometime in the
 * future. The function timer_expired() will evaluate to true after
 * the timer has expired.
 *
 * \param t A pointer to the timer
 * \param interval The interval before the timer expires.
 *
 */
void timer_set(struct timer *t, clock_time_t interval)
{
    t->interval = (clock_time_t)interval;
    t->start    = clock_time();
}
/*---------------------------------------------------------------------------*/
/**
 * Reset the timer with the same interval.
 *
 * This function resets the timer with the same interval that was
 * given to the timer_set() function. The start point of the interval
 * is the exact time that the timer last expired. Therefore, this
 * function will cause the timer to be stable over time, unlike the
 * timer_rester() function.
 *
 * \param t A pointer to the timer.
 *
 * \sa timer_restart()
 */
void timer_reset(struct timer *t)
{
    t->start += t->interval;
}
/*---------------------------------------------------------------------------*/
/**
 * Restart the timer from the current point in time
 *
 * This function restarts a timer with the same interval that was
 * given to the timer_set() function. The timer will start at the
 * current time.
 *
 * \note A periodic timer will drift if this function is used to reset
 * it. For preioric timers, use the timer_reset() function instead.
 *
 * \param t A pointer to the timer.
 *
 * \sa timer_reset()
 */
void timer_restart(struct timer *t)
{
    t->start = clock_time();
}
/*---------------------------------------------------------------------------*/
/**
 * Check if a timer has expired.
 *
 * This function tests if a timer has expired and returns true or
 * false depending on its status.
 *
 * \param t A pointer to the timer
 *
 * \return Non-zero if the timer has expired, zero otherwise.
 *
 */
char timer_expired(struct timer *t)
{
    return ((clock_time_t)(clock_time() - t->start) >= (clock_time_t)t->interval) ? 1 : 0;
}
/*---------------------------------------------------------------------------*/
/**
 * @brief Gets a time from the timer setting time.
 * @param t A pointer to the timer
 * @return clock_time_t a time what you want
 */
clock_time_t timer_GetCount(struct timer *t)
{
    return (clock_time_t)(clock_time() - t->start);
}
void system_timer_init(void)
{
    timer_set(&system_timer.task_10ms, CLOCK_MS * TASK_10MS_TIME);
    timer_set(&system_timer.task_50ms, CLOCK_MS * TASK_50MS_TIME);
    timer_set(&system_timer.task_100ms, CLOCK_MS * TASK_100MS_TIME);
    timer_set(&system_timer.task_1s, CLOCK_MS * TASK_1S_TIME);
    timer_set(&system_timer.task_500ms, CLOCK_MS * TASK_500MS_TIME);
}
u8 *yxt = 0;
void TIM3_IRQHandler() // 50um
{
    if ((TIM_GetITStatus(TIM3, TIM_IT_Update)) != RESET) {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        timetimes++;
        yxt = alm_send(12);
        com_task_50us();
        if (timetimes > 20) {
            timetimes = 0;
            Clock_Time_Add();
        }
    }
}
