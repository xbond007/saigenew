#include "uart.h"
#include "cw32f003.h"
#include "cw32f003_uart.h"
#include "cw32f003_gpio.h"
#include "common.h"
#include "cw32f003_rcc.h"
#include "SEGGER_RTT.h"
// RS485控制宏定义 - 只有RE控制引脚
// 开启发送模式（RE=1 → 禁止接收，允许发送）
/*
        SEGGER_RTT_printf(0, "[RS485] Set to TX mode (RE=1)\n");
        SEGGER_RTT_printf(0, "[RS485] Set to RX mode (RE=0)\n");
         */
#define RS485_SET_TX()                                            \
    do {                                                          \
        GPIO_WritePin(EN485_RE_PORT, EN485_RE_PIN, GPIO_Pin_SET); \
                                                                  \
    } while (0)

// 开启接收模式（RE=0 → 允许接收，禁止发送）
#define RS485_SET_RX()                                              \
    do {                                                            \
        GPIO_WritePin(EN485_RE_PORT, EN485_RE_PIN, GPIO_Pin_RESET); \
                                                                    \
    } while (0)

// 函数声明
uint16_t modbus_crc16(uint8_t *data, uint8_t len);

// 全局变量定义 - 改为extern以便在main.c中访问
uint8_t *uart1_tx_buf;  // 发送缓冲区指针
uint8_t uart1_tx_len;   // 发送总长度
uint8_t uart1_tx_index; // 当前发送索引
uint8_t uart1_busy = 0; // 发送中标志位，防止重复发送

// 添加全局发送缓冲区
// static uint8_t uart1_tx_buffer[128]; // 全局发送缓冲区

/**
 * @brief 初始化UART1串口
 * @note 配置为9600波特率，8位数据位，1位停止位，无校验位
 */
void uart1_init(void)
{
    USART_InitTypeDef uart_cfg;
    GPIO_InitTypeDef gpio_init;

    // 使能UART2和GPIO时钟（修正：使用UART2）
    __RCC_UART2_CLK_ENABLE();
    __RCC_GPIOA_CLK_ENABLE();
    __RCC_GPIOB_CLK_ENABLE();

    // 配置RS485 RE/DE控制引脚 (PA4) 为推挽输出
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pins = EN485_RE_PIN;
    GPIO_Init(EN485_RE_PORT, &gpio_init);

    // 配置TX引脚（PA1）为UART2_TXD
    PA01_AFx_UART2TXD();    // 复用功能
    PA01_DIGTAL_ENABLE();   // 数字模式
    PA01_DIR_OUTPUT();      // 输出方向
    PA01_PUSHPULL_ENABLE(); // 推挽输出
    PA01_PUR_DISABLE();     // 禁用上拉
    PA01_PDR_DISABLE();     // 禁用下拉

    // 配置RX引脚（PB3）为UART2_RXD
    PB03_AFx_UART2RXD();    // 复用功能
    PB03_DIGTAL_ENABLE();   // 数字模式
    PB03_DIR_INPUT();       // 输入方向
    PB03_PUSHPULL_ENABLE(); // 推挽输出
    PB03_PUR_DISABLE();     // 禁用上拉
    PB03_PDR_DISABLE();     // 禁用下拉

    // 初始化UART配置结构体
    USART_StructInit(&uart_cfg);
    uart_cfg.USART_BaudRate = 9600;                          // 9600波特率
    uart_cfg.USART_StopBits = USART_StopBits_1;              // 1位停止位
    uart_cfg.USART_Parity   = USART_Parity_No;               // 无校验
    uart_cfg.USART_Mode     = USART_Mode_Tx | USART_Mode_Rx; // 收发模式
    uart_cfg.USART_Source   = USART_Source_PCLK;             // 时钟源
    uart_cfg.USART_Over     = USART_Over_16;                 // 16倍过采样
    uart_cfg.USART_UclkFreq = SystemCoreClock;               // 系统时钟频率
    USART_Init(CW_UART2, &uart_cfg);                         // 修正：使用UART2

    // 使能发送完成中断和接收中断
    USART_ITConfig(CW_UART2, USART_IT_TC, ENABLE); // 发送完成中断
    USART_ITConfig(CW_UART2, USART_IT_RC, ENABLE); // 接收完成中断

    // 使能UART2中断（修正）
    NVIC_EnableIRQ(UART2_IRQn);
    // 使能收发方向
    USART_DirectionModeCmd(CW_UART2, USART_Mode_Tx | USART_Mode_Rx, ENABLE);

    // 初始化RS485为接收模式
    RS485_SET_RX();

    // SEGGER_RTT_printf(0, "[UART] UART2 and RS485 pins initialized successfully\n");
}

