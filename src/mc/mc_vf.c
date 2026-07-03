/**
 * @file mc_vf.c
 * @brief V/F开环控制实现
 *
 * 适用场景：异步电机驱动，无需编码器
 * 电压跟随频率变化，低频时加电压补偿提升转矩
 */
#include "mc_vf.h"
#include "mc_svpwm.h"
#include "drv_adc.h"
#include "drv_pwm.h"
#include "global_ctx.h"
#include <math.h>

#define TWO_PI  6.2831853f

typedef struct {
    float theta;           /* 电角度积分累加 */
    float target_freq;     /* 目标频率 Hz */
    float current_freq;    /* 当前频率 Hz */
    float v_boost;         /* 低频转矩补偿电压 V */
    float rated_voltage;   /* 额定电压 V */
    float rated_freq;      /* 额定频率 Hz */
    uint8_t dir;           /* 方向 0=正转 1=反转 */
    uint8_t enable;        /* 使能标志 */
} VfHandle;

static VfHandle g_vf = {0};

/* V/F曲线表（频率点, 电压比例），默认5点 */
static const float g_vf_curve[][2] = {
    {5.0f,  0.15f},   /* 5Hz:  15%电压（补偿启动转矩） */
    {10.0f, 0.30f},   /* 10Hz: 30%电压 */
    {25.0f, 0.65f},   /* 25Hz: 65%电压 */
    {40.0f, 0.90f},   /* 40Hz: 90%电压 */
    {50.0f, 1.00f},   /* 50Hz: 100%额定电压 */
};

/**
 * VF_GetVoltageFromFreq - V/F曲线查表，获取目标电压
 * 使用线性插值计算任意频率点的电压值
 */
static float VF_GetVoltageFromFreq(float freq_hz)
{
    float voltage_ratio = 0.0f;
    uint8_t i;
    float f0, f1, v0, v1;

    /* 频率超出范围钳位 */
    if (freq_hz <= 0.0f) return 0.0f;
    if (freq_hz >= 50.0f) return g_vf.rated_voltage;

    /* 线性插值查表 */
    for (i = 0; i < (sizeof(g_vf_curve) / sizeof(g_vf_curve[0])) - 1; i++) {
        f0 = g_vf_curve[i][0];
        f1 = g_vf_curve[i+1][0];
        if (freq_hz >= f0 && freq_hz <= f1) {
            v0 = g_vf_curve[i][1];
            v1 = g_vf_curve[i+1][1];
            voltage_ratio = v0 + (v1 - v0) * (freq_hz - f0) / (f1 - f0);
            break;
        }
    }
    return voltage_ratio * g_vf.rated_voltage;
}

/**
 * MC_VF_Init - 初始化V/F控制
 */
void MC_VF_Init(void)
{
    memset(&g_vf, 0, sizeof(VfHandle));
    g_vf.rated_voltage = g_ctx.param.motor_voltage;
    g_vf.rated_freq    = g_ctx.param.motor_freq;
    g_vf.v_boost       = 20.0f; /* 20V低频补偿 */
    g_vf.enable        = 1;
}

/**
 * MC_VF_SetFrequency - 设置目标频率
 */
void MC_VF_SetFrequency(float freq_hz, uint8_t dir)
{
    g_vf.target_freq = freq_hz;
    g_vf.dir = dir;
}

/**
 * MC_VF_GetFrequency - 获取当前频率
 */
float MC_VF_GetFrequency(void)
{
    return g_vf.current_freq;
}

/**
 * MC_VF_Control - V/F主控制函数（10kHz PWM中断中调用）
 *
 * 1. 频率斜坡上升/下降（加减速时间）
 * 2. V/F曲线查电压
 * 3. 低频转矩补偿
 * 4. 生成Vα、Vβ
 * 5. SVPWM计算占空比
 */
void MC_VF_Control(void)
{
    if (!g_vf.enable) return;

    float dt = 0.0001f; /* 100us = 10kHz */
    SvpwmOut pwm_out;
    float Udc = DRV_ADC_GetUdc();

    /* ① 频率斜坡跟踪 */
    float ramp_rate = g_ctx.param.accel_time > 0.0f
        ? (g_ctx.param.motor_freq / g_ctx.param.accel_time) /* Hz/s */
        : 50.0f; /* 默认50Hz/s */

    if (g_vf.current_freq < g_vf.target_freq) {
        g_vf.current_freq += ramp_rate * dt;
        if (g_vf.current_freq > g_vf.target_freq)
            g_vf.current_freq = g_vf.target_freq;
    } else if (g_vf.current_freq > g_vf.target_freq) {
        g_vf.current_freq -= ramp_rate * dt;
        if (g_vf.current_freq < g_vf.target_freq)
            g_vf.current_freq = g_vf.target_freq;
    }

    /* ② V/F曲线查表 */
    float target_v = VF_GetVoltageFromFreq(g_vf.current_freq);

    /* ③ 低频转矩补偿（<10Hz时叠加补偿电压） */
    if (g_vf.current_freq < 10.0f) {
        float boost = g_vf.v_boost * (10.0f - g_vf.current_freq) / 10.0f;
        target_v += boost;
    }

    /* ④ 频率→角度积分（电气角速度） */
    float omega = TWO_PI * g_vf.current_freq; /* rad/s */
    g_vf.theta += omega * dt;
    if (g_vf.theta > TWO_PI) g_vf.theta -= TWO_PI;

    /* ⑤ 生成αβ轴电压（幅值=target_v，角度=theta） */
    float Valpha = target_v * cosf(g_vf.theta);
    float Vbeta  = target_v * sinf(g_vf.theta);

    /* ⑥ SVPWM计算 */
    SVPWM_Calc(&g_svpwm, Valpha, Vbeta, &pwm_out);

    /* ⑦ 写入PWM寄存器 */
    DRV_PWM_SetDuty(pwm_out.CCR1, pwm_out.CCR2, pwm_out.CCR3);
}
