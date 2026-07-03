/**
 * @file mc_svpwm.c
 * @brief SVPWM空间矢量调制实现
 *
 * 七段式对称SVPWM，谐波含量低，工业变频器标准算法
 * 支持过调制处理（电压矢量超出内切圆时钳位）
 */
#include "mc_svpwm.h"
#include <math.h>
#include <stdlib.h>

#define SQRT3           1.7320508f
#define SQRT3_2         0.8660254f
#define SQRT3_6         0.2886751f
#define TWO_PI          6.2831853f
#define INV_SQRT3       0.5773503f

SvpwmHandle g_svpwm = {0};

/* 死区时间（计数周期，STM32F4下1格≈13.9ns） */
#define DEADTIME_TICKS  90   /* ~1.25us死区 */

/**
 * SVPWM_GetSector - 扇区判断
 * 输入：Vα, Vβ（α轴朝上，β轴朝右的直角坐标系）
 * 输出：扇区号 1~6
 *
 * 判断原理：六个扇区的边界线方程
 *   扇区1: Vβ>=0 且 c<0 且 b>=0
 *   扇区2: Vβ>=0 且 c>=0
 *   扇区3: b<0 且 c>=0
 *   扇区4: Vβ<0 且 b<0
 *   扇区5: Vβ<0 且 c<0
 *   扇区6: b>=0 且 c<0
 */
static uint8_t SVPWM_GetSector(float v_alpha, float v_beta)
{
    uint8_t sector;
    float a, b, c;

    a = v_beta;
    b = SQRT3_2 * v_alpha - 0.5f * v_beta;
    c = -SQRT3_2 * v_alpha - 0.5f * v_beta;

    if (a >= 0)  sector = 1; else sector = 0;
    if (c >= 0)  sector += 2;
    if (b < 0)   sector += 4;

    return sector + 1;
}

/**
 * SVPWM_CalcTimes - 计算各矢量作用时间
 *
 * 扇区与基本矢量对应关系：
 *   扇区1: U0(100) + U1(110) + U7(111)  0°~60°
 *   扇区2: U1(110) + U2(010) + U7       60°~120°
 *   扇区3: U2(010) + U3(011) + U7       120°~180°
 *   扇区4: U3(011) + U4(001) + U7       180°~240°
 *   扇区5: U4(001) + U5(101) + U7       240°~300°
 *   扇区6: U5(101) + U0(100) + U7       300°~360°
 */
static void SVPWM_CalcTimes(SvpwmHandle *h)
{
    float Ta, Tb, Tc;
    float pwm_half = (float)h->carr_period * 0.5f;
    float Vref_a = h->Valpha / (h->Udc * 0.5f);
    float Vref_b = h->Vbeta  / (h->Udc * 0.5f);

    /* 通用公式 */
    Ta = SQRT3_6 * h->carr_period * (SQRT3 * Vref_a - Vref_b);
    Tb = SQRT3_6 * h->carr_period * (SQRT3 * Vref_a + Vref_b);
    Tc = SQRT3_6 * h->carr_period * (-2.0f * Vref_b);

    /* 根据扇区分配到Ta/Tb/Tc */
    switch (h->sector) {
        case 1: h->Ta = Tc; h->Tb = Ta; h->Tc = Tb; break;
        case 2: h->Ta = Tb; h->Tb = Tc; h->Tc = Ta; break;
        case 3: h->Ta = Ta; h->Tb = Tc; h->Tc = Tb; break;
        case 4: h->Ta = Tb; h->Tb = Ta; h->Tc = Tc; break;
        case 5: h->Ta = Ta; h->Tb = Tb; h->Tc = Tc; break;
        case 6: h->Ta = Tc; h->Tb = Tb; h->Tc = Ta; break;
        default: h->Ta = h->Tb = h->Tc = 0.0f; break;
    }

    (void)Ta; (void)Tb; (void)Tc; /* suppress warning */
    (void)pwm_half;
}

/**
 * SVPWM_Init - 初始化SVPWM模块
 */
void SVPWM_Init(SvpwmHandle *h, uint16_t carr_period, float udc)
{
    h->carr_period = carr_period;
    h->Udc = udc;
    h->sector = 1;
    h->Valpha = 0.0f;
    h->Vbeta  = 0.0f;
    h->Ta = h->Tb = h->Tc = 0.0f;
    h->init_ok = 1;
}

/**
 * SVPWM_Calc - 核心计算函数
 * @param h      SVPWM句柄
 * @param v_alpha α轴电压（来自反Park变换）
 * @param v_beta  β轴电压（来自反Park变换）
 * @param out     三相PWM比较值输出
 *
 * 调用时机：每个PWM周期（10~16kHz），在PWM中断中调用
 */
void SVPWM_Calc(SvpwmHandle *h, float v_alpha, float v_beta, SvpwmOut *out)
{
    float T1, T2, T0;
    float t_a, t_b, t_c;
    float pwm_half;

    if (!h->init_ok) return;

    h->Valpha = v_alpha;
    h->Vbeta  = v_beta;
    pwm_half  = (float)h->carr_period * 0.5f;

    /* ① 确定扇区 */
    h->sector = SVPWM_GetSector(v_alpha, v_beta);

    /* ② 计算各矢量作用时间 */
    SVPWM_CalcTimes(h);

    T1 = h->Ta;
    T2 = h->Tb;
    T0 = (float)h->carr_period - T1 - T2;

    /* ③ 七段式对称调制：T0分两份首尾对称插入
     *   T0/4  T1/2  T2/2  T7  T2/2  T1/2  T0/4
     *   载波三角波中点对齐，形成中心对齐PWM
     */

    /* ④ 计算三相占空比分子（center-aligned对称式） */
    t_a = pwm_half - (h->Ta + h->Tb) * 0.5f;
    t_b = pwm_half - (h->Tb + h->Tc) * 0.5f;
    t_c = pwm_half - (h->Tc + h->Ta) * 0.5f;

    /* ⑤ 过调制处理：超出内切圆时等比例钳位 */
    float v_mag = sqrtf(v_alpha * v_alpha + v_beta * v_beta);
    float v_max = h->Udc * INV_SQRT3; /* 内切圆半径 = Udc/√3 */
    if (v_mag > v_max) {
        float k = v_max / v_mag;
        v_alpha *= k;
        v_beta  *= k;
        h->Valpha = v_alpha;
        h->Vbeta  = v_beta;
        h->sector = SVPWM_GetSector(v_alpha, v_beta);
        SVPWM_CalcTimes(h);
        T1 = h->Ta;
        T2 = h->Tb;
        T0 = (float)h->carr_period - T1 - T2;
        t_a = pwm_half - (h->Ta + h->Tb) * 0.5f;
        t_b = pwm_half - (h->Tb + h->Tc) * 0.5f;
        t_c = pwm_half - (h->Tc + h->Ta) * 0.5f;
    }

    /* ⑥ 转换为CCR寄存器值 */
    out->CCR1 = (uint16_t)fmaxf(1.0f, fminf(t_a, (float)h->carr_period - 1));
    out->CCR2 = (uint16_t)fmaxf(1.0f, fminf(t_b, (float)h->carr_period - 1));
    out->CCR3 = (uint16_t)fmaxf(1.0f, fminf(t_c, (float)h->carr_period - 1));

    /* ⑦ 过零点保护（下限死区时间） */
    #define DT DEADTIME_TICKS
    if (out->CCR1 < DT) out->CCR1 = DT;
    if (out->CCR2 < DT) out->CCR2 = DT;
    if (out->CCR3 < DT) out->CCR3 = DT;
    #undef DT
}
