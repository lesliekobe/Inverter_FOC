/**
 ******************************************************************************
 * @file    bsp_init.c
 * @brief   BSP层初始化实现文件 - 三相逆变器FOC项目
 * @version V1.0.0
 * @date    2024-01-01
 * @note    STM32F4系列BSP层初始化实现
 *          包含时钟、GPIO、ADC、PWM、UART、编码器、NVIC等初始化
 *
 * 主控制循环说明:
 * - TIM1_UP_IRQHandler (10kHz): 调用 MC_FOC_CurrentLoop() 或 MC_VF_Control()
 * - ADC_IRQHandler: 读取ADC采样值（备用方案，非DMA模式）
 * - USART1_IRQHandler: 调用 Modbus_ReceiveISR() 处理Modbus接收
 * - EXTI0-3_IRQHandler: 数字输入去抖和故障检测
 ******************************************************************************
 */

/* ============================================================
 * 第一部分：包含头文件
 * ============================================================ */
#include "bsp_hw_config.h"
#include "bsp_init.h"
#include "global_def.h"
#include "global_ctx.h"

/* ============================================================
 * 第二部分：全局变量定义
 * ============================================================ */

/**
 * @brief   ADC DMA循环缓冲区
 * @note    DMA传输目的地，存放ADC多通道采样数据
 *          格式: [Ia, Ib, Udc, NTC, Ia, Ib, Udc, NTC, ...]
 *          大小应足够一个PWM周期的采样需求
 */
uint16_t bsp_adc_dma_buf[64] = {0};

/**
 * @brief   ADC DMA缓冲区大小
 */
uint32_t bsp_adc_dma_buf_size = 64;

/**
 * @brief   校准后的ADC零点偏移量
 * @note    在系统初始化后通过自校准获得
 */
int16_t bsp_adc_offset_ia = 2048;   /* 默认中间值，真实值需校准 */
int16_t bsp_adc_offset_ib = 2048;
int16_t bsp_adc_offset_vdc = 0;

/**
 * @brief   系统主频记录
 * @note    在SystemClock_Config()后更新
 */
uint32_t g_SystemCoreClock = 168000000UL;

/**
 * @brief   PWM载波频率记录
 */
uint32_t g_PwmFrequency = 10000UL;

/**
 * @brief   编码器Z相标志
 * @note    置1表示检测到Z相（零位）脉冲
 */
volatile uint8_t g_EncoderZFlag = 0;

/**
 * @brief   编码器Z相位置计数器
 * @note    记录Z相出现的次数
 */
volatile uint32_t g_EncoderZCount = 0;

/**
 * @brief   数字输入去抖计数器
 * @note    每个DI对应一个计数器，超过阈值才确认状态变化
 */
typedef struct {
    uint8_t state;        /* 当前稳定状态 */
    uint8_t debounce_cnt;  /* 去抖计数器 */
    uint8_t last_raw;      /* 上次原始值 */
} DI_Debounce_t;

static DI_Debounce_t g_di_debounce[8] = {0};

/**
 * @brief   数字输入当前状态
 * @note    位域表示，bit0=DI0, bit1=DI1, ...
 */
volatile uint8_t g_DI_Status = 0;

/**
 * @brief   数字输出当前状态
 * @note    位域表示，bit0=DO0, bit1=DO1, ...
 */
volatile uint16_t g_DO_Status = 0;

/**
 * @brief   SysTick计时器
 * @note    毫秒级计时，用于延时和超时判断
 */
volatile uint32_t g_SysTickMs = 0;

/* ============================================================
 * 第三部分：系统时钟配置
 * ============================================================ */

/**
 * @brief   系统时钟配置函数
 * @note    配置PLL使用外部8MHz晶振，产生168MHz系统时钟
 *          - HSE = 8MHz
 *          - PLL_M = 4  -> 8/4 = 2MHz
 *          - PLL_N = 168 -> 2*168 = 336MHz
 *          - PLL_P = 2  -> 336/2 = 168MHz (SYSCLK)
 *          - PLL_Q = 7  -> 336/7 = 48MHz (USB OTG)
 *
 *          AHB = 168MHz, APB1 = 42MHz, APB2 = 84MHz
 */
void BSP_SystemClock_Config(void)
{
    /* 等待HSE稳定 */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* 配置Flash预取、缓存和等待周期 */
    /* FLASH_ACR: ART使能, 预取使能, 等待周期=5 (168MHz需要5个周期) */
    *(volatile uint32_t *)0x40023C00UL = 0x00000005UL; /* FLASH->ACR */

    /* 配置AHB分频: HCLK = SYSCLK / 1 = 168MHz */
    RCC->CFGR &= ~(0x0FUL << 4);  /* 清除HPRE位 */
    RCC->CFGR |= (0x00UL << 4);   /* HPRE = 0000, AHB不分频 */

    /* 配置APB1分频: APB1 = HCLK / 4 = 42MHz (最大42MHz) */
    RCC->CFGR &= ~(0x07UL << 10); /* 清除PPRE1位 */
    RCC->CFGR |= (0x05UL << 10);  /* PPRE1 = 101, APB1 = HCLK/4 */

    /* 配置APB2分频: APB2 = HCLK / 2 = 84MHz */
    RCC->CFGR &= ~(0x07UL << 13); /* 清除PPRE2位 */
    RCC->CFGR |= (0x04UL << 13);  /* PPRE2 = 100, APB2 = HCLK/2 */

    /* 配置PLL参数 */
    /* PLL_M = 4, PLL_N = 168, PLL_P = 2, PLL_Q = 7 */
    RCC->PLLCFGR = (4UL << 0) | (168UL << 6) | (0UL << 16) | (7UL << 24);
    /* 设置PLL输入源为HSE */
    RCC->PLLCFGR |= (1UL << 22);  /* PLLSRC = 1, HSE作为PLL输入 */

    /* 使能PLL */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* 选择PLL作为系统时钟源 */
    RCC->CFGR &= ~0x03UL;         /* 清除SW位 */
    RCC->CFGR |= 0x02UL;          /* SW = 10, PLL作为SYSCLK */
    while ((RCC->CFGR & 0x0CUL) != 0x08UL); /* 等待PLL作为SYSCLK确认 */

    /* 更新系统全局变量 */
    g_SystemCoreClock = 168000000UL;

#ifdef ENABLE_DEBUG
    printf("[BSP] SystemClock: %lu Hz\r\n", g_SystemCoreClock);
#endif
}

/* ============================================================
 * 第四部分：GPIO初始化
 * ============================================================ */

/**
 * @brief   GPIO初始化函数
 * @note    初始化所有GPIO引脚，按功能分配模式:
 *          - PA7/8/9/10: TIM1 PWM输出 (复用推挽)
 *          - PA0/1/2/3:   ADC模拟输入
 *          - PA6/7:       编码器AB相 (复用)
 *          - PB0/1:       TIM1 PWM下桥输出
 *          - PB6/7/8:     编码器ABZ相
 *          - PC0-7:       数字输入 (上拉)
 *          - PC8-15:      数字输出 (推挽)
 *          - PD0/1/2:     LED输出
 *          - PD12:        RS485 DE方向控制
 */
