#ifndef __DRV_ADC_H
#define __DRV_ADC_H

#include <stdint.h>

/* ADC原始采样结果 */
typedef struct {
    uint16_t Ia_adc;   // U相电流ADC原始值
    uint16_t Ib_adc;   // V相电流ADC原始值
    uint16_t Udc_adc;   // 直流母线电压ADC原始值
    uint16_t Ntc_adc;   // NTC温度传感器ADC原始值
    uint16_t AI1_adc;   // 模拟输入1 (0-10V / 4-20mA)
    uint16_t AI2_adc;   // 模拟输入2
} AdcRaw;

extern AdcRaw g_adc_raw;

void DRV_ADC_Init(void);
void DRV_ADC_Start(void);
void DRV_ADC_Stop(void);
float DRV_ADC_GetIa(void);       // 转换为电流值（安培）
float DRV_ADC_GetIb(void);
float DRV_ADC_GetUdc(void);      // 转换为电压值（伏特）
float DRV_ADC_GetNtcTemp(void);  // 转换为温度（摄氏度）
void DRV_ADC_DMA_Callback(void);  // 由DMA中断调用

#endif
