/**
 * @file drv_adc.c
 * @brief ADC驱动 - STM32F4硬件抽象层
 *
 * 使用ADC1+ADC2同步采样，通过DMA2传输数据。
 * 采样通道：
 *   CH0 (PA0) -> Ia  U相电流
 *   CH1 (PA1) -> Ib  V相电流
 *   CH2 (PA2) -> Udc 直流母线电压
 *   CH3 (PA3) -> NTC 温度传感器
 *   CH4 (PA4) -> AI1 模拟输入1
 *   CH5 (PA5) -> AI2 模拟输入2
 */

#include <drv_adc.h>
#include <stm32f4xx_hal.h>

/* DMA缓冲区 - 双缓冲设计 */
static uint16_t g_adc_dma_buf[6] __attribute__((aligned(4)));

/* ADC原始数据（由DMA回调更新） */
AdcRaw g_adc_raw = {0};

/* 数据就绪标志（供上层查询） */
static volatile uint8_t g_adc_data_ready = 0;

/* ========== 电流/电压转换参数（根据硬件设计调整）========== */
#define ADC_VREF         3.30f      // ADC参考电压 (V)
#define ADC_RESOLUTION   4096.0f    // 12位ADC: 2^12 = 4096

/* 电流传感器参数（以ACS712或类似20A传感器为例） */
#define CURRENT_SENSOR_VCC    3.30f   // 传感器供电电压
#define CURRENT_SENSOR_SENS  0.100f   // 灵敏度: 100mV/A (ACS712-20A)
#define CURRENT_SENSOR_ZERO  (CURRENT_SENSOR_VCC / 2.0f)  // 零点电压 1.65V

/* 电压分压网络参数 */
#define VDC_DIVIDER_R1   100.0f    // R1 = 100kΩ
#define VDC_DIVIDER_R2   10.0f    // R2 = 10kΩ
#define VDC_DIVIDER_RATIO  ((VDC_DIVIDER_R1 + VDC_DIVIDER_R2) / VDC_DIVIDER_R2)

/* NTC热敏电阻参数（10kΩ B=3950 NTC） */
#define NTC_BETA         3950.0f   // B常数
#define NTC_R0           10000.0f  // 25°C时的阻值 (Ω)
#define NTC_T0_K         298.15f   // 25°C转换为开尔文
#define NTC_SERIES_R     10000.0f  // 上拉电阻 (Ω)
#define NTC_ADC_MAX      4095.0f   // ADC最大值

/* ========== ADC内部状态 ========== */
static ADC_HandleTypeDef hadc1;
static DMA_HandleTypeDef hdma_adc1;

/**
 * @brief ADC初始化
 *
 * 配置流程：
 *  1. 使能ADC1/2时钟
 *  2. 配置GPIO (PA0~PA5为模拟输入)
 *  3. 配置ADC1为多通道扫描模式，DMA连续传输
 *  4. 配置DMA2 Stream0 Channel0
 *  5. 启动ADC自校准
 */
void DRV_ADC_Init(void)
{
    /* 使能ADC1/2时钟和GPIOA时钟 */
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_ADC2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* 配置GPIO: PA0~PA5 模拟输入（无需重映射，已是默认位置） */
    /* PA0: ADC1_IN0 -> Ia
       PA1: ADC1_IN1 -> Ib
       PA2: ADC1_IN2 -> Udc
       PA3: ADC1_IN3 -> NTC
       PA4: ADC1_IN4 -> AI1
       PA5: ADC1_IN5 -> AI2
    */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |
                           GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 配置ADC1参数 */
    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;  // 84MHz/4=21MHz
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;        // 12位分辨率
    hadc1.Init.ScanConvMode          = ENABLE;                     // 扫描模式（多通道）
    hadc1.Init.ContinuousConvMode    = ENABLE;                     // 连续转换
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion       = 6;                          // 6个通道
    hadc1.Init.DMAContinuousRequests = ENABLE;                     // DMA连续请求
    hadc1.Init.EOCSelection          = ADC_EOC_SEQ_CONV;          // 序列结束才置EOC
    HAL_ADC_Init(&hadc1);

    /* 配置ADC通道参数：采样时间479周期（最慢但最精确）
       通道顺序: IN0(Ia), IN1(Ib), IN2(Udc), IN3(NTC), IN4(AI1), IN5(AI2)
    */
    ADC_ChannelConfTypeDef sConfig = {0};

    /* 通道0: PA0 - Ia */
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank    = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_479CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /* 通道1: PA1 - Ib */
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank    = 2;
    sConfig.SamplingTime = ADC_SAMPLETIME_479CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /* 通道2: PA2 - Udc */
    sConfig.Channel = ADC_CHANNEL_2;
    sConfig.Rank    = 3;
    sConfig.SamplingTime = ADC_SAMPLETIME_479CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /* 通道3: PA3 - NTC */
    sConfig.Channel = ADC_CHANNEL_3;
    sConfig.Rank    = 4;
    sConfig.SamplingTime = ADC_SAMPLETIME_479CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /* 通道4: PA4 - AI1 */
    sConfig.Channel = ADC_CHANNEL_4;
    sConfig.Rank    = 5;
    sConfig.SamplingTime = ADC_SAMPLETIME_479CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /* 通道5: PA5 - AI2 */
    sConfig.Channel = ADC_CHANNEL_5;
    sConfig.Rank    = 6;
    sConfig.SamplingTime = ADC_SAMPLETIME_479CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /* 配置DMA2 Stream0 */
    hdma_adc1.Instance                 = DMA2_Stream0;
    hdma_adc1.Init.Channel             = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction           = DMA_PERIPH_TO_MEMORY;   // 外设到内存
    hdma_adc1.Init.PeriphInc           = DMA_PINC_DISABLE;       // 外设地址不递增
    hdma_adc1.Init.MemInc              = DMA_MINC_ENABLE;         // 内存地址递增
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD; // 16位半字
    hdma_adc1.Init.MemDataAlignment    = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode                = DMA_CIRCULAR;            // 循环模式
    hdma_adc1.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_adc1.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_adc1);

    /* 将DMA与ADC关联 */
    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);

    /* 使能DMA传输完成中断 */
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

    /* 启动ADC自校准（减少偏置误差） */
    HAL_ADCEx_Calibration_Start(&hadc1);
}

