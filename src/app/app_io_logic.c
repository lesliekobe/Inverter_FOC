/**
 * @file app_io_logic.c
 * @brief 数字输入输出逻辑处理
 */
#include "app_io_logic.h"
#include "drv_gpio.h"
#include "drv_adc.h"
#include "global_ctx.h"
#include "app_speed.h"

static uint8_t s_last_di = 0;
static uint8_t s_run_cmd_pending = 0;
static uint8_t s_stop_cmd_pending = 0;
static float s_fan_on_time = 0.0f;

/**
 * APP_IO_Task - DI/DO逻辑处理（1ms调用）
 */
void APP_IO_Task(void)
{
    float dt = 0.001f;
    uint8_t di = DRV_GPIO_GetDI();
    uint8_t di_changed = di ^ s_last_di;
    s_last_di = di;

    /* ===== 数字输入处理 ===== */

    /* 启动按钮（上升沿检测） */
    if ((di & DI_START) && (di_changed & DI_START)) {
        s_run_cmd_pending = 1;
    }
    /* 停止按钮（上升沿检测） */
    if ((di & DI_STOP) && (di_changed & DI_STOP)) {
        s_stop_cmd_pending = 1;
    }
    /* 急停 */
    if ((di & DI_EMERGENCY) && (di_changed & DI_EMERGENCY)) {
        g_ctx.run_state = RUN_STATE_FAULT;
        g_ctx.fault_code = FAULT_EMERGENCY;
        s_run_cmd_pending = 0;
        s_stop_cmd_pending = 0;
    }
    /* 外部故障输入 */
    if ((di & DI_FAULT_IN) && (di_changed & DI_FAULT_IN)) {
        g_ctx.run_state = RUN_STATE_FAULT;
        g_ctx.fault_code = FAULT_EXTERNAL;
    }

    /* 执行启动/停止命令 */
    if (s_run_cmd_pending && g_ctx.run_state == RUN_STATE_STOP) {
        g_ctx.run_state = RUN_STATE_STANDBY;
        s_run_cmd_pending = 0;
    }
    if (s_stop_cmd_pending && g_ctx.run_state != RUN_STATE_FAULT) {
        APP_Speed_SetTarget(0.0f, 0, SOURCE_PANEL);
        if (g_ctx.run_state != RUN_STATE_STOP)
            g_ctx.run_state = RUN_STATE_DECEL;
        s_stop_cmd_pending = 0;
    }

    /* ===== 数字输出处理 ===== */

    /* 故障输出继电器：故障锁定时闭合 */
    uint8_t do_fault = (g_ctx.run_state == RUN_STATE_FAULT) ? 1 : 0;
    DRV_GPIO_SetDO(DO_FAULT_OUT, do_fault);

    /* 运行接触器：运行中且无故障时闭合 */
    uint8_t do_run = (g_ctx.run_state == RUN_STATE_RUN ||
                      g_ctx.run_state == RUN_STATE_ACCEL ||
                      g_ctx.run_state == RUN_STATE_CONST_SPD ||
                      g_ctx.run_state == RUN_STATE_DECEL) ? 1 : 0;
    DRV_GPIO_SetDO(DO_RUN_CONTACT, do_run);

    /* 运行LED */
    DRV_GPIO_SetDO(DO_RUN_LED, do_run);

    /* 散热风扇：温度>40°C 或 运行中 > 35°C 时开启 */
    float temp = MC_ADC_GetNtcTemp();
    uint8_t fan_on = (temp > 40.0f) ||
                     ((g_ctx.run_state != RUN_STATE_STOP) && (temp > 35.0f));
    DRV_GPIO_SetDO(DO_FAN, fan_on);
    if (fan_on) s_fan_on_time += dt;
    else s_fan_on_time = 0.0f;

    /* 故障LED */
    uint8_t do_fault_led = (g_ctx.run_state == RUN_STATE_FAULT) ? 1 : 0;
    DRV_GPIO_SetDO(DO_FAULT_LED, do_fault_led);
}
