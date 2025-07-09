#include "common.h"
extern uint16_t timetimes;
#define TOSC32     16
#define BUFNUMB    1048
#define start_addr 0x08002800 // code 10k
uint8_t usart1_tx[46] = {0};
uint8_t rx_nub, tx_nub1, tx_nub2, time_out, recv_flag, tx_over;
uint8_t lock_answer = 0, time_out;
uint16_t sum, rx_numb;
uint8_t code_buf[BUFNUMB] = {0};
typedef union {
    uint32_t Word;
    struct
    {
        uint8_t Rf_tim;
        u8 Rf_high;
        u8 Rf_bit;
        u8 Rf_byte;
    } Bytes;
} RF_REG;
__align(4) RF_REG RF_reg = {0};

uint8_t yxt_tx[12] = {0};
uint8_t step2      = 0;

uint8_t *alm_send(uint8_t len)
{
    static uint8_t bit = 0, byte = 0;
    static uint16_t tim = 0;
    static uint8_t *p;
    p = yxt_tx;

    switch (step2) {
        case 0: // 初始静默期，拉低持续一段时间
        {
            OUT_YXT = 0; // 输出低电平
            tim++;
            if (tim > TOSC32 * 60) {
                tim = 0;
                step2++;
            }
            break;
        }

        case 1: // 起始拉高
        {
            OUT_YXT = 1; // 输出高电平
            tim++;
            if (tim > TOSC32 * 5) {
                tim = 0;
                step2++;
            }
            break;
        }

        case 2: // 主发码阶段（低位先发）
        {
            uint8_t current_bit = (p[byte] >> bit) & 0x01;

            if (current_bit) {
                if (tim < TOSC32)
                    OUT_YXT = 0;
                else if (tim < TOSC32 * 3)
                    OUT_YXT = 1;
                else {
                    tim = 0;
                    bit++;
                    if (bit > 7) {
                        bit = 0;
                        byte++;
                    }
                }
            } else {
                if (tim < TOSC32 * 2)
                    OUT_YXT = 0;
                else if (tim < TOSC32 * 3)
                    OUT_YXT = 1;
                else {
                    tim = 0;
                    bit++;
                    if (bit > 7) {
                        bit = 0;
                        byte++;
                    }
                }
            }

            if (byte == len) {
                tim  = 0;
                bit  = 0;
                byte = 0;
                step2++;
                OUT_YXT = 0; // 发完拉低
            }

            break;
        }

        default:
            break;
    }

    return &step2;
}

u8 NFC_read = 0;

/**
 * @brief 每 50us 检测一次 NFC 低电平引导信号，解析成 bit 流
 */
void com_task_50us(void)
{
    static u8 data[14] __attribute__((aligned(4)));
    static u8 check = 0;
    static u32 rf_io;

    if (IS_NFC_LOW != rf_io) {
        rf_io = IS_NFC_LOW;
        if (IS_NFC_LOW == 0) {
            if (RF_reg.Bytes.Rf_tim == 65 || RF_reg.Bytes.Rf_tim < 25 || RF_reg.Bytes.Rf_high < 5) {
                RF_reg.Word = 0;
                check       = 0;
                return;
            }

            RF_reg.Bytes.Rf_tim -= RF_reg.Bytes.Rf_high;
            if (RF_reg.Bytes.Rf_tim < 5) {
                RF_reg.Word = 0;
                check       = 0;
                return;
            }

            data[RF_reg.Bytes.Rf_byte] <<= 1;
            if (RF_reg.Bytes.Rf_tim > RF_reg.Bytes.Rf_high) {
                data[RF_reg.Bytes.Rf_byte] |= 1;
            }

            RF_reg.Bytes.Rf_tim = 0;
            RF_reg.Bytes.Rf_bit++;

            if (RF_reg.Bytes.Rf_bit > 10) {
                NFC_read = 1;
            } else if ((RF_reg.Bytes.Rf_bit & 7) == 0) {
                check ^= data[RF_reg.Bytes.Rf_byte];
                RF_reg.Bytes.Rf_byte++;
            }

        } else {
            RF_reg.Bytes.Rf_high = RF_reg.Bytes.Rf_tim;
        }
    } else if (RF_reg.Bytes.Rf_tim < 65) {
        RF_reg.Bytes.Rf_tim++;
    }
}

u8 NFC_read = 0;
u8 recv_over = 0;
extern u16 rx_numb;
extern u16 sum;
extern u8 usart1_tx[46];
extern u8 tx_nub1, tx_nub2;
extern u8 time_out;
extern u8 code_buf[BUFNUMB];

/**
 * @brief 初始化 UART1，使用 CW32 官方库结构体
 */
