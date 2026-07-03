/**
 * @file global_ctx.h
 * @brief 全局上下文结构体定义
 *
 * 全局上下文保存变频器运行时的所有状态数据，包括参数、ADC采样、
 * IO状态、通讯状态、保护标志等。
 */

#ifndef GLOBAL_CTX_H
#define GLOBAL_CTX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "global_def.h"

/* ============================================================
 * 子结构体定义
 * ============================================================ */

/**
 * @brief 参数组结构体
 * @note 包含所有可配置参数，分为：基础参数、电机参数、保护参数、通讯参数、工艺参数
 */
typedef struct {
    /* --- 基础参数 --- */
    ParamItem rated_power;      /**< 电机额定功率 (0.01kW) */
    ParamItem rated_voltage;    /**< 电机额定电压 (0.1V) */
    ParamItem rated_current;    /**< 电机额定电流 (0.01A) */
    ParamItem rated_freq;       /**< 电机额定频率 (0.01Hz) */
    ParamItem rated_speed;      /**< 电机额定转速 (1RPM) */
    ParamItem pole_pairs;       /**< 电机极对数 */
    ParamItem direction;        /**< 旋转方向 (0:正向, 1:反向) */

    /* --- V/F曲线参数 --- */
    ParamItem vfg_v0;           /**< V/F曲线起始电压百分比 (0.1%) */
    ParamItem vfg_v1;           /**< V/F拐点1电压百分比 */
    ParamItem vfg_v2;           /**< V/F拐点2电压百分比 */
    ParamItem vfg_v3;           /**< V/F曲线终止电压百分比 */
    ParamItem vfg_f1;           /**< V/F拐点1频率 (0.01Hz) */
    ParamItem vfg_f2;           /**< V/F拐点2频率 */
    ParamItem vfg_f3;           /**< V/F拐点3频率 */
    ParamItem vfg_torque_boost; /**< 转矩提升 (0.1%) */

    /* --- 保护参数 --- */
    ParamItem prot_overcurrent; /**< 过流保护阈值 (0.01A) */
    ParamItem prot_overvoltage; /**< 过压保护阈值 (0.1V) */
    ParamItem prot_undervoltage;/**< 欠压保护阈值 */
    ParamItem prot_overtemp;    /**< 过温保护阈值 (0.1°C) */
    ParamItem prot_overload;    /**< 过载保护阈值 (0.01%) */
    ParamItem prot_oc_delay;    /**< 过流保护延时 (ms) */
    ParamItem prot_ov_delay;    /**< 过压保护延时 */
    ParamItem prot_uv_delay;    /**< 欠压保护延时 */

    /* --- 通讯参数 --- */
    ParamItem comm_addr;        /**< Modbus从机地址 */
    ParamItem comm_baudrate;   /**< 波特率 (bps) */
    ParamItem comm_parity;     /**< 校验位 (0:无, 1:奇, 2:偶) */
    ParamItem comm_stopbit;    /**< 停止位 (0:1位, 1:2位) */
    ParamItem comm_timeout;    /**< 通讯超时 (ms) */

    /* --- 工艺参数 --- */
    ParamItem proc_accel_time;  /**< 加速时间 (0.01s) */
    ParamItem proc_decel_time;  /**< 减速时间 */
    ParamItem proc_min_freq;    /**< 最小频率 (0.01Hz) */
    ParamItem proc_max_freq;    /**< 最大频率 */
    ParamItem proc_start_freq;  /**< 启动频率 */
    ParamItem proc_stop_mode;   /**< 停机方式 (0:减速, 1:自由) */
    ParamItem proc_dc_brake_time;/**< 直流制动时间 (ms) */
    ParamItem proc_dc_brake_volt;/**< 直流制动电压 (0.1%) */

    /* --- PID参数 --- */
    ParamItem pid_speed_kp;     /**< 速度环KP */
    ParamItem pid_speed_ki;     /**< 速度环KI */
    ParamItem pid_speed_kd;     /**< 速度环KD */
    ParamItem pid_torque_kp;    /**< 转矩环KP */
    ParamItem pid_torque_ki;    /**< 转矩环KI */

    /* --- 保留扩展区 --- */
    ParamItem reserved[16];
} ParamGroup;

/**
 * @brief 电机控制输出结构体
 * @note 算法计算产生的输出量
 */
typedef struct {
    float freq_out;        /**< 输出频率 (Hz) */
    float speed_out;       /**< 电机转速 (RPM) */
    float current_out;    /**< 输出电流 (A) */
    float voltage_out;    /**< 输出电压 (V) */
    float torque_out;     /**< 输出转矩 (Nm) */
    float power_out;      /**< 输出功率 (kW) */
} MotorCtrlOut;

