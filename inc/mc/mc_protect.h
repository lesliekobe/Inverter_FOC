/**
 * @file mc_protect.h
 * @brief 电机保护头文件
 */
#ifndef __MC_PROTECT_H
#define __MC_PROTECT_H

#include <stdint.h>

/* 公共接口 */
void MC_Protect_Init(void);

/**
 * MC_Protect_Check - 保护检测主函数
 * 调用时机：1ms定时任务
 */
void MC_Protect_Check(void);

/**
 * MC_Protect_FaultTrigger - 触发故障并封锁PWM
 */
void MC_Protect_FaultTrigger(uint16_t fault_code);

#endif
