#ifndef __ONEWIRE_H__
#define __ONEWIRE_H__

#include <stdint.h>

#define YXT_FRAME_LEN 12 // 一线通协议固定帧长 12 字节

// 状态结构体定义，用于设置协议数据帧中 Status 字段
typedef struct {
    uint8_t status1;  // 挡位、车型等
    uint8_t status2;  // 故障信息
    uint8_t status3;  // 三速、刹车等
    uint8_t status4;  // 模式、倒车等
    uint8_t status5;  // 电流值
    uint16_t speed;   // 速度值（两个字节，高+低）
    uint8_t reserved; // 保留位
    uint8_t voltage;  // 电压标志位
} YXT_Status_t;

void yxt_update_status(const YXT_Status_t *s);
void yxt_start_send(void);
void yxt_stop_send(void);
void onewire_task(void);
void onewire_fixed_task(void);
#endif
