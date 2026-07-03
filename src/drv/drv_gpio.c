/**
 * @file drv_gpio.c
 * @brief GPIO输入/输出驱动 - STM32F4硬件抽象层
 *
 * GPIO分配：
 *   GPIOC[0:7]  -> 数字输入 (DI0~DI7)
 *     PC0 = DI_START    PC1 = DI_STOP     PC2 = DI_REVERSE
 *     PC3 = DI_FAULT_IN PC4 = DI_MS0      PC5 = DI_MS1
 *     PC6 = DI_MS2      PC7 = DI_EMERGENCY
 *   GPIOC[8:15] -> 数字输出 (DO0~DO7)
 *     PC8 = DO_RUN_CONTACT PC9 = DO_FAULT_OUT  PC10 = DO_FAN
 *     PC11 = DO_BRAKE      PC12 = DO_RUN_LED   PC13 = DO_FAULT_LED
 *     PC14/PC15 保留
 *   GPIOD12     -> RS485 DE引脚（方向控制）
 */

#include <drv_gpio.h>
#include <stm32f4xx_hal.h>

/* 全局变量 */
uint8_t g_di_state = 0;   // 当前DI状态
uint8_t g_do_state = 0;   // 当前DO状态

/**
 * @brief GPIO初始化
 *
 * DI配置：PC0~PC7 输入，上拉电阻（按钮未按下时为高电平）
 * DO配置：PC8~PC15 推挽输出，初始全低（安全状态）
 * RS485 DE：PD12 推挽输出，初始低（接收模式）
 */
void DRV_GPIO_Init(void)
{
    /* 使能GPIOC和GPIOD时钟 */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 配置DI (PC0~PC7): 输入模式 + 上拉电阻 */
    GPIO_InitStruct.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                            GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;     // 输入模式
    GPIO_InitStruct.Pull  = GPIO_PULLUP;          // 上拉（按钮接地为低，按下为高）
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* 配置DO (PC8~PC15): 输出推挽，初始低电平 */
    GPIO_InitStruct.Pin   = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 |
                            GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP; // 推挽输出
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* 初始DO全部输出低电平（安全状态） */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 |
                            GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
                            GPIO_PIN_RESET);
    g_do_state = 0;

    /* 配置RS485 DE (PD12): 输出推挽，初始低（接收模式） */
    GPIO_InitStruct.Pin   = GPIO_PIN_12;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
}

/**
 * @brief 读取数字输入状态
 * @return 8位DI状态，每位对应一个输入
 *
 * 注意：去抖在应用层完成，此处直接返回原始电平状态。
 */
uint8_t DRV_GPIO_GetDI(void)
{
    /* 读取GPIOC低8位（IDR寄存器） */
    uint8_t di = (uint8_t)(GPIOC->IDR & 0xFF);
    g_di_state = di;
    return di;
}

/**
 * @brief 设置数字输出
 * @param mask  要修改的位掩码（1=修改，0=不变）
 * @param value 要设置的值（1=高，0=低）
 *
 * 示例：DRV_GPIO_SetDO(DO_FAN | DO_BRAKE, 1) -> 开启风扇和制动
 */
void DRV_GPIO_SetDO(uint8_t mask, uint8_t value)
{
    uint16_t odr;

    /* 计算新的ODR值 */
    odr = GPIOC->ODR & (uint16_t)(~mask);   /* 清除要修改的位 */
    odr |= (uint16_t)(mask & value);         /* 设置新值 */

    GPIOC->ODR = odr;

    /* 更新全局状态 */
    g_do_state = (uint8_t)(odr & 0xFF);
}
