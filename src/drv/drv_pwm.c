/**
 * @file drv_pwm.c
 * @brief TIM1三相PWM输出驱动 - STM32F4硬件抽象层
 *
 * 使用TIM1高级定时器产生6路PWM（3路+3路互补），
 * 支持中心对齐模式，用于FOC逆变器驱动。
 */

#include <drv_pwm.h>
#include <stm32f4xx_hal.h>

/* STM32F407 TIM1时钟为84MHz */
#define TIM1_CLK_HZ  84000000UL

DrvPwmHandle g_pwm = {0};

/**
 * @brief PWM初始化
 * @param freq_hz PWM频率（Hz），推荐8~20kHz
 *
 * 配置流程：
 *  1. 使能TIM1时钟
 *  2. 设置为中心对齐模式3（上下计数，索引在溢出点）
 *  3. 配置ARR得到目标频率
 *  4. 配置CCER：3路互补输出（CH1/2/3 + CH1N/2N/3N）
 *  5. 配置BDTR：死区时间~100ns（DTG位）
 *  6. 初始状态Disable（MOE=0，安全优先）
 */
void DRV_PWM_Init(uint32_t freq_hz)
{
    /* 避免频率为0 */
    if (freq_hz == 0) freq_hz = 10000;

    /* 计算ARR：中心对齐模式下 PWM_freq = TIM1_clk / (2 * (ARR+1))
       ARR = TIM1_clk / (2 * freq) - 1
    */
    uint32_t arr = (TIM1_CLK_HZ / (freq_hz * 2U)) - 1U;
    if (arr > 65535U) arr = 65535U;
    g_pwm.arr_period = (uint16_t)arr;
    g_pwm.freq_hz    = freq_hz;
    g_pwm.enable     = 0;
    g_pwm.fault_mask = 0;

    /* 使能TIM1时钟 */
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    /* 复位TIM1配置 */
    TIM1->CR1  = 0;
    TIM1->CR2  = 0;
    TIM1->SMCR = 0;
    TIM1->DIER = 0;
    TIM1->EGR  = 0;
    TIM1->CCMR1 = 0;
    TIM1->CCMR2 = 0;
    TIM1->CCER  = 0;
    TIM1->BDTR  = 0;
    TIM1->DTR   = 0;
    TIM1->AF1   = 0;
    TIM1->AF2   = 0;

    /* 配置CR1: 中心对齐模式3 (CMS=11), 自动重装载 (ARPE=1) */
    TIM1->CR1 = TIM_CR1_ARPE |    /* ARPE=1: ARR预装载使能 */
                (0x03U << 5);    /* CMS=11: 中心对齐模式3，上溢和下溢都产生更新 */

    /* 配置ARR */
    TIM1->ARR = g_pwm.arr_period;

    /* 配置PSC: 不分频 */
    TIM1->PSC = 0;

    /* 配置RCR: 重复计数值为0（每个PWM周期都更新） */
    TIM1->RCR = 0;

    /* 配置BDTR: 死区时间设置
       DTG[7:5]=01b -> DTG[5:0]*Tdtg, Tdtg=Tint=1/84MHz≈11.9ns
       DTG=0x10 -> 死区时间 = 2*11.9ns ≈ 24ns
       DTG=0x1F -> 死区时间 = 31*11.9ns ≈ 369ns
       实际设置DTG=0x08 -> 死区≈95ns
    */
    TIM1->BDTR = (0x01U << 8) |  /* MOE=1 主输出使能（初始失能，SetDuty后Enable） */
                 (0x08U << 0);  /* DTG=8  死区时间≈95ns */

    /* 配置CCMR: PWM模式1
       CH1: CC1S=00(输出), OC1M=110(PWM模式1), OC1PE=1(预装载)
       CH2: CC2S=00, OC2M=110, OC2PE=1
       CH3: CC3S=00, OC3M=110, OC3PE=1
    */
    /* CCMR1: CH1和CH2 */
    TIM1->CCMR1 = (0x06U << 4) |  /* OC1M=110: PWM模式1 */
                  (0x01U << 3) |  /* OC1PE=1: 预装载使能 */
                  (0x06U << 12) | /* OC2M=110: PWM模式1 */
                  (0x01U << 11);  /* OC2PE=1: 预装载使能 */

    /* CCMR2: CH3 */
    TIM1->CCMR2 = (0x06U << 4) |  /* OC3M=110: PWM模式1 */
                  (0x01U << 3);   /* OC3PE=1: 预装载使能 */

    /* 配置CCER: 使能三路互补输出
       CC1E=1, CC1NE=1: CH1 + CH1N
       CC2E=1, CC2NE=1: CH2 + CH2N
       CC3E=1, CC3NE=1: CH3 + CH3N
       极性：正极性（CCxP=0, CCxNP=0）
    */
    TIM1->CCER = TIM_CCER_CC1E  | TIM_CCER_CC1NE |
                 TIM_CCER_CC2E  | TIM_CCER_CC2NE |
                 TIM_CCER_CC3E  | TIM_CCER_CC3NE;

    /* 初始占空比为0 */
    TIM1->CCR1 = 0;
    TIM1->CCR2 = 0;
    TIM1->CCR3 = 0;

    /* 生成更新事件，确保ARR和CCR预装载值生效 */
    TIM1->EGR = TIM_EGR_UG;

    /* 初始状态：所有输出失能（安全优先） */
    DRV_PWM_Disable();
}

