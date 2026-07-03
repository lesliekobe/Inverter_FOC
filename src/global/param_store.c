/**
 * @file param_store.c
 * @brief 参数存储模块
 *
 * 实现参数的默认初始化、校验、加载和保存功能。
 * 使用EEPROM/Flash作为永久存储介质。
 */

#include "global_ctx.h"
#include <string.h>

/* 启用参数存储功能（可由编译开关控制） */
#ifndef PARAM_STORE_ENABLED
#define PARAM_STORE_ENABLED 1
#endif

#if PARAM_STORE_ENABLED
#include "drv_eeprom.h"   /* 外部EEPROM驱动 */
#endif

/* 参数区在EEPROM中的起始地址（可配置） */
#ifndef PARAM_EEPROM_BASE_ADDR
#define PARAM_EEPROM_BASE_ADDR   0x0000
#endif

/* 参数区大小（字节），留足扩展空间 */
#ifndef PARAM_EEPROM_SIZE
#define PARAM_EEPROM_SIZE        512
#endif

/* 参数版本号（升级时递增，用于迁移判断） */
#ifndef PARAM_VERSION
#define PARAM_VERSION            1
#endif

/* ============================================================
 * 参数范围定义
 * ============================================================ */

#define RATED_POWER_MIN          1       /* 0.01kW */
#define RATED_POWER_MAX          99999
#define RATED_VOLTAGE_MIN        100     /* 0.1V */
#define RATED_VOLTAGE_MAX        9999
#define RATED_CURRENT_MIN        1       /* 0.01A */
#define RATED_CURRENT_MAX        99999
#define RATED_FREQ_MIN           100    /* 0.01Hz */
#define RATED_FREQ_MAX           99999
#define POLE_PAIRS_MIN           1
#define POLE_PAIRS_MAX           30
#define ACCEL_TIME_MIN           1       /* 0.01s */
#define ACCEL_TIME_MAX           60000
#define DECEL_TIME_MIN           1
#define DECEL_TIME_MAX           60000
#define MIN_FREQ_MIN             0
#define MIN_FREQ_MAX             9999
#define MAX_FREQ_MIN             100
#define MAX_FREQ_MAX             9999
#define START_FREQ_MIN           0
#define START_FREQ_MAX           9999
#define OVERCURRENT_MIN          1
#define OVERCURRENT_MAX          99999
#define OVERVOLTAGE_MIN          100
#define OVERVOLTAGE_MAX          9999
#define UNDERVOLTAGE_MIN         100
#define UNDERVOLTAGE_MAX         9999
#define OVERTEMP_MIN             0
#define OVERTEMP_MAX             1500    /* 0.1°C */
#define OVERLOAD_MIN             100
#define OVERLOAD_MAX             9999
#define TORQUE_BOOST_MIN         0
#define TORQUE_BOOST_MAX         300     /* 0.1% */
#define V_F_MIN                  0
#define V_F_MAX                  3000    /* 0.1% */
#define VF_FREQ_MIN              0
#define VF_FREQ_MAX              9999
#define COMM_ADDR_MIN            1
#define COMM_ADDR_MAX            255
#define BAUDRATE_MIN             1200
#define BAUDRATE_MAX             115200
#define PROT_DELAY_MIN           0
#define PROT_DELAY_MAX           60000
#define DC_BRAKE_TIME_MIN        0
#define DC_BRAKE_TIME_MAX        20000
#define DC_BRAKE_VOLT_MIN        0
#define DC_BRAKE_VOLT_MAX        300     /* 0.1% */

/* ============================================================
 * 参数头结构（EEPROM存储时附加版本信息）
 * ============================================================ */

typedef struct {
    uint16_t version;      /* 参数版本号 */
    uint16_t crc16;        /* CRC16校验 */
    uint16_t reserved[2];
} ParamHeader;

/* ============================================================
 * 默认参数表
 * ============================================================ */

/**
 * @brief 设置默认参数
 * @param pg 指向参数组的指针
 *
 * 将所有参数设置为出厂默认值。
 * 该函数在首次上电或参数丢失时调用。
 */
