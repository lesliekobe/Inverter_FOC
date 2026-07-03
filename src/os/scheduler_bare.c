/**
 * @file scheduler_bare.c
 * @brief 裸机分时调度器（无RTOS方案）
 *
 * 1ms SysTick基准，分时轮询各任务
 * 优先级：数值越小优先级越高
 */
#include "scheduler_bare.h"
#include "bsp_init.h"
#include "mc_foc.h"
#include "mc_vf.h"
#include "mc_protect.h"
#include "drv_adc.h"
#include "app_speed.h"
#include "app_pid.h"
#include "app_io_logic.h"
#include "app_modbus_cmd.h"
#include "hmi_key.h"
#include "hmi_display.h"
#include "global_ctx.h"

/* 任务定义 */
typedef struct {
    void (*func)(void);
    uint16_t period_ms;
    uint32_t last_tick;
    uint8_t  priority;
    uint8_t  enable;
    const char *name;
} TaskDef;

static TaskDef s_tasks[] = {
    /* 优先级0：最高，1ms周期 */
    { MC_Protect_Check,      1,  0, 0, 1, "MC_PROTECT"  },
    /* 优先级1：1ms周期 */
    { APP_Speed_Task,        1,  0, 1, 1, "APP_SPEED"   },
    /* 优先级2：1ms周期 */
    { APP_IO_Task,           1,  0, 2, 1, "APP_IO"      },
    /* 优先级3：5ms周期 */
    { APP_MODBUS_ProcessRx,  5,  0, 3, 1, "APP_MODBUS"  },
    /* 优先级4：10ms周期 */
    { HMI_KEY_Scan,         10,  0, 4, 1, "HMI_KEY"     },
    /* 优先级5：10ms周期 */
    { MC_FOC_SpeedLoop,     10,  0, 5, 1, "MC_SPEED"    },
    /* 优先级6：100ms周期 */
    { MC_ADC_Sample,       100,  0, 6, 1, "MC_SAMPLE"   },
    /* 优先级7：100ms周期 */
    { HMI_DISP_Update,     100,  0, 7, 1, "HMI_DISP"    },
    /* 优先级8：100ms周期 */
    { Param_Save_Check,    100,  0, 8, 1, "PARAM_SAVE"  },
};

static volatile uint32_t s_tick_ms = 0;

/**
 * Scheduler_Init - 初始化调度器
 */
void Scheduler_Init(void)
{
    s_tick_ms = 0;
    for (uint8_t i = 0; i < sizeof(s_tasks)/sizeof(s_tasks[0]); i++) {
        s_tasks[i].last_tick = 0;
    }
}

/**
 * Scheduler_TickISR - 1ms时基中断服务（SysTick或TIM2）
 */
void Scheduler_TickISR(void)
{
    s_tick_ms++;
}

/**
 * Scheduler_GetTick - 获取当前Tick计数
 */
uint32_t Scheduler_GetTick(void)
{
    return s_tick_ms;
}

/**
 * Scheduler_Run - 调度器主循环（main函数中调用）
 */
void Scheduler_Run(void)
{
    uint32_t now;
    TaskDef *t;
    uint8_t i;

    while (1) {
        now = s_tick_ms;

        for (i = 0; i < sizeof(s_tasks)/sizeof(s_tasks[0]); i++) {
            t = &s_tasks[i];
            if (!t->enable) continue;
            if ((now - t->last_tick) >= t->period_ms) {
                t->last_tick = now;
                t->func();
            }
        }

        /* 喂狗（如果启用） */
        /* IWDG->KR = 0xAAAA; */
    }
}

/**
 * 任务：参数存储检查（100ms调用）
 * 仅在参数有变化时写入EEPROM，避免频繁写入
 */
static uint8_t s_param_dirty = 0;

void Param_SetDirty(void) { s_param_dirty = 1; }

void Param_Save_Check(void)
{
    if (s_param_dirty) {
        Param_Save();
        s_param_dirty = 0;
    }
}
