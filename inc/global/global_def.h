/**
 ******************************************************************************
 * @file    global_def.h
 * @brief   全局定义头文件 - 三相逆变器FOC项目
 * @version V1.0.0
 * @date    2024-01-01
 * @note    适用于 STM32F4/F7/H7 或 DSP28335 平台
 *          低压 0.75kW~75kW 三相永磁同步电机/异步电机 FOC 控制
 *          纯 C 语言，无 C++ 依赖
 ******************************************************************************
 */

#ifndef __GLOBAL_DEF_H
#define __GLOBAL_DEF_H

/* ============================================================
 * 第一部分：标准库头文件
 * ============================================================ */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================
 * 第二部分：MCU 平台检测与选择
 * ============================================================ */

/* 优先级：用户在编译选项中定义 > 自动检测 */

/* STM32F4 系列 (ARM Cortex-M4) */
#if defined(STM32F4) || defined(STM32F405) || defined(STM32F415) || \
    defined(STM32F407) || defined(STM32F417) || defined(STM32F427) || \
    defined(STM32F437) || defined(STM32F429) || defined(STM32F439) || \
    defined(STM32F446) || defined(STM32F469) || defined(STM32F479)
    #define MCU_STM32F4
    #define MCU_FAMILY_STM32
    #define MCU_TYPE_ARM_CORTEX_M4
    #define MCU_HAS_FPU
    #define ISR_PREFIX
/* STM32F7 系列 (ARM Cortex-M7) */
#elif defined(STM32F7) || defined(STM32F746) || defined(STM32F756) || \
      defined(STM32F767) || defined(STM32F769) || defined(STM32F777) || \
      defined(STM32F779)
    #define MCU_STM32F7
    #define MCU_FAMILY_STM32
    #define MCU_TYPE_ARM_CORTEX_M7
    #define MCU_HAS_FPU
    #define ISR_PREFIX
/* STM32H7 系列 (ARM Cortex-M7 + Cortex-M4) */
#elif defined(STM32H7) || defined(STM32H743) || defined(STM32H745) || \
      defined(STM32H747) || defined(STM32H753) || defined(STM32H755) || \
      defined(STM32H757)
    #define MCU_STM32H7
    #define MCU_FAMILY_STM32
    #define MCU_TYPE_ARM_CORTEX_M7
    #define MCU_HAS_FPU
    #define MCU_HAS_DCB
    #define ISR_PREFIX
/* TI C2000 DSP28335 */
#elif defined(DSP28335) || defined(__TMS320F28335__)
    #define MCU_DSP28335
    #define MCU_FAMILY_TI_C2000
    #define MCU_TYPE_DSP
    #define ISR_PREFIX __interrupt
/* TI C2000 DSP280049 (可选扩展) */
#elif defined(DSP280049) || defined(__TMS320F280049__)
    #define MCU_DSP280049
    #define MCU_FAMILY_TI_C2000
    #define MCU_TYPE_DSP
    #define MCU_HAS_FPU
    #define ISR_PREFIX __interrupt
/* 未识别平台 */
#else
    #warning "未识别 MCU 平台！默认使用 STM32F4 参数，请确保在编译选项中定义平台宏"
    #define MCU_STM32F4
    #define MCU_FAMILY_STM32
    #define MCU_TYPE_ARM_CORTEX_M4
    #define MCU_HAS_FPU
    #define ISR_PREFIX
#endif

/* ARM DSP 库支持检测 */
#if defined(MCU_TYPE_ARM_CORTEX_M4) || defined(MCU_TYPE_ARM_CORTEX_M7)
    #define ARM_DSP_LIB_AVAILABLE
#endif

/* ============================================================
 * 第三部分：编译期特性开关（默认关闭，可按需开启）
 * ============================================================ */

/* FOC 控制使能 */
#define ENABLE_FOC                     1

/* V/F 开环/闭环控制使能 */
#define ENABLE_VF_CONTROL              1

/* PID 控制使能 */
#define ENABLE_PID                     1

/* 调制方式使能 */
#define ENABLE_SVPWM                   1   /* 空间矢量PWM */
#define ENABLE_SVM                     1   /* 空间矢量调制（同SVPWM） */
#define ENABLE_SPWM                    1   /* 正弦PWM */
#define ENABLE_DPWM                    1   /* 断续PWM */

/* 通信使能 */
#define ENABLE_MODBUS                  1   /* Modbus RTU/ASCII */
#define ENABLE_CANOPEN                 0   /* CANopen 协议 */
#define ENABLE_ETHERNET                0   /* 以太网通信 */

/* 人机界面使能 */
#define ENABLE_HMI                     1   /* HMI 显示屏 */
#define ENABLE_KEYPAD                  1   /* 键盘按键 */
#define ENABLE_LED_INDICATOR           1   /* LED 指示灯 */

/* 操作系统使能 */
#define ENABLE_FREE_RTOS               0   /* FreeRTOS 操作系统 */
#define ENABLE_OS_TASK                 0   /* 多任务支持 */

/* 编码器与传感器使能 */
#define ENABLE_ENCODER                 1   /* 增量式编码器 */
#define ENABLE_HALL                    1   /* Hall 传感器 */
#define ENABLE_RESOLVER                0   /* 旋变传感器 */
#define ENABLE_SENSORLESS              1   /* 无传感器模式 */

/* 保护功能使能 */
#define ENABLE_PROTECTION              1   /* 总保护使能 */
#define ENABLE_OVERCURRENT_PROT        1   /* 过流保护 */
#define ENABLE_OVERVOLTAGE_PROT        1   /* 过压保护 */
#define ENABLE_UNDERVOLTAGE_PROT       1   /* 欠压保护 */
#define ENABLE_OVERHEAT_PROT           1   /* 过温保护 */
#define ENABLE_OVERLOAD_PROT           1   /* 过载保护 */
#define ENABLE_GROUND_FAULT_PROT       1   /* 接地故障保护 */

/* 调试功能使能 */
#define ENABLE_DEBUG                   1
#define ENABLE_SCOPE_TRACE             0   /* 示波器波形跟踪 */
#define ENABLE_UART_DEBUG              1   /* UART 调试串口 */

/* 参数存储使能 */
#define ENABLE_EEPROM_EMULATION        1   /* EEPROM 模拟（STM32） */
#define ENABLE_FLASH_STORE             1   /* Flash 存储参数 */

/* 升级支持 */
#define ENABLE_BOOTLOADER              0   /*  bootloader 升级功能 */
#define ENABLE_IAP                     0   /*  应用内升级 */

/* ============================================================
 * 第四部分：全局枚举类型定义
 * ============================================================ */

/**
 * @brief 运行状态枚举
 */
typedef enum {
    RUN_STATE_STOP = 0,      /**< 停机状态 */
    RUN_STATE_STANDBY = 1,   /**< 待机状态（已上电但未运行） */
    RUN_STATE_RUN = 2,       /**< 运行状态 */
    RUN_STATE_ACCEL = 3,     /**< 加速状态 */
    RUN_STATE_CONST_SPD = 4, /**< 恒速运行状态 */
    RUN_STATE_DECEL = 5,     /**< 减速状态 */
    RUN_STATE_FAULT = 6,     /**< 故障状态 */
    RUN_STATE_PRECHARGE = 7, /**< 预充电状态（直流母线电容充电） */
    RUN_STATE_BRAKE = 8,     /**< 制动状态（能耗制动/回馈制动） */
    RUN_STATE_LOCK = 9       /**< 锁定状态（防止意外启动） */
} RunState_t;

/**
 * @brief 故障代码枚举（至少20种故障类型）
 */
typedef enum {
    FAULT_NONE = 0,              /**< 无故障 */

    /* 电流相关故障 */
    FAULT_OVERCURRENT = 1,       /**< 过流故障 */
    FAULT_SHORT_CIRCUIT = 2,     /**< 短路故障 */
    FAULT_PHASE_LOSS_U = 3,      /**< U相缺失 */
    FAULT_PHASE_LOSS_V = 4,      /**< V相缺失 */
    FAULT_PHASE_LOSS_W = 5,      /**< W相缺失 */
    FAULT_PHASE_LOSS = 6,        /**< 缺相故障（总称） */
    FAULT_CURRENT_IMBALANCE = 7, /**< 电流不平衡 */

    /* 电压相关故障 */
    FAULT_OVERVOLTAGE = 10,      /**< 直流母线过压 */
    FAULT_UNDERVOLTAGE = 11,     /**< 直流母线欠压 */
    FAULT_BUS_VOLTAGE_SPIKE = 12,/**< 母线电压尖峰 */
    FAULT_INPUT_OVERVOLTAGE = 13,/**< 输入侧过压 */
    FAULT_INPUT_UNDERVOLTAGE = 14,/**< 输入侧欠压 */

    /* 温度相关故障 */
    FAULT_OVERHEAT_IPM = 20,     /**< IPM 模块过温 */
    FAULT_OVERHEAT_MOTOR = 21,   /**< 电机过温 */
    FAULT_OVERHEAT_INVERTER = 22,/**< 逆变器过温 */
    FAULT_OVERHEAT_PFC = 23,     /**< PFC 过温（若有） */
    FAULT_UNDERTEMP = 24,        /**< 温度过低 */

    /* 负载与机械故障 */
    FAULT_OVERLOAD = 30,         /**< 过载故障 */
    FAULT_LOCKED_ROTOR = 31,     /**< 电机堵转 */
    FAULT_BROKEN_SHAFT = 32,     /**< 断轴故障 */
    FAULT_BELT_BREAK = 33,       /**< 皮带断裂（风机类负载） */

    /* 传感器与编码器故障 */
    FAULT_ENCODER = 40,          /**< 编码器故障（信号丢失/数据异常） */
    FAULT_HALL_FAULT = 41,       /**< Hall 传感器故障 */
    FAULT_RESOLVER_FAULT = 42,   /**< 旋变传感器故障 */
    FAULT_SENSOR_LOSS = 43,      /**< 速度/位置传感器丢失 */
    FAULT_BACK_EMF_FAULT = 44,   /**< 反电动势异常（无传感器估算失败） */

    /* 通信与外设故障 */
    FAULT_MODBUS_TIMEOUT = 50,   /**< Modbus 通信超时 */
    FAULT_CAN_ERROR = 51,        /**< CAN 总线错误 */
    FAULT_UART_ERROR = 52,       /**< UART 通信错误 */
    FAULT_SPI_ERROR = 53,        /**< SPI 通信错误 */
    FAULT_I2C_ERROR = 54,        /**< I2C 通信错误 */
    FAULT_ETHERNET_ERROR = 55,   /**< 以太网通信错误 */

    /* 系统与内部故障 */
    FAULT_GROUND_FAULT = 60,     /**< 接地故障 */
    FAULT_DC_BUS_FAULT = 61,     /**< 直流母线故障 */
    FAULT_PFC_FAULT = 62,        /**< PFC 故障 */
    FAULT_SOFTWARE_ERROR = 70,   /**< 软件看门狗/内部错误 */
    FAULT_HARDWARE_ERROR = 71,   /**< 硬件自检失败 */
    FAULT_PARAM_CORRUPT = 72,    /**< 参数损坏/校验失败 */
    FAULT_CALIB_ERROR = 73,      /**< 校准参数错误 */
    FAULT_FLASH_ERROR = 74,      /**< Flash 读写错误 */
    FAULT_EEPROM_ERROR = 75,     /**< EEPROM 错误 */

    /* 运行模式相关故障 */
    FAULT_START_FAILURE = 80,    /**< 启动失败 */
    FAULT_SELF_TEST_FAIL = 81,   /**< 自检失败 */
    FAULT_MODE_SWITCH_ERROR = 82,/**< 模式切换错误 */

    /* 外部故障输入 */
    FAULT_EXT_FAULT_0 = 90,      /**< 外部故障输入0 */
    FAULT_EXT_FAULT_1 = 91,      /**< 外部故障输入1 */
    FAULT_EXT_FAULT_2 = 92,      /**< 外部故障输入2 */
    FAULT_EXT_FAULT_3 = 93,      /**< 外部故障输入3 */

    FAULT_RESERVED_MAX = 255     /**< 保留最大值 */
} FaultCode_t;