/**
 * @brief 启动ADC转换
 */
void DRV_ADC_Start(void)
{
    g_adc_data_ready = 0;
    /* 启动DMA循环传输 + ADC连续转换 */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)g_adc_dma_buf, 6);
}

/**
 * @brief 停止ADC转换
 */
void DRV_ADC_Stop(void)
{
    HAL_ADC_Stop_DMA(&hadc1);
    g_adc_data_ready = 0;
}

/**
 * @brief 获取U相电流（安培）
 * 转换公式：I = (V_adc - V_zero) / Sensitivity
 * V_adc = Raw * VREF / 4096
 */
float DRV_ADC_GetIa(void)
{
    float v_adc = (float)g_adc_raw.Ia_adc * ADC_VREF / ADC_RESOLUTION;
    float i_a   = (v_adc - CURRENT_SENSOR_ZERO) / CURRENT_SENSOR_SENS;
    return i_a;
}

/**
 * @brief 获取V相电流（安培）
 */
float DRV_ADC_GetIb(void)
{
    float v_adc = (float)g_adc_raw.Ib_adc * ADC_VREF / ADC_RESOLUTION;
    float i_b   = (v_adc - CURRENT_SENSOR_ZERO) / CURRENT_SENSOR_SENS;
    return i_b;
}

/**
 * @brief 获取直流母线电压（伏特）
 * 转换公式：V_dc = V_adc * (R1+R2)/R2
 */
float DRV_ADC_GetUdc(void)
{
    float v_adc = (float)g_adc_raw.Udc_adc * ADC_VREF / ADC_RESOLUTION;
    return v_adc * VDC_DIVIDER_RATIO;
}

/**
 * @brief 获取NTC温度（摄氏度）
 * 使用Beta方程：T = 1 / (1/T0 + (1/B)*ln(R/R0))
 * 其中 R = R_series * V_adc / (Vcc - V_adc)
 */
float DRV_ADC_GetNtcTemp(void)
{
    float v_adc = (float)g_adc_raw.Ntc_adc * ADC_VREF / NTC_ADC_MAX;

    /* 防止除零和越界 */
    if (v_adc < 0.001f) v_adc = 0.001f;
    if (v_adc > (ADC_VREF - 0.001f)) v_adc = ADC_VREF - 0.001f;

    /* 计算NTC电阻值 */
    float r_ntc = NTC_SERIES_R * v_adc / (ADC_VREF - v_adc);

    /* Beta方程计算温度 */
    float temp_k = 1.0f / ( (1.0f / NTC_T0_K) +
                            (1.0f / NTC_BETA) * logf(r_ntc / NTC_R0) );
    float temp_c = temp_k - 273.15f;

    return temp_c;
}

/**
 * @brief DMA传输完成回调
 * 将DMA缓冲区数据复制到全局结构体
 */
void DRV_ADC_DMA_Callback(void)
{
    /* 复制DMA缓冲区到全局变量 */
    g_adc_raw.Ia_adc   = g_adc_dma_buf[0];
    g_adc_raw.Ib_adc   = g_adc_dma_buf[1];
    g_adc_raw.Udc_adc  = g_adc_dma_buf[2];
    g_adc_raw.Ntc_adc  = g_adc_dma_buf[3];
    g_adc_raw.AI1_adc  = g_adc_dma_buf[4];
    g_adc_raw.AI2_adc  = g_adc_dma_buf[5];

    g_adc_data_ready = 1;
}

/**
 * @brief DMA2 Stream0中断处理（由bsp_interrupt.c调用）
 */
void DMA2_Stream0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_adc1);
}