void BSP_GPIO_Init(void)
{
    /* 使能GPIO时钟 */
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                     RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                     RCC_AHB1ENR_GPIOEEN);

    /* 简单的延时等待时钟稳定 */
    for (volatile uint32_t i = 0; i < 10; i++);

    /* ========== GPIOA 配置 ========== */

    /* PA7: PWM_UL (TIM1_CH1N) - 复用输出, 高速, 推挽, 无上下拉 */
    /* MODER[15:14] = 10 (AF), OSPEEDR[15:14] = 11 (VeryHigh), OTYPER[7] = 0 (PP), PUPDR[15:14] = 00 (None) */
    GPIOA->MODER  = (GPIOA->MODER & ~((3UL << 14))) | (2UL << 14);  /* PA7 AF */
    GPIOA->OSPEEDR |= (3UL << 14);   /* PA7 100MHz */
    GPIOA->PUPDR  &= ~(3UL << 14);  /* PA7 无上下拉 */

    /* PA8: PWM_UH (TIM1_CH1) - 复用输出 */
    GPIOA->MODER  = (GPIOA->MODER & ~((3UL << 16))) | (2UL << 16);  /* PA8 AF */
    GPIOA->OSPEEDR |= (3UL << 16);   /* PA8 100MHz */
    GPIOA->PUPDR  &= ~(3UL << 16);  /* PA8 无上下拉 */

    /* PA9: PWM_VH (TIM1_CH2) - 复用输出 */
    GPIOA->MODER  = (GPIOA->MODER & ~((3UL << 18))) | (2UL << 18);  /* PA9 AF */
    GPIOA->OSPEEDR |= (3UL << 18);   /* PA9 100MHz */
    GPIOA->PUPDR  &= ~(3UL << 18);

    /* PA10: PWM_WH (TIM1_CH3) - 复用输出 */
    GPIOA->MODER  = (GPIOA->MODER & ~((3UL << 20))) | (2UL << 20);  /* PA10 AF */
    GPIOA->OSPEEDR |= (3UL << 20);   /* PA10 100MHz */
    GPIOA->PUPDR  &= ~(3UL << 20);

    /* PA0: ADC1_IN0 (Ia电流) - 模拟输入 */
    GPIOA->MODER  |= (3UL << 0);    /* PA0 模拟模式 */

    /* PA1: ADC1_IN1 (Ib电流) - 模拟输入 */
    GPIOA->MODER  |= (3UL << 2);    /* PA1 模拟模式 */

    /* PA2: ADC1_IN2 (Udc母线电压) - 模拟输入 */
    GPIOA->MODER  |= (3UL << 4);    /* PA2 模拟模式 */

    /* PA3: ADC1_IN3 (NTC温度) - 模拟输入 */
    GPIOA->MODER  |= (3UL << 6);    /* PA3 模拟模式 */

    /* PA6: 编码器A相 (TIM3_CH1) - 复用输入 */
    GPIOA->MODER  = (GPIOA->MODER & ~((3UL << 12))) | (2UL << 12);  /* PA6 AF */
    GPIOA->PUPDR  &= ~(3UL << 12); /* PA6 无上下拉 */

    /* PA7: 编码器B相 (TIM3_CH2) - 复用输入 (PA7已被PWM_UL占用，此处实际用PB6/7) */

    /* PA9/PA10: USART1 TX/RX (Modbus) - 复用输出/输入 */
    /* 已在上面配置为PWM，这里配置USART复用 */
    /* PA9 (TX): USART1_TX - AF7 */
    /* PA10 (RX): USART1_RX - AF7 */

    /* 配置GPIOA复用功能选择寄存器 */
    /* AFR[0] = PA0-PA7, AFR[1] = PA8-PA15 */
    /* PA7 -> TIM1_CH1N (AF1) */
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFUL << 28)) | (0x01UL << 28); /* PA7 AF1 */
    /* PA8 -> TIM1_CH1 (AF1) */
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~((0xFUL << 0))) | (0x01UL << 0); /* PA8 AF1 */
    /* PA9 -> TIM1_CH2 (AF1) and USART1_TX (AF7) - 保持TIM1 */
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~((0xFUL << 4))) | (0x01UL << 4); /* PA9 AF1 TIM1 */
    /* PA10 -> TIM1_CH3 (AF1) and USART1_RX (AF7) - 保持TIM1 */
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~((0xFUL << 8))) | (0x01UL << 8); /* PA10 AF1 TIM1 */

    /* ========== GPIOB 配置 ========== */

    /* PB0: PWM_VL (TIM1_CH2N) - 复用输出, 推挽 */
    GPIOB->MODER  = (GPIOB->MODER & ~((3UL << 0))) | (2UL << 0);   /* PB0 AF */
    GPIOB->OSPEEDR |= (3UL << 0);   /* PB0 100MHz */
    GPIOB->PUPDR  &= ~(3UL << 0);
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~((0xFUL << 0))) | (0x01UL << 0); /* PB0 AF1 TIM1 */

    /* PB1: PWM_WL (TIM1_CH3N) - 复用输出, 推挽 */
    GPIOB->MODER  = (GPIOB->MODER & ~((3UL << 2))) | (2UL << 2);   /* PB1 AF */
    GPIOB->OSPEEDR |= (3UL << 2);   /* PB1 100MHz */
    GPIOB->PUPDR  &= ~(3UL << 2);
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~((0xFUL << 4))) | (0x01UL << 4); /* PB1 AF1 TIM1 */

    /* PB6: 编码器A相 (TIM4_CH1) - 复用输入 */
    GPIOB->MODER  = (GPIOB->MODER & ~((3UL << 12))) | (2UL << 12);  /* PB6 AF */
    GPIOB->PUPDR  &= ~(3UL << 12);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~((0xFUL << 24))) | (0x02UL << 24); /* PB6 AF2 TIM4 */

    /* PB7: 编码器B相 (TIM4_CH2) - 复用输入 */
    GPIOB->MODER  = (GPIOB->MODER & ~((3UL << 14))) | (2UL << 14);  /* PB7 AF */
    GPIOB->PUPDR  &= ~(3UL << 14);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~((0xFUL << 28))) | (0x02UL << 28); /* PB7 AF2 TIM4 */

    /* PB8: 编码器Z相 (TIM4_CH3) - 复用输入 */
    GPIOB->MODER  = (GPIOB->MODER & ~((3UL << 16))) | (2UL << 16);  /* PB8 AF */
    GPIOB->PUPDR  &= ~(3UL << 16);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~((0xFUL << 0))) | (0x02UL << 0); /* PB8 AF2 TIM4 */

    /* ========== GPIOC 配置 ========== */

    /* PC0-PC7: 数字输入 (DI0-DI7) - 输入模式, 上拉 */
    /* DI0: 启动按钮 - PC0 */
    GPIOC->MODER  &= ~(3UL << 0);   /* PC0 输入 */
    GPIOC->PUPDR  = (GPIOC->PUPDR & ~(3UL << 0)) | (1UL << 0); /* PC0 上拉 */

    /* DI1: 停止按钮 - PC1 */
    GPIOC->MODER  &= ~(3UL << 2);   /* PC1 输入 */
    GPIOC->PUPDR  = (GPIOC->PUPDR & ~(3UL << 2)) | (1UL << 2); /* PC1 上拉 */

    /* DI2: 方向选择 - PC2 */
    GPIOC->MODER  &= ~(3UL << 4);   /* PC2 输入 */
    GPIOC->PUPDR  = (GPIOC->PUPDR & ~(3UL << 4)) | (1UL << 4); /* PC2 上拉 */

    /* DI3: 外部故障输入 - PC3 */
    GPIOC->MODER  &= ~(3UL << 6);   /* PC3 输入 */
    GPIOC->PUPDR  = (GPIOC->PUPDR & ~(3UL << 6)) | (1UL << 6); /* PC3 上拉 */

    /* DI4-DI7: 多段速选择和急停 - PC4-PC7 */
    for (uint8_t i = 4; i <= 7; i++) {
        GPIOC->MODER &= ~(3UL << (i * 2));
        GPIOC->PUPDR = (GPIOC->PUPDR & ~(3UL << (i * 2))) | (1UL << (i * 2));
    }

    /* PC8-PC15: 数字输出 (DO0-DO7) - 输出模式, 推挽 */
    /* DO0: 运行接触器 - PC8 */
    GPIOC->MODER  = (GPIOC->MODER & ~((3UL << 16))) | (1UL << 16); /* PC8 输出 */
    GPIOC->OSPEEDR |= (2UL << 16);  /* PC8 中速 */
    GPIOC->OTYPER &= ~(1UL << 8);   /* PC8 推挽 */
    GPIOC->ODR &= ~(1UL << 8);      /* 初始输出低 */

    /* DO1: 故障继电器 - PC9 */
    GPIOC->MODER  = (GPIOC->MODER & ~((3UL << 18))) | (1UL << 18); /* PC9 输出 */
    GPIOC->OSPEEDR |= (2UL << 18);
    GPIOC->OTYPER &= ~(1UL << 9);
    GPIOC->ODR &= ~(1UL << 9);

    /* DO2: 风扇 - PC10 */
    GPIOC->MODER  = (GPIOC->MODER & ~((3UL << 20))) | (1UL << 20); /* PC10 输出 */
    GPIOC->OSPEEDR |= (2UL << 20);
    GPIOC->OTYPER &= ~(1UL << 10);
    GPIOC->ODR &= ~(1UL << 10);

    /* DO3: 制动电阻 - PC11 */
    GPIOC->MODER  = (GPIOC->MODER & ~((3UL << 22))) | (1UL << 22); /* PC11 输出 */
    GPIOC->OSPEEDR |= (2UL << 22);
    GPIOC->OTYPER &= ~(1UL << 11);
    GPIOC->ODR &= ~(1UL << 11);

    /* DO4-DO7: 备用输出 - PC12-PC15 */
    for (uint8_t i = 12; i <= 15; i++) {
        GPIOC->MODER = (GPIOC->MODER & ~((3UL << (i * 2)))) | (1UL << (i * 2));
        GPIOC->OSPEEDR |= (2UL << (i * 2));
        GPIOC->OTYPER &= ~(1UL << i);
        GPIOC->ODR &= ~(1UL << i);
    }

    /* ========== GPIOD 配置 ========== */

    /* PD0: LED电源指示 - 输出 */
    GPIOD->MODER  = (GPIOD->MODER & ~((3UL << 0))) | (1UL << 0);  /* PD0 输出 */
    GPIOD->OSPEEDR |= (1UL << 0);
    GPIOD->OTYPER &= ~(1UL << 0);
    GPIOD->ODR |= (1UL << 0);       /* LED初始亮 (低电平点亮或高电平点亮取决于电路) */

    /* PD1: LED运行指示 - 输出 */
    GPIOD->MODER  = (GPIOD->MODER & ~((3UL << 2))) | (1UL << 2);  /* PD1 输出 */
    GPIOD->OSPEEDR |= (1UL << 2);
    GPIOD->OTYPER &= ~(1UL << 1);
    GPIOD->ODR &= ~(1UL << 1);

    /* PD2: LED故障指示 - 输出 */
    GPIOD->MODER  = (GPIOD->MODER & ~((3UL << 4))) | (1UL << 4);  /* PD2 输出 */
    GPIOD->OSPEEDR |= (1UL << 4);
    GPIOD->OTYPER &= ~(1UL << 2);
    GPIOD->ODR &= ~(1UL << 2);

    /* PD12: RS485 DE (方向控制) - 输出 */
    GPIOD->MODER  = (GPIOD->MODER & ~((3UL << 24))) | (1UL << 24); /* PD12 输出 */
    GPIOD->OSPEEDR |= (2UL << 24);
    GPIOD->OTYPER &= ~(1UL << 12);
    GPIOD->ODR &= ~(1UL << 12);     /* 初始为接收模式 (DE=低) */