/**
 * @brief 运行模式枚举
 */
typedef enum {
    RUN_MODE_VF_OPEN_LOOP = 0,   /**< V/F 开环控制（V/Hz 控制） */
    RUN_MODE_VF_CLOSED_LOOP = 1, /**< V/F 闭环控制（带速度反馈） */
    RUN_MODE_FOC = 2,            /**< FOC 磁场定向控制（矢量控制） */
    RUN_MODE_PID = 3,            /**< PID 控制模式（通用） */
    RUN_MODE_MULTI_SPEED = 4,    /**< 多段速控制模式 */
    RUN_MODE_PWM_MODE = 5,       /**< 外部 PWM 输入模式 */
    RUN_MODE_HAND = 6,           /**< 手动模式（键盘直接给定） */
    RUN_MODE_AUTO = 7,           /**< 自动模式（根据负载自适应） */
    RUN_MODE_NETWORK = 8,        /**< 网络控制模式（Modbus/CAN） */
    RUN_MODE_DROOPING = 9        /**< 负载分配（Drooping）模式 */
} RunMode_t;

/**
 * @brief PID 工作模式枚举
 */
typedef enum {
    PID_MODE_PRESSURE = 0,   /**< 压力控制模式 */
    PID_MODE_CURRENT = 1,    /**< 电流控制模式 */
    PID_MODE_FLOW = 2,       /**< 流量控制模式 */
    PID_MODE_SPEED = 3,      /**< 速度控制模式 */
    PID_MODE_TEMPERATURE = 4,/**< 温度控制模式 */
    PID_MODE_LEVEL = 5,      /**< 液位控制模式 */
    PID_MODE_TORQUE = 6      /**< 转矩控制模式 */
} PIDMode_t;

/**
 * @brief PWM 调制方式枚举
 */
typedef enum {
    PWM_MODE_SVPWM = 0,      /**< 空间矢量PWM（SVPWM/7段式） */
    PWM_MODE_SVM = 1,        /**< 空间矢量调制（等同于SVPWM） */
    PWM_MODE_SPWM = 2,       /**< 正弦PWM（SPWM） */
    PWM_MODE_DPWM1 = 3,      /**< 断续PWM类型1 */
    PWM_MODE_DPWM2 = 4,      /**< 断续PWM类型2 */
    PWM_MODE_DPWM3 = 5,      /**< 断续PWM类型3 */
    PWM_MODE_DPWM_MAX = 6    /**< 最大值断续PWM */
} PWMMode_t;

/**
 * @brief 电机类型枚举
 */
typedef enum {
    MOTOR_TYPE_PMSM = 0,     /**< 永磁同步电机（PMSM） */
    MOTOR_TYPE_IPMSM = 1,    /**< 嵌入式永磁同步电机（IPMSM） */
    MOTOR_TYPE_SPM = 2,      /**< 表贴式永磁同步电机（SPM） */
    MOTOR_TYPE_IM = 3,       /**< 异步电机（感应电机） */
    MOTOR_TYPE_SYN_RELUCTANCE = 4, /**< 同步磁阻电机 */
    MOTOR_TYPE_DC = 5,       /**< 直流电机（备用） */
    MOTOR_TYPE_UNKNOWN = 15  /**< 未知电机类型 */
} MotorType_t;

/**
 * @brief 编码器类型枚举
 */
typedef enum {
    ENCODER_TYPE_INCREMENTAL = 0,   /**< 增量式编码器 */
    ENCODER_TYPE_ABSOLUTE_SSI = 1,  /**< 绝对值编码器（SSI） */
    ENCODER_TYPE_ABSOLUTE_BISS = 2, /**< 绝对值编码器（BiSS） */
    ENCODER_TYPE_ABSOLUTE_ENDAT = 3,/**< 绝对值编码器（EnDat） */
    ENCODER_TYPE_HALL = 4,          /**< Hall 传感器 */
    ENCODER_TYPE_RESOLVER = 5,      /**< 旋变传感器 */
    ENCODER_TYPE_SENSORLESS = 10    /**< 无传感器模式 */
} EncoderType_t;

/**
 * @brief 通信协议类型枚举
 */
typedef enum {
    PROTOCOL_MODBUS_RTU = 0,    /**< Modbus RTU */
    PROTOCOL_MODBUS_ASCII = 1,  /**< Modbus ASCII */
    PROTOCOL_CANOPEN = 2,       /**< CANopen */
    PROTOCOL_ETHERNET_IP = 3,   /**< Ethernet/IP */
    PROTOCOL_PROFINET = 4,      /**< PROFINET */
    PROTOCOL_PPI = 5,           /**< 西门子 PPI 协议 */
    PROTOCOL_CUSTOM = 99        /**< 自定义协议 */
} ProtocolType_t;

/**
 * @brief 制动方式枚举
 */
typedef enum {
    BRAKE_MODE_NONE = 0,        /**< 无制动 */
    BRAKE_MODE_DYNAMIC = 1,     /**< 动态制动（电阻耗能） */
    BRAKE_MODE_REGEN = 2,       /**< 回馈制动（能量回馈电网） */
    BRAKE_MODE_DC_INJECTION = 3,/**< 直流制动（定子注入直流） */
    BRAKE_MODE_SHORT_BRAKE = 4  /**< 短接制动 */
} BrakeMode_t;

/**
 * @brief 控制方向枚举
 */
typedef enum {
    DIR_FORWARD = 0,    /**< 正转 */
    DIR_REVERSE = 1,    /**< 反转 */
    DIR_BRAKE = 2       /**< 制动/停机 */
} Dir_t;

/**
 * @brief 启动模式枚举
 */
typedef enum {
    START_MODE_DIRECT = 0,      /**< 直接启动 */
    START_MODE_SOFT = 1,        /**< 软启动（斜坡） */
    START_MODE_VEC = 2,         /**< 变频启动 */
    START_MODE_AUTO = 3         /**< 自动选择启动方式 */
} StartMode_t;

/**
 * @brief 停机方式枚举
 */
typedef enum {
    STOP_MODE_COAST = 0,        /**< 自由停车 */
    STOP_MODE_RAMP = 1,         /**< 斜坡停车 */
    STOP_MODE_BRAKE = 2,        /**< 制动停车 */
    STOP_MODE_EMERGENCY = 3     /**< 紧急停车 */
} StopMode_t;

/* ============================================================
 * 第五部分：全局宏定义 - 系统参数
 * ============================================================ */

/** @brief 系统主频（Hz），默认 168MHz（STM32F4） */
#ifndef SYSTEM_FREQ_HZ
    #define SYSTEM_FREQ_HZ          168000000UL
#endif

/** @brief PWM 载波频率（Hz），默认 10kHz */
#ifndef PWM_FREQUENCY_HZ
    #define PWM_FREQUENCY_HZ        10000UL
#endif

/** @brief PWM 计数器周期（ARR+1） */
#define PWM_PERIOD_COUNT           (SYSTEM_FREQ_HZ / PWM_FREQUENCY_HZ)

/** @brief ADC 采样频率（Hz），通常与 PWM 频率相同或整数倍 */
#ifndef ADC_SAMPLE_RATE_HZ
    #define ADC_SAMPLE_RATE_HZ      PWM_FREQUENCY_HZ
#endif

/** @brief 控制环路周期（秒），默认 100us（10kHz） */
#define CONTROL_PERIOD_S           (1.0f / (float)PWM_FREQUENCY_HZ)
#define CONTROL_PERIOD_MS          (1000.0f / (float)PWM_FREQUENCY_HZ)
#define CONTROL_PERIOD_US          (1000000.0f / (float)PWM_FREQUENCY_HZ)

/** @brief 控制环路周期（整数，微秒） */
#define CONTROL_PERIOD_US_INT      ((uint32_t)(1000000UL / PWM_FREQUENCY_HZ))

/** @brief 电机默认极对数 */
#ifndef MOTOR_POLE_PAIRS
    #define MOTOR_POLE_PAIRS        4
#endif

/** @brief 默认编码器线数（每转脉冲数 PPR） */
#ifndef ENCODER_PPR
    #define ENCODER_PPR             2500
#endif

/** @brief 编码器分辨率（4倍频后） */
#define ENCODER_RESOLUTION         (ENCODER_PPR * 4)

/** @brief 电机额定转速（rpm） */
#ifndef MOTOR_RATED_SPEED_RPM
    #define MOTOR_RATED_SPEED_RPM   1500
#endif

/** @brief 电机额定频率（Hz） */
#ifndef MOTOR_RATED_FREQ_HZ
    #define MOTOR_RATED_FREQ_HZ     50.0f
#endif

/** @brief 电机额定电压（V） */
#ifndef MOTOR_RATED_VOLTAGE_V
    #define MOTOR_RATED_VOLTAGE_V   380.0f
#endif

