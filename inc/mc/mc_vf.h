/**
 * @file mc_vf.h
 * @brief V/F开环控制头文件
 */
#ifndef __MC_VF_H
#define __MC_VF_H

#include <stdint.h>

/* V/F公共接口 */
void MC_VF_Init(void);
void MC_VF_SetFrequency(float freq_hz, uint8_t dir);
float MC_VF_GetFrequency(void);

/**
 * MC_VF_Control - V/F主控制函数
 * 调用时机：10kHz PWM中断（V/F模式下替代MC_FOC_CurrentLoop）
 */
void MC_VF_Control(void);

#endif
