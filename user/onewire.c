#include "onewire.h"
#include <string.h>
#include "cw32f003.h"
#include "cw32f003_gpio.h"

static uint8_t yxt_data[YXT_FRAME_LEN] = {0}; // 最终发送的数据帧

static uint8_t yxt_bit_index  = 0; // 当前发送 bit 索引
static uint8_t yxt_byte_index = 0; // 当前发送 byte 索引
static uint8_t yxt_sending    = 0; // 是否在发送中标志

static uint8_t bit_timer   = 0; // 单 bit 时序控制器
static uint8_t current_bit = 0; // 当前 bit 电平值

static uint8_t seq_l           = 0;   // 流水号低字节（递增）
static uint8_t seq_h           = 0;   // 流水号高字节（递增）
static YXT_Status_t yxt_status = {0}; // 当前状态缓存（由外部设置）

// 发送状态：同步码→数据→空闲
typedef enum {
    YXT_STATE_SYNC, // 发送同步码（32Tosc）
    YXT_STATE_DATA, // 发送数据位
    YXT_STATE_IDLE  // 空闲状态（>40ms低电平）
} YXT_State;

// 修正状态机和时序参数
static YXT_State yxt_state = YXT_STATE_SYNC;
static uint16_t timer      = 0; // 0.5ms

// 设置 GPIOB.2 为高电平
static inline void YXT_Out_High(void)
{
    CW_GPIOB->BSRR = GPIO_PIN_2;
}

// 设置 GPIOB.2 为低电平
static inline void YXT_Out_Low(void)
{
    CW_GPIOB->BRR = GPIO_PIN_2;
}

// 更新发送状态内容（外部调用）
void yxt_update_status(const YXT_Status_t *s)
{
    // 复制内存
    memcpy(&yxt_status, s, sizeof(YXT_Status_t));
}

// 加密算法：生成 PULSE 字节，用于数据加密
static uint8_t yxt_calc_pulse(uint8_t sl, uint8_t sh)
{
    uint8_t pulse = sl + 0x6B;
    pulse ^= 0x54;
    pulse += 0x19;
    pulse ^= 0x25;
    pulse += (sh & 0x0F);
    pulse ^= 0x6B;
    pulse += 0x3B;
    pulse ^= 0x3A;
    pulse &= 0x7F;
    return pulse;
}

// 构造完整的一线通帧，包含自动递增流水号与状态加密
void yxt_encode_frame(void)
{
    uint8_t pulse, checksum = 0;

    seq_l++;
    if (seq_l == 0) seq_h++;

    pulse = yxt_calc_pulse(seq_l, seq_h);

    yxt_data[0]  = 0x08; // 固定设备号
    yxt_data[1]  = seq_l;
    yxt_data[2]  = ((seq_h & 0x0F) << 4) | (yxt_status.status1 & 0x0F);
    yxt_data[3]  = yxt_status.status2 + pulse;
    yxt_data[4]  = yxt_status.status3 + pulse;
    yxt_data[5]  = yxt_status.status4 + pulse;
    yxt_data[6]  = yxt_status.status5; // 不加密字段
    yxt_data[7]  = (yxt_status.speed >> 8) + pulse;
    yxt_data[8]  = (yxt_status.speed & 0xFF) + pulse;
    yxt_data[9]  = yxt_status.reserved + pulse;
    yxt_data[10] = yxt_status.voltage + pulse;
    uint8_t i;
    for (i = 0; i < 11; i++) checksum ^= yxt_data[i];
    yxt_data[11] = checksum; // 异或校验
}

