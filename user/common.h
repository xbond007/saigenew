#ifndef COMMON_H
#define COMMON_H

#include "cw32f003.h"
#include "cw32f003_gpio.h"
#define LIGHT_PORT      CW_GPIOC
#define LIGHT_PIN       GPIO_PIN_0

#define ABS_PORT        CW_GPIOB
#define ABS_PIN         GPIO_PIN_4

#define ONEWIRE_PORT    CW_GPIOB
#define ONEWIRE_PIN     GPIO_PIN_2

#define NFC_PORT        CW_GPIOB
#define NFC_PIN         GPIO_PIN_5

#define ACC_DET_PORT    CW_GPIOA
#define ACC_DET_PIN     GPIO_PIN_7

#define ACC_OUT_PORT        CW_GPIOC
#define ACC_OUT_PIN         GPIO_PIN_1

#define EN485_TX_PORT   CW_GPIOA
#define EN485_TX_PIN    GPIO_PIN_1

#define EN485_RX_PORT   CW_GPIOB
#define EN485_RX_PIN    GPIO_PIN_3

void gpio_init(void);
void light_task(void);
void abs_task(void);
void onewire_task(void);
void nfc_task(void);
void acc_task(void);

#endif