void Param_SetDefaults(ParamGroup *pg)
{
    if (pg == NULL) return;

    memset(pg, 0, sizeof(ParamGroup));

    /* --- 基础参数 --- */
    pg->rated_power.value      = 1500;   /* 15.00kW */
    pg->rated_voltage.value    = 3800;   /* 380.0V */
    pg->rated_current.value    = 3200;   /* 32.00A */
    pg->rated_freq.value       = 5000;   /* 50.00Hz */
    pg->rated_speed.value      = 1500;   /* 1500RPM */
    pg->pole_pairs.value       = 2;
    pg->direction.value        = 0;

    /* --- V/F曲线 --- */
    pg->vfg_v0.value           = 50;     /* 5.0% */
    pg->vfg_v1.value           = 100;    /* 10.0% */
    pg->vfg_v2.value           = 250;    /* 25.0% */
    pg->vfg_v3.value           = 1000;   /* 100.0% */
    pg->vfg_f1.value           = 500;    /* 5.00Hz */
    pg->vfg_f2.value           = 2500;   /* 25.00Hz */
    pg->vfg_f3.value           = 5000;   /* 50.00Hz */
    pg->vfg_torque_boost.value = 20;     /* 2.0% */

    /* --- 保护参数 --- */
    pg->prot_overcurrent.value = 4500;  /* 45.00A */
    pg->prot_overvoltage.value = 7600;  /* 760.0V */
    pg->prot_undervoltage.value= 3000;  /* 300.0V */
    pg->prot_overtemp.value    = 850;    /* 85.0°C */
    pg->prot_overload.value    = 1200;  /* 120% */
    pg->prot_oc_delay.value    = 100;    /* 100ms */
    pg->prot_ov_delay.value    = 1000;   /* 1000ms */
    pg->prot_uv_delay.value    = 1000;

    /* --- 通讯参数 --- */
    pg->comm_addr.value        = 1;
    pg->comm_baudrate.value    = 9600;
    pg->comm_parity.value      = 0;      /* 无校验 */
    pg->comm_stopbit.value     = 0;      /* 1位停止位 */
    pg->comm_timeout.value     = 500;    /* 500ms */

    /* --- 工艺参数 --- */
    pg->proc_accel_time.value  = 1000;  /* 10.00s */
    pg->proc_decel_time.value  = 1000;
    pg->proc_min_freq.value    = 500;    /* 5.00Hz */
    pg->proc_max_freq.value    = 5000;   /* 50.00Hz */
    pg->proc_start_freq.value  = 50;     /* 0.50Hz */
    pg->proc_stop_mode.value   = 0;      /* 减速停机 */
    pg->proc_dc_brake_time.value = 0;
    pg->proc_dc_brake_volt.value= 50;     /* 5.0% */

    /* --- PID参数 --- */
    pg->pid_speed_kp.value     = 1000;   /* 典型速度环PID */
    pg->pid_speed_ki.value     = 100;
    pg->pid_speed_kd.value     = 0;
    pg->pid_torque_kp.value    = 500;
    pg->pid_torque_ki.value    = 50;
}

/* ============================================================
 * 参数校验
 * ============================================================ */

/**
 * @brief 校验单个参数值是否在有效范围内
 * @param item 参数项
 * @param min 下限
 * @param max 上限
 * @return 0=有效, -1=小于最小值, 1=大于最大值
 */
static int Param_CheckRange(ParamItem *item, int min, int max)
{
    if (item == NULL) return -1;
    if (item->value < min) return -1;
    if (item->value > max) return 1;
    return 0;
}

/**
 * @brief 校验所有参数的有效性
 * @param pg 指向参数组的指针
 * @return 0=全部有效, 正数=无效参数数量
 *
 * 逐项检查参数范围，超限时自动钳位到边界值。
 * 建议在校验后提示用户参数已修正。
 */