#ifdef ENABLE_DEBUG
    printf("[BSP] GPIO Init OK\r\n");
#endif
}

/* ============================================================
 * 第五部分：ADC初始化
 * ============================================================ */

/**
 * @brief   ADC初始化函数
 * @note    配置ADC1为多通道扫描模式，DMA循环传输:
 *          - 通道0(PA0): Ia电流
 *          - 通道1(PA1): Ib电流
 *          - 通道2(PA2): 母线电压Udc
 *          - 通道3(PA3): NTC温度
 *
 *          配置:
 *          - 12位分辨率
 *          - 采样时间15个ADC周期
 *          - 连续转换模式
 *          - DMA循环模式
 */
void BSP_ADC_Init(void)
{
    /* 使能ADC1和ADC2时钟 */
    RCC->APB2ENR |= (RCC_APB2ENR_ADC1EN | RCC_APB2ENR_ADC2EN);

    /* 使能DMA2时钟 */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

    /* 简单的延时 */
    for (volatile uint32_t i = 0; i < 10; i++);

    /* ========== ADC1 配置 ========== */

    /* 关闭ADC先 */
    ADC1->CR2 &= ~ADC_CR2_ADON;

    /* 配置CR1: 12位分辨率, 扫描模式 */
    ADC1->CR1 = ADC_CR1_SCAN | ADC_CR1_RES_12BIT;

    /* 配置CR2: DMA使能, 连续转换, 外部触发关闭 */
    ADC1->CR2 = ADC_CR2_DMA | ADC_CR2_CONT | ADC_CR2_DDS;

    /* 配置采样时间: SMP0-3 = 15cycles (通道0-3) */
    /* SMPR2: 通道0-9采样时间 */
    ADC1->SMPR2 = (0x02 << 0) | (0x02 << 3) | (0x02 << 6) | (0x02 << 9);

    /* 配置规则序列: 序列长度=4, 通道0/1/2/3 */
    /* SQR1: L[4:0]=4-1=3 表示4个通道 */
    ADC1->SQR1 = (3UL << 20);  /* L=4 (4个转换) */

    /* SQR3: SQ1=通道0, SQ2=通道1, SQ3=通道2, SQ4=通道3 */
    ADC1->SQR3 = (0UL << 0) | (1UL << 5) | (2UL << 10) | (3UL << 15);

    /* ========== DMA2 配置 (ADC1 -> Memory) ========== */
    /* DMA2_Stream0, Channel0, APB外设->内存, 循环模式 */

    /* 关闭DMA Stream先 */
    DMA2->S[0].CR &= ~DMA_SxCR_EN;

    /* 等待DMA Stream关闭 */
    while (DMA2->S[0].CR & DMA_SxCR_EN);

    /* 配置DMA: 外设地址固定, 内存递增, 16位传输 */
    DMA2->S[0].CR = (0UL << 6)     | /* DIR: 外设到内存 */
                    (1UL << 10)    | /* MINC: 内存递增 */
                    (0UL << 11)    | /* PINC: 外设不增 */
                    (1UL << 13)    | /* MSIZE: 16位内存 */
                    (1UL << 11)    | /* PSIZE: 16位外设 */
                    (1UL << 8)     | /* CIRC: 循环模式 */
                    (0UL << 25);    /* CHSEL: 通道0 (ADC1) */

    /* 设置外设地址: ADC1数据寄存器 */
    DMA2->S[0].PAR = (uint32_t)(&(ADC1->DR));

    /* 设置内存地址: DMA缓冲区 */
    DMA2->S[0].M0AR = (uint32_t)(bsp_adc_dma_buf);

    /* 设置传输数量: 缓冲区大小 */
    DMA2->S[0].NDTR = bsp_adc_dma_buf_size;

    /* 使能DMA Stream */
    DMA2->S[0].CR |= DMA_SxCR_EN;

    /* 使能ADC */
    ADC1->CR2 |= ADC_CR2_ADON;

    /* 启动第一次校准 */
    ADC1->CR2 |= (1UL << 30);  /* CAL */
    while (ADC1->CR2 & (1UL << 30)); /* 等待校准完成 */

    /* 启动第一次转换 (SWSTART) */
    ADC1->CR2 |= ADC_CR2_SWSTART;

#ifdef ENABLE_DEBUG
    printf("[BSP] ADC Init OK, DMA Mode\r\n");
#endif
}

