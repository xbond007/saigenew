#include "uart.h"
#include "cw32f003.h"
#include "cw32f003_uart.h"
#include "cw32f003_gpio.h"
#include "common.h" // 包含 EN485_TX_PIN / PORT 等定义
#include "cw32f003_rcc.h"

/**
 * @brief 初始化 UART1 发送功能（波特率 9600，仅发送）
 *        使用 PA2 作为 TX（引脚复用需另外设置）
 */
void uart1_init(void)
{
    USART_InitTypeDef uart_cfg;

    // 初始化结构体为默认值
    USART_StructInit(&uart_cfg);

    // 设置 UART 配置参数
    uart_cfg.USART_BaudRate            = 9600;              // 设置波特率为 9600bps
    uart_cfg.USART_Over                = USART_Over_16;     // 16倍过采样
    uart_cfg.USART_Source              = USART_Source_PCLK; // 使用外设时钟源（PCLK）
    uart_cfg.USART_UclkFreq            = SystemCoreClock;   // 时钟频率 = 主系统时钟
    uart_cfg.USART_StartBit            = USART_StartBit_FE; // 起始位识别：帧起始下降沿
    uart_cfg.USART_StopBits            = USART_StopBits_1;  // 1个停止位
    uart_cfg.USART_Parity              = USART_Parity_No;   // 无校验位
    uart_cfg.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    uart_cfg.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件流控

    // 开启 UART1 时钟
    __RCC_UART1_CLK_ENABLE();

    // 初始化 UART1 外设
    USART_Init(CW_UART1, &uart_cfg);
}

void uart1_send485(uint8_t *data, uint8_t len)
{
    // 开启485发送
    GPIO_WritePin(EN485_RX_PORT, EN485_RX_PIN, GPIO_Pin_SET);
    uint8_t i;
    for (i = 0; i < len; i++) {
        while (USART_GetFlagStatus(CW_UART1, USART_FLAG_TXE) == RESET); // 等待TXE空
        USART_SendData_8bit(CW_UART1, data[i]);
    }

    // 等待发送完成
    while (USART_GetFlagStatus(CW_UART1, USART_FLAG_TC) == RESET);

    // 关闭485发送
    GPIO_WritePin(EN485_TX_PORT, EN485_TX_PIN, GPIO_Pin_RESET);
}

// uint8_t test_cmd[] = {0x08, 0x61, 0x00, 0x00, 0x02, 0x00, 0x1F, 0x00, 0x00, 0xE4, 0x02, 0x92};

void send_test_command(void)
{
    /*     uint8_t test_cmd[] = {
            0x06, 0x10, 0xB1, 0x01,
            0x00, 0x01, 0x02,
            0x00, 0x01,
            0x00, 0x00 // CRC占位
        };
        uint16_t crc = modbus_crc16(test_cmd, 9); // 计算前9字节
        test_cmd[9]       = crc & 0xFF;           // CRC低字节
        test_cmd[10]      = (crc >> 8);           // CRC高字节

        uart1_send485(test_cmd, sizeof(test_cmd)); */
    // 设置总里程为 100（0x00000064）单位0.1km，即10.0km
    uint8_t total_mileage_cmd[] = {
        0x06,       // 仪表地址
        0x10,       // 写多个寄存器
        0xB1, 0x03, // 起始寄存器地址
        0x00, 0x02, // 写2个寄存器（4字节）
        0x04,       // 字节数
        0x00, 0x00, // 数据高字
        0x00, 0x64, // 数据低字 100 
        0x00, 0x00  // CRC16 占位
    };

    uint16_t crc          = modbus_crc16(total_mileage_cmd, 11);
    total_mileage_cmd[11] = crc & 0xFF;
    total_mileage_cmd[12] = (crc >> 8);
    uart1_send485(total_mileage_cmd, sizeof(total_mileage_cmd));
   
}

uint16_t modbus_crc16(const uint8_t *data, uint8_t len)
{
    uint16_t crc = 0xFFFF;
    uint8_t i, j;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}