void usartInit(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBENR_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1ENR_UART1, ENABLE);

    GPIO_InitTypeDef gpio = {0};

    // PA9: TX
    gpio.Pins = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    GPIO_Init(CW_GPIOA, &gpio);

    // PA10: RX
    gpio.Pins = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    GPIO_Init(CW_GPIOA, &gpio);

    // 初始化 UART
    USART_InitTypeDef uart = {0};
    uart.USART_BaudRate = 9600;
    uart.USART_Over     = USART_Over_16;
    uart.USART_Source   = USART_Source_PCLK;
    uart.USART_UclkFreq = 8000000;  // 默认使用 8MHz UCLK
    uart.USART_StartBit = USART_StartBit_FE;
    uart.USART_StopBits = USART_StopBits_1;
    uart.USART_Parity   = USART_Parity_No;
    uart.USART_Mode     = USART_Mode_Tx | USART_Mode_Rx;
    uart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    USART_Init(CW_UART1, &uart);
    USART_DMACmd(CW_UART1, USART_DMAReq_Rx, ENABLE);
    USART_ITConfig(CW_UART1, USART_IT_RC, ENABLE);
    USART_Cmd(CW_UART1, ENABLE);

    // NVIC 配置
    NVIC_InitTypeDef nvic = {0};
    nvic.NVIC_IRQChannel = UART1_IRQn;
    nvic.NVIC_IRQChannelPriority = 0;
    NVIC_Init(&nvic);
}

/**
 * @brief 初始化 DMA 接收通道（UART1 RX）
 */
void DMA_INIT(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBENR_DMA, ENABLE);

    DMA_InitTypeDef dma = {0};
    dma.DMA_DIR = DMA_DIR_PeripheralSRC;
    dma.DMA_M2M = DMA_M2M_Disable;
    dma.DMA_BufferSize = BUFNUMB;
    dma.DMA_MemoryBaseAddr = (u32)code_buf;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_Mode = DMA_Mode_Normal;
    dma.DMA_PeripheralBaseAddr = (u32)&CW_UART1->DATAR;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_Priority = DMA_Priority_High;

    DMA_Init(CW_DMA1_Channel5, &dma);
    DMA_Cmd(CW_DMA1_Channel5, ENABLE);
}

/**
 * @brief UART1 中断服务函数（IDLE和TC处理）
 */
void UART1_IRQHandler(void)
{
    if (USART_GetITStatus(CW_UART1, USART_IT_RC) != RESET)
    {
        time_out = 0;
        DMA_Cmd(CW_DMA1_Channel5, DISABLE);
        rx_numb = BUFNUMB - CW_DMA1_Channel5->CNTR;
        recv_over = 1;
        CW_UART1->STATR;
        CW_UART1->DATAR;
        CW_DMA1->INTFCR = 0x0FFFFFFF;
        CW_DMA1_Channel5->CNTR = BUFNUMB;
        DMA_Cmd(CW_DMA1_Channel5, ENABLE);
    }

    if (USART_GetITStatus(CW_UART1, USART_IT_TC) != RESET)
    {
        USART_ClearITPendingBit(CW_UART1, USART_IT_TC);
        if (tx_nub1 < tx_nub2)
        {
            tx_nub1++;
            USART_SendData(CW_UART1, usart1_tx[tx_nub1]);
        }
        else
        {
            EN485_L;
        }
    }
}


void send_order()
{
    uint8_t i;
    sum           = 0;
    usart1_tx[0]  = 0x06;
    usart1_tx[1]  = 0x10;
    usart1_tx[2]  = 0xB1;
    usart1_tx[3]  = 0x00;
    usart1_tx[4]  = 0x00;
    usart1_tx[5]  = 0x12;
    usart1_tx[6]  = 0x24;
    usart1_tx[7]  = 0x00;
    usart1_tx[8]  = 0x00;
    usart1_tx[9]  = 0x00;
    usart1_tx[10] = 0x21;
    usart1_tx[11] = 0x00;
    usart1_tx[12] = 0x3C;
    usart1_tx[13] = 0x00;
    usart1_tx[14] = 0x00;
    usart1_tx[15] = 0x00;
    usart1_tx[16] = 0x00;
    usart1_tx[17] = 0x00;
    usart1_tx[18] = 0x00;
    usart1_tx[19] = 0x02;
    usart1_tx[20] = 0x02;
    usart1_tx[21] = 0x00;
    usart1_tx[22] = 0x00;
    usart1_tx[23] = 0x02;
    usart1_tx[24] = 0xDB;
    usart1_tx[25] = 0x00;
    usart1_tx[26] = 0x64;
    usart1_tx[27] = 0x00;
    usart1_tx[28] = 0x00;
    usart1_tx[29] = 0x00;
    usart1_tx[30] = 0x00;
    usart1_tx[31] = 0x00;
    usart1_tx[32] = 0x00;
    usart1_tx[33] = 0x0f;
    usart1_tx[34] = 0x10;
    usart1_tx[35] = 0x0A;
    usart1_tx[36] = 0x00;
    usart1_tx[37] = 0x00;
    usart1_tx[38] = 0x1E;
    usart1_tx[39] = 0x00;
    usart1_tx[40] = 0x00;
    usart1_tx[41] = 0x02;
    usart1_tx[42] = 0x00;
    usart1_tx[43] = 0x89;
    usart1_tx[44] = 0x5c;
    EN485_H;
    tx_nub1 = 0;
    tx_nub2 = 44;
    USART_ITConfig(USART1, USART_IT_TC, ENABLE);
    USART_SendData(USART1, usart1_tx[tx_nub1]);
}
