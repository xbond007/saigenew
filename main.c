#include "cw32f003.h" // 包含芯片头文件
#include "timer.h"
#include "common.h"
#include "SEGGER_RTT.h"
#include "cw32f003_gpio.h"
#include <stdarg.h>


// 初始化 RTT 打印接口
void segger_rtt_init(char *str)
{
    SEGGER_RTT_Init();
    print_log(str); // 封装接口，无需填写 BUFFER_INDEX
}

// 全局变量定义
extern uint8_t lock_answer, tx_over, rx_nub;
uint8_t Com_Buffer[128];
uint8_t Lock[10];
uint8_t error, send_cmd, start_work = 0, step, start_state;
uint16_t cout;
uint16_t timebase[10];
// 假设 LOCK_T 是一个自定义类型，这里未定义，保留原样
typedef struct {
    // 可以在这里添加具体成员
} LOCK_T;
LOCK_T state;
RCC_ClocksTypeDef RCC_Clocks;


// 定义 EXTI_InitTypeDef 结构体
typedef struct {
    uint32_t EXTI_Line;       // 外部中断线
    uint32_t EXTI_Mode;       // 中断模式（如中断或事件）
    uint32_t EXTI_Trigger;    // 触发方式（如上升沿、下降沿等）
    FunctionalState EXTI_LineCmd; // 使能或禁用外部中断线
} EXTI_InitTypeDef;

// 定义 NVIC_InitTypeDef 结构体
typedef struct {
    uint8_t NVIC_IRQChannel;                   // 中断通道
    uint8_t NVIC_IRQChannelPreemptionPriority; // 抢占优先级
    uint8_t NVIC_IRQChannelSubPriority;        // 子优先级
    FunctionalState NVIC_IRQChannelCmd;        // 使能或禁用中断通道
} NVIC_InitTypeDef;



// GPIO 初始化函数
void GPIO_Toggle_INIT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    EXTI_InitTypeDef EXTI_InitStruct    = {0};
    NVIC_InitTypeDef NVIC_InitStruct    = {0};

    // 使能 GPIO 时钟
    CW_SYSCTRL->AHBEN_f.GPIOA = 1;
    CW_SYSCTRL->AHBEN_f.GPIOB = 1;
    CW_SYSCTRL->AHBEN_f.GPIOC = 1;

    // 初始化 GPIOA
    GPIO_InitStructure.Pins = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init(CW_GPIOA, &GPIO_InitStructure);

    // 初始化 GPIOB
    GPIO_InitStructure.Pins = GPIO_PIN_15;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init(CW_GPIOB, &GPIO_InitStructure);

    // 初始化 GPIOB 输入模式
    GPIO_InitStructure.Pins = GPIO_PIN_9;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_Init(CW_GPIOB, &GPIO_InitStructure);

    // 初始化 GPIOA 上拉输入模式
    GPIO_InitStructure.Pins = GPIO_PIN_3;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT_PULLUP;
    GPIO_Init(CW_GPIOA, &GPIO_InitStructure);

    // 配置外部中断
    // 假设这里有合适的宏定义来替代原有的 GPIO_EXTILineConfig
    // 原代码中使用的是标准库函数，这里根据现有代码无法完全对应，保留概念
    // GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource3);
    EXTI_InitStruct.Line    = EXTI_LINE_3;
    EXTI_InitStruct.Mode    = EXTI_MODE_INTERRUPT;
    EXTI_InitStruct.Trigger = EXTI_TRIGGER_FALLING;
    EXTI_InitStruct.LineCmd = ENABLE;
    // 假设这里有合适的 EXTI 初始化函数，原代码中使用的是标准库函数，这里根据现有代码无法完全对应，保留概念
    // EXTI_Init(&EXTI_InitStruct);

    NVIC_InitStruct.IRQChannel         = EXTI3_IRQn;
    NVIC_InitStruct.PreemptionPriority = 1;
    NVIC_InitStruct.SubPriority        = 2;
    NVIC_InitStruct.IRQChannelCmd      = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

// 外部中断处理函数
void EXTI3_IRQHandler()
{
    // 假设这里有合适的函数来获取和清除中断标志位，原代码中使用的是标准库函数，这里根据现有代码无法完全对应，保留概念
    // if (EXTI_GetITStatus(EXTI_Line3) != RESET) {
    //     EXTI_ClearITPendingBit(EXTI_Line3);
    start_work = !start_work;
    // }
}

int main(void)
{
    // 系统初始化
    SystemInit();

    // 初始化 RTT 打印接口
    segger_rtt_init("System started!\r\n");

    // 外设初始化（例如GPIO、UART等）
    GPIO_Toggle_INIT();

    // 主循环
    while (1) {
        // 应用程序代码
    }
}