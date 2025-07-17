// common.c - 通用功能实现文件
// 包含GPIO初始化、1-Wire通信、灯光、ABS、NFC、ACC等任务逻辑
#include "common.h"
#include "cw32f003_rcc.h"
#include "timer.h"
#include "debug.h"
#include "SEGGER_RTT.h"
#include <string.h>
#include "onewire.h"
#include "cw32f003_uart.h"
#include "cw32f003_btim.h"
static uint16_t tick_50us_counter = 0; // 设置软件计数调度
static volatile uint8_t acc_state = 0; // 0=关，1=开 开机状态

/**
 * @brief GP初始化函数
 * 使能GPIOA/B/C时钟，并初始化各外设相关引脚
 */
void gpio_init(void)
{
    __RCC_GPIOA_CLK_ENABLE();
    __RCC_GPIOB_CLK_ENABLE();
    __RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef init = {0};

    // 1. 灯光引脚（输出）
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Pins = LIGHT_PIN;
    GPIO_Init(LIGHT_PORT, &init);

    // 2. ABS引脚（输出）
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Pins = ABS_PIN;
    GPIO_Init(ABS_PORT, &init);

    // 3. 1-Wire引脚（输出）
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Pins = ONEWIRE_PIN;
    GPIO_Init(ONEWIRE_PORT, &init);

    // 4. ACC输出引脚（输出）——注意这里要用ACC_OUT_PIN和ACC_OUT_PORT
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Pins = ACC_OUT_PIN;
    GPIO_Init(ACC_OUT_PORT, &init);

    // 5. 485发送使能（输出）
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Pins = EN485_TX_PIN;
    GPIO_Init(EN485_TX_PORT, &init);

    // 6. 485接收使能（输出）
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Pins = EN485_RX_PIN;
    GPIO_Init(EN485_RX_PORT, &init);

    // 7. NFC引脚（输入）
    init.Mode = GPIO_MODE_INPUT;
    init.Pins = NFC_PIN;
    GPIO_Init(NFC_PORT, &init);

    // 8. ACC检测引脚（输入）
    init.Mode = GPIO_MODE_INPUT_PULLUP;
    init.Pins = ACC_DET_PIN;
    GPIO_Init(ACC_DET_PORT, &init);
}
/**
 * @brief 灯光任务，周期性控制灯光引脚
 */
void light_task(void)
{
    // uint32_t t = timer_ms() % 2000; // 2秒周期
    // if (t < 500 || t >= 1500) {
    //     GPIO_WritePin(LIGHT_PORT, LIGHT_PIN, GPIO_Pin_RESET); // 灯灭
    // } else {
    //     GPIO_WritePin(LIGHT_PORT, LIGHT_PIN, GPIO_Pin_SET); // 灯亮
    // }
}

/**
 * @brief ABS任务，周期性控制ABS引脚
 */
void abs_task(void)
{
    // uint32_t t = timer_ms() % 2000; // 2秒周期
    // if (t < 500 || t >= 1500) {
    //     GPIO_WritePin(ABS_PORT, ABS_PIN, GPIO_Pin_RESET); // ABS灭
    // } else {
    //     GPIO_WritePin(ABS_PORT, ABS_PIN, GPIO_Pin_SET); // ABS亮
    // }
}

/**
 * @brief NFC任务，检测NFC引脚低电平，置位ac_state
 */
void nfc_task(void)
{
    static uint8_t low_cnt   = 0;
    static uint8_t high_cnt  = 0;
    static uint8_t triggered = 0;

    GPIO_PinState pin = GPIO_ReadPin(NFC_PORT, NFC_PIN);

    if (pin == GPIO_Pin_RESET) {
        high_cnt = 0;

        if (!triggered) {
            if (++low_cnt >= 5) {
                triggered = 1;
                low_cnt   = 0;

                acc_state = !acc_state;
                // SEGGER_RTT_printf(0, "NFC run : %d\n", acc_state);

                if (acc_state) {
                    GPIO_WritePin(ACC_OUT_PORT, ACC_OUT_PIN, GPIO_Pin_SET); // 开
                } else {
                    GPIO_WritePin(ACC_OUT_PORT, ACC_OUT_PIN, GPIO_Pin_RESET); // 关
                }
            }
        }
    } else {
        low_cnt = 0;
        if (triggered) {
            if (++high_cnt >= 5) {
                triggered = 0; // 允许下一次刷卡
                high_cnt  = 0;
            }
        }
    }
}

/**
 * @brief ACC任务，根据acc_state和ACC检测引脚控制light闪烁输出
 */
void acc_task(void)
{
    // SEGGER_RTT_printf(0, "[acc_task] Run acc_state = %d\n", acc_state);

    static uint8_t led_on = 0;

    if (acc_state) {
        led_on = !led_on; // 翻转状态
        GPIO_WritePin(LIGHT_PORT, LIGHT_PIN, led_on ? GPIO_Pin_SET : GPIO_Pin_RESET);
        // SEGGER_RTT_printf(0, "Blink LED: %s\n", led_on ? "ON" : "OFF");
    } else {
        led_on = 0;
        GPIO_WritePin(LIGHT_PORT, LIGHT_PIN, GPIO_Pin_RESET);
        // SEGGER_RTT_printf(0, "Run low = \n");
    }
    // GPIO_PinState s1 = GPIO_ReadPin(ACC_DET_PORT, ACC_DET_PIN);
    // SEGGER_RTT_printf(0, "ACC in at boot = %d\n", s1);
}

void user_tasks_50us(void)
{
    tick_50us_counter++;
    nfc_task();
    if (tick_50us_counter >= 30000) {        // 30000 * 50us = 1.5秒
        tick_50us_counter = 0;
        acc_task(); 
    } 
}