/**
 * @brief 计算Modbus CRC16校验码
 * @param data 数据指针
 * @param len 数据长度
 * @return CRC16校验值
 */
uint16_t modbus_crc16(uint8_t *data, uint8_t len)
{
    uint16_t crc = 0xFFFF; // CRC初始值
    uint8_t i, j;

    for (i = 0; i < len; i++) {
        crc ^= data[i]; // 异或操作
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001)              // 判断最低位是否为1
                crc = (crc >> 1) ^ 0xA001; // 右移并异或多项式
            else
                crc >>= 1; // 仅右移
        }
    }
    return crc;
}

/**
 * @brief 通过中断方式发送RS485数据
 * @param data 要发送的数据指针
 * @param len 数据长度
 * @note 使用中断方式逐字节发送，发送完成后自动切换到接收模式
 */
void uart1_send485_irq(uint8_t *data, uint8_t len)
{
    // 参数检查：数据长度不能为0，且不能正在发送中
    if (len == 0 || uart1_busy) {
        SEGGER_RTT_printf(0, "[UART] Send rejected: len=%d, busy=%d\n", len, uart1_busy);
        return;
    }

    // 设置发送参数
    uart1_tx_buf   = data; // 发送缓冲区指针
    uart1_tx_len   = len;  // 发送总长度
    uart1_tx_index = 0;    // 发送索引清零
    uart1_busy     = 1;    // 设置忙标志

    // SEGGER_RTT_printf(0, "[UART] Starting RS485 transmission...\n");
    RS485_SET_TX(); // 切换到发送模式

    // 延时一小段时间，确保RS485状态稳定
    volatile int i;
    for (i = 0; i < 10; i++);

    // 开始发送第一个字节
    // SEGGER_RTT_printf(0, "[TX START] 0x%02X, total_len=%d\n", uart1_tx_buf[uart1_tx_index], uart1_tx_len);
    USART_SendData_8bit(CW_UART2, uart1_tx_buf[uart1_tx_index]); // 修正：使用UART2
    USART_ITConfig(CW_UART2, USART_IT_TC, ENABLE);               // 使能发送完成中断
}

/**
 * @brief UART2中断服务函数（修正：从UART1改为UART2）
 * @note 处理发送完成中断和接收中断
 */
