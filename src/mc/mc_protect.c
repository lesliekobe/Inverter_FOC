/**
 * @file mc_protect.c
 * @brief 电机保护模块
 *
 * 包含：过流、过压、欠压、过温、堵转、反时限过载
 * 保护触发后封锁PWM并记录故障
 */
#include "mc_protect.h"
#include "drv_adc.h"
#include "drv_pwm.h"
#include "drv_gpio.h"
#include "global_ctx.h"
#include "fault_mgr.h"

/* 反时限过载曲线参数（I²t） */
#define OVERLOAD_TC     60.0f    /* 100%过载时60s跳闸 */
#define OVERLOAD_BASE   1.20f    /* 120%额定电流开始计时 */

/* 堵转检测 */
#define LOCKED_TIMEOUT  2.0f     /* 2秒堵转判定 */
#define LOCKED_I_THRESH 0.80f    /* 80%额定电流且转速=0 */

/* 欠压阈值 */
#define UDC_UNDERVolt   350.0f   /* V */
#define UDC_OVERVolt    750.0f   /* V */

typedef struct {
    float overload_integral;  /* 过载积分（I²t） */
    float locked_time;        /* 堵转计时 */
    uint8_t overload_active;  /* 过载计时中 */
} ProtectHandle;

static ProtectHandle g_prot = {0};

/**
 * MC_Protect_Init - 初始化保护模块
 */
void MC_Protect_Init(void)
{
    memset(&g_prot, 0, sizeof(ProtectHandle));
}

/**
 * MC_Protect_FaultTrigger - 触发故障停机
 */
void MC_Protect_FaultTrigger(uint16_t fault_code)
{
    /* 封锁PWM输出（安全状态） */
    DRV_PWM_Disable();

    /* 记录故障到全局上下文和故障管理器 */
    g_ctx.fault_code = fault_code;
    g_ctx.run_state  = RUN_STATE_FAULT;
    FaultMgr_Record(fault_code);

    /* 故障继电器输出 */
    DRV_GPIO_SetDO(DO_FAULT_OUT, 1);

#ifdef DEBUG
    /* 调试信息 */
    // printf("[PROTECT] Fault=%d, Ia=%.2f, Ib=%.2f, Udc=%.1f\r\n",
    //        fault_code, MC_ADC_GetIa(), MC_ADC_GetIb(), MC_ADC_GetUdc());
#endif
}

/**
 * MC_Protect_Check - 保护检测主函数
 *
 * 调用时机：1ms定时任务（优先级低于电流环）
 */
void MC_Protect_Check(void)
{
    float dt = 0.001f; /* 1ms */
    float I_u = fabsf(MC_ADC_GetIa());
    float I_v = fabsf(MC_ADC_GetIb());
    float Imax = I_u > I_v ? I_u : I_v;
    float Udc  = MC_ADC_GetUdc();
    float Temp = MC_ADC_GetNtcTemp();
    float I_rated = g_ctx.param.motor_current;
    float speed = g_ctx.motor_out.speed; /* 转速反馈 */

    /* ① 瞬时过流保护（优先级最高） */
    if (Imax > g_ctx.param.prot_i_max) {
        MC_Protect_FaultTrigger(FAULT_OVERCURRENT);
        return;
    }

    /* ② 母线过压保护 */
    if (Udc > UDC_OVER_VOLT) {
        MC_Protect_FaultTrigger(FAULT_OVERVOLTAGE);
        return;
    }

    /* ③ 母线欠压保护 */
    if (Udc < UDC_UNDER_VOLT) {
        MC_Protect_FaultTrigger(FAULT_UNDERVOLTAGE);
        return;
    }

    /* ④ IGBT模块过温保护 */
    if (Temp > g_ctx.param.prot_temp_max) {
        MC_Protect_FaultTrigger(FAULT_OVERHEAT);
        return;
    }

    /* ⑤ 反时限过载保护（I²t曲线） */
    if (Imax > OVERLOAD_BASE * I_rated) {
        float I_excess = Imax / I_rated - OVERLOAD_BASE;
        g_prot.overload_integral += I_excess * I_excess * dt / OVERLOAD_TC;
        g_prot.overload_active = 1;
    } else {
        /* 低于阈值，积分消退 */
        if (g_prot.overload_integral > 0.0f) {
            g_prot.overload_integral -= dt * 0.5f; /* 消退速率 */
            if (g_prot.overload_integral < 0.0f) g_prot.overload_integral = 0.0f;
        }
    }
    if (g_prot.overload_integral > 1.0f) {
        MC_Protect_FaultTrigger(FAULT_OVERLOAD);
        return;
    }

    /* ⑥ 堵转检测 */
    if (Imax > LOCKED_I_THRESH * I_rated && fabsf(speed) < 5.0f) {
        g_prot.locked_time += dt;
        if (g_prot.locked_time > LOCKED_TIMEOUT) {
            MC_Protect_FaultTrigger(FAULT_LOCKED_ROTOR);
            return;
        }
    } else {
        g_prot.locked_time = 0.0f;
    }
}
