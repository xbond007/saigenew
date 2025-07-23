#ifndef COMMON_H
#define COMMON_H

#include "cw32f003.h"
#include "cw32f003_gpio.h"
#define LIGHT_PORT    CW_GPIOC
#define LIGHT_PIN     GPIO_PIN_0

#define ABS_PORT      CW_GPIOB
#define ABS_PIN       GPIO_PIN_4

#define ONEWIRE_PORT  CW_GPIOB
#define ONEWIRE_PIN   GPIO_PIN_2

#define NFC_PORT      CW_GPIOB
#define NFC_PIN       GPIO_PIN_5

#define ACC_DET_PORT  CW_GPIOA
#define ACC_DET_PIN   GPIO_PIN_7

#define ACC_OUT_PORT  CW_GPIOC
#define ACC_OUT_PIN   GPIO_PIN_1

#define EN485_TX_PORT CW_GPIOA
#define EN485_TX_PIN  GPIO_PIN_1

#define EN485_RX_PORT CW_GPIOB
#define EN485_RX_PIN  GPIO_PIN_3

#define EN485_RE_PORT CW_GPIOA
#define EN485_RE_PIN  GPIO_PIN_4





// ADC相关定义
#define ADC_CHANNEL_ACC_DET    ADC_ExInputCH4  // PA7对应ADC通道4
#define ADC_SAMPLE_TIME        ADC_SampTime10Clk  // 10个时钟周期采样时间
#define ADC_VOLTAGE_THRESHOLD  700  // 电压阈值（mV），根据实际情况调整

// ADC相关函数声明
void adc_init(void);
uint16_t adc_read_acc_det(void);
uint16_t adc_to_voltage_mv(uint16_t adc_value);
uint8_t acc_det_check_voltage(void);


void gpio_init(void);
void light_task(void);
void abs_task(void);
void onewire_task(void);
void nfc_task(void);
void acc_task(void);
void user_tasks_50us(void);
#endif
