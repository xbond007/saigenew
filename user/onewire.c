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

static uint8_t seq_l = 0; // 流水号低字节（递增）
static uint8_t seq_h = 0; // 流水号高字节（递增）

static YXT_Status_t yxt_status = {0}; // 当前状态缓存（由外部设置）

// 发送状态：同步码→数据→空闲
typedef enum {
    YXT_STATE_SYNC, // 发送同步码（32Tosc）
    YXT_STATE_DATA, // 发送数据位
    YXT_STATE_IDLE  // 空闲状态（>40ms低电平）
} YXT_State;

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

    yxt_data[0] = 0x08; // 固定设备号
    yxt_data[1] = seq_l;
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

// 每 1ms 调用一次，用于推进一位 bit 的输出
void onewire_task(void)
{
    if (!yxt_sending) return;

    uint8_t byte = yxt_data[yxt_byte_index];
    current_bit  = (byte >> (7 - yxt_bit_index)) & 0x01;
    bit_timer++;

    if (current_bit) {
        // 发送 1：低0.5ms + 高1ms
        if (bit_timer == 1)
            YXT_Out_Low();
        else if (bit_timer == 2)
            YXT_Out_High();
        else if (bit_timer == 3) {
            bit_timer = 0;
            yxt_bit_index++;
        }
    } else {
        // 发送 0：低1ms + 高0.5ms
        if (bit_timer == 1 || bit_timer == 2)
            YXT_Out_Low();
        else if (bit_timer == 3)
            YXT_Out_High();
        else if (bit_timer == 4) {
            bit_timer = 0;
            yxt_bit_index++;
        }
    }

    if (yxt_bit_index >= 8) {
        yxt_bit_index = 0;
        yxt_byte_index++;
        if (yxt_byte_index >= YXT_FRAME_LEN) {
            yxt_byte_index = 0;
            bit_timer      = 0;
            yxt_encode_frame(); // 自动生成下一帧
        }
    }
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

// 修正状态机和时序参数
static YXT_State yxt_state = YXT_STATE_SYNC;
static uint16_t timer      = 0; // 1ms/次

void onewire_fixed_task(void)
{
    switch (yxt_state) {
        case YXT_STATE_SYNC:
            YXT_Out_Low();
            timer++;
            if (timer >= 2) { // 2ms sync code completed
                timer          = 0;
                yxt_state      = YXT_STATE_DATA;
                yxt_bit_index  = 0;
                yxt_byte_index = 0;
                SEGGER_RTT_printf(0, "Enter data transmission state\n"); // Debug info
            }
            break;

        case YXT_STATE_DATA: {
            uint8_t byte         = yxt_data[yxt_byte_index];
            uint8_t current_bit  = (byte >> (7 - yxt_bit_index)) & 0x01;
            uint8_t bit_complete = 0; // Mark if current bit transmission is complete

            if (current_bit) {
                // Logic 1: Low for 40 counts (2ms) → High for 80 counts (4ms), total 120 counts
                if (timer < 2) {
                    YXT_Out_Low();
                } else if (timer < 6) { // 40+80=120
                    YXT_Out_High();
                } else {
                    // Current bit transmission completed
                    bit_complete = 1;
                }
            } else {
                // Logic 0: Low for 80 counts (4ms) → High for 40 counts (2ms), total 120 counts
                if (timer < 4) {
                    YXT_Out_Low();
                } else if (timer < 6) { // 80+40=120
                    YXT_Out_High();
                } else {
                    // Current bit transmission completed
                    bit_complete = 1;
                }
            }

            if (bit_complete) {
                timer = 0;
                yxt_bit_index++;
                // SEGGER_RTT_printf(0, "Bit transmission completed. Current byte: %d, Current bit: %d\n",   yxt_byte_index, yxt_bit_index); // Debug info
            } else {
                // Only increment timer when transmission not complete (critical fix)
                timer++;
            }

            // Handle byte switch (after 8 bits transmitted)
            if (yxt_bit_index >= 8) {
                yxt_bit_index = 0;
                yxt_byte_index++;
                // SEGGER_RTT_printf(0, "Byte transmission completed. Current byte index: %d\n", yxt_byte_index); // Debug info

                // Handle frame completion (all bytes transmitted)
                if (yxt_byte_index >= YXT_FRAME_LEN) {
                    yxt_byte_index = 0;
                    yxt_state      = YXT_STATE_IDLE;
                    timer          = 0;
                    YXT_Out_Low();
                    // SEGGER_RTT_printf(0, "Frame transmission completed. Entering idle state\n"); // Debug info

                    // Update status and generate next frame (data will change here)
                    static uint8_t counter = 0;
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
                    yxt_encode_frame(); // Generate new frame with updated data
                }
            }
            break;
        }

        case YXT_STATE_IDLE:
            if (timer >= 40) { // 40ms idle time
                timer     = 0;
                yxt_state = YXT_STATE_SYNC;
                SEGGER_RTT_printf(0, "Idle period ended. Re-entering sync state\n"); // Debug info
            } else {
                timer++;
            }
            break;
    }

    // Print data only on state switch (reduce log noise)
    if (yxt_state == YXT_STATE_SYNC && timer == 0) {
        SEGGER_RTT_printf(0, "Sending new frame: ");
        int i;
        for (i = 0; i < YXT_FRAME_LEN; i++) {
            SEGGER_RTT_printf(0, "%02X ", yxt_data[i]);
        }
        SEGGER_RTT_printf(0, "\n");
    }
}