/* ============================================================
 * 第六部分：PWM初始化 (TIM1)
 * ============================================================ */

/**
 * @brief   PWM初始化函数
 * @note    配置TIM1为三相PWM输出 (高级定时器):
 *          - 中心对齐模式1 (对称PWM，电流采样同步)
 *          - 互补输出 (上下桥臂互补)
 *          - 死区时间100ns
 *          - ARR设置载波频率
 *          - CCR设置初始占空比 (0%)
 *
 * @param   freq_hz: PWM载波频率，单位Hz，默认10000 (10kHz)
 */
void BSP_PWM_Init(uint32_t freq_hz)
{
    /* 保存PWM频率 */
    g_PwmFrequency = freq_hz;

    /* 使能TIM1时钟 (高级定时器) */
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    /* 简单的延时 */
    for (volatile uint32_t i = 0; i < 10; i++);

    /* ========== TIM1 配置 ========== */

    /* 关闭TIM1先 */
    TIM1->CR1 &= ~TIM_CR1_CEN;

    /* 配置CR1: 中心对齐模式1, ARR预装载使能, 计数器使能 */
    TIM1->CR1 = TIM_CR1_ARPE | TIM_CR1_CMS_MODE1;

    /* 配置CR2: 主模式选择: 计数器复位 (UEV on CNT=0) */
    TIM1->CR2 = (0x04 << 4);  /* MMS = 100: Reset */

    /* 配置PSC: 预分频, 使计数频率 = 168MHz / (PSC+1) */
    /* 最终计数器频率 = 168MHz / 1 = 168MHz */
    TIM1->PSC = 0;

    /* 配置ARR: 自动重装载值，决定PWM频率 */
    /* PWM频率 = 计数器频率 / (ARR+1) */
    /* ARR = 计数器频率 / 频率 - 1 = 168MHz / 10kHz - 1 = 16799 */
    uint32_t arr = g_SystemCoreClock / freq_hz - 1;
    TIM1->ARR = arr;

    /* 配置RCR: 重复计数寄存器，0=每次溢出都更新 (10kHz) */
    TIM1->RCR = 0;

    /* 配置CNT: 计数器初始值 */
    TIM1->CNT = 0;

    /* ========== 配置PWM模式 (CCMR1/CCMR2) ========== */

    /* CCMR1: 配置CH1和CH2 */
    /* CH1 (U相): PWM模式1 (CNT < CCR1时输出有效) */
    TIM1->CCMR1 = (TIM_CCMR1_OC1PE) |                   /* 预装载使能 */
                  (TIM_CCMR1_OC1M_PWM1) |               /* PWM模式1 */
                  (TIM_CCMR1_OC1M << 4);               /* 同理CH2 */

    /* CH1N (U相下桥): 同样PWM模式 */
    TIM1->CCMR1 |= (0x06 << 12);  /* OC2M[2:0] = 110 = PWM1 */

    /* CCMR2: 配置CH3和CH4 */
    /* CH3 (W相): PWM模式 */
    TIM1->CCMR2 = (TIM_CCMR2_OC3PE) |
                  (TIM_CCMR2_OC3M_PWM1);

    /* CH3N (W相下桥): */
    TIM1->CCMR2 |= (0x06 << 12);  /* OC4M[2:0] = 110 = PWM1 */

    /* ========== 配置死区和刹车 (BDTR) ========== */
    /* 死区时间 = DTG[7:5] * 2 * Tdts + DTG[4:0] * Tdts */
    /* 当Tdts = 1/168MHz ≈ 6ns, DTG=0x7F ≈ 212*6ns ≈ 1272ns */
    /* 目标死区100ns: DTG ≈ 100/6 ≈ 16, 取0x10 */
    uint8_t dtg = 0x10;
    TIM1->BDTR = (TIM_BDTR_MOE) |                /* 主输出使能 */
                 (0x7FUL << 0) |                  /* DTG = 0x7F (约1272ns死区) */
                 (0x00UL << 8);                    /* AOE=0, BKE=0 */

    /* ========== 配置CCER: 使能各通道的互补输出 ========== */
    /* CCER: CCxE=1(输出使能), CCxP=0(高电平有效), CCxNE=1(下桥使能), CCxNP=0 */
    TIM1->CCER = (TIM_CCER_CC1E) | (TIM_CCER_CC1NE) | /* U相 */
                 (TIM_CCER_CC2E) | (TIM_CCER_CC2NE) | /* V相 */
                 (TIM_CCER_CC3E) | (TIM_CCER_CC3NE) | /* W相 */
                 (0UL << 2) | (0UL << 6) | (0UL << 10); /* NP=0, 极性正常 */

    /* ========== 配置各通道CCR (初始占空比为0) ========== */
    TIM1->CCR1 = 0;  /* U相 */
    TIM1->CCR2 = 0;  /* V相 */
    TIM1->CCR3 = 0;  /* W相 */
    TIM1->CCR4 = 0;

    /* ========== 配置DIER: 使能更新中断 ========== */
    /* 注意: 主要控制循环在TIM1_UP_IRQHandler中执行 */
    TIM1->DIER = TIM_DIER_UIE;  /* 更新中断使能 */

    /* ========== 使能TIM1计数器 ========== */
    TIM1->CR1 |= TIM_CR1_CEN;

#ifdef ENABLE_DEBUG
    printf("[BSP] PWM Init: Freq=%lu Hz, ARR=%lu\r\n", freq_hz, arr);
#endif
}