int Param_Validate(ParamGroup *pg)
{
    int err = 0;

    if (pg == NULL) return -1;

    /* 基础参数 */
    if (Param_CheckRange(&pg->rated_power,    RATED_POWER_MIN,    RATED_POWER_MAX))    { pg->rated_power.value = 1500; err++; }
    if (Param_CheckRange(&pg->rated_voltage, RATED_VOLTAGE_MIN,  RATED_VOLTAGE_MAX))  { pg->rated_voltage.value = 3800; err++; }
    if (Param_CheckRange(&pg->rated_current,  RATED_CURRENT_MIN,   RATED_CURRENT_MAX))  { pg->rated_current.value = 3200; err++; }
    if (Param_CheckRange(&pg->rated_freq,     RATED_FREQ_MIN,     RATED_FREQ_MAX))     { pg->rated_freq.value = 5000; err++; }
    if (Param_CheckRange(&pg->pole_pairs,    POLE_PAIRS_MIN,     POLE_PAIRS_MAX))     { pg->pole_pairs.value = 2; err++; }

    /* 工艺参数 */
    if (Param_CheckRange(&pg->proc_accel_time, ACCEL_TIME_MIN, ACCEL_TIME_MAX))        { pg->proc_accel_time.value = 1000; err++; }
    if (Param_CheckRange(&pg->proc_decel_time, DECEL_TIME_MIN, DECEL_TIME_MAX))        { pg->proc_decel_time.value = 1000; err++; }
    if (Param_CheckRange(&pg->proc_min_freq,   MIN_FREQ_MIN,   MIN_FREQ_MAX))          { pg->proc_min_freq.value = 500; err++; }
    if (Param_CheckRange(&pg->proc_max_freq,   MAX_FREQ_MIN,   MAX_FREQ_MAX))          { pg->proc_max_freq.value = 5000; err++; }
    if (Param_CheckRange(&pg->proc_start_freq,  START_FREQ_MIN, START_FREQ_MAX))        { pg->proc_start_freq.value = 50; err++; }

    /* V/F曲线 */
    if (Param_CheckRange(&pg->vfg_torque_boost, TORQUE_BOOST_MIN, TORQUE_BOOST_MAX))   { pg->vfg_torque_boost.value = 20; err++; }
    if (Param_CheckRange(&pg->vfg_v0, V_F_MIN, V_F_MAX))                                { pg->vfg_v0.value = 50; err++; }
    if (Param_CheckRange(&pg->vfg_v3, V_F_MIN, V_F_MAX))                               { pg->vfg_v3.value = 1000; err++; }
    if (Param_CheckRange(&pg->vfg_f1, VF_FREQ_MIN, VF_FREQ_MAX))                       { pg->vfg_f1.value = 500; err++; }
    if (Param_CheckRange(&pg->vfg_f3, VF_FREQ_MIN, VF_FREQ_MAX))                       { pg->vfg_f3.value = 5000; err++; }

    /* 保护参数 */
    if (Param_CheckRange(&pg->prot_overcurrent, OVERCURRENT_MIN, OVERCURRENT_MAX))     { pg->prot_overcurrent.value = 4500; err++; }
    if (Param_CheckRange(&pg->prot_overvoltage, OVERVOLTAGE_MIN, OVERVOLTAGE_MAX))     { pg->prot_overvoltage.value = 7600; err++; }
    if (Param_CheckRange(&pg->prot_undervoltage,UNDERVOLTAGE_MIN,UNDERVOLTAGE_MAX))   { pg->prot_undervoltage.value = 3000; err++; }
    if (Param_CheckRange(&pg->prot_overtemp,    OVERTEMP_MIN,    OVERTEMP_MAX))        { pg->prot_overtemp.value = 850; err++; }
    if (Param_CheckRange(&pg->prot_overload,    OVERLOAD_MIN,    OVERLOAD_MAX))        { pg->prot_overload.value = 1200; err++; }
    if (Param_CheckRange(&pg->prot_oc_delay,    PROT_DELAY_MIN,  PROT_DELAY_MAX))      { pg->prot_oc_delay.value = 100; err++; }
    if (Param_CheckRange(&pg->prot_ov_delay,    PROT_DELAY_MIN,  PROT_DELAY_MAX))      { pg->prot_ov_delay.value = 1000; err++; }
    if (Param_CheckRange(&pg->prot_uv_delay,    PROT_DELAY_MIN,  PROT_DELAY_MAX))      { pg->prot_uv_delay.value = 1000; err++; }

    /* 通讯参数 */
    if (Param_CheckRange(&pg->comm_addr,       COMM_ADDR_MIN,    COMM_ADDR_MAX))       { pg->comm_addr.value = 1; err++; }
    if (Param_CheckRange(&pg->comm_baudrate,   BAUDRATE_MIN,     BAUDRATE_MAX))        { pg->comm_baudrate.value = 9600; err++; }

    /* 直流制动 */
    if (Param_CheckRange(&pg->proc_dc_brake_time, DC_BRAKE_TIME_MIN, DC_BRAKE_TIME_MAX)){ pg->proc_dc_brake_time.value = 0; err++; }
    if (Param_CheckRange(&pg->proc_dc_brake_volt, DC_BRAKE_VOLT_MIN, DC_BRAKE_VOLT_MAX)){ pg->proc_dc_brake_volt.value = 50; err++; }

    return err;
}

