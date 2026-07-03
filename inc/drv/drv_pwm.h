#ifndef __DRV_PWM_H
#define __DRV_PWM_H

#include <stdint.h>

/* DRV PWM handle */
typedef struct {
    uint32_t freq_hz;        // PWM频率 (Hz)
    uint16_t arr_period;     // ARR寄存器值
    uint8_t  enable;         // PWM使能标志
    uint8_t  fault_mask;     // 故障屏蔽状态
} DrvPwmHandle;

extern DrvPwmHandle g_pwm;

void DRV_PWM_Init(uint32_t freq_hz);
void DRV_PWM_SetDuty(uint16_t ch1, uint16_t ch2, uint16_t ch3);  // 三相占空比
void DRV_PWM_Enable(void);
void DRV_PWM_Disable(void);  // 所有输出拉低（安全状态）
void DRV_PWM_SetFaultMask(uint8_t mask);

#endif