/* ============================================================
 * 第七部分：编码器接口初始化 (TIM4)
 * ============================================================ */

/**
 * @brief   编码器接口初始化函数
 * @note    配置TIM4为ABZ编码器接口:
 *          - TI1FP1 = A相 (PB6)
 *          - TI2FP2 = B相 (PB7)
 *          - TI3FP3 = Z相 (PB8)
 *          - 编码器模式3: TI1和TI2边沿都计数 (4倍频)
 *
 *          Z相用于零点定位，触发中断
 */
void BSP_TIM_Encoder_Init(void)
{
    /* 使能TIM4时钟 (通用定时器) */
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

    /* 简单的延时 */
    for (volatile uint32_t i = 0; i < 10; i++);

    /* 关闭TIM4先 */
    TIM4->CR1 &= ~TIM_CR1_CEN;

    /* 配置SMCR: 编码器模式3 */
    /* SMS = 011: 编码器模式3, TI1和TI2边沿都计数 */
    TIM4->SMCR = (0x03 << 0);  /* SMS = 011 */

    /* 配置CCMR1: IC1映射到TI1, IC2映射到TI2 */
    /* CC1S = 01: IC1映射到TI1 */
    /* CC2S = 01: IC2映射到TI2 */
    TIM4->CCMR1 = (0x01 << 0) | (0x01 << 8);

    /* 配置CCMR2: IC3映射到TI3 (Z相) */
    /* CC3S = 01: IC3映射到TI3 */
    TIM4->CCMR2 = (0x01 << 0);

    /* 配置CCER: 使能各通道, 上升沿触发 */
    /* CC1E = 1, CC1P = 0 (上升沿) */
    /* CC2E = 1, CC2P = 0 (上升沿) */
    /* CC3E = 1, CC3P = 0 (上升沿) */
    TIM4->CCER = (1 << 0) | (1 << 4) | (1 << 8);

    /* 配置ARR: 最大计数值 (16位) */
    TIM4->ARR = 0xFFFF;

    /* 配置PSC: 预分频 (计数输入不分频) */
    TIM4->PSC = 0;

    /* 配置DIER: 使能捕获/比较3中断 (Z相中断) */
    TIM4->DIER = (1 << 3);  /* CC3IE = 1, Z相捕获中断 */

    /* 清零计数器 */
    TIM4->CNT = 0;

    /* 使能TIM4 */
    TIM4->CR1 |= TIM_CR1_CEN;

#ifdef ENABLE_DEBUG
    printf("[BSP] Encoder Init: TIM4, ABZ Mode\r\n");
#endif
}

/* ============================================================
 * 第八部分：UART初始化 (USART1)
 * ============================================================ */

/**
 * @brief   UART初始化函数
 * @note    配置USART1用于Modbus RTU通信:
 *          - 波特率可配置 (默认115200)
 *          - 8位数据位, 无奇偶校验, 1位停止位 (8N1)
 *          - 接收/发送中断使能
 *          - RS485模式: PD12作为DE方向控制
 *
 * @param   baud: 波特率，如115200UL
 */
void BSP_UART_Init(uint32_t baud)
{
    /* 使能USART1时钟 */
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    /* 简单的延时 */
    for (volatile uint32_t i = 0; i < 10; i++);

    /* 关闭USART1先 */
    USART1->CR1 &= ~USART_CR1_UE;

    /* ========== 配置GPIO复用功能 ========== */
    /* PA9: USART1_TX, PA10: USART1_RX */
    /* 需要将PA9/PA10从TIM1复用切换到USART1复用 */
    /* 清除TIM1 AF设置 */
    GPIOA->AFR[1] &= ~((0xFUL << 4) | (0xFUL << 8));
    /* 设置USART1 AF7 */
    GPIOA->AFR[1] |= (0x07UL << 4) | (0x07UL << 8);

    /* ========== USART1 配置 ========== */

    /* 配置CR1: 8位数据位, 发送/接收使能 */
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE;

    /* 配置CR2: 1位停止位 */
    USART1->CR2 = USART_CR2_STOP_1BIT;

    /* 配置CR3: 无硬件流控 */
    USART1->CR3 = 0;

    /* 配置BRR: 波特率设置 */
    /* BRR = USARTDIV = fPCLK2 / baud */
    /* fPCLK2 = 84MHz, baud = 115200 */
    /* USARTDIV = 84000000 / 115200 = 728.125 */
    /* 12.125 * 16 = 194.125 -> 194 = 0xC2 */
    uint32_t brr = g_SystemCoreClock / 2 / baud;  /* PCLK2 = SYSCLK/2 = 84MHz */
    USART1->BRR = brr;

    /* 配置中断: 接收中断使能 */
    USART1->CR1 |= USART_CR1_RXNEIE;

    /* 使能USART1 */
    USART1->CR1 |= USART_CR1_UE;

#ifdef ENABLE_DEBUG
    printf("[BSP] UART1 Init: Baud=%lu, BRR=%lu\r\n", baud, brr);
#endif
}

/* ============================================================
 * 第九部分：NVIC中断优先级配置
 * ============================================================ */

/**
 * @brief   NVIC中断优先级配置函数
 * @note    配置各外设中断的抢占优先级和响应优先级:
 *          - TIM1_UP: 优先级1 (最高, PWM周期中断)
 *          - ADC: 优先级2 (电流采样完成)
 *          - USART1: 优先级5 (Modbus通信)
 *          - EXTI0-3: 优先级6 (数字输入)
 *          - DMA2_Stream0: 优先级3 (ADC DMA)
 *
 *          STM32F4使用4位优先级 (分组设为2: 2位抢占+2位响应)
 */
