// common.c - 通用功能实现文件
// 包含GPIO初始化、1-Wire通信、灯光、ABS、NFC、ACC等任务逻辑
#include "common.h"
#include "cw32f003_rcc.h"
#include "timer.h"
#include "debug.h"
#include "SEGGER_RTT.h"
#include <string.h>
// #include "onewire.h"
// #include "cw32f003_uart.h"
#include "cw32f003_btim.h"
#include "cw32f003_adc.h"
static uint16_t tick_50us_counter     = 0; // 设置软件计数调度
static volatile uint8_t acc_state     = 0; // 0=关，1=开 开机状态
static volatile uint8_t acc_adc_state = 0; // 0=关，1=开 开机状态
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
    GPIO_WritePin(LIGHT_PORT,LIGHT_PIN,GPIO_Pin_RESET);

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
    GPIO_WritePin(ACC_OUT_PORT, ACC_OUT_PIN, GPIO_Pin_RESET);

    // 5. 485发送使能（输出）
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Pins = EN485_TX_PIN;
    GPIO_Init(EN485_TX_PORT, &init);

    // 6. 485接收使能（输出）
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Pins = EN485_RX_PIN;
    GPIO_Init(EN485_RX_PORT, &init);

    // 7. 485接收使能引脚（RE），默认拉低使能接收
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Pins = EN485_RE_PIN;
    GPIO_Init(EN485_RE_PORT, &init);
    GPIO_WritePin(EN485_RE_PORT, EN485_RE_PIN, GPIO_Pin_RESET); // 默认使能接收

    // 7. NFC引脚（输入）
    init.Mode = GPIO_MODE_INPUT;
    init.Pins = NFC_PIN;
    GPIO_Init(NFC_PORT, &init);
    GPIO_WritePin(NFC_PORT, NFC_PIN, GPIO_Pin_RESET);

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
 * @brief NFC任务，检测NFC引脚低电平，置位pc1_state
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
    static uint8_t led_on        = 0;
    uint8_t acc_voltage_detected = acc_det_check_voltage();
    // SEGGER_RTT_printf(0, "[ACC] ACC_DET: %d\n", acc_voltage_detected);
    if (acc_voltage_detected) {
        // 检测到电压说明NFC已验证通过，直接开机
        acc_state = 1;
        led_on = !led_on; // 翻转状态
        GPIO_WritePin(LIGHT_PORT, LIGHT_PIN, led_on ? GPIO_Pin_SET : GPIO_Pin_RESET);
    } else {
        // 无电压，关机
        led_on    = 0;
        acc_state = 0;
        GPIO_WritePin(LIGHT_PORT, LIGHT_PIN, GPIO_Pin_RESET);
        GPIO_WritePin(ACC_OUT_PORT, ACC_OUT_PIN, GPIO_Pin_RESET); // 关

    }
    // SEGGER_RTT_printf(0, "[ACC] ACC_OUT: %d\n", GPIO_ReadPin(ACC_OUT_PORT, ACC_OUT_PIN));
}

void user_tasks_50us(void)
{

    tick_50us_counter++;
    nfc_task();
    if (tick_50us_counter >= 15000) { // 15000 * 100us = 1.5秒
        tick_50us_counter = 0;
        acc_task();
    }
}

// ... existing code ...

/**
 * @brief 初始化ADC
 * @note 配置ADC采集ACC_DET引脚的电压
 */
void adc_init(void)
{
    ADC_InitTypeDef ADC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    // 使能ADC和GPIO时钟
    __RCC_ADC_CLK_ENABLE();
    __RCC_GPIOA_CLK_ENABLE();

    // 配置PA7为模拟输入
    GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStructure.Pins = ACC_DET_PIN;
    GPIO_Init(ACC_DET_PORT, &GPIO_InitStructure);

    // 配置ADC
    ADC_StructInit(&ADC_InitStructure);
    ADC_InitStructure.ADC_OpMode     = ADC_SingleChOneMode; // 单通道单次转换模式
    ADC_InitStructure.ADC_ClkDiv     = ADC_Clk_Div8;        // ADC时钟分频
    ADC_InitStructure.ADC_SampleTime = ADC_SampTime10Clk;   // 采样时间
    ADC_InitStructure.ADC_VrefSel    = ADC_Vref_VDD;        // 参考电压为VDD
    ADC_InitStructure.ADC_InBufEn    = ADC_BufEnable;       // 使能输入缓冲
    ADC_InitStructure.ADC_TsEn       = ADC_TsDisable;       // 禁用温度传感器
    ADC_InitStructure.ADC_Align      = ADC_AlignRight;      // 右对齐
    ADC_InitStructure.ADC_AccEn      = ADC_AccDisable;      // 禁用累加
    ADC_Init(&ADC_InitStructure);

    // 配置单通道转换
    ADC_SingleChTypeDef ADC_SingleChStruct;
    ADC_SingleChStruct.ADC_Chmux      = ADC_CHANNEL_ACC_DET; // 通道4 (PA7)
    ADC_SingleChStruct.ADC_DiscardEn  = ADC_DiscardNull;     // 不丢弃数据
    ADC_SingleChStruct.ADC_InitStruct = ADC_InitStructure;   // 使用上面的配置
    ADC_SingleChOneModeCfg(&ADC_SingleChStruct);             // 配置单通道单次转换

    // 使能ADC
    ADC_Enable();
}

/**
 * @brief 读取ADC值
 * @return ADC原始值（0-4095）
 */
uint16_t adc_read_acc_det(void)
{
    uint16_t adc_value = 0;

    // 启动ADC转换
    ADC_SoftwareStartConvCmd(ENABLE);

    // 等待转换完成
    while (!ADC_GetITStatus(ADC_IT_EOC));

    // 读取ADC值
    adc_value = ADC_GetConversionValue();

    // 清除中断标志
    ADC_ClearITPendingBit(ADC_IT_EOC);

    return adc_value;
}

/**
 * @brief 将ADC值转换为电压（mV）
 * @param adc_value ADC原始值
 * @return 电压值（mV）
 */
uint16_t adc_to_voltage_mv(uint16_t adc_value)
{
    // 假设VDD为3.3V，12位ADC
    // 电压 = (ADC值 / 4095) * 3300mV
    return (uint16_t)((adc_value * 3300) / 4095);
}

/**
 * @brief 检测ACC_DET引脚电压状态
 * @return 1:有电压, 0:无电压
 */
uint8_t acc_det_check_voltage(void)
{
    uint16_t adc_value  = adc_read_acc_det();
    uint16_t voltage_mv = adc_to_voltage_mv(adc_value);
    // SEGGER_RTT_printf(0, "[ADC] ACC_DET: ADC=%d, Voltage=%dmV\n", adc_value, voltage_mv);
    // 根据电压阈值判断状态
    if (voltage_mv > ADC_VOLTAGE_THRESHOLD) {
        return 1; // 有电压
    } else {
        return 0;                                                 // 无电压
    }
}