/** @brief 电机额定电流（A） */
#ifndef MOTOR_RATED_CURRENT_A
    #define MOTOR_RATED_CURRENT_A   10.0f
#endif

/** @brief 电机额定功率（kW） */
#ifndef MOTOR_RATED_POWER_KW
    #define MOTOR_RATED_POWER_KW    5.5f
#endif

/** @brief 电机额定功率（W） */
#define MOTOR_RATED_POWER_W        (MOTOR_RATED_POWER_KW * 1000.0f)

/** @brief 电机额定转矩（N·m） */
#ifndef MOTOR_RATED_TORQUE_NM
    #define MOTOR_RATED_TORQUE_NM   35.0f
#endif

/** @brief 电机额定功率因数（cosφ） */
#ifndef MOTOR_POWER_FACTOR
    #define MOTOR_POWER_FACTOR      0.85f
#endif

/** @brief 电机效率（%） */
#ifndef MOTOR_EFFICIENCY
    #define MOTOR_EFFICIENCY        0.92f
#endif

/** @brief 电机定子电阻（Ω） */
#ifndef MOTOR_STATOR_RESISTANCE
    #define MOTOR_STATOR_RESISTANCE 2.0f
#endif

/** @brief 电机转子电阻（Ω，仅异步电机） */
#ifndef MOTOR_ROTOR_RESISTANCE
    #define MOTOR_ROTOR_RESISTANCE  1.5f
#endif

/** @brief 电机漏感（H） */
#ifndef MOTOR_LEAKAGE_INDUCTANCE
    #define MOTOR_LEAKAGE_INDUCTANCE 0.008f
#endif

/** @brief 电机互感（H） */
#ifndef MOTOR_MUTUAL_INDUCTANCE
    #define MOTOR_MUTUAL_INDUCTANCE 0.2f
#endif

/** @brief 反电动势常数（V/krpm） */
#ifndef MOTOR_BEMF_CONSTANT
    #define MOTOR_BEMF_CONSTANT     100.0f
#endif

/** @brief 电机转动惯量（kg·m²） */
#ifndef MOTOR_INERTIA
    #define MOTOR_INERTIA           0.01f
#endif

/** @brief 机械时间常数（秒） */
#define MOTOR_MECHANICAL_TC        (MOTOR_INERTIA * MOTOR_RATED_SPEED_RPM / (MOTOR_RATED_TORQUE_NM * 9.5493f))

/* ============================================================
 * 第六部分：全局宏定义 - V/F 控制参数
 * ============================================================ */

/** @brief V/F 曲线起始电压百分比（%） */
#ifndef VF_START_VOLT_RATIO
    #define VF_START_VOLT_RATIO     10.0f
#endif

/** @brief V/F 曲线中频点电压百分比（%） */
#ifndef VF_MID_VOLT_RATIO
    #define VF_MID_VOLT_RATIO       50.0f
#endif

/** @brief V/F 曲线中频点频率百分比（%） */
#ifndef VF_MID_FREQ_RATIO
    #define VF_MID_FREQ_RATIO       50.0f
#endif

/** @brief V/F 曲线转折点（弱磁起点）频率百分比（%） */
#ifndef VF_Knee_FREQ_RATIO
    #define VF_KNEE_FREQ_RATIO      85.0f
#endif

/** @brief V/F 补偿增益（提升比例） */
#ifndef VF_BOOST_GAIN
    #define VF_BOOST_GAIN           0.05f
#endif

/** @brief V/F 斜坡加速时间（秒，从0到额定频率） */
#ifndef VF_ACCEL_TIME_S
    #define VF_ACCEL_TIME_S         5.0f
#endif

/** @brief V/F 斜坡减速时间（秒） */
#ifndef VF_DECEL_TIME_S
    #define VF_DECEL_TIME_S         5.0f
#endif

/** @brief V/F 频率下限（Hz） */
#ifndef VF_FREQ_MIN_HZ
    #define VF_FREQ_MIN_HZ          5.0f
#endif

/** @brief V/F 频率上限（Hz） */
#ifndef VF_FREQ_MAX_HZ
    #define VF_FREQ_MAX_HZ          100.0f
#endif

/* ============================================================
 * 第七部分：全局宏定义 - FOC 控制参数
 * ============================================================ */

/** @brief FOC 电流采样频率（Hz），通常等于 PWM 频率 */
#define FOC_CURRENT_SAMPLE_FREQ    PWM_FREQUENCY_HZ

/** @brief FOC 速度环默认带宽（Hz） */
#ifndef FOC_SPEED_BW_HZ
    #define FOC_SPEED_BW_HZ         50.0f
#endif

/** @brief FOC 电流环默认带宽（Hz） */
#ifndef FOC_CURRENT_BW_HZ
    #define FOC_CURRENT_BW_HZ       500.0f
#endif

/** @brief 默认电流环 Kp（d轴） */
#ifndef FOC_ID_KP_DEFAULT
    #define FOC_ID_KP_DEFAULT       1.0f
#endif

/** @brief 默认电流环 Ki（d轴） */
#ifndef FOC_ID_KI_DEFAULT
    #define FOC_ID_KI_DEFAULT       0.1f
#endif

/** @brief 默认电流环 Kd（d轴） */
#ifndef FOC_ID_KD_DEFAULT
    #define FOC_ID_KD_DEFAULT       0.0f
#endif

/** @brief 默认电流环 Kp（q轴） */
#ifndef FOC_IQ_KP_DEFAULT
    #define FOC_IQ_KP_DEFAULT       1.0f
#endif

/** @brief 默认电流环 Ki（q轴） */
#ifndef FOC_IQ_KI_DEFAULT
    #define FOC_IQ_KI_DEFAULT       0.1f
#endif

/** @brief 默认电流环 Kd（q轴） */
#ifndef FOC_IQ_KD_DEFAULT
    #define FOC_IQ_KD_DEFAULT       0.0f
#endif

/** @brief 默认速度环 Kp */
#ifndef FOC_SPEED_KP_DEFAULT
    #define FOC_SPEED_KP_DEFAULT    0.5f
#endif

/** @brief 默认速度环 Ki */
#ifndef FOC_SPEED_KI_DEFAULT
    #define FOC_SPEED_KI_DEFAULT    0.01f
#endif

/** @brief 默认速度环 Kd */
#ifndef FOC_SPEED_KD_DEFAULT
    #define FOC_SPEED_KD_DEFAULT    0.0f
#endif

/** @brief 默认磁链环 Kp（用于 IPMSM 弱磁） */
#ifndef FOC_FLUX_KP_DEFAULT
    #define FOC_FLUX_KP_DEFAULT     0.2f
#endif

/** @brief 默认磁链环 Ki */
#ifndef FOC_FLUX_KI_DEFAULT
    #define FOC_FLUX_KI_DEFAULT     0.01f
#endif

/** @brief d轴电流限幅（额定电流的百分比） */
#ifndef FOC_ID_MAX_RATIO
    #define FOC_ID_MAX_RATIO        0.0f   /* PMSM 通常 d轴给0，IPMSM 可负向弱磁 */
#endif

/** @brief q轴电流限幅（额定电流的百分比） */
#ifndef FOC_IQ_MAX_RATIO
    #define FOC_IQ_MAX_RATIO        1.0f
#endif

/** @brief FOC 弱磁系数（MTPA + 弱磁控制） */
#ifndef FOC_WEAK_FLUX_GAIN
    #define FOC_WEAK_FLUX_GAIN      0.5f
#endif

/** @brief MTPA（最大转矩每安培）使能 */
#define FOC_ENABLE_MTPA            1

/** @brief 弱磁控制使能 */
#define FOC_ENABLE_FLUX_WEAKENING  1

/* ============================================================
 * 第八部分：全局宏定义 - PID 参数
 * ============================================================ */

/** @brief PID 输出限幅百分比（相对于最大值） */
#ifndef PID_OUTPUT_MAX_RATIO
    #define PID_OUTPUT_MAX_RATIO    1.0f
#endif

/** @brief PID 积分限幅百分比 */
#ifndef PID_I_LIMIT_RATIO
    #define PID_I_LIMIT_RATIO       0.8f
#endif

/** @brief PID 微分滤波系数 */
#ifndef PID_D_FILTER_COEFF
    #define PID_D_FILTER_COEFF      0.1f
#endif

/** @brief PID 默认比例增益 */
#ifndef PID_KP_DEFAULT
    #define PID_KP_DEFAULT          1.0f
#endif

/** @brief PID 默认积分增益 */
#ifndef PID_KI_DEFAULT
    #define PID_KI_DEFAULT          0.05f
#endif

/** @brief PID 默认微分增益 */
#ifndef PID_KD_DEFAULT
    #define PID_KD_DEFAULT          0.0f
#endif

/** @brief PID 死区阈值（绝对值） */
#ifndef PID_DEADZONE
    #define PID_DEADZONE            0.5f
#endif

/** @brief PID 不灵敏区（相对值 %） */
#ifndef PID_INSENSITIVE_ZONE
    #define PID_INSENSITIVE_ZONE    0.02f
#endif

/* ============================================================
 * 第九部分：全局宏定义 - 保护阈值
 * ============================================================ */

/** @brief 直流母线过压阈值（V） */
#ifndef DCBUS_OVERVOLTAGE_THRESH_V
    #define DCBUS_OVERVOLTAGE_THRESH_V  800.0f
#endif

/** @brief 直流母线欠压阈值（V） */
#ifndef DCBUS_UNDERVOLTAGE_THRESH_V
    #define DCBUS_UNDERVOLTAGE_THRESH_V 350.0f
#endif

/** @brief 直流母线过压恢复阈值（V）（低于此值解除故障） */
#ifndef DCBUS_OVERVOLTAGE_RECOVERY_V
    #define DCBUS_OVERVOLTAGE_RECOVERY_V 750.0f
#endif

/** @brief 直流母线欠压恢复阈值（V） */
#ifndef DCBUS_UNDERVOLTAGE_RECOVERY_V
    #define DCBUS_UNDERVOLTAGE_RECOVERY_V 380.0f
#endif

/** @brief 过流阈值（峰值电流 A） */
#ifndef OVERCURRENT_THRESH_A
    #define OVERCURRENT_THRESH_A    25.0f
#endif

/** @brief 过流阈值（额定电流倍数） */
#ifndef OVERCURRENT_RATIO
    #define OVERCURRENT_RATIO       2.0f
#endif

/** @brief 短路保护阈值（A），通常为过流的2倍 */
#ifndef SHORT_CIRCUIT_THRESH_A
    #define SHORT_CIRCUIT_THRESH_A  50.0f
