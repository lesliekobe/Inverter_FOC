/**
 * @file mc_adc_sample.h
 * @brief ADC采样头文件
 */
#ifndef __MC_ADC_SAMPLE_H
#define __MC_ADC_SAMPLE_H

#include <stdint.h>

/* 获取物理量 */
float MC_ADC_GetIa(void);
float MC_ADC_GetIb(void);
float MC_ADC_GetIc(void);
float MC_ADC_GetUdc(void);
float MC_ADC_GetNtcTemp(void);

/**
 * MC_ADC_Sample - 采样同步到全局上下文
 * 在DMA完成中断或ADC中断中调用
 */
void MC_ADC_Sample(void);

#endif
