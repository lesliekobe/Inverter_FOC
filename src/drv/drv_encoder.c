/**
 * @file drv_encoder.c
 * @brief 增量编码器驱动 - STM32F4硬件抽象层
 *
 * 使用TIM3作为正交编码器接口（Quadrature Encoder Interface）。
 *
 * 引脚分配：
 *   TIM3_CH1 -> PA6 (Encoder A相)
 *   TIM3_CH2 -> PA7 (Encoder B相)
 *   TIM3_CH3 -> PB0 (Z脉冲/索引信号，可选)
 *
 * 正交编码器模式：
 *   - 每转脉冲数(PPR) = 编码器线数（通常500/1000/2500等）
 *   - 4倍频后每转计数 = PPR × 4
 *   - DIR位根据A/B相相位关系判断方向
 *
 * 测速方法：
 *   - M法测速（固定时间窗口）：
 *     speed = (curr_count - prev_count) / (sample_period × counts_per_rev)
 */

#include <drv_encoder.h>
#include <stm32f4xx_hal.h>

/* ========== 编码器参数（根据实际硬件调整）========== */
#define ENCODER_PPR            2500    // 编码器每转脉冲数（线数）
#define ENCODER_COUNTS_PER_REV (ENCODER_PPR * 4)  // 4倍频后每转计数

/* 速度滤波器参数 */
#define SPEED_FILTER_SIZE      8        // 滑动窗口滤波，8次平均
#define SPEED_SAMPLE_RATE_HZ   1000     // 速度采样频率（Hz）= TIM3更新频率

/* 全局编码器状态 */
EncoderHandle g_encoder = {0};

/* 内部变量 */
static int32_t  g_encoder_last_count = 0;      // 上次计数值
static int32_t  g_encoder_delta_sum   = 0;      // 增量累加（用于计算速度）
static int16_t  g_speed_buf[SPEED_FILTER_SIZE] = {0};  // 速度滑动窗口
static uint8_t  g_speed_buf_idx       = 0;      // 滑动窗口索引
static int32_t  g_encoder_overflow    = 0;      // 计数器溢出次数

/**
 * @brief 编码器初始化
 *
 * 配置TIM3为正交编码器模式：
 *   - TI1FP1 (CH1) 作为计数器输入
 *   - TI2FP2 (CH2) 作为方向/计数输入
 *   - 边沿计数模式：上升沿+下降沿（4倍频）
 *   - 根据A/B相位确定方向
 */
void DRV_ENCODER_Init(void)
{
    /* 使能TIM3和GPIOA/GPIOB时钟 */
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 配置GPIO: PA6=TIM3_CH1(A相), PA7=TIM3_CH2(B相) */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;   // 编码器通常为开漏输出，需要上拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;  // TIM3复用功能
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 可选：配置PB0作为Z脉冲输入（TIM3_CH3） */
    GPIO_InitStruct.Pin   = GPIO_PIN_0;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* 复位TIM3配置 */
    TIM3->CR1  = 0;
    TIM3->CR2  = 0;
    TIM3->SMCR = 0;
    TIM3->DIER = 0;
    TIM3->EGR  = 0;
    TIM3->CCMR1 = 0;
    TIM3->CCMR2 = 0;
    TIM3->CCER  = 0;

    /* 配置TIM3为正交编码器模式：
       SMCR: SMS=011 (Encoder Mode 3: TI1FP1和TI2FP2都计数) */
    TIM3->SMCR = (0x03U << 0);  /* SMS=011: 两个边沿都计数 */

    /* 配置CCMR1: CH1为输入（TI1），带滤波
       CC1S=01: IC1映射到TI1FP1
       IC1F=0011: f萨采样 = fCK_INT, N=4
    */
    TIM3->CCMR1 = (0x01U << 0) |   /* CC1S=01: TI1FP1 */
                  (0x03U << 4);    /* IC1F=0011: 滤波 */

    /* 配置CCMR2: CH2为输入（TI2）
       CC2S=01: IC2映射到TI2FP2
       IC2F=0011: 滤波
    */
    TIM3->CCMR2 = (0x01U << 8) |   /* CC2S=01: TI2FP2 */
                  (0x03U << 12);   /* IC2F=0011: 滤波 */

    /* 配置CCER: 使能CH1和CH2，不反向 */
    TIM3->CCER = 0;  /* 默认极性：不反向 */

    /* 配置ARR为最大（65535），作为计数器上限 */
    TIM3->ARR = 0xFFFF;

    /* PSC = 0：计数器直接由编码器A/B相驱动 */
    TIM3->PSC = 0;

    /* 清除计数器 */
    TIM3->CNT = 0;

    /* 配置Z脉冲（索引）：CH3作为输入，在PB0检测Z信号 */
    /* CH3配置为输入，捕获Z脉冲上/下边沿 */
    TIM3->CCMR2 |= (0x01U << 10);   /* CC3S=01: 映射到TI3FP3 */
    TIM3->CCER  |= (0x01U << 8);    /* CC3E=1: 使能CH3 */
    /* 上升沿捕获 */
    TIM3->CCER  &= ~(0x01U << 9);   /* CC3P=0: 上升沿 */

    /* 使能TIM3更新中断（用于速度计算） */
    TIM3->DIER |= TIM_DIER_UIE;     /* 更新中断使能 */

    /* 设置优先级并使能中断 */
    HAL_NVIC_SetPriority(TIM3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);

    /* 使能计数器 */
    TIM3->CR1 |= TIM_CR1_CEN;

    /* 初始化全局变量 */
    g_encoder.position    = 0;
    g_encoder.speed_rpm   = 0;
    g_encoder.dir         = 0;
    g_encoder.index_found = 0;
    g_encoder_last_count  = 0;
    g_encoder_delta_sum   = 0;

    for (uint8_t i = 0; i < SPEED_FILTER_SIZE; i++) {
        g_speed_buf[i] = 0;
    }
}