/**
 * @brief ADC原始采样数据
 * @note 中断中更新，存放传感器原始值
 */
typedef struct {
    uint16_t vdc;         /**< 母线电压采样值 (ADC原始码) */
    uint16_t ia;          /**< A相电流采样值 */
    uint16_t ib;          /**< B相电流采样值 */
    uint16_t ic;          /**< C相电流采样值 */
    uint16_t ntc;         /**< NTC温度采样值 */
    uint16_t ai1;         /**< 模拟输入1 (0-10V/4-20mA) */
    uint16_t ai2;         /**< 模拟输入2 */
} AdcRawData;

/**
 * @brief IO状态结构体
 * @note 数字输入/输出状态及模拟量
 */
typedef struct {
    uint16_t di;          /**< 数字输入位域 (DI0-DI7) */
    uint16_t do_mask;     /**< 数字输出位域 (DO0-DO3) */
    uint16_t do_state;    /**< 数字输出当前状态 */
    uint16_t ai1_raw;     /**< AI1原始ADC值 */
    uint16_t ai2_raw;     /**< AI2原始ADC值 */
    float ai1_phys;       /**< AI1物理值 (V或mA) */
    float ai2_phys;       /**< AI2物理值 */
} IOState;

/**
 * @brief 通讯状态结构体
 * @note 记录Modbus通讯统计
 */
typedef struct {
    uint32_t rx_count;        /**< 接收帧计数 */
    uint32_t tx_count;        /**< 发送帧计数 */
    uint32_t error_count;     /**< 错误帧计数 */
    uint32_t last_reg_access; /**< 最近访问的寄存器地址 */
    uint16_t last_error;      /**< 最近错误码 */
    uint8_t rx_buf[256];      /**< 接收缓冲区 */
    uint8_t tx_buf[256];      /**< 发送缓冲区 */
} CommState;

/**
 * @brief 保护阈值标志结构体
 * @note 每位对应一种保护触发状态
 */
typedef struct {
    uint16_t overcurrent  : 1; /**< 过流标志 */
    uint16_t overvoltage   : 1; /**< 过压标志 */
    uint16_t undervoltage  : 1; /**< 欠压标志 */
    uint16_t overtemp      : 1; /**< 过温标志 */
    uint16_t overload      : 1; /**< 过载标志 */
    uint16_t short_circuit  : 1; /**< 短路标志 */
    uint16_t stall         : 1; /**< 堵转标志 */
    uint16_t lost_phase    : 1; /**< 缺相标志 */
    uint16_t comm_timeout  : 1; /**< 通讯超时标志 */
    uint16_t ext_fault     : 1; /**< 外部故障标志 */
    uint16_t reserved      : 6;
} ProtectFlag;

/* ============================================================
 * 主结构体：全局上下文
 * ============================================================ */

/**
 * @brief 全局上下文结构体
 * @note 贯穿整个系统使用的全局数据结构
 */
typedef struct {
    /* --- 运行状态 --- */
    RunState run_state;       /**< 运行状态机 */
    FaultCode fault_code;     /**< 故障代码 */
    RunMode run_mode;         /**< 运行模式 */

    /* --- 参数组 --- */
    ParamGroup param;         /**< 全部参数 */

    /* --- 控制输出 --- */
    MotorCtrlOut ctrl_out;    /**< 电机控制输出 */

    /* --- 采样数据 --- */
    AdcRawData adc;           /**< ADC原始数据 */
    IOState io;               /**< IO状态 */

    /* --- 通讯 --- */
    CommState comm;           /**< 通讯状态 */

    /* --- 保护 --- */
    ProtectFlag prot_flag;    /**< 保护标志位 */
    uint16_t prot_counter;    /**< 保护计数器 (用于消抖) */

    /* --- 时间戳 --- */
    uint32_t tick_1ms;        /**< 1ms tick计数 */
    uint32_t tick_10ms;       /**< 10ms tick计数 */
    uint32_t tick_100ms;      /**< 100ms tick计数 */
} GlobalCtx;

/* ============================================================
 * 外部声明及辅助函数声明
 * ============================================================ */

extern GlobalCtx g_ctx;

/**
 * @brief 初始化全局上下文
 * @note 将所有变量设置为安全默认值
 */
void GlobalCtx_Init(void);

/**
 * @brief 重置全局上下文（不停机）
 * @note 清除输出、状态，但保持参数
 */
void GlobalCtx_Reset(void);

#ifdef __cplusplus
}
#endif

#endif /* GLOBAL_CTX_H */
