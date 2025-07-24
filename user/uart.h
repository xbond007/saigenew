#ifndef __UART_H
#define __UART_H

#include <stdint.h>

// 初始化串口 + 控制IO
void uart1_init(void);
void uart1_send485_irq(uint8_t *data, uint8_t len);
// 数据分析函数
void send_gear_p_command(void);
#endif