void BSP_NVIC_Config(void)
{
    /* 配置优先级分组: 2位抢占, 2位子优先级 */
    /* SCB->AIRCR: 请求优先级分组 */
    *(volatile uint32_t *)0xE000ED0CUL = (0x05FA0000UL) | (0x500UL);

    /* ========== TIM1_UP 中断 (PWM周期中断) ========== */
    /* TIM1_UP_IRQn: 中断号25 */
    /* 优先级1 (最高) */
    /* NVIC->IP[25] = 0x10 (高2位=01, 低2位=00) */
    *(volatile uint8_t *)0xE000ED41UL = (1 << 4);  /* IP[25] = 0x10 */

    /* 使能TIM1_UP中断 */
    *(volatile uint32_t *)0xE000E100UL = (1UL << 25);  /* ISER[0] bit25 */

    /* ========== USART1 中断 (Modbus接收) ========== */
    /* USART1_IRQn: 中断号37 */
    /* 优先级5 */
    /* NVIC->IP[37] = 0x50 */
    *(volatile uint8_t *)0xE000ED4FUL = (5 << 4);  /* IP[37] = 0x50 */

    /* 使能USART1中断 */
    *(volatile uint32_t *)0xE000E104UL = (1UL << 5);  /* ISER[1] bit5 */

    /* ========== DMA2_Stream0 中断 (ADC DMA完成) ========== */
    /* DMA2_Stream0_IRQn: 中断号56 */
    /* 优先级3 */
    /* NVIC->IP[56] = 0x30 */
    *(volatile uint8_t *)0xE000ED61UL = (3 << 4);  /* IP[56] = 0x30 */

    /* 使能DMA2_Stream0中断 */
    *(volatile uint32_t *)0xE000E108UL = (1UL << 24);  /* ISER[1] bit24 */

    /* ========== EXTI0-3 中断 (数字输入) ========== */
    /* EXTI0_IRQn: 中断号6 */
    /* 优先级6 */
    *(volatile uint8_t *)0xE000ED1BUL = (6 << 4);  /* IP[6] = 0x60 */
    *(volatile uint32_t *)0xE000E100UL |= (1UL << 6);  /* ISER[0] bit6 */

    /* EXTI1_IRQn: 中断号7 */
    *(volatile uint8_t *)0xE000ED1CUL = (6 << 4);
    *(volatile uint32_t *)0xE000E100UL |= (1UL << 7);

    /* EXTI2_IRQn: 中断号8 */
    *(volatile uint8_t *)0xE000ED1DUL = (6 << 4);
    *(volatile uint32_t *)0xE000E100UL |= (1UL << 8);

    /* EXTI3_IRQn: 中断号9 */
    *(volatile uint8_t *)0xE000ED1EUL = (6 << 4);
    *(volatile uint32_t *)0xE000E100UL |= (1UL << 9);

#ifdef ENABLE_DEBUG
    printf("[BSP] NVIC Config: PWM=1, ADC=2, UART=5, EXTI=6\r\n");
#endif
}

/* ============================================================
 * 第十部分：PWM故障安全输出
 * ============================================================ */

/**
 * @brief   PWM故障安全输出控制
 * @note    当检测到严重故障时，强制关闭所有PWM输出
 *          将TIM1所有通道设置为复位状态 (低电平)
 *          同时关闭主输出使能 (MOE)
 *
 * @param   enable: 0=关闭PWM输出(故障安全), 1=使能PWM输出
 */
void BSP_FAULT_PWM_Output(uint8_t enable)
{
    if (enable == 0) {
        /* 故障安全: 关闭所有PWM输出 */

        /* 关闭主输出使能 (MOE) - 立即关断所有输出 */
        TIM1->BDTR &= ~TIM_BDTR_MOE;

        /* 可选: 将CCR设置为0，确保下次使能时安全 */
        TIM1->CCR1 = 0;
        TIM1->CCR2 = 0;
        TIM1->CCR3 = 0;

        /* 清除所有比较使能 */
        TIM1->CCER = 0;

#ifdef ENABLE_DEBUG
        printf("[BSP] FAULT: PWM Outputs Disabled!\r\n");
#endif
    } else {
        /* 恢复正常: 重新使能PWM输出 */

        /* 重新配置CCER (互补输出) */
        TIM1->CCER = (TIM_CCER_CC1E) | (TIM_CCER_CC1NE) |
                     (TIM_CCER_CC2E) | (TIM_CCER_CC2NE) |
                     (TIM_CCER_CC3E) | (TIM_CCER_CC3NE);

        /* 使能主输出 (MOE) */
        TIM1->BDTR |= TIM_BDTR_MOE;

#ifdef ENABLE_DEBUG
        printf("[BSP] PWM Outputs Enabled\r\n");
#endif
    }
}

/* ============================================================
 * 第十一部分：GPIO操作函数
 * ============================================================ */

void BSP_GPIO_WritePin(BSP_GPIO_TypeDef *port, uint16_t pin, uint8_t state)
{
    if (state == 0) {
        port->BSRR = (1UL << (pin + 16));  /* BR: 低位写1清零 */
    } else {
        port->BSRR = (1UL << pin);          /* BS: 低位写1置位 */
    }
}

uint8_t BSP_GPIO_ReadPin(BSP_GPIO_TypeDef *port, uint16_t pin)
{
    return (port->IDR & pin) ? 1 : 0;
}

void BSP_GPIO_WritePort(BSP_GPIO_TypeDef *port, uint16_t value)
{
    port->ODR = value;
}

uint16_t BSP_GPIO_ReadPort(BSP_GPIO_TypeDef *port)
{
    return (uint16_t)(port->IDR);
}

/* ============================================================
 * 第十二部分：延时和系统时钟函数
 * ============================================================ */

void BSP_Delay_US(uint32_t us)
{
    /* 简单的软件延时 */
    /* 168MHz / 4 = 42MHz, 约10个周期 = 0.25us @ 42MHz */
    volatile uint32_t count = us * (g_SystemCoreClock / 4000000UL);
    while (count--) {
        __asm__ __volatile__("nop");
    }
}

void BSP_Delay_MS(uint32_t ms)
{
    for (volatile uint32_t i = 0; i < ms; i++) {
        BSP_Delay_US(1000);
    }
}

uint32_t BSP_GetSysTick(void)
{
    return g_SysTickMs;
}

uint32_t BSP_GetSystemClock(void)
{
    return g_SystemCoreClock;
}

/* ============================================================
 * 第十三部分：看门狗函数
 * ============================================================ */

/**
 * @brief   独立看门狗初始化
 * @note    初始化IWDG，设置超时时间
 *          IWDG时钟 = LSI = 32kHz
 *          超时时间 = (4 * 2^prescaler) * reload / 32kHz
 *
 * @param   timeout_ms: 看门狗超时时间，单位ms
 */
void BSP_IWDG_Init(uint32_t timeout_ms)
{
    /* IWDG寄存器基地址 */
    volatile uint32_t *IWDG_KR = (volatile uint32_t *)0x40003000UL;
    volatile uint32_t *IWDG_PR = (volatile uint32_t *)0x40003004UL;
    volatile uint32_t *IWDG_RLR = (volatile uint32_t *)0x40003008UL;

    /* 解除写保护 */
    *IWDG_KR = 0x5555;

    /* 设置预分频: 近似timeout_ms对应的分频值 */
    /* timeout = (4 * 2^pr) * rlr / 32000 */
    uint8_t prescaler = 0;
    uint32_t reload = 0;

    /* 简单计算: timeout_ms / (4/32) = timeout_ms * 8000 */
    uint32_t temp = timeout_ms * 8000UL;

    /* 选择合适的预分频和重装载值 */
    if (temp < 65536) {
        prescaler = 0;   /* 4分频 */
        reload = temp;
    } else if (temp < 131072) {
        prescaler = 1;   /* 8分频 */
        reload = temp / 2;
    } else if (temp < 262144) {
        prescaler = 2;   /* 16分频 */
        reload = temp / 4;
    } else if (temp < 524288) {
        prescaler = 3;   /* 32分频 */
        reload = temp / 8;
    } else {
        prescaler = 4;   /* 64分频 */
        reload = temp / 16;
    }

    if (reload > 0xFFF) reload = 0xFFF;

    *IWDG_PR = prescaler;
    *IWDG_RLR = reload;

    /* 启动IWDG */
    *IWDG_KR = 0xCCCC;

#ifdef ENABLE_DEBUG
    printf("[BSP] IWDG Init: Timeout=%lu ms, PR=%u, RLR=%lu\r\n",
           timeout_ms, prescaler, reload);
#endif
}