/* ============================================================
 * CRC16校验
 * ============================================================ */

/**
 * @brief 计算参数区CRC16
 * @param data 数据指针
 * @param len 数据长度（字节）
 * @return CRC16值
 *
 * 使用Modbus标准CRC16算法。
 */
static uint16_t Param_CalcCRC16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    uint16_t i, j;

    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/* ============================================================
 * 参数加载与保存
 * ============================================================ */

#if PARAM_STORE_ENABLED

/**
 * @brief 从EEPROM加载参数
 * @param pg 指向参数组的指针
 * @return 0=成功, -1=驱动错误, -2=CRC错误, -3=版本不匹配
 *
 * 从EEPROM指定地址读取参数数据，验证版本和CRC，
 * 校验通过后复制到参数组中。
 *
 * @note 若驱动未就绪或校验失败，自动加载默认值
 */
int Param_Load(ParamGroup *pg)
{
    ParamHeader hdr;
    uint8_t *buf;
    int ret = 0;

    if (pg == NULL) return -1;

    /* 读取参数头 */
    if (EEPROM_Read(PARAM_EEPROM_BASE_ADDR, (uint8_t *)&hdr, sizeof(ParamHeader)) != 0) {
        Param_SetDefaults(pg);
        return -1;
    }

    /* 检查版本号 */
    if (hdr.version != PARAM_VERSION) {
        /* 版本不匹配：可在此处插入迁移逻辑 */
        Param_SetDefaults(pg);
        return -3;
    }

    /* 分配临时缓冲区 */
    buf = (uint8_t *)pg;
    memset(buf, 0, sizeof(ParamGroup));

    /* 读取参数数据（跳过头部） */
    if (EEPROM_Read(PARAM_EEPROM_BASE_ADDR + sizeof(ParamHeader),
                    buf, sizeof(ParamGroup)) != 0) {
        Param_SetDefaults(pg);
        return -1;
    }

    /* 验证CRC */
    uint16_t crc = Param_CalcCRC16(buf, sizeof(ParamGroup));
    if (crc != hdr.crc16) {
        Param_SetDefaults(pg);
        return -2;
    }

    /* 校验参数合法性 */
    ret = Param_Validate(pg);

    return ret;
}

/**
 * @brief 保存参数到EEPROM
 * @param pg 指向参数组的指针
 * @return 0=成功, -1=驱动错误, -2=写入失败
 *
 * 将参数组数据写入EEPROM，同时写入版本号和CRC16。
 * 写操作前先校验参数合法性。
 *
 * @note 写操作前会自动擦除目标区域
 */
int Param_Save(const ParamGroup *pg)
{
    ParamHeader hdr;
    uint16_t crc;
    uint8_t *buf;

    if (pg == NULL) return -1;

    /* 保存前先校验 */
    Param_Validate((ParamGroup *)pg);

    /* 填充头部 */
    hdr.version  = PARAM_VERSION;
    hdr.crc16    = 0;
    hdr.reserved[0] = 0;
    hdr.reserved[1] = 0;

    /* 计算参数区CRC */
    buf = (uint8_t *)pg;
    hdr.crc16 = Param_CalcCRC16(buf, sizeof(ParamGroup));

    /* 写入头部 */
    if (EEPROM_Write(PARAM_EEPROM_BASE_ADDR,
                     (const uint8_t *)&hdr, sizeof(ParamHeader)) != 0) {
        return -1;
    }

    /* 写入参数数据 */
    if (EEPROM_Write(PARAM_EEPROM_BASE_ADDR + sizeof(ParamHeader),
                     buf, sizeof(ParamGroup)) != 0) {
        return -2;
    }

    return 0;
}

#else /* PARAM_STORE_ENABLED == 0 */

/* 参数存储功能被禁用时的桩函数 */
int Param_Load(ParamGroup *pg)
{
    if (pg == NULL) return -1;
    Param_SetDefaults(pg);
    return 0;
}

int Param_Save(const ParamGroup *pg)
{
    (void)pg;
    return -1; /* 无存储驱动，返回错误 */
}

#endif /* PARAM_STORE_ENABLED */
