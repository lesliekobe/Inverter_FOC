/**
 * @file app_speed.c
 * @brief 速度给定与加减速控制
 */
#include "app_speed.h"
#include "global_ctx.h"
#include <math.h>

#define SOURCE_PANEL     0
#define SOURCE_AI        1
#define SOURCE_MODBUS    2
#define SOURCE_MULTISPD  3

AppSpeedHandle g_speed = {0};

/**
 * APP_Speed_Init - 初始化速度控制
 */
void APP_Speed_Init(void)
{
    memset(&g_speed, 0, sizeof(AppSpeedHandle));
    g_speed.current_freq = 0.0f;
    g_speed.target_freq  = 0.0f;
    g_speed.accel_rate   = 10.0f;  /* 10Hz/s 默认 */
    g_speed.decel_rate   = 10.0f;
    g_speed.dir          = 0;
    g_speed.source       = SOURCE_PANEL;
}

/**
 * APP_Speed_SetTarget - 设置目标频率
 */
void APP_Speed_SetTarget(float freq, uint8_t dir, uint8_t source)
{
    /* 来源优先级：MODBUS > MULTISPD > AI > PANEL */
    if (source > g_speed.source) {
        g_speed.target_freq = freq;
        g_speed.dir = dir;
        g_speed.source = source;
    }
}

/**
 * APP_Speed_SetRamp - 设置加减速时间
 */
void APP_Speed_SetRamp(float accel_s, float decel_s)
{
    if (accel_s > 0.1f)
        g_speed.accel_rate = g_ctx.param.motor_freq / accel_s;
    if (decel_s > 0.1f)
        g_speed.decel_rate = g_ctx.param.motor_freq / decel_s;
}

/**
 * APP_Speed_Task - 速度斜坡更新（1ms调用）
 */
void APP_Speed_Task(void)
{
    float dt = 0.001f;
    float freq_max = g_ctx.param.freq_max;
    float freq_min = g_ctx.param.freq_min;

    /* 钳位目标频率 */
    if (g_speed.target_freq > freq_max) g_speed.target_freq = freq_max;
    if (g_speed.target_freq < freq_min) g_speed.target_freq = freq_min;

    /* 斜坡跟踪 */
    if (g_speed.current_freq < g_speed.target_freq) {
        float delta = g_speed.accel_rate * dt;
        if (g_speed.current_freq + delta > g_speed.target_freq)
            g_speed.current_freq = g_speed.target_freq;
        else
            g_speed.current_freq += delta;
    } else if (g_speed.current_freq > g_speed.target_freq) {
        float delta = g_speed.decel_rate * dt;
        if (g_speed.current_freq - delta < g_speed.target_freq)
            g_speed.current_freq = g_speed.target_freq;
        else
            g_speed.current_freq -= delta;
    }

    /* 同步到全局上下文 */
    g_ctx.motor_out.freq = g_speed.current_freq;
}

/**
 * APP_Speed_GetCurrentFreq - 获取当前频率
 */
float APP_Speed_GetCurrentFreq(void)
{
    return g_speed.current_freq;
}
