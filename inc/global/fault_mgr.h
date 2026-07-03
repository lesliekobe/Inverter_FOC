/**
 * @file  fault_mgr.h
 * @brief 故障管理模块头文件
 *
 * 提供故障记录、故障锁定、故障历史查询等功能。
 * 采用环形缓冲区存储最近32条故障记录。
 */

#ifndef FAULT_MGR_H
#define FAULT_MGR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 常量定义
 * ============================================================================ */

/** 故障记录环形缓冲区深度 */
#define FAULT_MGR_DEPTH  32

/* ============================================================================
 * 类型定义
 * ============================================================================ */

/**
 * @brief 单条故障记录结构
 */
typedef struct {
    uint32_t timestamp;      /**< 故障发生时间戳（运行计数器，单位由系统定义） */
    uint16_t fault_code;     /**< 故障代码 */
    int16_t  motor_speed;    /**< 故障发生时电机转速（机械RPM） */
    float    bus_voltage;    /**< 故障发生时母线电压（V） */
    float    output_current; /**< 故障发生时输出相电流峰值（A） */
} FaultRecord;

/**
 * @brief 故障管理器句柄
 */
typedef struct {
    FaultRecord records[FAULT_MGR_DEPTH]; /**< 环形缓冲区 */
    uint8_t     write_index;              /**< 写索引，指向下一条写入位置 */
    uint8_t     fault_count;              /**< 已记录的故障总数（不超过32） */
    uint16_t    last_fault_code;          /**< 最近一次故障代码 */
    bool        fault_locked;             /**< 故障锁定标志，锁定后不再记录新故障 */
} FaultMgrHandle;

/* ============================================================================
 * 外部全局变量
 * ============================================================================ */

/** 全局故障管理器实例（供各模块引用） */
extern FaultMgrHandle g_fault_mgr;

/* ============================================================================
 * 函数声明
 * ============================================================================ */

/**
 * @brief 初始化故障管理器
 *
 * 调用此函数重置所有内部状态，清空环形缓冲区，
 * 解除故障锁定，为系统启动或复位后调用。
 */
void FaultMgr_Init(void);

/**
 * @brief 记录一条故障
 *
 * 检查故障是否已锁定，若已锁定则直接返回不做任何操作。
 * 若未锁定，则：
 *   1. 设置故障代码到全局上下文
 *   2. 将运行状态置为 FAULT
 *   3. 锁定故障（fault_locked = true），防止重复记录
 *   4. 采集当前电机转速、母线电压、输出电流等快照
 *   5. 将故障记录写入环形缓冲区
 *   6. 更新写索引和故障计数
 *
 * @param fault_code  故障代码，由各驱动/应用模块定义
 */
void FaultMgr_Record(uint16_t fault_code);

/**
 * @brief 复位故障状态（解除锁定并清除故障代码）
 *
 * 将故障锁定标志复位为 false，清除故障代码为 FAULT_NONE，
 * 并将系统运行状态恢复为 STOP。
 * 此函数通常由外部在确认故障已排除后调用。
 */
void FaultMgr_Reset(void);

/**
 * @brief 获取故障历史记录
 *
 * 将环形缓冲区中的故障记录按时间从新到旧依次填入输出数组。
 * 最多返回 min(32, fault_count) 条记录。
 *
 * @param out   输出数组指针，调用方分配 FaultRecord* 数组空间
 * @param count 输出参数，返回实际填充的记录条数
 */
void FaultMgr_GetHistory(FaultRecord* out[], int* count);

/**
 * @brief 清除故障历史记录
 *
 * 仅清除环形缓冲区的内容（fault_count 和 write_index 复位），
 * 不影响故障锁定状态和 last_fault_code。
 * 如需完整复位，请配合 FaultMgr_Reset 使用。
 */
void FaultMgr_ClearHistory(void);

#ifdef __cplusplus
}
#endif

#endif /* FAULT_MGR_H */
