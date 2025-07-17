#ifndef __UART_H
#define __UART_H

#include <stdint.h>

// 初始化串口 + 控制IO
void uart1_init(void);

void uart1_send485(uint8_t *data, uint8_t len);

void send_test_command(void);

#endif