/**
 * @brief 复位编码器位置
 */
void DRV_ENCODER_Reset(void)
{
    __disable_irq();
    TIM3->CNT = 0;
    g_encoder.position     = 0;
    g_encoder_last_count  = 0;
    g_encoder_delta_sum   = 0;
    g_encoder.speed_rpm   = 0;
    g_encoder.dir         = 0;
    for (uint8_t i = 0; i < SPEED_FILTER_SIZE; i++) {
        g_speed_buf[i] = 0;
    }
    __enable_irq();
}

/**
 * @brief 获取电机转速
 * @return 机械转速（RPM）
 *
 * 使用滑动窗口平均滤波：
 *   1. 计算本次增量 = curr_count - last_count
 *   2. 处理计数器翻转（+65536或-65536）
 *   3. 累加到增量累加器
 *   4. 每SPEED_FILTER_SIZE次采样后计算平均速度
 *
 * 公式：speed_rpm = (delta_sum / SPEED_FILTER_SIZE) × 采样频率 × 60 / counts_per_rev
 */
int16_t DRV_ENCODER_GetSpeedRPM(void)
{
    return g_encoder.speed_rpm;
}

/**
 * @brief TIM3更新中断处理（速度计算）
 *
 * 每隔1ms触发一次（假设TIM3使用APB1时钟21MHz，PSC=20999）
 * 或直接使用系统Tick作为采样时钟。
 */
void DRV_ENCODER_TIM_IRQHandler(void)
{
    static uint32_t sample_count = 0;

    if ((TIM3->SR & TIM_SR_UIF) == 0) {
        return;  // 不是更新中断
    }
    TIM3->SR = ~TIM_SR_UIF;  // 清除更新标志

    /* 读取当前计数器值 */
    int32_t curr_count = (int32_t)(TIM3->CNT);

    /* 计算增量（处理翻转） */
    int32_t delta = curr_count - g_encoder_last_count;

    /* 处理计数器翻转：正转时CNT从65535回到0，delta为负大数 */
    if (delta >  32768) delta -= 65536;  /* 向下翻转（正向运动时CNT上溢） */
    if (delta < -32768) delta += 65536;  /* 向上翻转（反向运动时CNT下溢） */

    g_encoder_last_count = curr_count;

    /* 更新位置（累计计数） */
    g_encoder.position += delta;

    /* 累加增量用于速度计算 */
    g_encoder_delta_sum += delta;
    sample_count++;

    /* 每SPEED_FILTER_SIZE次采样计算一次速度 */
    if (sample_count >= SPEED_FILTER_SIZE) {
        sample_count = 0;

        /* 增量总和转为RPM：
           speed = delta_sum × (60 / counts_per_rev) × sample_rate
           其中 sample_rate = SPEED_SAMPLE_RATE_HZ / SPEED_FILTER_SIZE
        */
        int32_t delta_avg = g_encoder_delta_sum / SPEED_FILTER_SIZE;
        g_encoder_delta_sum = 0;

        /* 计算机械转速RPM
           RPM = delta × 60 × 采样频率 / 每转计数
           = delta × 60 × SPEED_SAMPLE_RATE_HZ / (counts_per_rev × SPEED_FILTER_SIZE)
           = delta × 60 × 1000 / (2500×4 × 8)   当SPEED_SAMPLE_RATE_HZ=1000
           = delta × 60 × 1000 / 80000
           = delta × 0.75
        */
        int32_t rpm = (int32_t)((float)delta_avg * 0.75f);

        /* 限幅 */
        if (rpm >  3000) rpm =  3000;
        if (rpm < -3000) rpm = -3000;

        g_encoder.speed_rpm = (int16_t)rpm;

        /* 判断方向：正值为CW，负值为CCW */
        g_encoder.dir = (delta_avg >= 0) ? 0 : 1;
    }
}

/**
 * @brief TIM3_IRQn中断处理（由stm32f4xx_it.c调用）
 */
void TIM3_IRQHandler(void)
{
    DRV_ENCODER_TIM_IRQHandler();
}
