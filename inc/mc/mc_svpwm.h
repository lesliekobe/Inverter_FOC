/**
 * @file mc_svpwm.h
 * @brief SVPWM空间矢量调制头文件
 */
#ifndef __MC_SVPWM_H
#define __MC_SVPWM_H

#include <stdint.h>

/* SVPWM输出结构体（三相PWM比较值） */
typedef struct {
    uint16_t CCR1;  /* TA通道 - U相 */
    uint16_t CCR2;  /* TB通道 - V相 */
    uint16_t CCR3;  /* TC通道 - W相 */
} SvpwmOut;

/* SVPWM内部状态 */
typedef struct {
    float Valpha;        /* α轴电压（来自反Park） */
    float Vbeta;         /* β轴电压（来自反Park） */
    float Udc;           /* 母线电压 */
    uint8_t sector;      /* 当前扇区 1~6 */
    float Ta;            /* 扇区内Ta时间 */
    float Tb;            /* 扇区内Tb时间 */
    float Tc;            /* 扇区内Tc时间 */
    uint16_t carr_period;/* PWM周期值（ARR寄存器） */
    uint8_t  init_ok;    /* 初始化标志 */
} SvpwmHandle;

extern SvpwmHandle g_svpwm;

/* 公共接口 */
void SVPWM_Init(SvpwmHandle *h, uint16_t carr_period, float udc);
void SVPWM_Calc(SvpwmHandle *h, float v_alpha, float v_beta, SvpwmOut *out);

#endif