void BSP_IWDG_Feed(void)
{
    /* 喂狗: 向KR写入0xAAAA */
    volatile uint32_t *IWDG_KR = (volatile uint32_t *)0x40003000UL;
    *IWDG_KR = 0xAAAA;
}

/* ============================================================
 * 第十四部分：PWM占空比设置
 * ============================================================ */

/**
 * @brief   PWM占空比设置函数
 * @note    设置三相PWM的占空比
 *          占空比 = CCR / ARR (中心对齐模式下为对称PWM)
 *
 * @param   uh/ul: U相上下桥臂占空比 (0-65535)
 * @param   vh/vl: V相上下桥臂占空比 (0-65535)
 * @param   wh/wl: W相上下桥臂占空比 (0-65535)
 */
void BSP_PWM_SetDuty(uint16_t uh, uint16_t ul,
                     uint16_t vh, uint16_t vl,
                     uint16_t wh, uint16_t wl)
{
    TIM1->CCR1 = uh;   /* U相上桥 */
    TIM1->CCR2 = vh;   /* V相上桥 */
    TIM1->CCR3 = wh;   /* W相上桥 */
    /* 下桥占空比由硬件自动生成 (互补输出) */
}

/* ============================================================
 * 第十五部分：ADC读取函数
 * ============================================================ */

uint16_t BSP_ADC_GetValue(uint8_t channel)
{
    /* DMA缓冲区数据排列: [Ia, Ib, Udc, NTC, Ia, Ib, Udc, NTC, ...] */
    if (channel < 4) {
        return bsp_adc_dma_buf[channel];
    }
    return 0;
}

/* ============================================================
 * 第十六部分：编码器函数
 * ============================================================ */

int16_t BSP_Encoder_GetCount(void)
{
    return (int16_t)(TIM4->CNT);
}

float BSP_Encoder_GetSpeed_RPM(void)
{
    /* 简化实现: 根据两次读数的时间差计算速度 */
    /* 实际实现需要记录上次CNT和上次时间 */
    static int16_t last_cnt = 0;
    static uint32_t last_tick = 0;

    int16_t curr_cnt = BSP_Encoder_GetCount();
    uint32_t curr_tick = g_SysTickMs;

    int16_t delta_cnt = curr_cnt - last_cnt;
    uint32_t delta_tick = curr_tick - last_tick;

    if (delta_tick == 0) return 0.0f;

    /* 转换为RPM: delta_cnt个脉冲 / delta_tick ms */
    /* 编码器分辨率 = ENCODER_PPR * 4 */
    float ppr = (float)ENCODER_PPR * 4.0f;
    float rpm = (float)delta_cnt * 60000.0f / (float)delta_tick / ppr;

    last_cnt = curr_cnt;
    last_tick = curr_tick;

    return rpm;
}

void BSP_Encoder_Reset(void)
{
    TIM4->CNT = 0;
}

/* ============================================================
 * 第十七部分：系统函数
 * ============================================================ */

void BSP_SystemReset(void)
{
    /* 触发系统复位 */
    volatile uint32_t *SCB_AIRCR = (volatile uint32_t *)0xE000ED0CUL;
    *SCB_AIRCR = (0x05FA0000UL) | (1UL << 2);  /* VECTRESET */
}

uint32_t BSP_GetUID(uint8_t idx)
{
    if (idx > 2) return 0;
    return *(volatile uint32_t *)(BSP_UID_BASE + idx * 4);
}

uint16_t BSP_GetFlashSize(void)
{
    return *(volatile uint16_t *)BSP_FLASH_SIZE_BASE;
}

/* ============================================================
 * 第十八部分：外设时钟使能/禁止
 * ============================================================ */

void BSP_EnablePeriphClock(BSP_Periph_t periph)
{
    switch (periph) {
        case BSP_PERIPH_GPIOA: RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; break;
        case BSP_PERIPH_GPIOB: RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; break;
        case BSP_PERIPH_GPIOC: RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; break;
        case BSP_PERIPH_GPIOD: RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN; break;
        case BSP_PERIPH_GPIOE: RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN; break;
        case BSP_PERIPH_TIM1:  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; break;
        case BSP_PERIPH_TIM2:  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; break;
        case BSP_PERIPH_TIM3:  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN; break;
        case BSP_PERIPH_TIM4:  RCC->APB1ENR |= RCC_APB1ENR_TIM4EN; break;
        case BSP_PERIPH_ADC1:  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; break;
        case BSP_PERIPH_ADC2:  RCC->APB2ENR |= RCC_APB2ENR_ADC2EN; break;
        case BSP_PERIPH_USART1: RCC->APB2ENR |= RCC_APB2ENR_USART1EN; break;
        case BSP_PERIPH_USART2: RCC->APB1ENR |= RCC_APB1ENR_USART2EN; break;
        case BSP_PERIPH_USART3: RCC->APB1ENR |= RCC_APB1ENR_USART3EN; break;
        case BSP_PERIPH_DMA1:  RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN; break;
        case BSP_PERIPH_DMA2:  RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN; break;
        default: break;
    }
}

void BSP_DisablePeriphClock(BSP_Periph_t periph)
{
    /* 实际实现时注意: 外设时钟关闭前应先确保外设已停止 */
    /* 此处简化处理 */
    switch (periph) {
        case BSP_PERIPH_GPIOA: RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOAEN; break;
        case BSP_PERIPH_GPIOB: RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOBEN; break;
        case BSP_PERIPH_GPIOC: RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOCEN; break;
        case BSP_PERIPH_GPIOD: RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIODEN; break;
        case BSP_PERIPH_GPIOE: RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOEEN; break;
        case BSP_PERIPH_TIM1:  RCC->APB2ENR &= ~RCC_APB2ENR_TIM1EN; break;
        case BSP_PERIPH_TIM2:  RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN; break;
        case BSP_PERIPH_TIM3:  RCC->APB1ENR &= ~RCC_APB1ENR_TIM3EN; break;
        case BSP_PERIPH_TIM4:  RCC->APB1ENR &= ~RCC_APB1ENR_TIM4EN; break;
        case BSP_PERIPH_ADC1:  RCC->APB2ENR &= ~RCC_APB2ENR_ADC1EN; break;
        case BSP_PERIPH_ADC2:  RCC->APB2ENR &= ~RCC_APB2ENR_ADC2EN; break;
        case BSP_PERIPH_USART1: RCC->APB2ENR &= ~RCC_APB2ENR_USART1EN; break;
        case BSP_PERIPH_USART2: RCC->APB1ENR &= ~RCC_APB1ENR_USART2EN; break;
        case BSP_PERIPH_USART3: RCC->APB1ENR &= ~RCC_APB1ENR_USART3EN; break;
        case BSP_PERIPH_DMA1:  RCC->AHB1ENR &= ~RCC_AHB1ENR_DMA1EN; break;
        case BSP_PERIPH_DMA2:  RCC->AHB1ENR &= ~RCC_AHB1ENR_DMA2EN; break;
        default: break;
    }
}

