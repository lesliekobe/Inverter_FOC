/**
 * @file hmi_display.c
 * @brief LCD/OLED显示驱动（模拟串行显示）
 */
#include "hmi_display.h"
#include "global_ctx.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define DISP_ROWS  4
#define DISP_COLS  20

DispHandle g_disp = {0};
static char s_line_buf[DISP_ROWS][DISP_COLS+1];

/**
 * HMI_DISP_Init - 初始化显示
 */
void HMI_DISP_Init(void)
{
    memset(&g_disp, 0, sizeof(DispHandle));
    g_disp.page = 0;
    g_disp.update_req = 1;
    /* 清除全屏 */
    for (uint8_t i = 0; i < DISP_ROWS; i++)
        strcpy(s_line_buf[i], "");
}

/**
 * HMI_DISP_SetPage - 切换显示页面
 */
void HMI_DISP_SetPage(uint8_t page)
{
    if (page > 2) page = 0;
    g_disp.page = page;
    g_disp.update_req = 1;
}

/**
 * HMI_DISP_Refresh - 强制刷新
 */
void HMI_DISP_Refresh(void)
{
    g_disp.update_req = 1;
}

/**
 * HMI_DISP_Update - 显示更新（100ms调用）
 *
 * 根据当前页面构建显示内容
 */
void HMI_DISP_Update(void)
{
    if (!g_disp.update_req) return;

    char tmp[DISP_COLS+1];

    switch (g_disp.page) {
        case 0: {
            /* 运行状态页面 */
            float freq = g_ctx.motor_out.freq;
            float i_u  = fabsf(MC_ADC_GetIa());
            float Udc  = MC_ADC_GetUdc();
            float rpm  = g_ctx.motor_out.speed;
            float power = freq * 0.1f; /* 简化功率估算 */

            snprintf(s_line_buf[0], DISP_COLS+1,
                "F:%05.1fHz  I:%04.1fA", freq, i_u);
            snprintf(s_line_buf[1], DISP_COLS+1,
                "U:%04.0fV  n:%5.0frpm", Udc, rpm);
            snprintf(s_line_buf[2], DISP_COLS+1,
                "P:%04.1fkW cos:%03.2f", power, 0.85f);
            const char *state_str = "STOP";
            if (g_ctx.run_state == RUN_STATE_RUN) state_str = "RUN ";
            else if (g_ctx.run_state == RUN_STATE_FAULT) state_str = "FAULT";
            else if (g_ctx.run_state == RUN_STATE_ACCEL) state_str = "ACCEL";
            else if (g_ctx.run_state == RUN_STATE_DECEL) state_str = "DECEL";
            const char *mode_str = "VF";
            if (g_ctx.run_mode == RUN_MODE_FOC) mode_str = "FOC";
            else if (g_ctx.run_mode == RUN_MODE_PID) mode_str = "PID";
            snprintf(s_line_buf[3], DISP_COLS+1, "%s  %s-MODE", state_str, mode_str);
            break;
        }
        case 1: {
            /* 参数监视页面 */
            snprintf(s_line_buf[0], DISP_COLS+1,
                "P000=%04.1fkW P001=%03.0fV", g_ctx.param.motor_power, g_ctx.param.motor_voltage);
            snprintf(s_line_buf[1], DISP_COLS+1,
                "P002=%04.1fHz P003=%dp", g_ctx.param.motor_freq, g_ctx.param.motor_pole_pairs);
            snprintf(s_line_buf[2], DISP_COLS+1,
                "P010=%04.1fA P011=%03.0fA", g_ctx.param.motor_current, g_ctx.param.prot_i_max);
            const char *fault_str = "NONE";
            if (g_ctx.fault_code != FAULT_NONE) {
                snprintf(tmp, DISP_COLS+1, "%d", g_ctx.fault_code);
                fault_str = tmp;
            }
            snprintf(s_line_buf[3], DISP_COLS+1, "FAULT: %s", fault_str);
            break;
        }
        case 2: {
            /* 故障历史页面 */
            snprintf(s_line_buf[0], DISP_COLS+1, "==== FAULT LOG ====");
            FaultRecord *recs[32];
            int cnt = 0;
            FaultMgr_GetHistory(recs, &cnt);
            for (uint8_t i = 0; i < 3 && i < cnt; i++) {
                snprintf(s_line_buf[i+1], DISP_COLS+1, "E%02d CODE=%04d",
                    i+1, recs[i]->fault_code);
            }
            if (cnt == 0) snprintf(s_line_buf[1], DISP_COLS+1, "No fault records");
            break;
        }
    }

    g_disp.update_req = 0;
    g_disp.blink_tick++;
}

/**
 * HMI_DISP_GetLine - 获取指定行显示内容（供显示驱动调用）
 */
const char* HMI_DISP_GetLine(uint8_t row)
{
    if (row >= DISP_ROWS) return "";
    return s_line_buf[row];
}
