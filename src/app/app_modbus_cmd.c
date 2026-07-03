/**
 * @file app_modbus_cmd.c
 * @brief Modbus RTU通讯协议处理
 *
 * 支持功能码：0x03读保持寄存器、0x06写单个寄存器、0x10写多个寄存器
 */
#include "app_modbus_cmd.h"
#include "drv_uart.h"
#include "global_ctx.h"
#include "app_speed.h"
#include <string.h>

#define MB_FUNC_NONE          0
#define MB_FUNC_READ_HOLDING  3
#define MB_FUNC_WRITE_SINGLE  6
#define MB_FUNC_WRITE_MULTI   0x10

ModbusHandle g_modbus = {0};

/* Modbus寄存器映射 */
static uint16_t s_reg_map[64] = {0};

/**
 * APP_MODBUS_Init - 初始化Modbus
 */
void APP_MODBUS_Init(void)
{
    g_modbus.addr = g_ctx.param.modbus_addr;
    g_modbus.reg_map = s_reg_map;

    /* 初始化寄存器默认值 */
    s_reg_map[0] = 0;     /* 运行命令 */
    s_reg_map[1] = 0;     /* 频率给定(0.01Hz) */
    s_reg_map[2] = 0;     /* 电流反馈(0.01A) */
    s_reg_map[3] = 0;     /* 电压反馈(0.1V) */
    s_reg_map[4] = 0;     /* 转速反馈(1RPM) */
    s_reg_map[5] = 0;     /* 运行状态字 */
    s_reg_map[6] = 0;     /* 故障代码 */
    s_reg_map[7] = 0;     /* 母线电压(0.1V) */
}

/**
 * APP_MODBUS_CalcCRC - 计算CRC16
 */
static uint16_t Modbus_CRC16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/**
 * APP_MODBUS_Process - 处理接收到的Modbus帧
 * @frame 接收帧缓冲区
 * @len   帧长度（含CRC）
 * @return 0=无需回复, >0=回复帧长度
 */
uint16_t APP_MODBUS_Process(uint8_t *frame, uint16_t len)
{
    if (len < 5) return 0; /* 太短，无效 */

    /* 验证CRC */
    uint16_t crc_recv = (frame[len-1] << 8) | frame[len-2];
    uint16_t crc_calc = Modbus_CRC16(frame, len - 2);
    if (crc_recv != crc_calc) return 0; /* CRC错误，丢弃 */

    uint8_t addr = frame[0];
    uint8_t func = frame[1];

    /* 过滤非本机地址 */
    if (addr != g_modbus.addr && addr != 0) return 0; /* 广播地址0也处理 */

    /* 功能码：读保持寄存器 0x03 */
    if (func == MB_FUNC_READ_HOLDING) {
        uint16_t start = (frame[2] << 8) | frame[3];
        uint16_t count = (frame[4] << 8) | frame[5];
        if (start + count > 64) count = 64 - start;

        /* 同步g_ctx到寄存器 */
        s_reg_map[2] = (uint16_t)(fabsf(MC_ADC_GetIa()) * 100.0f); /* 电流×100 */
        s_reg_map[3] = (uint16_t)(MC_ADC_GetUdc() * 10.0f);        /* 电压×10 */
        s_reg_map[4] = (uint16_t)g_ctx.motor_out.speed;            /* 转速RPM */
        s_reg_map[5] = g_ctx.run_state;
        s_reg_map[6] = g_ctx.fault_code;
        s_reg_map[7] = (uint16_t)(MC_ADC_GetUdc() * 10.0f);

        /* 构造回复：地址+功能码+字节数+数据+CRC */
        frame[0] = addr;
        frame[1] = func;
        frame[2] = (uint8_t)(count * 2);
        for (uint16_t i = 0; i < count; i++) {
            uint16_t val = s_reg_map[start + i];
            frame[3 + i*2]     = (uint8_t)(val >> 8);
            frame[3 + i*2 + 1] = (uint8_t)(val & 0xFF);
        }
        uint16_t reply_len = 3 + count * 2;
        uint16_t crc = Modbus_CRC16(frame, reply_len);
        frame[reply_len]   = (uint8_t)(crc & 0xFF);
        frame[reply_len+1] = (uint8_t)(crc >> 8);
        return reply_len + 2;
    }

    /* 功能码：写单个寄存器 0x06 */
    if (func == MB_FUNC_WRITE_SINGLE) {
        uint16_t reg = (frame[2] << 8) | frame[3];
        uint16_t val = (frame[4] << 8) | frame[5];

        if (reg == 0) {
            /* 0=运行命令：1启动，0停止 */
            if (val == 1) {
                APP_Speed_SetTarget((float)s_reg_map[1] / 100.0f, 0, SOURCE_MODBUS);
            } else if (val == 0) {
                APP_Speed_SetTarget(0.0f, 0, SOURCE_MODBUS);
            }
        } else if (reg == 1) {
            /* 1=频率给定(0.01Hz) */
            APP_Speed_SetTarget((float)val / 100.0f, 0, SOURCE_MODBUS);
        } else if (reg < 64) {
            s_reg_map[reg] = val; /* 写参数 */
        }

        /* 回复：回显原帧 */
        uint16_t crc = Modbus_CRC16(frame, 6);
        frame[6] = (uint8_t)(crc & 0xFF);
        frame[7] = (uint8_t)(crc >> 8);
        return 8;
    }

    return 0; /* 不支持的功能码 */
}

/**
 * APP_MODBUS_ProcessRx - 从UART接收缓冲区处理Modbus帧
 */
void APP_MODBUS_ProcessRx(void)
{
    static uint8_t rx_buf[256];
    static uint16_t rx_len = 0;

    while (DRV_UART_Available() > 0) {
        uint16_t ch = DRV_UART_ReadByte();
        if (ch == 0xFFFF) break;
        rx_buf[rx_len++] = (uint8_t)ch;

        /* Modbus帧结尾：3.5字符时间无新数据（简化：读到0x0D或缓冲区满） */
        if (rx_len >= 3 && rx_buf[rx_len-1] == 0x0A && rx_buf[rx_len-2] == 0x0D) {
            uint16_t reply_len = APP_MODBUS_Process(rx_buf, rx_len);
            if (reply_len > 0) {
                DRV_UART_SendBuf(rx_buf, reply_len);
            }
            rx_len = 0;
        }
        if (rx_len >= sizeof(rx_buf)) rx_len = 0; /* 溢出保护 */
    }
}
