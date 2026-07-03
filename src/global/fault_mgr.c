/**
 * @file  fault_mgr.c
 * @brief 故障管理模块实现
 *
 * 采用环形缓冲区保存最近32条故障记录，支持故障锁定、
 * 故障历史查询及清除等功能。
 */

#include "global/fault_mgr.h"
#include "global/globals.h"       /* 提供 g_ctx（全局运行上下文） */

/* ============================================================================
 * 静态变量
 * ============================================================================ */

/** 运行计数器，作为简易时间戳使用（每次记录累加1） */
static uint32_t s_tick_counter = 0U;

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 获取当前时间戳
 * @return 运行计数器值
 */
static inline uint32_t FaultMgr_GetTick(void)
{
    return s_tick_counter;
}

/* ============================================================================
 * 公共函数实现
 * ============================================================================ */

void FaultMgr_Init(void)
{
    /* 清零环形缓冲区 */
    for (int i = 0; i < FAULT_MGR_DEPTH; i++) {
        g_fault_mgr.records[i].timestamp      = 0U;
        g_fault_mgr.records[i].fault_code     = 0U;
        g_fault_mgr.records[i].motor_speed    = 0;
        g_fault_mgr.records[i].bus_voltage    = 0.0f;
        g_fault_mgr.records[i].output_current = 0.0f;
    }

    g_fault_mgr.write_index     = 0U;
    g_fault_mgr.fault_count     = 0U;
    g_fault_mgr.last_fault_code = 0U;
    g_fault_mgr.fault_locked    = false;

    s_tick_counter = 0U;
}

void FaultMgr_Record(uint16_t fault_code)
{
    /* 故障已锁定，不记录后续故障，防止重复写入 */
    if (g_fault_mgr.fault_locked) {
        return;
    }

    /* 将故障代码写入全局上下文，通知上层当前处于故障状态 */
    g_ctx.fault_code = fault_code;
    g_ctx.run_state  = RUN_STATE_FAULT;   /* 切换到故障状态 */

    /* 锁定：此后 FaultMgr_Record 不再响应，直到外部调用 FaultMgr_Reset */
    g_fault_mgr.fault_locked      = true;
    g_fault_mgr.last_fault_code   = fault_code;

    /* 采集故障发生时的系统快照 */
    FaultRecord rec;
    rec.timestamp      = FaultMgr_GetTick();   /* 简易时间戳 */
    rec.fault_code     = fault_code;
    rec.motor_speed    = (int16_t)g_ctx.motor_speed;   /* 机械RPM，强制截断为int16_t */
    rec.bus_voltage    = g_ctx.vbus;
    rec.output_current = g_ctx.i_alpha;        /* 取一相电流快照（可按需改为峰值的 sqrt(2) 倍） */

    /* 写入环形缓冲区 */
    g_fault_mgr.records[g_fault_mgr.write_index] = rec;

    /* 更新写索引（环形wrap） */
    g_fault_mgr.write_index = (g_fault_mgr.write_index + 1U) % FAULT_MGR_DEPTH;

    /* 更新故障计数，上限为缓冲区深度32 */
    if (g_fault_mgr.fault_count < FAULT_MGR_DEPTH) {
        g_fault_mgr.fault_count++;
    }

    /* 累加时间戳计数器 */
    s_tick_counter++;
}

void FaultMgr_Reset(void)
{
    g_fault_mgr.fault_locked    = false;
    g_ctx.fault_code            = FAULT_NONE;
    g_ctx.run_state             = RUN_STATE_STOP;
}

void FaultMgr_GetHistory(FaultRecord* out[], int* count)
{
    if (out == NULL || count == NULL) {
        return;
    }

    int n = (g_fault_mgr.fault_count < FAULT_MGR_DEPTH)
            ? (int)g_fault_mgr.fault_count
            : FAULT_MGR_DEPTH;

    *count = n;

    if (n == 0 || out == NULL) {
        return;
    }

    /* 环形缓冲区布局：
     * write_index 指向下一条写入位置（最新数据之后）。
     * 最新记录在 (write_index - 1 + DEPTH) % DEPTH 位置，
     * 之前的记录依次往前推。 */
    for (int i = 0; i < n; i++) {
        /* 计算从新到旧的第 i 条记录在缓冲区中的实际索引 */
        uint8_t idx = (g_fault_mgr.write_index + FAULT_MGR_DEPTH - 1U - (uint8_t)i) % FAULT_MGR_DEPTH;
        out[i] = &g_fault_mgr.records[idx];
    }
}

void FaultMgr_ClearHistory(void)
{
    g_fault_mgr.fault_count = 0U;
    g_fault_mgr.write_index = 0U;
}
