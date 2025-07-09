#include "common.h"

#include "cw32f003_rcc.h"
#include "timer.h"
#include "debug.h"

static uint8_t pc1_state = 0;

void gpio_init(void)
{
    __RCC_GPIOA_CLK_ENABLE();
    __RCC_GPIOB_CLK_ENABLE();
    __RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef init = {0};

    init.Mode = GPIO_MODE_OUTPUT_PP;

    init.Pins = LIGHT_PIN;
    GPIO_Init(LIGHT_PORT, &init);

    init.Pins = ABS_PIN;
    GPIO_Init(ABS_PORT, &init);

    init.Pins = ONEWIRE_PIN;
    GPIO_Init(ONEWIRE_PORT, &init);

    init.Pins = ACC_DET_PIN;
    GPIO_Init(ACC_OUT_PORT, &init);

    init.Pins = EN485_TX_PIN;
    GPIO_Init(EN485_TX_PORT, &init);

    init.Pins = EN485_RX_PIN;
    GPIO_Init(EN485_RX_PORT, &init);

    init.Mode = GPIO_MODE_INPUT;
    init.Pins = NFC_PIN;
    GPIO_Init(NFC_PORT, &init);

    init.Pins = ACC_DET_PIN;
    GPIO_Init(ACC_DET_PORT, &init);
}

static void onewire_send_bit(uint8_t bit)
{
    GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_Pin_SET);/**/
    if (bit) {
        Delay_Us(60);
        GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_Pin_RESET);
        Delay_Us(30);
    } else {
        Delay_Us(30);
        GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_Pin_RESET);
        Delay_Us(60);
    }
}

static void onewire_send_byte(uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++) {
        onewire_send_bit(byte & 0x01);
        byte >>= 1;
    }
}

void onewire_task(void)
{
    static uint32_t last = 0;
    if (timer_ms() - last >= 1000) {
        last = timer_ms();
        onewire_send_byte(0x55);
    }
}

void light_task(void)
{
    uint32_t t = timer_ms() % 2000;
    if (t < 500 || t >= 1500) {
        GPIO_WritePin(LIGHT_PORT, LIGHT_PIN, GPIO_Pin_RESET);
    } else {
        GPIO_WritePin(LIGHT_PORT, LIGHT_PIN, GPIO_Pin_SET);
    }
}

void abs_task(void)
{
    uint32_t t = timer_ms() % 2000;
    if (t < 500 || t >= 1500) {
        GPIO_WritePin(ABS_PORT, ABS_PIN, GPIO_Pin_RESET);
    } else {
        GPIO_WritePin(ABS_PORT, ABS_PIN, GPIO_Pin_SET);
    }
}

void nfc_task(void)
{
    if (GPIO_ReadPin(NFC_PORT, NFC_PIN) == GPIO_Pin_RESET) {
        pc1_state = 1;
    }
}

void acc_task(void)
{
    if (pc1_state) {
        GPIO_WritePin(ACC_OUT_PORT, ACC_OUT_PIN, GPIO_Pin_SET);
        pc1_state = 0;
    } else {
        if (GPIO_ReadPin(ACC_DET_PORT, ACC_DET_PIN) == GPIO_Pin_SET) {
            GPIO_WritePin(ACC_OUT_PORT, ACC_OUT_PIN, GPIO_Pin_RESET);
        }
    }
}