#endif

/** @brief 过载阈值（额定电流倍数，持续一定时间） */
#ifndef OVERLOAD_CURRENT_RATIO
    #define OVERLOAD_CURRENT_RATIO  1.5f
#endif

/** @brief 过载判定时间（秒） */
#ifndef OVERLOAD_TIME_S
    #define OVERLOAD_TIME_S         60.0f
#endif

/** @brief IPM 模块过温阈值（℃） */
#ifndef IPM_OVERTEMP_THRESH_C
    #define IPM_OVERTEMP_THRESH_C   80.0f
#endif

/** @brief IPM 模块过温恢复阈值（℃） */
#ifndef IPM_OVERTEMP_RECOVERY_C
    #define IPM_OVERTEMP_RECOVERY_C 65.0f
#endif

/** @brief 电机过温阈值（℃，使用PTC或热敏电阻） */
#ifndef MOTOR_OVERTEMP_THRESH_C
    #define MOTOR_OVERTEMP_THRESH_C 120.0f
#endif

/** @brief 电机过温恢复阈值（℃） */
#ifndef MOTOR_OVERTEMP_RECOVERY_C
    #define MOTOR_OVERTEMP_RECOVERY_C 100.0f
#endif

/** @brief 接地故障电流阈值（mA） */
#ifndef GROUND_FAULT_THRESH_MA
    #define GROUND_FAULT_THRESH_MA  500.0f
#endif

/** @brief 电流不平衡阈值（%） */
#ifndef CURRENT_IMBALANCE_THRESH_PCT
    #define CURRENT_IMBALANCE_THRESH_PCT 20.0f
#endif

/** @brief 堵转判定转速阈值（rpm） */
#ifndef LOCKED_ROTOR_SPD_THRESH_RPM
    #define LOCKED_ROTOR_SPD_THRESH_RPM  30.0f
#endif

/** @brief 堵转判定时间（秒） */
#ifndef LOCKED_ROTOR_TIME_S
    #define LOCKED_ROTOR_TIME_S     2.0f
#endif

/** @brief 启动失败判定时间（秒） */
#ifndef START_FAILURE_TIME_S
    #define START_FAILURE_TIME_S    5.0f
#endif

/** @brief 故障复位尝试最大次数 */
#ifndef FAULT_RESET_MAX_ATTEMPTS
    #define FAULT_RESET_MAX_ATTEMPTS 3
#endif

/** @brief 故障复位后重启延迟（秒） */
#ifndef FAULT_RESET_DELAY_S
    #define FAULT_RESET_DELAY_S     2.0f
#endif

/* ============================================================
 * 第十部分：全局宏定义 - 通信参数
 * ============================================================ */

/** @brief Modbus 从机默认地址 */
#ifndef MODBUS_SLAVE_ADDR
    #define MODBUS_SLAVE_ADDR       1
#endif

/** @brief Modbus 波特率（bps） */
#ifndef MODBUS_BAUD_RATE
    #define MODBUS_BAUD_RATE        9600UL
#endif

/** @brief Modbus 数据位 */
#ifndef MODBUS_DATA_BITS
    #define MODBUS_DATA_BITS        8
#endif

/** @brief Modbus 停止位 */
#ifndef MODBUS_STOP_BITS
    #define MODBUS_STOP_BITS        1
#endif

/** @brief Modbus 校验方式（0=无, 1=奇, 2=偶） */
#ifndef MODBUS_PARITY
    #define MODBUS_PARITY           0
#endif

/** @brief Modbus 超时时间（ms） */
#ifndef MODBUS_TIMEOUT_MS
    #define MODBUS_TIMEOUT_MS       500
#endif

/** @brief Modbus 帧间隔时间（ms） */
#ifndef MODBUS_INTER_FRAME_DELAY_MS
    #define MODBUS_INTER_FRAME_DELAY_MS 10
#endif

/** @brief Modbus CRC16 计算器预设（用于查表法） */
#define MODBUS_CRC16_INIT          0xFFFF

/** @brief CANopen 节点 ID */
#ifndef CANOPEN_NODE_ID
    #define CANOPEN_NODE_ID         0x10
#endif

/** @brief CANopen 波特率（kbps） */
#ifndef CANOPEN_BAUD_RATE
    #define CANOPEN_BAUD_RATE       500
#endif

/* ============================================================
 * 第十一部分：全局宏定义 - HMI 与显示参数
 * ============================================================ */

/** @brief HMI 显示屏类型 */
typedef enum {
    HMI_TYPE_NONE = 0,           /**< 无显示屏 */
    HMI_TYPE_LED_7SEG = 1,       /**< 7段数码管 */
    HMI_TYPE_LCD_1602 = 2,       /**< 1602 LCD */
    HMI_TYPE_LCD_2004 = 3,       /**< 2004 LCD */
    HMI_TYPE_OLED_12864 = 4,     /**< OLED 128x64 */
    HMI_TYPE_TFT_320240 = 5,     /**< TFT 320x240 */
    HMI_TYPE_TFT_480272 = 6,     /**< TFT 480x272 */
    HMI_TYPE_TFT_800480 = 7      /**< TFT 800x480 */
} HMIType_t;

/** @brief 默认 HMI 类型 */
#ifndef HMI_TYPE_DEFAULT
    #define HMI_TYPE_DEFAULT        HMI_TYPE_LED_7SEG
#endif

/** @brief 按键去抖时间（ms） */
#ifndef KEY_DEBOUNCE_TIME_MS
    #define KEY_DEBOUNCE_TIME_MS    50
#endif

/** @brief LED 闪烁频率（Hz） */
#ifndef LED_BLINK_FREQ_HZ
    #define LED_BLINK_FREQ_HZ       2.0f
#endif

/** @brief 显示刷新周期（ms） */
#ifndef DISPLAY_REFRESH_MS
    #define DISPLAY_REFRESH_MS      200
#endif

/** @brief 参数修改后自动保存延时（ms） */
#ifndef PARAM_AUTO_SAVE_DELAY_MS
    #define PARAM_AUTO_SAVE_DELAY_MS 3000
#endif

/* ============================================================
 * 第十二部分：全局宏定义 - 定时器与DMA配置
 * ============================================================ */

/** @brief ADC DMA 缓冲区大小（每通道采样次数） */
#ifndef ADC_DMA_BUF_SIZE
    #define ADC_DMA_BUF_SIZE        64
#endif

/** @brief 速度计算滑动平均窗口大小 */
#ifndef SPEED_AVG_WINDOW
    #define SPEED_AVG_WINDOW        16
#endif

/** @brief 电流采样偏移量校准次数 */
#ifndef CURRENT_OFFSET_CALIB_SAMPLES
    #define CURRENT_OFFSET_CALIB_SAMPLES 256
#endif

/** @brief 编码器计数溢出值（用于测速） */
#ifndef ENCODER_OVF_COUNT
    #define ENCODER_OVF_COUNT       0xFFFF
#endif

/** @brief PWM 中心对称模式中心值 */
#define PWM_CENTER_VALUE           (PWM_PERIOD_COUNT / 2)

/** @brief PWM 占空比上限（防止上下桥臂同时导通） */
#define PWM_DUTY_MAX               (PWM_PERIOD_COUNT - 10)
#define PWM_DUTY_MIN               10

/** @brief 最小脉宽（ns），低于此值认为关断 */
#define PWM_MIN_PULSE_NS           200

/* ============================================================
 * 第十三部分：硬件引脚定义（STM32F4 示例）
 * ============================================================ */

/* ---------- PWM 输出引脚（定时器1/8 用于三相逆变器） ---------- */

/* U相上桥臂 - TIM1_CH1 (PA8) 或 TIM8_CH1 (PC6) */
#define PWM_UH_PIN                 GPIO_PIN_8
#define PWM_UH_PORT                GPIOA
#define PWM_UH_AF                  GPIO_AF1_TIM1

/* U相下桥臂 - TIM1_CH1N (PA7) 或 TIM8_CH1N (PC7) */
#define PWM_UL_PIN                 GPIO_PIN_7
#define PWM_UL_PORT                GPIOA
#define PWM_UL_AF                  GPIO_AF1_TIM1

/* V相上桥臂 - TIM1_CH2 (PA9) 或 TIM8_CH2 (PC8) */
#define PWM_VH_PIN                 GPIO_PIN_9
#define PWM_VH_PORT                GPIOA
#define PWM_VH_AF                  GPIO_AF1_TIM1

/* V相下桥臂 - TIM1_CH2N (PB0) 或 TIM8_CH2N (PC9) */
#define PWM_VL_PIN                 GPIO_PIN_0
#define PWM_VL_PORT                GPIOB
#define PWM_VL_AF                  GPIO_AF1_TIM1

/* W相上桥臂 - TIM1_CH3 (PA10) 或 TIM8_CH3 (PC10) */
#define PWM_WH_PIN                 GPIO_PIN_10
#define PWM_WH_PORT                GPIOA
#define PWM_WH_AF                  GPIO_AF1_TIM1

/* W相下桥臂 - TIM1_CH3N (PB1) 或 TIM8_CH3N (PC11) */
#define PWM_WL_PIN                 GPIO_PIN_1
#define PWM_WL_PORT                GPIOB
#define PWM_WL_AF                  GPIO_AF1_TIM1

/* ---------- ADC 电流采样引脚 ---------- */

/* U相电流 - ADC1_IN0 (PA0) */
#define ADC_IU_PIN                 GPIO_PIN_0
#define ADC_IU_PORT                GPIOA
#define ADC_IU_CHANNEL             ADC_CHANNEL_0

/* V相电流 - ADC1_IN1 (PA1) */
#define ADC_IV_PIN                 GPIO_PIN_1
#define ADC_IV_PORT                GPIOA
#define ADC_IV_CHANNEL             ADC_CHANNEL_1

/* 直流母线电压 - ADC1_IN2 (PA2) */
#define ADC_VDC_PIN                GPIO_PIN_2
#define ADC_VDC_PORT               GPIOA
#define ADC_VDC_CHANNEL            ADC_CHANNEL_2

/* ---------- 编码器接口引脚 ---------- */

/* 编码器 A 相 - TIM3_CH1 (PA6) */
#define ENC_A_PIN                  GPIO_PIN_6
#define ENC_A_PORT                 GPIOA
#define ENC_A_AF                   GPIO_AF2_TIM3

