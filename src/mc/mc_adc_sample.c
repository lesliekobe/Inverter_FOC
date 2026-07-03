/**
 * @file mc_adc_sample.c
 * @brief ADC采样与物理量换算
 *
 * 将STM32 ADC原始12位数值转换为物理量（电流/电压/温度）
 * 使用DMA时在DMA完成中断中调用
 */
#include "drv_adc.h"
#include "global_ctx.h"
#include <math.h>

/* 电流传感器：ACS712-20A → 100mV/A，采样电阻10Ω，单片机3.3V REF
 * ADC值 = (电流*灵敏度 + 零点电压) / Vref * 4096
 * 零点电压 = 0.5 * Vref = 1650mV 对应 ADC=2048
 */
#define IA_ZERO_ADC     2048
#define IB_ZERO_ADC     2048
#define CURRENT_SENSE   0.040f  /* ACS712-20A: 40mV/A */
#define ADC_VREF        3.30f   /* 3.3V参考电压 */
#define ADC_RES         4096    /* 12位ADC */
#define RSENSE          0.01f   /* 10mΩ采样电阻，用于功率计算 */

/* 分压电阻：母线电压 540V → 3.3V */
#define UDC_R_TOP       47000.0f  /* 47kΩ上分压 */
#define UDC_R_BOTTOM    100.0f    /* 100Ω下分压 */
#define UDC_DIV_RATIO   (UDC_R_BOTTOM / (UDC_R_TOP + UDC_R_BOTTOM)) /* ~0.00212 */

/* NTC热敏电阻：B3950 10kΩ，25℃=10kΩ，典型工业IGBT模块温度检测 */
#define NTC_BETA        3950.0f
#define NTC_R0          10000.0f  /* 10kΩ @ 25℃ */
#define NTC_T0_K        298.15f   /* 25℃ in Kelvin */
#define NTC_R_PULLUP    10000.0f  /* 10kΩ上拉电阻 */

/**
 * MC_ADC_GetIa - 获取U相电流（安培）
 */
float MC_ADC_GetIa(void)
{
    int32_t raw = (int32_t)g_adc_raw.Ia_adc - IA_ZERO_ADC;
    float voltage = (float)raw * ADC_VREF / ADC_RES;
    return voltage / CURRENT_SENSE;
}

/**
 * MC_ADC_GetIb - 获取V相电流（安培）
 */
float MC_ADC_GetIb(void)
{
    int32_t raw = (int32_t)g_adc_raw.Ib_adc - IB_ZERO_ADC;
    float voltage = (float)raw * ADC_VREF / ADC_RES;
    return voltage / CURRENT_SENSE;
}

/**
 * MC_ADC_GetIc - 获取W相电流（安培，通过三相和为零计算）
 */
float MC_ADC_GetIc(void)
{
    return -(MC_ADC_GetIa() + MC_ADC_GetIb());
}

/**
 * MC_ADC_GetUdc - 获取母线电压（伏特）
 */
float MC_ADC_GetUdc(void)
{
    float voltage = (float)g_adc_raw.Udc_adc * ADC_VREF / ADC_RES;
    /* 分压电阻换算 */
    return voltage / UDC_DIV_RATIO;
}

/**
 * MC_ADC_GetNtcTemp - 获取模块温度（摄氏度）
 *
 * NTC电阻计算：R_ntc = R_pullup * (Vadc / (Vref - Vadc))
 * 温度换算：B常数方程：T = 1 / (1/T0 + (1/B)*ln(R/R0))
 */
float MC_ADC_GetNtcTemp(void)
{
    float Vadc = (float)g_adc_raw.Ntc_adc * ADC_VREF / ADC_RES;
    if (Vadc < 0.01f) Vadc = 0.01f; /* 防除零 */
    if (Vadc > ADC_VREF - 0.01f) Vadc = ADC_VREF - 0.01f;

    float R_ntc = NTC_R_PULLUP * (Vadc / (ADC_VREF - Vadc));
    float T_k = 1.0f / (1.0f/NTC_T0_K + (1.0f/NTC_BETA) * logf(R_ntc / NTC_R0));
    return T_k - 273.15f;
}

/**
 * MC_ADC_Sample - 采样同步（DMA回调中调用）
 *
 * 将g_adc_raw数据同步到g_ctx.adc_raw供上层使用
 */
void MC_ADC_Sample(void)
{
    g_ctx.adc_raw.Ia_adc   = g_adc_raw.Ia_adc;
    g_ctx.adc_raw.Ib_adc   = g_adc_raw.Ib_adc;
    g_ctx.adc_raw.Udc_adc  = g_adc_raw.Udc_adc;
    g_ctx.adc_raw.Ntc_adc  = g_adc_raw.Ntc_adc;
    g_ctx.adc_raw.AI1_adc  = g_adc_raw.AI1_adc;
    g_ctx.adc_raw.AI2_adc  = g_adc_raw.AI2_adc;
}