/* ============================================================
 * 第十九部分：BSP自检函数
 * ============================================================ */

int BSP_SelfTest(void)
{
    int result = 0;

#ifdef ENABLE_DEBUG
    printf("[BSP] Running Self-Test...\r\n");
#endif

    /* 检查系统时钟配置 */
    if (g_SystemCoreClock != 168000000UL) {
#ifdef ENABLE_DEBUG
        printf("[BSP] ERR: System Clock = %lu Hz (expected 168MHz)\r\n",
               g_SystemCoreClock);
#endif
        result = -1;
    }

    /* 检查ADC是否工作 (DMA缓冲区有数据) */
    uint16_t test_adc = BSP_ADC_GetValue(0);
    if (test_adc == 0 || test_adc == 0xFFFF) {
#ifdef ENABLE_DEBUG
        printf("[BSP] WARN: ADC may not be working (value=%u)\r\n", test_adc);
#endif
        /* ADC可能未连接传感器，不算错误 */
    }

    /* 检查GPIO基本功能 */
    /* 写入ODR并读回，验证GPIO寄存器是否正常 */
    GPIOD->ODR |= BSP_LED_POWER_PIN;
    BSP_Delay_MS(1);
    if (!(GPIOD->ODR & BSP_LED_POWER_PIN)) {
#ifdef ENABLE_DEBUG
        printf("[BSP] ERR: GPIO write/read failed\r\n");
#endif
        result = -2;
    }
    GPIOD->ODR &= ~BSP_LED_POWER_PIN;

#ifdef ENABLE_DEBUG
    if (result == 0) {
        printf("[BSP] Self-Test PASSED\r\n");
    } else {
        printf("[BSP] Self-Test FAILED (code=%d)\r\n", result);
    }
#endif

    return result;
}

/* ============================================================
 * 第二十部分：BSP版本信息
 * ============================================================ */

void BSP_PrintVersion(void)
{
#ifdef ENABLE_DEBUG
    printf("========================================\r\n");
    printf("[BSP] Board Support Package V1.0.0\r\n");
    printf("[BSP] Build Date: %s %s\r\n", __DATE__, __TIME__);
    printf("[BSP] MCU: STM32F4 Series\r\n");
    printf("[BSP] Core Clock: %lu Hz\r\n", g_SystemCoreClock);
    printf("[BSP] Flash Size: %u KB\r\n", BSP_GetFlashSize());
    printf("[BSP] UID: %08lX-%08lX-%08lX\r\n",
           BSP_GetUID(0), BSP_GetUID(1), BSP_GetUID(2));
    printf("========================================\r\n");
#endif
}

void BSP_PrintPinConfig(void)
{
#ifdef ENABLE_DEBUG
    printf("[BSP] GPIO Pin Configuration:\r\n");
    printf("  PA7:  TIM1_CH1N (PWM_U_L)\r\n");
    printf("  PA8:  TIM1_CH1   (PWM_U_H)\r\n");
    printf("  PA9:  TIM1_CH2   (PWM_V_H)\r\n");
    printf("  PA10: TIM1_CH3   (PWM_W_H)\r\n");
    printf("  PB0:  TIM1_CH2N  (PWM_V_L)\r\n");
    printf("  PB1:  TIM1_CH3N  (PWM_W_L)\r\n");
    printf("  PB6:  TIM4_CH1   (ENC_A)\r\n");
    printf("  PB7:  TIM4_CH2   (ENC_B)\r\n");
    printf("  PB8:  TIM4_CH3   (ENC_Z)\r\n");
    printf("  PC0-7: Digital Inputs (DI0-DI7)\r\n");
    printf("  PC8-15: Digital Outputs (DO0-DO7)\r\n");
    printf("  PD0-2: LEDs\r\n");
    printf("  PD12: RS485 DE (Direction Control)\r\n");
#endif
}

/* ============================================================
 * 第二十一部分：BSP初始化总入口
 * ============================================================ */

/**
 * @brief   BSP层初始化总入口
 * @note    按顺序调用各个子模块初始化函数
 *          在main()或系统初始化时调用一次
 *
 *          初始化顺序:
 *          1. 系统时钟 (最高优先级)
 *          2. GPIO
 *          3. ADC
 *          4. PWM
 *          5. 编码器
 *          6. UART
 *          7. NVIC
 *          8. 看门狗
 *          9. 自检和版本打印
 */
void BSP_Init(void)
{
#ifdef ENABLE_DEBUG
    printf("\r\n[BSP] Initializing BSP Layer...\r\n");
#endif

    /* 1. 系统时钟配置 */
    BSP_SystemClock_Config();

    /* 2. GPIO初始化 */
    BSP_GPIO_Init();

    /* 3. ADC初始化 */
    BSP_ADC_Init();

    /* 4. PWM初始化 (10kHz) */
    BSP_PWM_Init(PWM_FREQUENCY_HZ);

    /* 5. 编码器接口初始化 */
    BSP_TIM_Encoder_Init();

    /* 6. UART初始化 (115200) */
    BSP_UART_Init(MODBUS_BAUD_RATE);

    /* 7. NVIC中断优先级配置 */
    BSP_NVIC_Config();

    /* 8. 看门狗初始化 (3秒超时) */
    BSP_IWDG_Init(BSP_IWDG_TIMEOUT_MS);

    /* 9. 自检 */
    BSP_SelfTest();

    /* 10. 版本信息打印 */
    BSP_PrintVersion();
    BSP_PrintPinConfig();

#ifdef ENABLE_DEBUG
    printf("[BSP] BSP Initialization Complete!\r\n\r\n");
#endif
}

/* ============================================================
 * 第二十二部分：SysTick初始化（用于延时和计时）
 * ============================================================ */

/**
 * @brief   SysTick初始化函数
 * @note    配置SysTick为1ms中断，用于系统计时
 *          由BSP_Init()隐式调用，或由系统调用
 */
void BSP_SysTick_Init(void)
{
    /* 配置SysTick: 168MHz / 168 = 1MHz,reload=1000 -> 1ms中断 */
    SysTick->LOAD = (g_SystemCoreClock / 1000) - 1;  /* 1ms */
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE |
                    SysTick_CTRL_TICKINT |
                    SysTick_CTRL_ENABLE;
}

/* ============================================================
 * 第二十三部分：文件结束
 * ============================================================ */

/**
 * @}
 */

/**
 * @}
 */
