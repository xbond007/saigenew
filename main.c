#include "cw32f003.h"
#include "cw32f003_rcc.h"
#include "debug.h"
#include "timer.h"
#include "common.h"

int main(void)
{
    SystemInit();
    Delay_Init();
    gpio_init();
    timer_init();

    while (1) {
        light_task();
        abs_task();
        nfc_task();
        acc_task();
        onewire_task();
    }}
