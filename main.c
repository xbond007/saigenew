#include "cw32f003.h"
#include "timer.h"
#include "common.h"
#include "onewire.h"
#include "cw32f003_btim.h"
#include "uart.h"
int main(void)
{
    SystemInit();
    gpio_init();
    uart1_init();
    systick_init();
    BTIM3_Init_350ms(); // BTIM3，每350ms中断一次
    BTIM2_Init_20ms();

    while (1) {
        // 
    }
}


void SysTick_Handler(void)
{
    user_tasks_50us();
}


void BTIM3_IRQHandler(void)
{
    if (BTIM_GetITStatus(CW_BTIM3, BTIM_IT_OV) == SET) {
        BTIM_ClearITPendingBit(CW_BTIM3, BTIM_IT_OV);
        onewire_fixed_task(); // 一线通发送
        
    }
}

void BTIM2_IRQHandler(void)
{
    if (BTIM_GetITStatus(CW_BTIM2, BTIM_IT_OV) != RESET) {
        BTIM_ClearITPendingBit(CW_BTIM2, BTIM_IT_OV);
        send_test_command(); //485发送
    }
}
