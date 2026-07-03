/**
 * @file mc_foc.c
 * @brief FOC矢量控制核心算法实现
 *
 * 包含：Clarke变换、Park变换、反Park变换、电流环PI、速度环PI
 * 调用周期：电流环10kHz（ PWM中断），速度环1kHz（1ms任务）
 */
#include "mc_foc.h"
#include "mc_svpwm.h"
#include "drv_adc.h"
#include "global_ctx.h"
#include "global_def.h"
#include <math.h>

#define SQRT3           1.7320508f
#define SQRT3_2         0.8660254f
#define TWO_PI          6.2831853f

/* PI调节器参数（电流环） */
#define KI_KP           0.15f
#define KI_KI           0.002f
#define KI_OUT_MAX      (g_ctx.param.vbus_nom * 0.577f)  // Udc/√3

/* PI调节器参数（速度环） */
#define SPEED_KP        0.08f
#define SPEED_KI        0.001f
#define SPEED_OUT_MAX   (g_ctx.param.motor_i_max * 0.9f)

FocHandle g_foc = {0};

/* 离散PI控制器（带积分限幅+输出限幅） */
static float PI_Control(float ref, float fb, float kp, float ki,
                        float out_max, float dt, float *integral, float *prev_err)
{
    float err = ref - fb;
    /* 比例项 */
    float p_term = kp * err;
    /* 积分项 */
    float i_term = ki * err * dt;
    /* 积分累加（防积分饱和） */
    *integral += i_term;
    if (*integral > out_max * 0.8f) *integral = out_max * 0.8f;
    if (*integral < -out_max * 0.8f) *integral = -out_max * 0.8f;
    /* 总输出 */
    float output = p_term + *integral;
    /* 输出限幅 */
    if (output > out_max) output = out_max;
    if (output < -out_max) output = -out_max;
    *prev_err = err;
    return output;
}

/**
 * Clarke变换：三相电流 → αβ坐标系
 * Ia + Ib + Ic = 0（对称三相）
 * Iα = Ia
 * Iβ = (Ia + 2*Ib) / √3
 */
static void Clarke_Transform(float Ia, float Ib, float *Ialpha, float *Ibeta)
{
    *Ialpha = Ia;
    *Ibeta  = (Ia + 2.0f * Ib) / SQRT3;
}

/**
 * Park变换：αβ → dq旋转坐标系
 * d轴：励磁电流（与磁链方向对齐）
 * q轴：转矩电流（与磁链垂直）
 */
static void Park_Transform(float Ialpha, float Ibeta, float theta,
                           float *Id, float *Iq)
{
    float cos_t = cosf(theta);
    float sin_t = sinf(theta);
    *Id =  Ialpha * cos_t + Ibeta * sin_t;
    *Iq = -Ialpha * sin_t + Ibeta * cos_t;
}

/**
 * 反Park变换：dq → αβ静止坐标系
 */
static void Inv_Park_Transform(float Vd, float Vq, float theta,
                               float *Valpha, float *Vbeta)
{
    float cos_t = cosf(theta);
    float sin_t = sinf(theta);
    *Valpha = Vd * cos_t - Vq * sin_t;
    *Vbeta  = Vd * sin_t + Vq * cos_t;
}

/* 内部PI状态（电流环d轴） */
static float g_Id_integral = 0.0f;
static float g_Id_prev_err = 0.0f;

/* 内部PI状态（电流环q轴） */
static float g_Iq_integral = 0.0f;
static float g_Iq_prev_err = 0.0f;

/* 速度环PI状态 */
static float g_speed_integral = 0.0f;
static float g_speed_prev_err = 0.0f;

/**
 * MC_FOC_Init - 初始化FOC模块
 */
void MC_FOC_Init(void)
{
    memset(&g_foc, 0, sizeof(FocHandle));
    g_foc.ctrl_mode = 0; /* 默认V/F模式，等待切FOC */
    g_Id_integral = 0.0f;
    g_Iq_integral = 0.0f;
    g_speed_integral = 0.0f;
    g_foc.init_ok = 1;
}

/**
 * MC_FOC_SetIdq - 设置d/q轴电流给定（速度环调用）
 */
void MC_FOC_SetIdq(float id_ref, float iq_ref)
{
    g_foc.Id_ref = id_ref;
    g_foc.Iq_ref = iq_ref;
}

/**
 * MC_FOC_SetTheta - 设置电角度（编码器或估算器调用）
 */
void MC_FOC_SetTheta(float theta)
{
    g_foc.theta = theta;
}

