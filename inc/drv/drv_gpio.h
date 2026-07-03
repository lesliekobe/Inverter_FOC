#ifndef __DRV_GPIO_H
#define __DRV_GPIO_H

#include <stdint.h>

/* DI/DO 位定义 */
#define DI_START      (1<<0)   // 启动按钮
#define DI_STOP       (1<<1)   // 停止按钮
#define DI_REVERSE    (1<<2)   // 正反转切换
#define DI_FAULT_IN  (1<<3)   // 外部故障输入
#define DI_MS0       (1<<4)   // 多速段选择0
#define DI_MS1       (1<<5)   // 多速段选择1
#define DI_MS2       (1<<6)   // 多速段选择2
#define DI_EMERGENCY (1<<7)   // 急停

#define DO_RUN_CONTACT  (1<<0)  // 运行接触器
#define DO_FAULT_OUT    (1<<1)  // 故障输出继电器
#define DO_FAN          (1<<2)  // 风扇控制
#define DO_BRAKE        (1<<3)  // 制动电阻继电器
#define DO_RUN_LED      (1<<4)  // 运行指示灯
#define DO_FAULT_LED    (1<<5)  // 故障指示灯

extern uint8_t g_di_state;   // 当前DI状态（8位）
extern uint8_t g_do_state;   // 当前DO状态

uint8_t DRV_GPIO_GetDI(void);
void DRV_GPIO_SetDO(uint8_t mask, uint8_t value);
void DRV_GPIO_Init(void);

#endif