void UART2_IRQHandler(void)
{

    
    // 处理发送完成中断
    if (USART_GetFlagStatus(CW_UART2, USART_FLAG_TC) == SET) {
        USART_ClearFlag(CW_UART2, USART_FLAG_TC); // 清除发送完成标志

        uart1_tx_index++; // 发送索引递增
        if (uart1_tx_index < uart1_tx_len) {
            // 还有数据需要发送
            // SEGGER_RTT_printf(0, "[TX] 0x%02X\n", uart1_tx_buf[uart1_tx_index]);
            USART_SendData_8bit(CW_UART2, uart1_tx_buf[uart1_tx_index]);
        } else {
            // 所有数据发送完成
            USART_ITConfig(CW_UART2, USART_IT_TC, DISABLE); // 禁用发送完成中断

            // 添加延时，确保发送完全完成
            volatile int i;
            for (i = 0; i < 20; i++);

            RS485_SET_RX(); // 切换到接收模式
            uart1_busy = 0; // 清除忙标志
            // SEGGER_RTT_printf(0, "[UART_IRQ] UART TX completed, switched to RX mode\n");
        }
    }

    // 处理接收中断
    if (USART_GetFlagStatus(CW_UART2, USART_FLAG_RC) == SET) {
        USART_ClearFlag(CW_UART2, USART_FLAG_RC); // 清除接收标志
        // 

        /*
        uint8_t data = USART_ReceiveData_8bit(CW_UART2); // 读取接收数据
        // 接收数据处理
          static uint8_t rx_buffer[64];
          static uint8_t rx_index       = 0;
          static uint32_t frame_timeout = 0;

          // 简单的帧超时检测（基于接收间隔）
          frame_timeout = 0; // 重置超时计数器

          rx_buffer[rx_index++] = data;

          // 打印接收到的数据
          // SEGGER_RTT_printf(0, "[RX] 0x%02X (index=%d)\n", data, rx_index - 1);

          // 检查是否接收到完整的Modbus RTU帧
          if (rx_index >= 4) { // 最小帧长度
              // 尝试解析帧
              uint8_t i;
              for (i = 0; i < rx_index - 1; i++) {
                  // 检查是否找到可能的帧边界
                  if (rx_buffer[i] == 0x06 && rx_buffer[i + 1] == 0x03) {
                      // 找到读寄存器响应帧
                      if (i + 7 < rx_index) { // 确保有足够的数据
                          uint16_t received_crc   = (rx_buffer[i + 6] << 8) | rx_buffer[i + 5];
                          uint16_t calculated_crc = modbus_crc16(&rx_buffer[i], 5);

                          if (received_crc == calculated_crc) {
                              SEGGER_RTT_printf(0, "[RX] Valid read response frame found!\n");
                              SEGGER_RTT_printf(0, "[RX] Frame: ");
                              int j;
                              for (j = i; j < i + 7; j++) {
                                  SEGGER_RTT_printf(0, "0x%02X ", rx_buffer[j]);
                              }
                              SEGGER_RTT_printf(0, "\n");

                              // 解析寄存器数据
                              uint8_t byte_count = rx_buffer[i + 2];
                              if (byte_count == 2) {
                                  uint16_t reg_value = (rx_buffer[i + 3] << 8) | rx_buffer[i + 4];
                                  SEGGER_RTT_printf(0, "[RX] Register 0xB100 value: 0x%04X (%d)\n", reg_value, reg_value);
                              }

                              // 移除已处理的帧
                              int k;
                              for (k = 0; k < rx_index - i - 7; k++) {
                                  rx_buffer[k] = rx_buffer[i + 7 + k];
                              }
                              rx_index = rx_index - i - 7;
                              break;
                          }
                      }
                  } else if (rx_buffer[i] == 0x06 && rx_buffer[i + 1] == 0x10) {
                      // 找到写寄存器响应帧
                      if (i + 5 < rx_index) { // 确保有足够的数据
                          uint16_t received_crc   = (rx_buffer[i + 4] << 8) | rx_buffer[i + 3];
                          uint16_t calculated_crc = modbus_crc16(&rx_buffer[i], 3);

                          if (received_crc == calculated_crc) {
                              SEGGER_RTT_printf(0, "[RX] Valid write response frame found!\n");
                              SEGGER_RTT_printf(0, "[RX] Frame: ");
                              int j;
                              for (j = i; j < i + 5; j++) {
                                  SEGGER_RTT_printf(0, "0x%02X ", rx_buffer[j]);
                              }
                              SEGGER_RTT_printf(0, "\n");

                              // 移除已处理的帧
                              int k;
                              for (k = 0; k < rx_index - i - 5; k++) {
                                  rx_buffer[k] = rx_buffer[i + 5 + k];
                              }
                              rx_index = rx_index - i - 5;
                              break;
                          }
                      }
                  }
              }
          }

          // 防止缓冲区溢出
          if (rx_index >= 64) {
              SEGGER_RTT_printf(0, "[RX] Buffer overflow, resetting\n");
              rx_index = 0;
          } */
    }
}

void send_gear_p_command(void)
{
    uint8_t gear_cmd[] = {
        0x06, 0x10, // 从机地址0x06，功能码0x10
        0xB1, 0x01, // 寄存器地址0xB101
        0x00, 0x01, // 寄存器数量：1个
        0x02,       // 字节数：2字节
        0x00, 0x80, // 数据：0x0080（低字节P挡）
        0x00, 0x00  // CRC占位符
    };
    uint16_t crc = modbus_crc16(gear_cmd, 9);
    gear_cmd[9]  = crc & 0xFF;
    gear_cmd[10] = crc >> 8;
    uart1_send485_irq(gear_cmd, 11);
}