/**
 * MC_FOC_SetSpeed - 设置电机目标转速RPM
 */
void MC_FOC_SetSpeed(float speed_rpm)
{
    g_foc.speed_rpm = speed_rpm;
}

/**
 * MC_FOC_CurrentLoop - 电流环（10kHz，PWM周期中断中调用）
 *
 * 执行顺序：
 * 1. 读取三相电流 Ia, Ib
 * 2. Clarke变换 → Iα, Iβ
 * 3. Park变换 → Id, Iq
 * 4. d轴电流PI → Vd
 * 5. q轴电流PI → Vq
 * 6. 反Park变换 → Vα, Vβ
 * 7. SVPWM计算占空比
 * 8. 写入TIM1 CCR寄存器
 */
void MC_FOC_CurrentLoop(void)
{
    if (!g_foc.init_ok) return;
    if (g_foc.ctrl_mode == 0) return; /* V/F模式走MC_VF_Control */

    float dt = 0.0001f; /* 100us = 10kHz */
    SvpwmOut pwm_out;

    /* ① 读取采样电流 */
    g_foc.Ia = DRV_ADC_GetIa();
    g_foc.Ib = DRV_ADC_GetIb();
    g_foc.Ic = -(g_foc.Ia + g_foc.Ib); /* 第三相电流计算得出 */

    /* ② Clarke变换 */
    Clarke_Transform(g_foc.Ia, g_foc.Ib, &g_foc.Ialpha, &g_foc.Ibeta);

    /* ③ Park变换 */
    Park_Transform(g_foc.Ialpha, g_foc.Ibeta, g_foc.theta,
                   &g_foc.Id, &g_foc.Iq);

    /* ④ d轴电流PI（励磁电流） */
    g_foc.Vd = PI_Control(
        g_foc.Id_ref, g_foc.Id,
        KI_KP, KI_KI,
        KI_OUT_MAX, dt,
        &g_Id_integral, &g_Id_prev_err
    );

    /* ⑤ q轴电流PI（转矩电流） */
    g_foc.Vq = PI_Control(
        g_foc.Iq_ref, g_foc.Iq,
        KI_KP, KI_KI,
        KI_OUT_MAX, dt,
        &g_Iq_integral, &g_Iq_prev_err
    );

    /* ⑥ 反Park变换 */
    Inv_Park_Transform(g_foc.Vd, g_foc.Vq, g_foc.theta,
                       &g_foc.Valpha, &g_foc.Vbeta);

    /* ⑦ SVPWM计算 */
    float Udc = DRV_ADC_GetUdc();
    SVPWM_Calc(&g_svpwm, g_foc.Valpha, g_foc.Vbeta, &pwm_out);

    /* ⑧ 写入PWM寄存器 */
    DRV_PWM_SetDuty(pwm_out.CCR1, pwm_out.CCR2, pwm_out.CCR3);
}

/**
 * MC_FOC_SpeedLoop - 速度环（1kHz，1ms任务中调用）
 *
 * 速度环PI → Iq_ref（转矩电流给定）
 * Id_ref = 0（表贴式永磁同步电机）
 * 弱磁区间：提高转速时自动降低Id进行弱磁
 */
void MC_FOC_SpeedLoop(void)
{
    if (!g_foc.init_ok) return;

    float dt = 0.001f; /* 1ms */
    float speed_fb = g_foc.speed_rpm; /* 从编码器或估算器获取 */
    float speed_ref = g_foc.speed_rpm; /* 目标转速 */

    /* 速度PI输出转矩电流 */
    float iq_ref = PI_Control(
        speed_ref, speed_fb,
        SPEED_KP, SPEED_KI,
        SPEED_OUT_MAX, dt,
        &g_speed_integral, &g_speed_prev_err
    );

    /* 弱磁控制：转速超过基速时，Id负向弱磁 */
    float id_ref = 0.0f;
    float base_speed = g_ctx.param.motor_rated_rpm; /* 额定转速 */
    if (fabsf(speed_ref) > base_speed * 1.1f) {
        /* 简单弱磁：按比例降低Id */
        float excess = (fabsf(speed_ref) - base_speed * 1.1f) / (base_speed * 0.5f);
        id_ref = -excess * g_foc.Id * 0.5f;
        if (id_ref < -g_foc.Id * 0.8f) id_ref = -g_foc.Id * 0.8f;
    }

    /* 下发电流给定 */
    MC_FOC_SetIdq(id_ref, iq_ref);
}