/* 编码器 B 相 - TIM3_CH2 (PA7) */
#define ENC_B_PIN                  GPIO_PIN_7
#define ENC_B_PORT                 GPIOA
#define ENC_B_AF                   GPIO_AF2_TIM3

/* 编码器 Z 相（零位脉冲）- TIM3_CH3 (PB0) */
#define ENC_Z_PIN                  GPIO_PIN_0
#define ENC_Z_PORT                 GPIOB
#define ENC_Z_AF                   GPIO_AF2_TIM3

/* ---------- Hall 传感器引脚 ---------- */

/* Hall U - TIM4_CH1 (PB6) */
#define HALL_U_PIN                 GPIO_PIN_6
#define HALL_U_PORT                GPIOB
#define HALL_U_AF                  GPIO_AF2_TIM4

/* Hall V - TIM4_CH2 (PB7) */
#define HALL_V_PIN                 GPIO_PIN_7
#define HALL_V_PORT                GPIOB
#define HALL_V_AF                  GPIO_AF2_TIM4

/* Hall W - TIM4_CH3 (PB8) */
#define HALL_W_PIN                 GPIO_PIN_8
#define HALL_W_PORT                GPIOB
#define HALL_W_AF                  GPIO_AF2_TIM4

/* ---------- 数字输入引脚（启停/方向/故障） ---------- */

/* 启动按钮 - PC0 */
#define DIN_START_PIN              GPIO_PIN_0
#define DIN_START_PORT             GPIOC

/* 停止按钮 - PC1 */
#define DIN_STOP_PIN               GPIO_PIN_1
#define DIN_STOP_PORT              GPIOC

/* 方向选择 - PC2 */
#define DIN_DIR_PIN                GPIO_PIN_2
#define DIN_DIR_PORT               GPIOC

/* 外部故障输入 - PC3 */
#define DIN_FAULT_PIN              GPIO_PIN_3
#define DIN_FAULT_PORT             GPIOC

/* ---------- 数字输出引脚（继电器/接触器/报警） ---------- */

/* 主接触器控制 - PD2 */
#define DOUT_CONTACTOR_PIN         GPIO_PIN_2
#define DOUT_CONTACTOR_PORT        GPIOD

/* 制动电阻接触器 - PD3 */
#define DOUT_BRAKE_PIN             GPIO_PIN_3
#define DOUT_BRAKE_PORT            GPIOD

/* 故障报警输出 - PD4 */
#define DOUT_ALARM_PIN             GPIO_PIN_4
#define DOUT_ALARM_PORT            GPIOD

/* 运行指示输出 - PD5 */
#define DOUT_RUNNING_PIN           GPIO_PIN_5
#define DOUT_RUNNING_PORT          GPIOD

/* ---------- 通信串口引脚 ---------- */

/* USART3 - Modbus RTU (RS485) - PB10=TX, PB11=RX */
#define MODBUS_TX_PIN              GPIO_PIN_10
#define MODBUS_TX_PORT             GPIOB
#define MODBUS_TX_AF               GPIO_AF7_USART3

#define MODBUS_RX_PIN              GPIO_PIN_11
#define MODBUS_RX_PORT             GPIOB
#define MODBUS_RX_AF               GPIO_AF7_USART3

/* USART3 RTS (RS485 方向控制) - PD1 */
#define MODBUS_RTS_PIN             GPIO_PIN_1
#define MODBUS_RTS_PORT            GPIOD

/* ---------- 调试串口引脚 ---------- */

/* USART1 - 调试串口 (PA9=TX, PA10=RX) */
#define DEBUG_TX_PIN               GPIO_PIN_9
#define DEBUG_TX_PORT              GPIOA
#define DEBUG_TX_AF                GPIO_AF7_USART1

#define DEBUG_RX_PIN               GPIO_PIN_10
#define DEBUG_RX_PORT              GPIOA
#define DEBUG_RX_AF                GPIO_AF7_USART1

/* ---------- 模拟量输入引脚 ---------- */

/* 速度给定电位器 - ADC1_IN3 (PA3) */
#define AI_SPEED_REF_PIN           GPIO_PIN_3
#define AI_SPEED_REF_PORT          GPIOA
#define AI_SPEED_REF_CHANNEL       ADC_CHANNEL_3

/* ---------- 温度采样引脚 ---------- */

/* IPM 温度检测 - ADC1_IN4 (PA4) */
#define AI_IPM_TEMP_PIN            GPIO_PIN_4
#define AI_IPM_TEMP_PORT           GPIOA
#define AI_IPM_TEMP_CHANNEL        ADC_CHANNEL_4

/* 电机温度检测（PTC 热敏电阻）- ADC1_IN5 (PA5) */
#define AI_MOTOR_TEMP_PIN          GPIO_PIN_5
#define AI_MOTOR_TEMP_PORT         GPIOA
#define AI_MOTOR_TEMP_CHANNEL      ADC_CHANNEL_5

/* ---------- LED 指示灯引脚 ---------- */

/* 电源指示灯 - PC4 */
#define LED_POWER_PIN              GPIO_PIN_4
#define LED_POWER_PORT             GPIOC

/* 运行指示灯 - PC5 */
#define LED_RUN_PIN                GPIO_PIN_5
#define LED_RUN_PORT               GPIOC

/* 故障指示灯 - PC6 */
#define LED_FAULT_PIN              GPIO_PIN_6
#define LED_FAULT_PORT             GPIOC

/* ---------- SWD 调试接口 ---------- */

/* SWDIO - PA13 */
#define SWDIO_PIN                  GPIO_PIN_13
#define SWDIO_PORT                 GPIOA

/* SWCLK - PA14 */
#define SWCLK_PIN                  GPIO_PIN_14
#define SWCLK_PORT                 GPIOA

/* ============================================================
 * 第十四部分：参数索引定义（PARAM_idx_xxx，从0开始）
 * 至少 50 个参数，覆盖电机、V/F、保护、通信、PID等
 * ============================================================ */

/**
 * @brief 参数索引枚举（统一参数系统）
 * 与 Modbus 保持一一对应，便于读写
 */
