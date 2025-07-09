#ifndef COMMON_H
#define COMMON_H
#include "cw32f003.h"
#include "stdbool.h"
#include "SEGGER_RTT.h"
#include "cw32f003_gpio.h"
#include "type.h"
typedef enum {
    lock = 1,
    unlock,
} LOCK_T;
typedef struct
{
    uint8_t filename[20];
    uint8_t filelenbuf[10];
    uint32_t FileLen;
    uint8_t transmission_start;
    uint8_t end_flag;
    uint32_t offset;
} FILE_T;

// 灯光输出检测e
#define OUT_LIGHT CW_GPIOC->BSRR = GPIO_PIN_0
// ABS 输出检测
#define OUT_ABS CW_GPIOB->BSRR = GPIO_PIN_4

// 一线通输出检测
#define OUT_YXT CW_GPIOB->BSRR = GPIO_PIN_2

// NFC输入检测
#define IN_NFC ((CW_GPIOB->IDR & GPIO_PIN_5) == 0)

// 485 控制使能
#define EN485_TXD CW_GPIOA->BSRR = GPIO_PIN_8
#define EN485_RXD CW_GPIOA->BRR = GPIO_PIN_8

// 电门检测 输出
#define IN_ACC_JC ((CW_GPIOA->IDR & GPIO_PIN_4) != 0)
#define OUT_ACC   CW_GPIOC->BSRR = GPIO_PIN_4

bool time_delay_check(uint8_t *lock, uint16_t *time_base, uint16_t deley_time);
void usartInit(void);
void send_c(void);
uint8_t *alm_send(uint8_t len);
void yxt_data_send(void);
void DMA_INIT(void);
void print_log(const char *sFormat, ...);
void com_task_50us(void);
void recv_dispose(void);
void send_order(void);
extern FILE_T file;
extern uint8_t recv_over, time_out, NFC_read;
#endif