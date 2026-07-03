/**
 * @file mc_foc.h
 * @brief FOC矢量控制核心算法头文件
 */
#ifndef __MC_FOC_H
#define __MC_FOC_H

#include <stdint.h>

/* FOC核心句柄 */
typedef struct {
    /* 电流采样 */
    float Ia;
    float Ib;
    float Ic;

    /* dq轴电流 */
    float Id;
    float Iq;
    float Id_ref;
    float Iq_ref;

    /* dq轴电压 */
    float Vd;
    float Vq;

    /* 角度与转速 */
    float theta;       /* 电角度(rad)，范围[0, 2π] */
    float speed_rpm;   /* 电机机械转速RPM */
    float speed_rads;  /* 电机电气角速度rad/s */

    /* dq轴电压输出 */
    float Vd_out;
    float Vq_out;

    /* 控制模式 */
    uint8_t ctrl_mode; /* 0=V/F, 1=FOC电流环, 2=FOC速度环 */

    /* 编码器 */
    int32_t enc_position;

    /* 中间变量（调试用） */
    float Ialpha;
    float Ibeta;
    float Valpha;
    float Vbeta;

    uint8_t init_ok;
} FocHandle;

extern FocHandle g_foc;

/* 公共接口 */
void MC_FOC_Init(void);
void MC_FOC_SetIdq(float id_ref, float iq_ref);
void MC_FOC_SetTheta(float theta);
void MC_FOC_SetSpeed(float speed_rpm);

/**
 * MC_FOC_CurrentLoop - 电流环主函数
 * 调用时机：PWM周期中断（10kHz）
 * 在TIM1_UP_IRQHandler中被调用
 */
void MC_FOC_CurrentLoop(void);

/**
 * MC_FOC_SpeedLoop - 速度环主函数
 * 调用时机：1ms软件定时任务
 */
void MC_FOC_SpeedLoop(void);

#endif
