#ifndef TIMER_C
#define TIMER_C
#include "cw32f003.h"
#include <stdint.h>
#define TASK_10MS_TIME (10)
#define TASK_50MS_TIME (50)
#define TASK_100MS_TIME (100)
#define TASK_500MS_TIME (500)
#define TASK_1S_TIME (1000)
#define clock_time_t    unsigned int

#define CLOCK_S         (clock_time_t)(10000)
#define CLOCK_MS        (clock_time_t)(1)

uint32_t clock_time(void);
void Clock_Time_Add(void);
/**
 * A timer.
 *
 * This structure is used for declaring a timer. The timer must be set
 * with timer_set() before it can be used.
 *
 * \hideinitializer
 */
struct timer 
{
	clock_time_t start;
	clock_time_t interval;
};
typedef struct
{
    struct timer task_10ms;
    struct timer task_50ms;
    struct timer task_100ms;
    struct timer task_1s;
    struct timer task_500ms;
} SystemTimer;
extern SystemTimer system_timer;
void timerinit(void);
void timer_set(struct timer *t, clock_time_t interval);
void timer_reset(struct timer *t);
void timer_restart(struct timer *t);
char timer_expired(struct timer *t);
clock_time_t timer_GetCount(struct timer *t);
void system_timer_init(void);
#endif