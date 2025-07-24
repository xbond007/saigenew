#include "SEGGER_RTT.h"
#include "cw32f003.h"
#include "timer.h"
#include "common.h"
#include "onewire.h"
#include "cw32f003_btim.h"
#include "uart.h"
#include "cw32f003_uart.h"


extern uint8_t uart1_busy;     // 发送中标志位

int main(void)
{
    SystemInit();
    systick_init();
    gpio_init();
    adc_init(); // 添加ADC初始化
    uart1_init();
    BTIM3_Init_100us(); // 100us
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
        onewire_fixed_task_singleframe(); // 固定发送
    }
}



void BTIM2_IRQHandler(void)
{
    if (BTIM_GetITStatus(CW_BTIM2, BTIM_IT_OV) == SET) {
        BTIM_ClearITPendingBit(CW_BTIM2, BTIM_IT_OV);

        static uint16_t uart_counter = 0;
        if (++uart_counter > 3) { // 每20ms × 20 = 400ms发送一次
            uart_counter = 0;
            if (!uart1_busy) {
                send_gear_p_command();
            }
        }
        
    }
}