typedef enum {
    /* ---- 电机参数 (0x0000 - 0x000F) ---- */
    PARAM_idxMotorType = 0,           /**< 0  电机类型 */
    PARAM_idxMotorPolePairs,          /**< 1  极对数 */
    PARAM_idxMotorRatedPower,         /**< 2  额定功率 (kW) */
    PARAM_idxMotorRatedVoltage,       /**< 3  额定电压 (V) */
    PARAM_idxMotorRatedCurrent,       /**< 4  额定电流 (A) */
    PARAM_idxMotorRatedSpeed,         /**< 5  额定转速 (rpm) */
    PARAM_idxMotorRatedFreq,          /**< 6  额定频率 (Hz) */
    PARAM_idxMotorRatedTorque,        /**< 7  额定转矩 (N·m) */
    PARAM_idxMotorPowerFactor,        /**< 8  功率因数 */
    PARAM_idxMotorEfficiency,         /**< 9  效率 */
    PARAM_idxMotorStatorResistance,   /**< 10 定子电阻 (Ω) */
    PARAM_idxMotorRotorResistance,    /**< 11 转子电阻 (Ω) - 仅IM */
    PARAM_idxMotorLeakageInductance,  /**< 12 漏感 (H) */
    PARAM_idxMotorMutualInductance,   /**< 13 互感 (H) */
    PARAM_idxMotorBemfConstant,       /**< 14 反电动势常数 */
    PARAM_idxMotorInertia,            /**< 15 转动惯量 */

    /* ---- 编码器与传感器参数 (0x0010 - 0x001F) ---- */
    PARAM_idxEncoderType = 16,        /**< 16 编码器类型 */
    PARAM_idxEncoderPPR,              /**< 17 编码器线数 (PPR) */
    PARAM_idxEncoderDirection,        /**< 18 编码器方向 */
    PARAM_idxHallSpeedRatio,          /**< 19 Hall/编码器速度比 */
    PARAM_idxResolverPolePairs,       /**< 20 旋变极对数 */
    PARAM_idxSensorlessObserverGain,  /**< 21 无传感器观测器增益 */

    /* ---- V/F 控制参数 (0x0020 - 0x002F) ---- */
    PARAM_idxVFStartVoltRatio = 32,   /**< 32 V/F 起始电压比 (%) */
    PARAM_idxVFMidVoltRatio,          /**< 33 V/F 中频电压比 (%) */
    PARAM_idxVFMidFreqRatio,          /**< 34 V/F 中频频率比 (%) */
    PARAM_idxVFKneeFreqRatio,         /**< 35 V/F 转折频率比 (%) */
    PARAM_idxVFBoostGain,             /**< 36 V/F 补偿增益 */
    PARAM_idxVFAccelTime,             /**< 37 V/F 加速时间 (s) */
    PARAM_idxVFDecelTime,             /**< 38 V/F 减速时间 (s) */
    PARAM_idxVFFreqMin,               /**< 39 最小频率 (Hz) */
    PARAM_idxVFFreqMax,               /**< 40 最大频率 (Hz) */

    /* ---- FOC 控制参数 (0x0030 - 0x004F) ---- */
    PARAM_idxFOCSpeedBW = 48,         /**< 48 FOC 速度环带宽 (Hz) */
    PARAM_idxFOCCurrentBW,            /**< 49 FOC 电流环带宽 (Hz) */
    PARAM_idxFOCIdKp,                 /**< 50 d轴电流 Kp */
    PARAM_idxFOCIdKi,                 /**< 51 d轴电流 Ki */
    PARAM_idxFOCIdKd,                 /**< 52 d轴电流 Kd */
    PARAM_idxFOCIdMax,                /**< 53 d轴电流限幅 */
    PARAM_idxFOCIqKp,                 /**< 54 q轴电流 Kp */
    PARAM_idxFOCIqKi,                 /**< 55 q轴电流 Ki */
    PARAM_idxFOCIqKd,                 /**< 56 q轴电流 Kd */
    PARAM_idxFOCIqMax,                /**< 57 q轴电流限幅 */
    PARAM_idxFOCSpeedKp,              /**< 58 速度环 Kp */
    PARAM_idxFOCSpeedKi,              /**< 59 速度环 Ki */
    PARAM_idxFOCSpeedKd,              /**< 60 速度环 Kd */
    PARAM_idxFOCFluxKp,               /**< 61 磁链环 Kp */
    PARAM_idxFOCFluxKi,               /**< 62 磁链环 Ki */
    PARAM_idxFOCFluxMin,              /**< 63 最小磁链 */
    PARAM_idxFOCEnableMTPA,           /**< 64 MTPA 使能 */
    PARAM_idxFOCEnableFluxWeak,       /**< 65 弱磁使能 */

    /* ---- PID 参数 (0x0050 - 0x006F) ---- */
    PARAM_idxPIDMode = 80,            /**< 80 PID 工作模式 */
    PARAM_idxPIDTarget,               /**< 81 PID 目标值 */
    PARAM_idxPIDFeedback,             /**< 82 PID 反馈值 */
    PARAM_idxPIDOutput,               /**< 83 PID 输出值 */
    PARAM_idxPIDKp,                   /**< 84 PID 比例增益 */
    PARAM_idxPIDKi,                   /**< 85 PID 积分增益 */
    PARAM_idxPIDKd,                   /**< 86 PID 微分增益 */
    PARAM_idxPIDOutputMax,            /**< 87 PID 输出上限 */
    PARAM_idxPIDOutputMin,            /**< 88 PID 输出下限 */
    PARAM_idxPIDDeadzone,             /**< 89 PID 死区 */
    PARAM_idxPIDILimit,               /**< 90 PID 积分限幅 */
    PARAM_idxPIDDerivFilter,          /**< 91 PID 微分滤波系数 */

    /* ---- 速度控制参数 (0x0070 - 0x007F) ---- */
    PARAM_idxSpeedRef = 112,          /**< 112 速度给定 (rpm) */
    PARAM_idxSpeedFdb,                /**< 113 速度反馈 (rpm) */
    PARAM_idxSpeedAccelTime,          /**< 114 加速时间 (s) */
    PARAM_idxSpeedDecelTime,          /**< 115 减速时间 (s) */
    PARAM_idxSpeedMax,                /**< 116 最大转速 (rpm) */
    PARAM_idxSpeedMin,                /**< 117 最小转速 (rpm) */
    PARAM_idxSpeedFilterCoeff,        /**< 118 速度滤波系数 */

    /* ---- 启动/停机参数 (0x0080 - 0x008F) ---- */
    PARAM_idxStartMode = 128,         /**< 128 启动模式 */
    PARAM_idxStartDelay,              /**< 129 启动延时 (s) */
    PARAM_idxStopMode,                /**< 130 停机模式 */
    PARAM_idxStopDelay,               /**< 131 停机延时 (s) */
    PARAM_idxBrakeMode,               /**< 132 制动方式 */
    PARAM_idxBrakeTime,               /**< 133 制动时间 (s) */
    PARAM_idxDCBrakeCurrent,          /**< 134 直流制动电流 (A) */
    PARAM_idxDCBrakeTime,             /**< 135 直流制动时间 (s) */

    /* ---- 保护参数 (0x0090 - 0x00AF) ---- */
    PARAM_idxProtectEn = 144,         /**< 144 保护总使能 */
    PARAM_idxOVCurrentThresh,         /**< 145 过流阈值 (A) */
    PARAM_idxOVCurrentTime,           /**< 146 过流时间 (s) */
    PARAM_idxShortCircuitThresh,      /**< 147 短路阈值 (A) */
    PARAM_idxOVVoltageThresh,         /**< 148 过压阈值 (V) */
    PARAM_idxOVVoltageRecovery,       /**< 149 过压恢复值 (V) */
    PARAM_idxUVVoltageThresh,         /**< 150 欠压阈值 (V) */
    PARAM_idxUVVoltageRecovery,       /**< 151 欠压恢复值 (V) */
    PARAM_idxIPMTempThresh,           /**< 152 IPM 过温阈值 (℃) */
    PARAM_idxIPMTempRecovery,         /**< 153 IPM 过温恢复 (℃) */
    PARAM_idxMotorTempThresh,         /**< 154 电机过温阈值 (℃) */
    PARAM_idxMotorTempRecovery,       /**< 155 电机过温恢复 (℃) */
    PARAM_idxOverloadRatio,           /**< 156 过载电流倍数 */
    PARAM_idxOverloadTime,            /**< 157 过载时间 (s) */
    PARAM_idxLockedRotorTime,         /**< 158 堵转时间 (s) */
    PARAM_idxGroundFaultThresh,       /**< 159 接地故障阈值 (mA) */

    /* ---- 通信参数 (0x00B0 - 0x00CF) ---- */
    PARAM_idxCommProtocol = 176,      /**< 176 通信协议 */
    PARAM_idxModbusAddr,              /**< 177 Modbus 从机地址 */
    PARAM_idxModbusBaudrate,          /**< 178 Modbus 波特率 */
    PARAM_idxModbusParity,            /**< 179 Modbus 校验 */
    PARAM_idxModbusTimeout,           /**< 180 Modbus 超时 (ms) */
    PARAM_idxCANNodeID,               /**< 181 CANopen 节点ID */
    PARAM_idxCANBaudrate,             /**< 182 CANopen 波特率 */

    /* ---- 显示与人机参数 (0x00D0 - 0x00DF) ---- */
    PARAM_idxHMIType = 208,           /**< 208 HMI 类型 */
    PARAM_idxDisplayBrightness,       /**< 209 显示亮度 */
    PARAM_idxDisplayContrast,         /**< 210 显示对比度 */
    PARAM_idxKeypadLock,              /**< 211 键盘锁定 */
    PARAM_idxLanguage,                /**< 212 语言选择 */
    PARAM_idxUnitSystem,              /**< 213 单位制 (公制/英制) */

    /* ---- 运行状态参数（只读）(0x00E0 - 0x00EF) ---- */
    PARAM_idxRunState = 224,          /**< 224 运行状态 */
    PARAM_idxRunMode,                 /**< 225 运行模式 */
    PARAM_idxFaultCode,               /**< 226 故障代码 */
    PARAM_idxFaultHistory1,           /**< 227 历史故障1 */
    PARAM_idxFaultHistory2,           /**< 228 历史故障2 */
    PARAM_idxFaultHistory3,           /**< 229 历史故障3 */
    PARAM_idxDCBusVoltage,            /**< 230 母线电压 (V) */
    PARAM_idxOutputFrequency,         /**< 231 输出频率 (Hz) */
    PARAM_idxOutputCurrent,           /**< 232 输出电流 (A) */
    PARAM_idxOutputPower,             /**< 233 输出功率 (kW) */
    PARAM_idxMotorTemp,               /**< 234 电机温度 (℃) */
    PARAM_idxIPMTemp,                 /**< 235 IPM 温度 (℃) */
    PARAM_idxTorqueActual,            /**< 236 实际转矩 (N·m) */
    PARAM_idxFluxActual,              /**< 237 实际磁链 (Wb) */
    PARAM_idxIdActual,                /**< 238 d轴电流 (A) */
    PARAM_idxIqActual,                /**< 239 q轴电流 (A) */

    /* ---- 校准参数 (0x00F0 - 0x00FF) ---- */
    PARAM_idxCurrentOffsetU = 240,    /**< 240 U相电流偏置 */
    PARAM_idxCurrentOffsetV,          /**< 241 V相电流偏置 */
    PARAM_idxVoltageOffset,           /**< 242 电压偏置 */
    PARAM_idxEncoderOffset,           /**< 243 编码器角度偏置 */
    PARAM_idxHallOffset,              /**< 244 Hall 角度偏置 */
    PARAM_idxGainAdjuFactor,          /**< 245 增益调整系数 */
    PARAM_idxCalibStatus,             /**< 246 校准状态 */
    PARAM_idxFactoryReset,            /**< 247 恢复出厂设置 */

    /* ---- 统计与运行记录 (0x0100 - 0x010F) ---- */
    PARAM_idxRunTimeHour = 256,       /**< 256 运行时间 (小时) */
    PARAM_idxPowerOnCount,            /**< 257 开机次数 */
    PARAM_idxEnergyConsumed,          /**< 258 累计能耗 (kWh) */
    PARAM_idxFaultCount,              /**< 259 故障总次数 */
    PARAM_idxSoftwareVersion,         /**< 260 软件版本 */
    PARAM_idxHardwareVersion,         /**< 261 硬件版本 */
    PARAM_idxSerialNumber,            /**< 262 产品序列号 */
    PARAM_idxManufactureDate,         /**< 263 生产日期 */

    /* ---- 多段速参数 (0x0110 - 0x011F) ---- */
    PARAM_idxMultiSpeed1 = 272,       /**< 272 段速1 (rpm) */
    PARAM_idxMultiSpeed2,             /**< 273 段速2 */
    PARAM_idxMultiSpeed3,             /**< 274 段速3 */
    PARAM_idxMultiSpeed4,             /**< 275 段速4 */
    PARAM_idxMultiSpeed5,             /**< 276 段速5 */
    PARAM_idxMultiSpeed6,             /**< 277 段速6 */
    PARAM_idxMultiSpeed7,             /**< 278 段速7 */
    PARAM_idxMultiSpeed8,             /**< 279 段速8 */
    PARAM_idxMultiAccelTime,          /**< 280 多段速加速时间 */
    PARAM_idxMultiDecelTime,          /**< 281 多段速减速时间 */

    /* ---- 预留参数 (0x0120 - 0x01FF) ---- */
    /* 可根据项目需求扩展至 512 个参数 */

    PARAM_idxReservedStart = 288,     /**< 288 预留参数起始 */
    /* ... 预留至 0x01FF (511) */

    PARAM_idxMAX = 512                /**< 参数总数（最大值标记） */
} ParamIndex_t;

/* ============================================================
 * 第十五部分：Modbus 寄存器映射定义
 * ============================================================ */

/**
 * @brief Modbus 保持寄存器映射表
 * Modbus 功能码 0x03（读保持寄存器）/ 0x06（写单个）/ 0x10（写多个）
 * 寄存器地址 = PARAM_idx / 2（16位对齐）
 */

/* Modbus 寄存器地址定义（从 0 开始） */
#define MODBUS_REG_MOTOR_TYPE       0       /**< 0x0000 电机类型 */
#define MODBUS_REG_MOTOR_PP         1       /**< 0x0001 极对数 */
#define MODBUS_REG_MOTOR_POWER      2       /**< 0x0002 额定功率 */
#define MODBUS_REG_MOTOR_VOLTAGE    3       /**< 0x0003 额定电压 */
#define MODBUS_REG_MOTOR_CURRENT    4       /**< 0x0004 额定电流 */
#define MODBUS_REG_MOTOR_SPEED      5       /**< 0x0005 额定转速 */
#define MODBUS_REG_MOTOR_FREQ       6       /**< 0x0006 额定频率 */
#define MODBUS_REG_MOTOR_TORQUE     7       /**< 0x0007 额定转矩 */

