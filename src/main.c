/**
 * @file main.c
 * @brief 变频器FOC主程序入口
 *
 * 初始化 → 调度器主循环
 */
#include "global_ctx.h"
#include "bsp_init.h"
#include "scheduler_bare.h"
#include "drv_pwm.h"
#include "drv_adc.h"
#include "drv_gpio.h"
#include "drv_uart.h"
#include "drv_encoder.h"
#include "mc_foc.h"
#include "mc_vf.h"
#include "mc_protect.h"
#include "app_speed.h"
#include "app_pid.h"
#include "app_io_logic.h"
#include "app_modbus_cmd.h"
#include "hmi_key.h"
#include "hmi_display.h"
#include "param_store.h"
#include "fault_mgr.h"

/**
 * System_Init - 系统初始化（启动阶段一次性执行）
 */
static void System_Init(void)
{
    /* 关闭全局中断 */
    __disable_irq();

    /* BSP硬件初始化 */
    BSP_Init();

    /* 全局上下文初始化 */
    GlobalCtx_Init();

    /* 参数加载（EEPROM/Flash） */
    Param_Load();

    /* 外设驱动初始化 */
    DRV_GPIO_Init();
    DRV_ADC_Init();
    DRV_PWM_Init(PWM_FREQUENCY_HZ); /* 10kHz PWM */
    DRV_UART_Init(g_ctx.param.modbus_baud);
    DRV_ENCODER_Init();

    /* 电机控制算法初始化 */
    MC_FOC_Init();
    MC_VF_Init();
    MC_Protect_Init();

    /* 应用层初始化 */
    APP_Speed_Init();
    APP_MODBUS_Init();
    HMI_KEY_Init();
    HMI_DISP_Init();

    /* 故障管理器初始化 */
    FaultMgr_Init();

    /* 调度器初始化 */
    Scheduler_Init();

    /* 参数验证（超出范围则恢复默认值） */
    Param_Validate();

    /* 使能全局中断 */
    __enable_irq();

    /* PWM初始状态：封锁输出（安全） */
    DRV_PWM_Disable();

#ifdef DEBUG
    /* 调试信息输出 */
    // printf("Inverter FOC v1.0\r\n");
    // printf("SYSCLK: %lu Hz\r\n", SystemCoreClock);
    // printf("PWM Freq: %d Hz\r\n", PWM_FREQUENCY_HZ);
#endif
}

/**
 * main - 主函数
 */
int main(void)
{
    /* 系统初始化 */
    System_Init();

    /* 调度器主循环（永不返回） */
    Scheduler_Run();

    /* 永远不应该执行到这里 */
    while (1) {}
    return 0;
}