/**
 * @brief 设置三相PWM占空比
 * @param ch1 CH1占空比（CCR值）
 * @param ch2 CH2占空比
 * @param ch3 CH3占空比
 *
 * 直接写入CCR寄存器。在中心对齐模式下，
 * PWM频率 = TIM1_clk / (2 * (ARR+1))
 */
void DRV_PWM_SetDuty(uint16_t ch1, uint16_t ch2, uint16_t ch3)
{
    /* 限制CCR不超过ARR（避免占空比>100%） */
    if (ch1 > g_pwm.arr_period) ch1 = g_pwm.arr_period;
    if (ch2 > g_pwm.arr_period) ch2 = g_pwm.arr_period;
    if (ch3 > g_pwm.arr_period) ch3 = g_pwm.arr_period;

    TIM1->CCR1 = ch1;
    TIM1->CCR2 = ch2;
    TIM1->CCR3 = ch3;
}

/**
 * @brief 使能PWM输出
 * 设置CEN=1（计数器使能）和MOE=1（主输出使能）
 */
void DRV_PWM_Enable(void)
{
    /* CEN: 计数器使能 */
    TIM1->CR1 |= TIM_CR1_CEN;
    /* MOE: 主输出使能（必须设置才能输出PWM） */
    TIM1->BDTR |= TIM_BDTR_MOE;
    g_pwm.enable = 1;
}

/**
 * @brief 失能PWM输出（安全状态）
 * 清零MOE，所有输出被强制拉低
 */
void DRV_PWM_Disable(void)
{
    /* MOE=0: 主输出失能，所有PWM输出被强制为0 */
    TIM1->BDTR &= ~TIM_BDTR_MOE;
    /* CEN=0: 计数器停止 */
    TIM1->CR1 &= ~TIM_CR1_CEN;
    g_pwm.enable = 0;
}

/**
 * @brief 设置故障屏蔽
 * @param mask 故障屏蔽位（1=屏蔽故障，0=检测故障）
 */
void DRV_PWM_SetFaultMask(uint8_t mask)
{
    /* BKE/BKP位需要在修改前了解当前状态 */
    if (mask) {
        /* 使能BKIN故障输入（如果需要硬件故障） */
        TIM1->BDTR |= (0x0FU << 16); /* SS[3:0]=1111: 触发后MOE自动清零 */
    } else {
        TIM1->BDTR &= ~(0x0FU << 16);
    }
    g_pwm.fault_mask = mask;
}