#define MODBUS_REG_SPEED_REF        20      /**< 0x0014 速度给定 */
#define MODBUS_REG_SPEED_FDB        21      /**< 0x0015 速度反馈 */
#define MODBUS_REG_SPEED_MAX        22      /**< 0x0016 最大速度 */

#define MODBUS_REG_RUN_STATE        30      /**< 0x001E 运行状态 */
#define MODBUS_REG_RUN_MODE         31      /**< 0x001F 运行模式 */
#define MODBUS_REG_FAULT_CODE       32      /**< 0x0020 故障代码 */
#define MODBUS_REG_FAULT_RESET      33      /**< 0x0021 故障复位（写1复位） */

#define MODBUS_REG_DCBUS_VOLTAGE    40      /**< 0x0028 母线电压 */
#define MODBUS_REG_OUTPUT_FREQ      41      /**< 0x0029 输出频率 */
#define MODBUS_REG_OUTPUT_CURRENT   42      /**< 0x002A 输出电流 */
#define MODBUS_REG_OUTPUT_POWER     43      /**< 0x002B 输出功率 */

#define MODBUS_REG_MODBUS_ADDR      100     /**< 0x0064 Modbus 地址 */
#define MODBUS_REG_MODBUS_BAUD      101     /**< 0x0065 波特率 */
#define MODBUS_REG_MODBUS_PARITY    102     /**< 0x0066 校验位 */

#define MODBUS_REG_PID_KP           120     /**< 0x0078 PID Kp */
#define MODBUS_REG_PID_KI           121     /**< 0x0079 PID Ki */
#define MODBUS_REG_PID_KD           122     /**< 0x007A PID Kd */
#define MODBUS_REG_PID_TARGET       123     /**< 0x007B PID 目标值 */

#define MODBUS_REG_PARAM_START      0       /**< 参数区起始地址 */
#define MODBUS_REG_PARAM_COUNT      512     /**< 参数总数 */
#define MODBUS_REG_RO_PARAM_START   224     /**< 只读参数起始 (RunState) */
#define MODBUS_REG_RO_PARAM_COUNT   48      /**< 只读参数数量 */

/* Modbus 错误码定义 */
#define MODBUS_EXC_ILLEGAL_FUNC     0x01    /**< 非法功能码 */
#define MODBUS_EXC_ILLEGAL_ADDR     0x02    /**< 非法数据地址 */
#define MODBUS_EXC_ILLEGAL_VALUE    0x03    /**< 非法数据值 */
#define MODBUS_EXC_SLAVE_FAILURE    0x04    /**< 从机故障 */
#define MODBUS_EXC_ACK              0x05    /**< 确认 */
#define MODBUS_EXC_SLAVE_BUSY       0x06    /**< 从机忙 */
#define MODBUS_EXC_MEM_PARITY       0x08    /**< 存储奇偶校验错误 */

/* ============================================================
 * 第十六部分：调试宏定义
 * ============================================================ */

#ifdef ENABLE_DEBUG
    /* 调试打印宏（需实现具体的打印函数） */
    #define DEBUG_PRINT(fmt, ...)    printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
    #define DEBUG_WARN(fmt, ...)     printf("[WARN]  " fmt "\n", ##__VA_ARGS__)
    #define DEBUG_ERROR(fmt, ...)    printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
    #define DEBUG_INFO(fmt, ...)     printf("[INFO]  " fmt "\n", ##__VA_ARGS__)

    /* 示波器跟踪宏（用于 DAC 或 UART 输出波形数据） */
    #define TRACE_VAR(var)          trace_send(#var, var)
    #define TRACE_3CH(a, b, c)      trace_3channel(a, b, c)
    #define TRACE_PULSE(ch, val)    trace_pulse(ch, val)

    /* 调试 LED 闪烁 */
    #define DEBUG_TOGGLE(pin)       HAL_GPIO_TogglePin(pin##_PORT, pin##_PIN)
#else
    #define DEBUG_PRINT(fmt, ...)    ((void)0)
    #define DEBUG_WARN(fmt, ...)     ((void)0)
    #define DEBUG_ERROR(fmt, ...)    ((void)0)
    #define DEBUG_INFO(fmt, ...)     ((void)0)
    #define TRACE_VAR(var)          ((void)0)
    #define TRACE_3CH(a, b, c)      ((void)0)
    #define TRACE_PULSE(ch, val)    ((void)0)
    #define DEBUG_TOGGLE(pin)       ((void)0)
#endif /* ENABLE_DEBUG */

/* 断言宏（调试模式下生效） */
#ifdef ENABLE_DEBUG
    #define ASSERT(expr)            ((expr) ? (void)0 : assert_failed(#expr, __FILE__, __LINE__))
    extern void assert_failed(const char *expr, const char *file, uint32_t line);
#else
    #define ASSERT(expr)            ((void)0)
#endif

/* 性能分析宏 */
#ifdef ENABLE_SCOPE_TRACE
    #define PROFILE_ENTER(name)     profile_enter(name)
    #define PROFILE_EXIT(name)      profile_exit(name)
    #define PROFILE_RESET()         profile_reset()
    extern void profile_enter(const char *name);
    extern void profile_exit(const char *name);
    extern void profile_reset(void);
#else
    #define PROFILE_ENTER(name)     ((void)0)
    #define PROFILE_EXIT(name)      ((void)0)
    #define PROFILE_RESET()         ((void)0)
#endif

/* ============================================================
 * 第十七部分：通用工具宏
 * ============================================================ */

/** @brief 禁用所有中断 */
#define DISABLE_INTERRUPTS()        __asm volatile ("cpsid i")

/** @brief 使能所有中断 */
#define ENABLE_INTERRUPTS()         __asm volatile ("cpsie i")

/** @brief 插入内存屏障（确保指令顺序） */
#define MEMORY_BARRIER()            __asm volatile ("dsb sy")

/** @brief 插入同步障碍 */
#define SYNC_BARRIER()              __asm volatile ("isb sy")

/** @brief 计算数组元素个数 */
#define ARRAY_SIZE(arr)             (sizeof(arr) / sizeof((arr)[0]))

/** @brief 计算结构体成员偏移量 */
#define OFFSET_OF(type, member)     ((uint32_t)(&((type *)0)->member))

/** @brief 根据指针和偏移量获取结构体成员地址 */
#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - OFFSET_OF(type, member)))

/** @brief 限制数值在指定范围内 */
#define LIMIT(val, min, max)        do { if (val < (min)) val = (min); if (val > (max)) val = (max); } while (0)

/** @brief 限制数值在 [0, max] 范围内 */
#define LIMIT_MAX(val, max)         do { if (val > (max)) val = (max); } while (0)

/** @brief 限制数值在 [min, +∞] 范围内 */
#define LIMIT_MIN(val, min)         do { if (val < (min)) val = (min); } while (0)

/** @brief 判断数值是否在范围内 */
#define IN_RANGE(val, min, max)     ((val) >= (min) && (val) <= (max))

/** @brief 将数值从一种范围映射到另一种范围（线性映射） */
#define MAP_RANGE(val, in_min, in_max, out_min, out_max) \
    (((val) - (in_min)) * ((out_max) - (out_min)) / ((in_max) - (in_min)) + (out_min))

/** @brief 将角度归一化到 [0, 360) 范围 */
#define NORMALIZE_ANGLE_360(angle)  fmodf(fmodf(angle, 360.0f) + 360.0f, 360.0f)

/** @brief 将角度归一化到 [-180, 180) 范围 */
#define NORMALIZE_ANGLE_180(angle)  do { \
    float _a = NORMALIZE_ANGLE_360(angle); \
    if (_a >= 180.0f) _a -= 360.0f; \
    angle = _a; \
} while (0)

/** @brief 将电角度归一化到 [0, 2π) 范围 */
#define NORMALIZE_ELEC_ANGLE(angle) fmodf(fmodf(angle, 6.28318530718f) + 6.28318530718f, 6.28318530718f)

/** @brief 角度转弧度 */
#define DEG_TO_RAD(deg)             ((deg) * 0.017453292519943f)

/** @brief 弧度转角度 */
#define RAD_TO_DEG(rad)             ((rad) * 57.2957795130823f)

/** @brief rpm 转速转 rad/s 角速度 */
#define RPM_TO_RADS(rpm)            ((rpm) * 0.10471975511966f)

/** @brief rad/s 角速度转 rpm 转速 */
#define RADS_TO_RPM(rads)           ((rads) * 9.5492965855137f)

/** @brief 判断浮点数是否有效（不是 NaN） */
#define IS_FLOAT_VALID(val)         (!isnan(val) && !isinf(val))

/** @brief 设置位 */
#define SET_BIT(reg, bit)           ((reg) |= (bit))

/** @brief 清除位 */
#define CLEAR_BIT(reg, bit)         ((reg) &= ~(bit))

/** @brief 读取位 */
#define READ_BIT(reg, bit)          ((reg) & (bit))

/** @brief 读取位并清除 */
#define READ_CLEAR_BIT(reg, bit)    ((reg) & (bit))

/** @brief 位域读写宏（reg: 变量, field: 位域名, value: 写入值） */
#define BF_SET(reg, field, value)   ((reg) = ((reg) & ~(field##_Msk)) | ((value) << (field##_Pos)))
#define BF_GET(reg, field)          (((reg) & (field##_Msk)) >> (field##_Pos))

/** @brief 无操作延迟（简单延时） */
#define NOP()                       __asm volatile ("nop")

/** @brief 软复位（系统复位） */
#define SYSTEM_RESET()              NVIC_SystemReset()

/** @brief 使能看门狗 */
#define WATCHDOG_ENABLE()           IWDG->KR = 0xCCCC

/** @brief 喂狗 */
#define WATCHDOG_FEED()             IWDG->KR = 0xAAAA

/* ============================================================
 * 第十八部分：结构体前向声明与类型别名
 * ============================================================ */

/* 前向声明 */
struct MotorParam_s;
struct FOC_s;
struct PID_s;
struct Modbus_s;
struct Protection_s;
struct Encoder_s;
struct HallSensor_s;

/* 类型别名（指针） */
typedef struct MotorParam_s   MotorParam_t;
typedef struct FOC_s          FOC_t;
typedef struct PID_s          PID_t;
typedef struct Modbus_s       Modbus_t;
typedef struct Protection_s   Protection_t;
typedef struct Encoder_s      Encoder_t;
typedef struct HallSensor_s   HallSensor_t;

