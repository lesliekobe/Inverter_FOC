/**
 * @file hmi_key.c
 * @brief 按键扫描驱动
 */
#include "hmi_key.h"
#include "drv_gpio.h"
#include <string.h>

/* 按键GPIO配置（4×3键盘矩阵） */
static const uint8_t ROW_PINS[4] = {0, 1, 2, 3};  /* PC0~PC3 作为输出 */
static const uint8_t COL_PINS[3] = {4, 5, 6};      /* PC4~PC6 作为输入 */

#define DEBOUNCE_COUNT   3
#define LONG_PRESS_TIME  1000   /* ms */
#define REPEAT_TIME      500    /* ms */

static KeyState g_key = {0};
static uint8_t s_debounce_buf[7] = {0};
static uint8_t s_key_pressed = KEY_NONE;

/**
 * HMI_KEY_Init - 初始化按键驱动
 */
void HMI_KEY_Init(void)
{
    memset(&g_key, 0, sizeof(KeyState));
    g_key.key = KEY_NONE;
}

/**
 * HMI_KEY_Scan - 矩阵键盘扫描（10ms调用）
 *
 * 扫描方式：行线输出低，列线读取消抖
 */
static uint8_t Key_Scan_Matrix(void)
{
    uint8_t key_val = KEY_NONE;
    /* 简化实现：直接读PC0~PC6的原始电平，
     * 实际产品用4×3矩阵扫描
     */
    uint8_t raw = GPIOC->IDR & 0x7F; /* PC0~PC6 */

    (void)ROW_PINS;
    (void)COL_PINS;
    (void)s_debounce_buf;

    /* 简化按键映射（实际产品根据硬件调整） */
    if ((raw & (1<<0)) == 0) key_val = KEY_RUN;
    else if ((raw & (1<<1)) == 0) key_val = KEY_STOP;
    else if ((raw & (1<<2)) == 0) key_val = KEY_UP;
    else if ((raw & (1<<3)) == 0) key_val = KEY_DOWN;
    else if ((raw & (1<<4)) == 0) key_val = KEY_LEFT;
    else if ((raw & (1<<5)) == 0) key_val = KEY_RIGHT;
    else if ((raw & (1<<6)) == 0) key_val = KEY_ENTER;
    else key_val = KEY_NONE;

    return key_val;
}

/**
 * HMI_KEY_Scan - 10ms任务：按键状态更新
 */
KeyCode HMI_KEY_Scan(void)
{
    KeyCode current_key = Key_Scan_Matrix();

    if (current_key == s_key_pressed) {
        /* 按键稳定 */
        g_key.hold_time += 10;
        if (g_key.hold_time >= LONG_PRESS_TIME) {
            g_key.state = 2; /* 长按状态 */
            if ((g_key.hold_time - LONG_PRESS_TIME) % REPEAT_TIME == 0) {
                g_key.repeat_cnt++;
            }
        }
    } else {
        /* 按键变化 */
        if (s_key_pressed != KEY_NONE && g_key.hold_time < LONG_PRESS_TIME) {
            g_key.state = 1; /* 短按 */
        } else {
            g_key.state = 0;
        }
        s_key_pressed = current_key;
        g_key.key = current_key;
        g_key.hold_time = 0;
        g_key.repeat_cnt = 0;
    }
    return current_key;
}

uint8_t HMI_KEY_IsPressed(KeyCode k)
{
    return (g_key.key == k && g_key.state == 1) ? 1 : 0;
}

uint8_t HMI_KEY_IsLongPress(KeyCode k)
{
    return (g_key.key == k && g_key.state == 2) ? 1 : 0;
}
