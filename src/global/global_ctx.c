/**
 * @file global_ctx.c
 * @brief 全局上下文实现
 *
 * 提供全局上下文的初始化和重置函数。
 */

#include "global_ctx.h"
#include <string.h>

/* ============================================================
 * 私有辅助函数
 * ============================================================ */

#ifdef TARGET_ARM
/* ARM平台使用memset */
#include "bsp.h"
#else
#include <stdlib.h>
#include <string.h>
#endif

/**
 * @brief 安全地将浮点/整数置零
 * @note 使用volatile防止编译器优化
 */
static void mem_zero_ctx(GlobalCtx *ctx, size_t size)
{
    memset(ctx, 0, size);
}

/* ============================================================
 * 公共函数实现
 * ============================================================ */

/**
 * @brief 全局上下文初始化
 *
 * 将所有字段初始化为安全默认值：
 * - 数值型字段 -> 0
 * - 指针型字段 -> NULL
 * - 运行状态 -> STOP
 * - 故障代码 -> FAULT_NONE
 * - 运行模式 -> VF_OPEN_LOOP
 */
void GlobalCtx_Init(void)
{
    mem_zero_ctx(&g_ctx, sizeof(GlobalCtx));

    /* 设置安全初始状态 */
    g_ctx.run_state  = RUN_STATE_STOP;
    g_ctx.fault_code = FAULT_NONE;
    g_ctx.run_mode   = RUN_MODE_VF_OPEN_LOOP;

    /* 保护计数器清零 */
    g_ctx.prot_counter = 0;

    /* Tick计数器清零 */
    g_ctx.tick_1ms   = 0;
    g_ctx.tick_10ms  = 0;
    g_ctx.tick_100ms = 0;

    /* 默认参数（在Param_SetDefaults中设置） */
#ifdef PARAM_STORE_ENABLED
    Param_SetDefaults(&g_ctx.param);
#endif
}

/**
 * @brief 全局上下文重置（软复位）
 *
 * 不清除参数，保留当前设置，仅重置：
 * - 运行状态 -> STOP
 * - 故障代码 -> NONE
 * - 控制输出 -> 0
 * - 保护标志 -> 全部清除
 * - 通讯统计 -> 保留（便于诊断）
 */
void GlobalCtx_Reset(void)
{
    /* 重置运行状态 */
    g_ctx.run_state  = RUN_STATE_STOP;
    g_ctx.fault_code = FAULT_NONE;

    /* 清除控制输出 */
    memset(&g_ctx.ctrl_out, 0, sizeof(MotorCtrlOut));

    /* 清除采样数据 */
    memset(&g_ctx.adc, 0, sizeof(AdcRawData));

    /* 清除IO状态 */
    memset(&g_ctx.io, 0, sizeof(IOState));

    /* 清除所有保护标志 */
    memset(&g_ctx.prot_flag, 0, sizeof(ProtectFlag));

    /* 保护计数器清零 */
    g_ctx.prot_counter = 0;
}