/* ============================================================
 * 第十九部分：版本与兼容性定义
 * ============================================================ */

/** @brief 软件主版本号 */
#define SW_VERSION_MAJOR            1

/** @brief 软件次版本号 */
#define SW_VERSION_MINOR            0

/** @brief 软件修订号 */
#define SW_VERSION_PATCH            0

/** @brief 软件版本字符串 */
#define SW_VERSION_STRING           "V1.0.0"

/** @brief 固件构建日期（由编译器自动生成） */
#define __BUILD_DATE_STR__          __DATE__ " " __TIME__

/** @brief 编译日期时间宏 */
#define BUILD_DATE                  __DATE__
#define BUILD_TIME                  __TIME__

/** @brief 代码编译标识（用于判断是否重新标定） */
#define BUILD_HASH                  0x00000000

/** @brief 参数版本号（参数结构体变更时递增） */
#define PARAM_TABLE_VERSION         1

/** @brief 兼容的最大参数表版本 */
#define PARAM_TABLE_COMPAT_MAX      1

/* ============================================================
 * 第二十部分：条件编译平台适配
 * ============================================================ */

/* STM32 HAL 库适配 */
#if defined(MCU_FAMILY_STM32)
    /* STM32 HAL 头文件 */
    #include "stm32f4xx_hal.h"  /* 根据实际芯片修改 */

    /* ADC 多通道 DMA 配置 */
    #define ADC_DMA_CHANNEL         DMA_CHANNEL_0
    #define ADC_DMA_STREAM          DMA_STREAM_0

    /* UART 句柄 */
    extern UART_HandleTypeDef huart3;  /* Modbus */
    extern UART_HandleTypeDef huart1;  /* Debug */

    /* TIM 句柄 */
    extern TIM_HandleTypeDef htim1;    /* PWM */
    extern TIM_HandleTypeDef htim3;    /* Encoder */

    /* 标志位类型 */
    #define FlagStatus              uint32_t
    #define SET                     1
    #define RESET                   0

    /* GPIO 操作宏（简化） */
    #define GPIO_WRITE(port, pin, val)  HAL_GPIO_WritePin(port, pin, val)
    #define GPIO_READ(port, pin)        HAL_GPIO_ReadPin(port, pin)
    #define GPIO_TOGGLE(port, pin)      HAL_GPIO_TogglePin(port, pin)

    /* 延时函数宏 */
    #define DELAY_MS(ms)            HAL_Delay(ms)
    #define DELAY_US(us)            delay_us(us)

    /* 获取系统时钟 */
    #define GET_SYSCLK_FREQ()       HAL_RCC_GetSysClockFreq()

/* DSP28335 适配 */
#elif defined(MCU_DSP28335)
    /* DSP28335 头文件 */
    #include "DSP2833x_Device.h"
    #include "DSP2833x_Examples.h"

    /* GPIO 寄存器定义（复用器配置） */
    extern Uint16 GpioDataRegs[];

    /* 中断使能/禁止 */
    #define ENABLE_INT()            IER |= 0x0001
    #define DISABLE_INT()           IER &= 0xFFFE

    /* 延时宏 */
    #define DELAY_US(us)            DELAY_US_A(us)
    #define DELAY_MS(ms)            delay_ms(ms)

    /* 看门狗 */
    #define WATCHDOG_ENABLE()       EALLOW; SysCtrlRegs.WDCR = 0x0028; EDIS
    #define WATCHDOG_FEED()         EALLOW; SysCtrlRegs.WDKEY = 0x0055; SysCtrlRegs.WDKEY = 0x00AA; EDIS

    /* 软复位 */
    #define SYSTEM_RESET()         asm("  .ref _c_int00\n  LB _c_int00")

#endif /* MCU_FAMILY_STM32 / MCU_DSP28335 */

/* ============================================================
 * 第二十一部分：函数属性声明
 * ============================================================ */

/* 编译期函数属性 */
#define __WEAK       __attribute__((weak))          /**< 弱符号（可被覆盖） */
#define __INLINE     __attribute__((always_inline)) /**< 强制内联 */
#define __ALIGN(n)   __attribute__((aligned(n)))    /**< 指定对齐 */
#define __PACKED     __attribute__((packed))        /**< 紧凑结构体 */
#define __SECTION(x) __attribute__((section(x)))   /**< 指定链接段 */

/* 内存区域属性 */
#define __RAM_FUNC   __attribute__((section(".ramfunc"))) /**< 运行于 RAM 的函数 */

/* 变量属性 */
#define __ZERO_INIT  __attribute__((zero_init))     /**< 零初始化 */

/* 声明一个变量/函数在指定段中（用于固件版本信息等） */
#define __VERSION_SECTION    __attribute__((section(".version_info")))

/* 废弃 API 警告 */
#define __DEPRECATED         __attribute__((deprecated))

/* 防止编译器优化掉（用于关键变量） */
#define __VOLATILE           volatile

/* ============================================================
 * 第二十二部分：固件信息段（链接脚本需配合）
 * ============================================================ */

typedef struct {
    uint32_t magic;             /* 魔术字 0xA5A5A5A5 */
    uint16_t version;           /* 固件版本 */
    uint16_t param_version;     /* 参数表版本 */
    uint32_t build_date;        /* 构建日期 */
    uint32_t build_time;        /* 构建时间 */
    uint32_t crc32;             /* 固件 CRC32 */
    uint32_t flash_size;        /* Flash 大小 */
    uint32_t ram_size;          /* RAM 大小 */
    uint32_t reserved[4];       /* 预留 */
} FirmwareInfo_t;

/* 固件信息符号（由链接脚本定义或手动放置） */
extern const FirmwareInfo_t __firmware_info __VERSION_SECTION;

/* ============================================================
 * 第二十三部分：校准与配置常量
 * ============================================================ */

/* 电流传感器增益（根据实际硬件配置） */
#ifndef CURRENT_SENSOR_GAIN
    #define CURRENT_SENSOR_GAIN     0.01f   /* 50A/5000mV -> 0.01 V/A */
#endif

/* 电压传感器分压比 */
#ifndef VOLTAGE_DIVIDER_RATIO
    #define VOLTAGE_DIVIDER_RATIO   0.00488f /* 1k/200k 分压 -> 1/204.8 */
#endif

/* ADC 参考电压（V） */
#ifndef ADC_VREF_MV
    #define ADC_VREF_MV             3300.0f
#endif

/* ADC 分辨率（位数） */
#ifndef ADC_RESOLUTION_BITS
    #define ADC_RESOLUTION_BITS     12
#endif

/* ADC 最大计数值 */
#define ADC_MAX_COUNT              ((1 << ADC_RESOLUTION_BITS) - 1)

/* 电流零点偏置校准值（默认值，需实际校准） */
#ifndef CURRENT_OFFSET_U_DEFAULT
    #define CURRENT_OFFSET_U_DEFAULT (ADC_MAX_COUNT / 2)
#endif
#ifndef CURRENT_OFFSET_V_DEFAULT
    #define CURRENT_OFFSET_V_DEFAULT (ADC_MAX_COUNT / 2)
#endif

/* NTC 热敏电阻 B 值（用于温度计算） */
#ifndef NTC_B_VALUE
    #define NTC_B_VALUE             3950.0f
#endif

/* NTC 25℃标称阻值（Ω） */
#ifndef NTC_R25
    #define NTC_R25                 10000.0f
#endif

/* ============================================================
 * 第二十四部分：运行时状态结构体（简化版示例）
 * ============================================================ */

/**
 * @brief 系统运行时状态结构体
 * @note 此类结构体用于在中断与主循环之间共享数据
 */
typedef struct {
    /* 运行状态（标志位方式，节省内存） */
    uint32_t flags;

    /* 速度与位置 */
    float speed_ref_rpm;        /**< 速度给定 (rpm) */
    float speed_fdb_rpm;        /**< 速度反馈 (rpm) */
    float elec_angle_rad;       /**< 电角度 (rad) */
    float mech_angle_rad;       /**< 机械角度 (rad) */

    /* 电流 */
    float iu_adc;               /**< U相电流原始 ADC 值 */
    float iv_adc;               /**< V相电流原始 ADC 值 */
    float iu_A;                 /**< U相电流 (A) */
    float iv_A;                 /**< V相电流 (A) */
    float iw_A;                 /**< W相电流 (A，三相和为0） */
    float id_A;                 /**< d轴电流 (A) */
    float iq_A;                 /**< q轴电流 (A) */

    /* 电压 */
    float vdc_bus_v;            /**< 直流母线电压 (V) */
    float vu_v;                 /**< U相电压 (V) */
    float vv_v;                 /**< V相电压 (V) */
    float vw_v;                 /**< W相电压 (V) */

    /* 磁链与转矩 */
    float flux_Wb;              /**< 磁链 (Wb) */
    float torque_nm;            /**< 转矩 (N·m) */

    /* 温度 */
    float ipm_temp_c;           /**< IPM 温度 (℃) */
    float motor_temp_c;         /**< 电机温度 (℃) */

    /* 功率 */
    float power_kw;             /**< 输出功率 (kW) */
    float energy_kwh;           /**< 累计能耗 (kWh) */

    /* 时间戳 */
    uint32_t tick_1ms;          /**< 1ms 计数器 */
    uint32_t tick_10ms;         /**< 10ms 计数器 */
    uint32_t tick_100ms;        /**< 100ms 计数器 */
    uint32_t tick_1s;           /**< 1s 计数器 */

    /* 故障状态 */
    FaultCode_t fault_active;   /**< 当前活动故障 */
    FaultCode_t fault_history[3]; /**< 历史故障 */
    uint8_t fault_reset_cnt;    /**< 复位尝试次数 */

    /* 运行统计 */
    uint32_t run_time_s;        /**< 累计运行时间 (s) */
    uint32_t power_on_time_s;   /**< 累计上电时间 (s) */
    uint32_t start_count;       /**< 启动次数 */
    uint32_t fault_count;       /**< 故障总次数 */

} SysRunData_t;

/* 全局运行时数据结构体声明（实际定义在 main.c 或 sys.c 中） */
extern SysRunData_t g_sys;

/* ============================================================
 * 第二十五部分：空行保留区（用于扩展）
 * ============================================================ */

/* 可在此处添加新的枚举、宏、结构体等，注意保持序号连续性 */

/* ============================================================
 * 文件结束标记
 * ============================================================ */
#endif /* __GLOBAL_DEF_H */

/**
 * @}
 */

/**
 * @}
 */
