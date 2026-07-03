/**
 * @file app_pid.c
 * @brief 通用离散PID控制器
 */
#include "app_pid.h"
#include <math.h>

/**
 * APP_PID_Init - 初始化PID
 */
void APP_PID_Init(PidCtrl *p)
{
    p->integral = 0.0f;
    p->prev_err = 0.0f;
    p->output   = 0.0f;
    p->enable   = 1;
}

/**
 * APP_PID_SetGains - 设置PID增益
 */
void APP_PID_SetGains(PidCtrl *p, float kp, float ki, float kd)
{
    p->kp = kp;
    p->ki = ki;
    p->kd = kd;
}

/**
 * APP_PID_Calc - 离散PID计算
 * @dt 单位：秒（如0.001=1ms）
 */
float APP_PID_Calc(PidCtrl *p, float setpoint, float feedback, float dt)
{
    if (!p->enable) return p->output;

    float err = setpoint - feedback;

    /* 比例项 */
    float p_term = p->kp * err;

    /* 积分项（防积分饱和） */
    p->integral += p->ki * err * dt;
    if (p->integral > p->out_max) p->integral = p->out_max;
    if (p->integral < p->out_min) p->integral = p->out_min;

    /* 微分项（一阶低通滤波避免微分突变） */
    float d_term = p->kd * (err - p->prev_err) / dt;
    /* 简单滤波：d_term = 0.9 * d_term_prev + 0.1 * new_d_term */
    static float d_term_filt = 0.0f;
    d_term_filt = 0.9f * d_term_filt + 0.1f * d_term;

    /* 总输出 */
    p->output = p_term + p->integral + d_term_filt;

    /* 输出限幅 */
    if (p->output > p->out_max) p->output = p->out_max;
    if (p->output < p->out_min) p->output = p->out_min;

    p->prev_err = err;
    return p->output;
}

/**
 * APP_PID_Reset - 复位PID（积分归零）
 */
void APP_PID_Reset(PidCtrl *p)
{
    p->integral = 0.0f;
    p->prev_err = 0.0f;
    p->output   = 0.0f;
}