void onewire_fixed_task(void)
{
    switch (yxt_state) {
        case YXT_STATE_SYNC:
            if (timer < 60) { // 926Tosc 低电平  60 Tosc试试
                YXT_Out_Low();
            } else if (timer < 60 + 32) { // 32Tosc 高电平
                YXT_Out_High();
            }
            timer++;
            if (timer >= 60 + 32) {
                timer          = 0;
                yxt_state      = YXT_STATE_DATA;
                yxt_bit_index  = 0;
                yxt_byte_index = 0;
            }
            break;
        case YXT_STATE_DATA: {
            uint8_t byte         = yxt_data[yxt_byte_index];
            uint8_t current_bit  = (byte >> (7 - yxt_bit_index)) & 0x01;
            uint8_t bit_complete = 0;

            if (current_bit) {
                // 逻辑1：低 0.5ms（5次） → 高 1.0ms（10次）
                if (timer < 5)
                    YXT_Out_Low();
                else if (timer < 15)
                    YXT_Out_High();
                else
                    bit_complete = 1;
            } else {
                // 逻辑0：低 1.0ms（10次） → 高 0.5ms（5次）
                if (timer < 10)
                    YXT_Out_Low();
                else if (timer < 15)
                    YXT_Out_High();
                else
                    bit_complete = 1;
            }

            if (bit_complete) {
                // SEGGER_RTT_printf(0, "[BIT] byte_idx=%d bit_idx=%d val=%d\n", yxt_byte_index, yxt_bit_index, current_bit);
                timer = 0;
                yxt_bit_index++;
                if (yxt_bit_index >= 8) {
                    // SEGGER_RTT_printf(0, "[BYTE] byte_idx=%d val=0x%02X\n", yxt_byte_index, byte);
                    yxt_bit_index = 0;
                    yxt_byte_index++;
                    if (yxt_byte_index >= YXT_FRAME_LEN) {
                        // SEGGER_RTT_printf(0, "[FRAME] all bytes sent\n");
                        yxt_byte_index = 0;
                        yxt_state      = YXT_STATE_IDLE;
                        timer          = 0;
                        YXT_Out_Low();

                        YXT_Status_t s = {
                            .status1  = 0x02, // +P档
                            .status2  = 0x00,
                            .status3  = 0x00,
                            .status4  = 0x00,
                            .status5  = 0x1F, // 电流值
                            .speed    = 0x00, // 小速度值
                            .reserved = 0x00,
                            .voltage  = 0x20 //
                        };
                        yxt_update_status(&s);
                        yxt_encode_frame();
                    }
                }
            } else {
                timer++;
            }
            break;
        }

        case YXT_STATE_IDLE:
            if (timer >= 50) { // 40ms idle time (100us * 400 = 40ms)
                timer     = 0;
                yxt_state = YXT_STATE_SYNC;
                // SEGGER_RTT_printf(0, "[IDLE->SYNC]\n");
            } else {
                timer++;
            }
            break;
    }

    // 打印新帧内容
    // if (yxt_state == YXT_STATE_SYNC && timer == 0) {
    //     SEGGER_RTT_printf(0, "[FRAME DATA] ");
    //     int i;
    //     for (i = 0; i < YXT_FRAME_LEN; i++) {
    //         SEGGER_RTT_printf(0, "%02X ", yxt_data[i]);
    //     }
    //     SEGGER_RTT_printf(0, "\n");
    // }
}

// 开始一线通发送流程
void yxt_start_send(void)
{
    if (yxt_sending) return;
    yxt_encode_frame();
    yxt_sending    = 1;
    yxt_bit_index  = 0;
    yxt_byte_index = 0;
    bit_timer      = 0;
}

// 停止发送流程，并置 GPIO 低电平
void yxt_stop_send(void)
{
    yxt_sending    = 0;
    yxt_bit_index  = 0;
    yxt_byte_index = 0;
    bit_timer      = 0;
    YXT_Out_Low();
}

void onewire_fixed_task_singleframe(void)
{
    static const uint8_t fixed_frame[YXT_FRAME_LEN] = {
        0x08, 0x61, 0x00, 0x00, 0x00, 0x00,
        0x1F, 0x00, 0x00, 0xE3, 0x02, 0x97};
    static YXT_State state = YXT_STATE_SYNC;
    switch (state) {
        case YXT_STATE_SYNC:
            // 926 463  60 50 200us
            if (timer < 926) {
                YXT_Out_Low();
            } else if (timer < 926 + 32) {
                YXT_Out_High();
            }
            timer++;
            if (timer >= 926 + 32) {
                timer          = 0;
                yxt_byte_index = 0;
                yxt_bit_index  = 0;
                state          = YXT_STATE_DATA;
            }
            break;

        case YXT_STATE_DATA: {
            uint8_t byte         = fixed_frame[yxt_byte_index];
            uint8_t current_bit  = (byte >> (7 - yxt_bit_index)) & 0x01;
            uint8_t bit_complete = 0;

            if (current_bit) {
                if (timer < 3)
                    YXT_Out_Low();
                else if (timer < 8)
                    YXT_Out_High();
                else
                    bit_complete = 1;
            } else {
                if (timer < 6)
                    YXT_Out_Low();
                else if (timer < 8)
                    YXT_Out_High();
                else
                    bit_complete = 1;
            }

            if (bit_complete) {
                timer = 0;
                yxt_bit_index++;
                if (yxt_bit_index >= 8) {
                    yxt_bit_index = 0;
                    yxt_byte_index++;
                    if (yxt_byte_index >= YXT_FRAME_LEN) {
                        yxt_byte_index = 0;
                        state          = YXT_STATE_IDLE;
                        timer          = 0;
                        YXT_Out_Low();
                    }
                }
            } else {
                timer++;
            }
            break;
        }

        case YXT_STATE_IDLE:
            if (timer >= 400) { // 40ms 400
                timer = 0;
                state = YXT_STATE_SYNC;
            } else {
                timer++;
            }
            break;
    }
}