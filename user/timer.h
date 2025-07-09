#ifndef TIMER_H
#define TIMER_H
#include "cw32f003.h"

void timer_init(void);
uint32_t timer_ms(void);
void delay_ms(uint32_t ms);
#endif
