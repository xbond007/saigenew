#include "cw32f003.h"
#include "timer.h"
#include "common.h"

int main(void)
{
    SystemInit();
    gpio_init();
    timer_init();

    while (1) {
        light_task();
        abs_task();
        nfc_task();
        acc_task();
        onewire_task();
    }
}
