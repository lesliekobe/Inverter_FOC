/**
 ******************************************************************************
 * @file    bsp_hw_config.h
 * @brief   BSP硬件引脚与外设配置头文件 - 三相逆变器FOC项目
 * @version V1.0.0
 * @date    2024-01-01
 * @note    STM32F4系列硬件配置
 *          包含GPIO、PWM、ADC、UART、编码器等引脚定义
 ******************************************************************************
 */

#ifndef __BSP_HW_CONFIG_H
#define __BSP_HW_CONFIG_H

/* ============================================================
 * 第一部分：包含头文件
 * ============================================================ */
#include "global_def.h"
#include "global_ctx.h"

/* STM32F4 系列必须先定义，以便条件编译 */
#define STM32F4

/* ============================================================
 * 第二部分：STM32F4 条件编译区域
 * ============================================================ */
#if defined(MCU_STM32F4) || defined(STM32F4)

/* ---------- 系统时钟配置 ---------- */
/** @brief 外部高速晶振频率 (Hz)，PCB上焊接的HSE */
#ifndef HSE_VALUE
    #define HSE_VALUE                8000000UL       /* 8MHz 外部晶振 */
#endif

/** @brief 外部低速晶振频率 (Hz)，可选外接LSE */
#ifndef LSE_VALUE
    #define LSE_VALUE                32768UL         /* 32.768kHz RTC晶振 */
#endif

/* ---------- GPIO端口使能 ---------- */
/** @brief GPIOA 外设时钟使能 */
#define BSP_GPIOA_CLK_ENABLE()       do { RCC->AHB1ENR |= (1 << 0); } while(0)
/** @brief GPIOB 外设时钟使能 */
#define BSP_GPIOB_CLK_ENABLE()       do { RCC->AHB1ENR |= (1 << 1); } while(0)
/** @brief GPIOC 外设时钟使能 */
#define BSP_GPIOC_CLK_ENABLE()       do { RCC->AHB1ENR |= (1 << 2); } while(0)
/** @brief GPIOD 外设时钟使能 */
#define BSP_GPIOD_CLK_ENABLE()       do { RCC->AHB1ENR |= (1 << 3); } while(0)
/** @brief GPIOE 外设时钟使能（用于扩展） */
#define BSP_GPIOE_CLK_ENABLE()       do { RCC->AHB1ENR |= (1 << 4); } while(0)

/* ---------- GPIO模式定义 ---------- */
/** @brief GPIO 输入模式 */
#define BSP_GPIO_MODE_IN             0x00
/** @brief GPIO 输出模式 */
#define BSP_GPIO_MODE_OUT            0x01
/** @brief GPIO 复用功能模式 */
#define BSP_GPIO_MODE_AF             0x02
/** @brief GPIO 模拟输入模式 */
#define BSP_GPIO_MODE_AN             0x03

/* ---------- GPIO输出类型 ---------- */
/** @brief GPIO 推挽输出 */
#define BSP_GPIO_OTYPE_PP            0x00
/** @brief GPIO 开漏输出 */
#define BSP_GPIO_OTYPE_OD            0x01

/* ---------- GPIO速度定义 ---------- */
/** @brief GPIO 低速 (2MHz) */
#define BSP_GPIO_SPEED_LOW           0x00
/** @brief GPIO 中速 (25MHz) */
#define BSP_GPIO_SPEED_MED           0x01
/** @brief GPIO 高速 (50MHz) */
#define BSP_GPIO_SPEED_HIGH          0x02
/** @brief GPIO 极高速度 (100MHz) */
#define BSP_GPIO_SPEED_VERY_HIGH     0x03

/* ---------- GPIO上拉/下拉定义 ---------- */
/** @brief 无上拉下拉 */
#define BSP_GPIO_PUPD_NONE          0x00
/** @brief 上拉 */
#define BSP_GPIO_PUPD_UP            0x01
/** @brief 下拉 */
#define BSP_GPIO_PUPD_DOWN          0x02

/* ============================================================
 * 第三部分：PWM输出引脚定义 (TIM1 三相逆变器)
 * ============================================================ */
/**
 * @brief TIM1 为高级定时器，专门用于三相PWM输出
 *        支持互补输出和死区插入，适合电机控制
 */

/* ---------- U相 (U-phase) ---------- */
/** @brief U相上桥臂: TIM1_CH1 -> PA8 */
#define BSP_PWM_UH_PORT              GPIOA
#define BSP_PWM_UH_PIN               (1 << 8)        /* GPIO_Pin_8 */
#define BSP_PWM_UH_AF                0x01             /* GPIO_AF1_TIM1 */

/** @brief U相下桥臂: TIM1_CH1N -> PA7 */
#define BSP_PWM_UL_PORT              GPIOA
#define BSP_PWM_UL_PIN               (1 << 7)        /* GPIO_Pin_7 */
#define BSP_PWM_UL_AF                0x01             /* GPIO_AF1_TIM1 */

/* ---------- V相 (V-phase) ---------- */
/** @brief V相上桥臂: TIM1_CH2 -> PA9 */
#define BSP_PWM_VH_PORT              GPIOA
#define BSP_PWM_VH_PIN               (1 << 9)        /* GPIO_Pin_9 */
#define BSP_PWM_VH_AF                0x01             /* GPIO_AF1_TIM1 */

/** @brief V相下桥臂: TIM1_CH2N -> PB0 */
#define BSP_PWM_VL_PORT              GPIOB
#define BSP_PWM_VL_PIN               (1 << 0)        /* GPIO_Pin_0 */
#define BSP_PWM_VL_AF                0x01             /* GPIO_AF1_TIM1 */

/* ---------- W相 (W-phase) ---------- */
/** @brief W相上桥臂: TIM1_CH3 -> PA10 */
#define BSP_PWM_WH_PORT              GPIOA
#define BSP_PWM_WH_PIN               (1 << 10)       /* GPIO_Pin_10 */
#define BSP_PWM_WH_AF                0x01             /* GPIO_AF1_TIM1 */

/** @brief W相下桥臂: TIM1_CH3N -> PB1 */
#define BSP_PWM_WL_PORT              GPIOB
#define BSP_PWM_WL_PIN               (1 << 1)        /* GPIO_Pin_1 */
#define BSP_PWM_WL_AF                0x01             /* GPIO_AF1_TIM1 */

/* ---------- TIM1 外设时钟 ---------- */
#define BSP_TIM1_CLK_ENABLE()        do { RCC->APB2ENR |= (1 << 0); } while(0)   /* TIM1EN */

/* ---------- PWM 配置参数 ---------- */
/** @brief PWM 死区时间 (ns)，防止上下桥臂直通短路 */
#define BSP_PWM_DEADTIME_NS          100

/** @brief PWM 中心对齐模式: 1=Up-Down (对称PWM) */
#define BSP_PWM_CENTER_MODE          1

/* ============================================================
 * 第四部分：ADC采样引脚定义
 * ============================================================ */
/**
 * @brief 使用ADC1的多通道扫描模式，配合DMA实现连续采样
 *        电流采样使用相电流传感器（隔离或非隔离）
 */

/* ---------- 电流采样通道 ---------- */
/** @brief Ia: U相电流 -> ADC1_IN0 -> PA0 */
#define BSP_ADC_IA_PORT              GPIOA
#define BSP_ADC_IA_PIN               (1 << 0)
#define BSP_ADC_IA_CHANNEL           0x00            /* ADC_Channel_0 */

/** @brief Ib: V相电流 -> ADC1_IN1 -> PA1 */
#define BSP_ADC_IB_PORT              GPIOA
#define BSP_ADC_IB_PIN               (1 << 1)
#define BSP_ADC_IB_CHANNEL           0x01            /* ADC_Channel_1 */

/* ---------- 电压与温度采样通道 ---------- */
/** @brief Udc: 直流母线电压 -> ADC1_IN2 -> PA2 */
#define BSP_ADC_UDC_PORT             GPIOA
#define BSP_ADC_UDC_PIN              (1 << 2)
#define BSP_ADC_UDC_CHANNEL          0x02            /* ADC_Channel_2 */

/** @brief NTC: IPM/电机温度 -> ADC1_IN3 -> PA3 */
#define BSP_ADC_NTC_PORT             GPIOA
#define BSP_ADC_NTC_PIN              (1 << 3)
#define BSP_ADC_NTC_CHANNEL          0x03            /* ADC_Channel_3 */

/* ---------- ADC2 额外通道 ---------- */
/** @brief 备用模拟输入1 -> ADC2_IN1 -> PA1 (与ADC1同步采样) */
#define BSP_ADC2_CH1_PORT            GPIOA
#define BSP_ADC2_CH1_PIN             (1 << 1)
#define BSP_ADC2_CH1_CHANNEL         0x01            /* ADC_Channel_1 */

/** @brief 备用模拟输入2 -> ADC2_IN2 -> PA2 */
#define BSP_ADC2_CH2_PORT            GPIOA
#define BSP_ADC2_CH2_PIN             (1 << 2)
#define BSP_ADC2_CH2_CHANNEL         0x02            /* ADC_Channel_2 */

/* ---------- ADC外设时钟 ---------- */
#define BSP_ADC1_CLK_ENABLE()        do { RCC->APB2ENR |= (1 << 8); } while(0)   /* ADC1EN */
#define BSP_ADC2_CLK_ENABLE()        do { RCC->APB2ENR |= (1 << 9); } while(0)   /* ADC2EN */
#define BSP_ADC_COMMON_CLK_ENABLE()  do { RCC->APB2ENR |= (1 << 10); } while(0)  /* ADC Common */

/* ---------- ADC配置参数 ---------- */
/** @brief ADC 分辨率: 12位 (0-4095) */
#define BSP_ADC_RESOLUTION            12

/** @brief ADC 采样时间选择 (周期数，越长越精确但越慢) */
/* ADC_SMPR_SMP_15CYC: 15个ADC时钟周期，较为平衡 */
#define BSP_ADC_SAMPLE_TIME          0x02            /* ADC_SMPR_SMP_15CYC */

/** @brief ADC DMA 通道 (DMA2 Stream 0 Channel 0 for ADC1) */
#define BSP_ADC_DMA_STREAM           DMA2_Stream0
#define BSP_ADC_DMA_CHANNEL          0x00            /* DMA_CHANNEL_0 */

/* ---------- ADC DMA时钟 ---------- */
#define BSP_DMA2_CLK_ENABLE()        do { RCC->AHB1ENR |= (1 << 22); } while(0)   /* DMA2EN */

/* ============================================================
 * 第五部分：数字输入引脚定义 (DI)
 * ============================================================ */
/**
 * @brief 数字输入用于启停、方向、故障信号、多段速选择等
 *        使用PC0~PC7，带上拉电阻，外部按钮接地触发
 */

/** @brief DI0: 启动按钮 -> PC0 */
#define BSP_DI_START_PORT            GPIOC
#define BSP_DI_START_PIN             (1 << 0)

/** @brief DI1: 停止按钮 -> PC1 */
#define BSP_DI_STOP_PORT             GPIOC
#define BSP_DI_STOP_PIN              (1 << 1)

/** @brief DI2: 正反转选择 -> PC2 */
#define BSP_DI_REVERSE_PORT          GPIOC
#define BSP_DI_REVERSE_PIN           (1 << 2)

/** @brief DI3: 外部故障输入 -> PC3 */
#define BSP_DI_FAULT_IN_PORT         GPIOC
#define BSP_DI_FAULT_IN_PIN          (1 << 3)

/** @brief DI4: 多段速选择0 -> PC4 */
#define BSP_DI_SPEED0_PORT           GPIOC
#define BSP_DI_SPEED0_PIN            (1 << 4)

/** @brief DI5: 多段速选择1 -> PC5 */
#define BSP_DI_SPEED1_PORT           GPIOC
#define BSP_DI_SPEED1_PIN            (1 << 5)

/** @brief DI6: 多段速选择2 -> PC6 */
#define BSP_DI_SPEED2_PORT           GPIOC
#define BSP_DI_SPEED2_PIN            (1 << 6)

/** @brief DI7: 急停输入 -> PC7 */
#define BSP_DI_EMERGENCY_PORT        GPIOC
#define BSP_DI_EMERGENCY_PIN         (1 << 7)

/* ---------- DI端口时钟（已包含在GPIOC中） ---------- */

/* ============================================================
 * 第六部分：数字输出引脚定义 (DO)
 * ============================================================ */
/**
 * @brief 数字输出用于控制继电器、接触器、风扇、制动电阻等
 *        PC8~PC15 推挽输出
 */

/** @brief DO0: 运行接触器 -> PC8 */
#define BSP_DO_RUN_CONTACT_PORT      GPIOC
#define BSP_DO_RUN_CONTACT_PIN       (1 << 8)

/** @brief DO1: 故障继电器 -> PC9 */
#define BSP_DO_FAULT_CONTACT_PORT    GPIOC
#define BSP_DO_FAULT_CONTACT_PIN     (1 << 9)

/** @brief DO2: 风扇控制 -> PC10 */
#define BSP_DO_FAN_PORT              GPIOC
#define BSP_DO_FAN_PIN               (1 << 10)

/** @brief DO3: 制动电阻接触器 -> PC11 */
#define BSP_DO_BRAKE_PORT            GPIOC
#define BSP_DO_BRAKE_PIN             (1 << 11)

/** @brief DO4: 备用输出1 -> PC12 */
#define BSP_DO_SPARE1_PORT           GPIOC
#define BSP_DO_SPARE1_PIN            (1 << 12)

/** @brief DO5: 备用输出2 -> PC13 */
#define BSP_DO_SPARE2_PORT           GPIOC
#define BSP_DO_SPARE2_PIN            (1 << 13)

/** @brief DO6: 备用输出3 -> PC14 */
#define BSP_DO_SPARE3_PORT           GPIOC
#define BSP_DO_SPARE3_PIN            (1 << 14)

/** @brief DO7: 备用输出4 -> PC15 */
#define BSP_DO_SPARE4_PORT           GPIOC
#define BSP_DO_SPARE4_PIN            (1 << 15)

/* ============================================================
 * 第七部分：UART通信引脚定义 (Modbus RS485)
 * ============================================================ */
/**
 * @brief USART1用于Modbus RTU通信 (RS485接口)
 *        PA9=TX, PA10=RX, PD12=DE(方向控制)
 */

/** @brief USART1 TX -> PA9 */
#define BSP_MODBUS_TX_PORT           GPIOA
#define BSP_MODBUS_TX_PIN            (1 << 9)
#define BSP_MODBUS_TX_AF            0x07            /* GPIO_AF7_USART1 */

/** @brief USART1 RX -> PA10 */
#define BSP_MODBUS_RX_PORT           GPIOA
#define BSP_MODBUS_RX_PIN            (1 << 10)
#define BSP_MODBUS_RX_AF            0x07            /* GPIO_AF7_USART1 */

/** @brief RS485 DE (Driver Enable) -> PD12 */
#define BSP_MODBUS_DE_PORT           GPIOD
#define BSP_MODBUS_DE_PIN            (1 << 12)

/** @brief USART1 外设时钟使能 */
#define BSP_USART1_CLK_ENABLE()      do { RCC->APB2ENR |= (1 << 4); } while(0)    /* USART1EN */

/* ---------- 调试串口 USART2 (可选) ---------- */
/** @brief USART2 TX -> PA2 (复用到Modbus预留或独立调试口) */
#define BSP_DEBUG_TX_PORT            GPIOA
#define BSP_DEBUG_TX_PIN             (1 << 2)
#define BSP_DEBUG_TX_AF             0x07            /* GPIO_AF7_USART2 */

/** @brief USART2 RX -> PA3 */
#define BSP_DEBUG_RX_PORT            GPIOA
#define BSP_DEBUG_RX_PIN             (1 << 3)
#define BSP_DEBUG_RX_AF             0x07            /* GPIO_AF7_USART2 */

#define BSP_USART2_CLK_ENABLE()      do { RCC->APB1ENR |= (1 << 17); } while(0)    /* USART2EN */

/* ============================================================
 * 第八部分：增量编码器引脚定义 (ABZ)
 * ============================================================ */
/**
 * @brief 使用TIM4作为编码器接口 (ABZ三通道)
 *        PB6=A相, PB7=B相, PB8=Z相(零位脉冲)
 */

/** @brief 编码器A相 -> PB6 (TIM4_CH1) */
#define BSP_ENC_A_PORT              GPIOB
#define BSP_ENC_A_PIN               (1 << 6)
#define BSP_ENC_A_AF                0x02            /* GPIO_AF2_TIM4 */

/** @brief 编码器B相 -> PB7 (TIM4_CH2) */
#define BSP_ENC_B_PORT              GPIOB
#define BSP_ENC_B_PIN               (1 << 7)
#define BSP_ENC_B_AF                0x02            /* GPIO_AF2_TIM4 */

/** @brief 编码器Z相(零位) -> PB8 (TIM4_CH3) */
#define BSP_ENC_Z_PORT              GPIOB
#define BSP_ENC_Z_PIN               (1 << 8)
#define BSP_ENC_Z_AF                0x02            /* GPIO_AF2_TIM4 */

/** @brief TIM4 外设时钟使能 (编码器接口) */
#define BSP_TIM4_CLK_ENABLE()       do { RCC->APB1ENR |= (1 << 2); } while(0)     /* TIM4EN */

/* ============================================================
 * 第九部分：中断优先级定义
 * ============================================================ */
/**
 * @brief 中断优先级分组:
 *        NVIC优先级分组设为2 (2位抢占优先级, 2位响应优先级)
 *        数值越小优先级越高
 */

/** @brief TIM1更新中断优先级 (PWM周期中断，主要控制循环) - 高优先级 */
#define BSP_NVIC_PWM_PRIORITY        1

/** @brief ADC中断优先级 (电流/电压采样完成) - 高优先级 */
#define BSP_NVIC_ADC_PRIORITY       2

/** @brief USART1中断优先级 (Modbus接收) - 中等优先级 */
#define BSP_NVIC_UART_PRIORITY      5

/** @brief EXTI外部中断优先级 (DI按钮/故障) - 中等优先级 */
#define BSP_NVIC_EXTI_PRIORITY      6

/** @brief TIM4编码器中断优先级 (Z相零位脉冲) */
#define BSP_NVIC_ENC_Z_PRIORITY      4

/** @brief DMA中断优先级 (ADC DMA传输完成) */
#define BSP_NVIC_DMA_PRIORITY       3

/* ---------- EXTI线路定义 ---------- */
/** @brief EXTI0 - PC0 (启动按钮) */
#define BSP_EXTI_DI_START_LINE       EXTI_Line0
/** @brief EXTI1 - PC1 (停止按钮) */
#define BSP_EXTI_DI_STOP_LINE        EXTI_Line1
/** @brief EXTI2 - PC2 (方向) */
#define BSP_EXTI_DI_REVERSE_LINE     EXTI_Line2
/** @brief EXTI3 - PC3 (故障输入) */
#define BSP_EXTI_DI_FAULT_LINE       EXTI_Line3

/* ============================================================
 * 第十部分：DMA通道定义
 * ============================================================ */
/**
 * @brief DMA2用于ADC1的DMA请求
 *        DMA2_Stream0: ADC1 -> 内存 (循环模式)
 */

/** @brief ADC1 DMA请求源 */
#define BSP_ADC_DMA_REQUEST          0x00            /* ADC1 -> DMA2_Stream0 */

/* ---------- DMA2_Stream0 ---------- */
#define BSP_DMA2_STREAM0_CLK_ENABLE() do { RCC->AHB1ENR |= (1 << 22); } while(0)

/* ---------- USART1 DMA定义 (可选) ---------- */
/** @brief USART1_TX DMA: DMA2_Stream7 */
#define BSP_USART1_TX_DMA_STREAM     DMA2_Stream7
#define BSP_USART1_TX_DMA_CHANNEL   0x04            /* DMA_CHANNEL_4 */
/** @brief USART1_RX DMA: DMA2_Stream2 */
#define BSP_USART1_RX_DMA_STREAM     DMA2_Stream2
#define BSP_USART1_RX_DMA_CHANNEL   0x04            /* DMA_CHANNEL_4 */

#define BSP_DMA2_STREAM7_CLK_ENABLE() do { RCC->AHB1ENR |= (1 << 22); } while(0)
#define BSP_DMA2_STREAM2_CLK_ENABLE() do { RCC->AHB1ENR |= (1 << 22); } while(0)

/* ============================================================
 * 第十一部分：LED指示灯定义
 * ============================================================ */
/**
 * @brief 板载LED用于状态指示
 */

/** @brief 电源指示灯 -> PD0 (绿色，常亮表示电源正常) */
#define BSP_LED_POWER_PORT           GPIOD
#define BSP_LED_POWER_PIN            (1 << 0)

/** @brief 运行指示灯 -> PD1 (蓝色，运行时常亮) */
#define BSP_LED_RUN_PORT             GPIOD
#define BSP_LED_RUN_PIN              (1 << 1)

/** @brief 故障指示灯 -> PD2 (红色，故障时闪烁) */
#define BSP_LED_FAULT_PORT           GPIOD
#define BSP_LED_FAULT_PIN            (1 << 2)

/* ============================================================
 * 第十二部分：看门狗配置
 * ============================================================ */
/** @brief 独立看门狗时钟: LSI = 32kHz */
#define BSP_IWDG_CLK_HZ             32000UL

/** @brief 看门狗超时时间 (ms)，默认3秒 */
#ifndef BSP_IWDG_TIMEOUT_MS
    #define BSP_IWDG_TIMEOUT_MS      3000
#endif

/* ============================================================
 * 第十三部分：系统时钟总线定义
 * ============================================================ */
/** @brief AHB1总线最大频率: 168MHz */
#define BSP_RCC_CFGR_HPRE_DIV1       0x00    /* HCLK = SYSCLK */
#define BSP_RCC_CFGR_HPRE_DIV2       0x08    /* HCLK = SYSCLK/2 */

/** @brief APB1总线最大频率: 42MHz (168MHz/4) */
#define BSP_RCC_CFGR_PPRE1_DIV4      0x05    /* APB1 = HCLK/4 */
#define BSP_RCC_CFGR_PPRE2_DIV2      0x04    /* APB2 = HCLK/2 */

/** @brief PLL参数: 8MHz HSE -> 168MHz SYSCLK */
/** @brief PLL_M = 4,  PLL_N = 168, PLL_P = 2, PLL_Q = 7 */
#define BSP_PLL_M                   4
#define BSP_PLL_N                   168
#define BSP_PLL_P                   2
#define BSP_PLL_Q                   7

/* ============================================================
 * 第十四部分：引脚复用功能编号定义
 * ============================================================ */
/**
 * @brief GPIO复用功能(AF)编号定义
 * @note 具体AF编号请参考STM32F4数据手册
 */
typedef enum {
    BSP_AF_TIM1   = 0x01,      /* AF1: TIM1/TIM2 */
    BSP_AF_TIM2   = 0x01,      /* AF1: TIM1/TIM2 */
    BSP_AF_TIM3   = 0x02,      /* AF2: TIM3/TIM4/TIM5 */
    BSP_AF_TIM4   = 0x02,      /* AF2: TIM3/TIM4/TIM5 */
    BSP_AF_TIM5   = 0x02,      /* AF2: TIM3/TIM4/TIM5 */
    BSP_AF_USART1 = 0x07,      /* AF7: USART1/2/3 */
    BSP_AF_USART2 = 0x07,      /* AF7: USART1/2/3 */
    BSP_AF_USART3 = 0x07,      /* AF7: USART1/2/3 */
    BSP_AF_SPI1   = 0x05,      /* AF5: SPI1/SPI2/I2S */
    BSP_AF_SPI2   = 0x05,      /* AF5: SPI1/SPI2/I2S */
    BSP_AF_I2C1   = 0x04,      /* AF4: I2C1/2/3 */
    BSP_AF_I2C2   = 0x04,      /* AF4: I2C1/2/3 */
    BSP_AF_I2C3   = 0x04,      /* AF4: I2C1/2/3 */
    BSP_AF_CAN1   = 0x09,      /* AF9: CAN1/CAN2 */
    BSP_AF_CAN2   = 0x09,      /* AF9: CAN1/CAN2 */
    BSP_AF_ETH    = 0x0B       /* AF11: Ethernet */
} BSP_AF_t;

/* ============================================================
 * 第十五部分：外设基地址定义 (STM32F407ZG)
 * ============================================================ */
/**
 * @brief STM32F407ZG 外设基地址
 *        适用于其他STM32F4系列时请根据数据手册调整
 */

/* ---------- 定时器 ---------- */
#define BSP_TIM1_BASE               0x40010000UL
#define BSP_TIM2_BASE               0x40000000UL
#define BSP_TIM3_BASE               0x40000400UL
#define BSP_TIM4_BASE               0x40000800UL
#define BSP_TIM5_BASE               0x40000C00UL

/* ---------- ADC ---------- */
#define BSP_ADC1_BASE               0x40012000UL
#define BSP_ADC2_BASE               0x40012100UL
#define BSP_ADC_COMMON_BASE         0x40012300UL

/* ---------- USART ---------- */
#define BSP_USART1_BASE             0x40011000UL
#define BSP_USART2_BASE             0x40004400UL
#define BSP_USART3_BASE             0x40004800UL

/* ---------- GPIO ---------- */
#define BSP_GPIOA_BASE              0x40020000UL
#define BSP_GPIOB_BASE              0x40020400UL
#define BSP_GPIOC_BASE              0x40020800UL
#define BSP_GPIOD_BASE              0x40020C00UL
#define BSP_GPIOE_BASE              0x40021000UL

/* ---------- RCC ---------- */
#define BSP_RCC_BASE                0x40023800UL

/* ---------- DMA ---------- */
#define BSP_DMA1_BASE               0x40026000UL
#define BSP_DMA2_BASE               0x40026400UL

/* ---------- EXTI ---------- */
#define BSP_EXTI_BASE               0x40013C00UL

/* ---------- NVIC ---------- */
#define BSP_NVIC_BASE               0xE000E100UL

/* ---------- FLASH ---------- */
#define BSP_FLASH_BASE              0x08000000UL

/* ---------- CRC ---------- */
#define BSP_CRC_BASE                0x40023000UL

/* ============================================================
 * 第十六部分：外设寄存器结构体声明
 * ============================================================ */
/**
 * @brief STM32F4 外设寄存器结构体
 * @note 简化版，仅包含本项目所需的外设寄存器
 */

/* ---------- RCC寄存器结构体 ---------- */
typedef struct {
    volatile uint32_t CR;           /* 0x00 时钟控制寄存器 */
    volatile uint32_t PLLCFGR;      /* 0x04 PLL配置寄存器 */
    volatile uint32_t CFGR;         /* 0x08 时钟配置寄存器 */
    volatile uint32_t CIR;          /* 0x0C 时钟中断寄存器 */
    volatile uint32_t AHB1RSTR;     /* 0x10 AHB1外设复位寄存器 */
    volatile uint32_t AHB2RSTR;     /* 0x14 AHB2外设复位寄存器 */
    volatile uint32_t AHB3RSTR;     /* 0x18 AHB3外设复位寄存器 */
    uint32_t RESERVED0;             /* 0x1C 保留 */
    volatile uint32_t APB1RSTR;     /* 0x20 APB1外设复位寄存器 */
    volatile uint32_t APB2RSTR;     /* 0x24 APB2外设复位寄存器 */
    uint32_t RESERVED1[2];          /* 0x28-0x2C 保留 */
    volatile uint32_t AHB1ENR;      /* 0x30 AHB1外设时钟使能寄存器 */
    volatile uint32_t AHB2ENR;      /* 0x34 AHB2外设时钟使能寄存器 */
    volatile uint32_t AHB3ENR;      /* 0x38 AHB3外设时钟使能寄存器 */
    uint32_t RESERVED2;             /* 0x3C 保留 */
    volatile uint32_t APB1ENR;      /* 0x40 APB1外设时钟使能寄存器 */
    volatile uint32_t APB2ENR;      /* 0x44 APB2外设时钟使能寄存器 */
    uint32_t RESERVED3[2];          /* 0x48-0x4C 保留 */
    volatile uint32_t BDCR;         /* 0x50 RTC域控制寄存器 */
    volatile uint32_t CSR;          /* 0x54 时钟控制状态寄存器 */
    uint32_t RESERVED4[2];          /* 0x58-0x5C 保留 */
    volatile uint32_t SSCGR;         /* 0x60 扩展时钟发生器寄存器 */
    volatile uint32_t PLLI2SCFGR;   /* 0x64 PLLI2S配置寄存器 */
    volatile uint32_t PLLSAICFGR;   /* 0x68 PLLSAI配置寄存器 */
    volatile uint32_t DCKCFGR;      /* 0x6C 专用时钟配置寄存器 */
    volatile uint32_t CKGATENR;     /* 0x70 时钟门控使能寄存器 */
    volatile uint32_t DCKCFGR2;     /* 0x74 专用时钟配置寄存器2 */
} BSP_RCC_TypeDef;

/* ---------- GPIO寄存器结构体 ---------- */
typedef struct {
    volatile uint32_t MODER;        /* 0x00 GPIO端口模式寄存器 */
    volatile uint32_t OTYPER;       /* 0x04 GPIO输出类型寄存器 */
    volatile uint32_t OSPEEDR;      /* 0x08 GPIO输出速度寄存器 */
    volatile uint32_t PUPDR;        /* 0x0C GPIO上拉/下拉寄存器 */
    volatile uint32_t IDR;          /* 0x10 GPIO输入数据寄存器 */
    volatile uint32_t ODR;          /* 0x14 GPIO输出数据寄存器 */
    volatile uint32_t BSRR;         /* 0x18 GPIO位设置/清除寄存器 */
    volatile uint32_t LCKR;         /* 0x1C GPIO配置锁定寄存器 */
    volatile uint32_t AFR[2];       /* 0x20-0x24 GPIO复用功能寄存器 */
} BSP_GPIO_TypeDef;

/* ---------- TIM1/TIM8高级定时器寄存器结构体 ---------- */
typedef struct {
    volatile uint32_t CR1;          /* 0x00 控制寄存器1 */
    volatile uint32_t CR2;          /* 0x04 控制寄存器2 */
    volatile uint32_t SMCR;         /* 0x08 从模式控制寄存器 */
    volatile uint32_t DIER;         /* 0x0C DMA/中断使能寄存器 */
    volatile uint32_t SR;           /* 0x10 状态寄存器 */
    volatile uint32_t EGR;          /* 0x14 事件生成寄存器 */
    volatile uint32_t CCMR1;        /* 0x18 捕获/比较模式寄存器1 */
    volatile uint32_t CCMR2;        /* 0x1C 捕获/比较模式寄存器2 */
    volatile uint32_t CCER;         /* 0x20 捕获/比较使能寄存器 */
    volatile uint32_t CNT;          /* 0x24 计数器 */
    volatile uint32_t PSC;          /* 0x28 预分频器 */
    volatile uint32_t ARR;          /* 0x2C 自动重装载寄存器 */
    volatile uint32_t RCR;          /* 0x30 重复计数寄存器 */
    volatile uint32_t CCR1;         /* 0x34 捕获/比较寄存器1 */
    volatile uint32_t CCR2;         /* 0x38 捕获/比较寄存器2 */
    volatile uint32_t CCR3;         /* 0x3C 捕获/比较寄存器3 */
    volatile uint32_t CCR4;         /* 0x40 捕获/比较寄存器4 */
    volatile uint32_t BDTR;         /* 0x44 断路和死区寄存器 */
    volatile uint32_t DCR;          /* 0x48 DMA控制寄存器 */
    volatile uint32_t DMAR;         /* 0x4C DMA连续请求寄存器 */
} BSP_TIM1_TypeDef;

/* ---------- TIM2/3/4/5通用定时器寄存器结构体 ---------- */
typedef struct {
    volatile uint32_t CR1;          /* 0x00 控制寄存器1 */
    volatile uint32_t CR2;          /* 0x04 控制寄存器2 */
    volatile uint32_t SMCR;         /* 0x08 从模式控制寄存器 */
    volatile uint32_t DIER;         /* 0x0C DMA/中断使能寄存器 */
    volatile uint32_t SR;           /* 0x10 状态寄存器 */
    volatile uint32_t EGR;          /* 0x14 事件生成寄存器 */
    volatile uint32_t CCMR1;        /* 0x18 捕获/比较模式寄存器1 */
    volatile uint32_t CCMR2;        /* 0x1C 捕获/比较模式寄存器2 */
    volatile uint32_t CCER;         /* 0x20 捕获/比较使能寄存器 */
    volatile uint32_t CNT;          /* 0x24 计数器 */
    volatile uint32_t PSC;          /* 0x28 预分频器 */
    volatile uint32_t ARR;          /* 0x2C 自动重装载寄存器 */
    volatile uint32_t RCR;          /* 0x30 重复计数寄存器 (TIM2有) */
    volatile uint32_t CCR1;         /* 0x34 捕获/比较寄存器1 */
    volatile uint32_t CCR2;         /* 0x38 捕获/比较寄存器2 */
    volatile uint32_t CCR3;         /* 0x3C 捕获/比较寄存器3 */
    volatile uint32_t CCR4;         /* 0x40 捕获/比较寄存器4 */
    volatile uint32_t DCR;          /* 0x44 DMA控制寄存器 */
    volatile uint32_t DMAR;         /* 0x48 DMA连续请求寄存器 */
} BSP_TIMX_TypeDef;

/* ---------- ADC寄存器结构体 ---------- */
typedef struct {
    volatile uint32_t SR;           /* 0x00 状态寄存器 */
    volatile uint32_t CR1;          /* 0x04 控制寄存器1 */
    volatile uint32_t CR2;          /* 0x08 控制寄存器2 */
    volatile uint32_t SMPR1;        /* 0x0C 采样时间寄存器1 */
    volatile uint32_t SMPR2;        /* 0x10 采样时间寄存器2 */
    volatile uint32_t JOFR1;        /* 0x14 注入通道数据偏移寄存器1 */
    volatile uint32_t JOFR2;        /* 0x18 注入通道数据偏移寄存器2 */
    volatile uint32_t JOFR3;        /* 0x1C 注入通道数据偏移寄存器3 */
    volatile uint32_t JOFR4;        /* 0x20 注入通道数据偏移寄存器4 */
    volatile uint32_t HTR;          /* 0x24 看门狗高阈值寄存器 */
    volatile uint32_t LTR;          /* 0x28 看门狗低阈值寄存器 */
    volatile uint32_t SQR1;         /* 0x2C 规则序列寄存器1 */
    volatile uint32_t SQR2;         /* 0x30 规则序列寄存器2 */
    volatile uint32_t SQR3;         /* 0x34 规则序列寄存器3 */
    volatile uint32_t JSQR;         /* 0x38 注入序列寄存器 */
    volatile uint32_t JDR1;         /* 0x3C 注入数据寄存器1 */
    volatile uint32_t JDR2;         /* 0x40 注入数据寄存器2 */
    volatile uint32_t JDR3;         /* 0x44 注入数据寄存器3 */
    volatile uint32_t JDR4;         /* 0x48 注入数据寄存器4 */
    volatile uint32_t DR;           /* 0x4C 规则数据寄存器 */
} BSP_ADC_TypeDef;

/* ---------- USART寄存器结构体 ---------- */
typedef struct {
    volatile uint32_t SR;           /* 0x00 状态寄存器 */
    volatile uint32_t DR;          /* 0x04 数据寄存器 */
    volatile uint32_t BRR;         /* 0x08 波特率寄存器 */
    volatile uint32_t CR1;         /* 0x0C 控制寄存器1 */
    volatile uint32_t CR2;         /* 0x10 控制寄存器2 */
    volatile uint32_t CR3;         /* 0x14 控制寄存器3 */
    volatile uint32_t GTPR;         /* 0x18 保护时间和预分频寄存器 */
} BSP_USART_TypeDef;

/* ---------- DMA寄存器结构体 ---------- */
typedef struct {
    volatile uint32_t CR;           /* 0x00 DMA_SxCR */
    volatile uint32_t NDTR;         /* 0x04 DMA_SxNDTR */
    volatile uint32_t PAR;          /* 0x08 DMA_SxPAR */
    volatile uint32_t M0AR;         /* 0x0C DMA_SxM0AR */
    volatile uint32_t M1AR;         /* 0x10 DMA_SxM1AR */
    volatile uint32_t FCR;          /* 0x14 DMA_SxFCR */
} BSP_DMA_Stream_TypeDef;

typedef struct {
    volatile uint32_t LISR;        /* 0x00 低中断状态寄存器 */
    volatile uint32_t HISR;         /* 0x04 高中断状态寄存器 */
    volatile uint32_t LIFCR;        /* 0x08 低中断标志清除寄存器 */
    volatile uint32_t HIFCR;         /* 0x0C 高中断标志清除寄存器 */
    BSP_DMA_Stream_TypeDef S[8];    /* 0x10-0x90 DMA Stream 0-7 */
} BSP_DMA_TypeDef;

/* ---------- EXTI寄存器结构体 ---------- */
typedef struct {
    volatile uint32_t IMR;          /* 0x00 中断屏蔽寄存器 */
    volatile uint32_t EMR;          /* 0x04 事件屏蔽寄存器 */
    volatile uint32_t RTSR;         /* 0x08 上升沿触发选择寄存器 */
    volatile uint32_t FTSR;         /* 0x0C 下降沿触发选择寄存器 */
    volatile uint32_t SWIER;        /* 0x10 软件中断事件寄存器 */
    volatile uint32_t PR;           /* 0x14 待挂起标志寄存器 */
} BSP_EXTI_TypeDef;

/* ---------- NVIC寄存器结构体 ---------- */
typedef struct {
    volatile uint32_t ISER[8];     /* 0x000-0x01F 设置使能寄存器 */
    uint32_t RESERVED0[24];
    volatile uint32_t ICER[8];     /* 0x080-0x09F 清除使能寄存器 */
    uint32_t RESERVED1[24];
    volatile uint32_t ISPR[8];     /* 0x100-0x11F 设置挂起寄存器 */
    uint32_t RESERVED2[24];
    volatile uint32_t ICPR[8];     /* 0x180-0x19F 清除挂起寄存器 */
    uint32_t RESERVED3[24];
    volatile uint32_t IABR[8];     /* 0x200-0x21F 激活位寄存器 */
    uint32_t RESERVED4[56];
    volatile uint8_t IP[240];      /* 0x300-0x3FF 中断优先级寄存器 */
} BSP_NVIC_TypeDef;

/* ============================================================
 * 第十七部分：外设声明（编译时替换为实际地址）
 * ============================================================ */
#define RCC                         ((BSP_RCC_TypeDef *) BSP_RCC_BASE)
#define GPIOA                       ((BSP_GPIO_TypeDef *) BSP_GPIOA_BASE)
#define GPIOB                       ((BSP_GPIO_TypeDef *) BSP_GPIOB_BASE)
#define GPIOC                       ((BSP_GPIO_TypeDef *) BSP_GPIOC_BASE)
#define GPIOD                       ((BSP_GPIO_TypeDef *) BSP_GPIOD_BASE)
#define GPIOE                       ((BSP_GPIO_TypeDef *) BSP_GPIOE_BASE)
#define TIM1                        ((BSP_TIM1_TypeDef *) BSP_TIM1_BASE)
#define TIM2                        ((BSP_TIMX_TypeDef *) 0x40000000UL)
#define TIM3                        ((BSP_TIMX_TypeDef *) BSP_TIM3_BASE)
#define TIM4                        ((BSP_TIMX_TypeDef *) BSP_TIM4_BASE)
#define TIM5                        ((BSP_TIMX_TypeDef *) BSP_TIM5_BASE)
#define ADC1                        ((BSP_ADC_TypeDef *) BSP_ADC1_BASE)
#define ADC2                        ((BSP_ADC_TypeDef *) BSP_ADC2_BASE)
#define USART1                      ((BSP_USART_TypeDef *) BSP_USART1_BASE)
#define USART2                      ((BSP_USART_TypeDef *) BSP_USART2_BASE)
#define USART3                      ((BSP_USART_TypeDef *) BSP_USART3_BASE)
#define DMA2                        ((BSP_DMA_TypeDef *) BSP_DMA2_BASE)
#define EXTI                        ((BSP_EXTI_TypeDef *) BSP_EXTI_BASE)
#define NVIC                        ((BSP_NVIC_TypeDef *) BSP_NVIC_BASE)

/* ============================================================
 * 第十八部分：RCC相关位定义
 * ============================================================ */
/* RCC->CR */
#define RCC_CR_HSION                (1 << 0)
#define RCC_CR_HSIRDY               (1 << 1)
#define RCC_CR_HSEON                (1 << 16)
#define RCC_CR_HSERDY               (1 << 17)
#define RCC_CR_PLLON                (1 << 24)
#define RCC_CR_PLLRDY               (1 << 25)

/* RCC->CFGR */
#define RCC_CFGR_SW                 0x03
#define RCC_CFGR_SW_HSI             0x00
#define RCC_CFGR_SW_HSE             0x01
#define RCC_CFGR_SW_PLL             0x02

/* RCC->APB2ENR */
#define RCC_APB2ENR_TIM1EN          (1 << 0)
#define RCC_APB2ENR_USART1EN        (1 << 4)
#define RCC_APB2ENR_ADC1EN          (1 << 8)
#define RCC_APB2ENR_ADC2EN          (1 << 9)
#define RCC_APB2ENR_ADC3EN          (1 << 10)

/* RCC->APB1ENR */
#define RCC_APB1ENR_TIM2EN          (1 << 0)
#define RCC_APB1ENR_TIM3EN          (1 << 1)
#define RCC_APB1ENR_TIM4EN          (1 << 2)
#define RCC_APB1ENR_TIM5EN          (1 << 3)
#define RCC_APB1ENR_TIM6EN          (1 << 4)
#define RCC_APB1ENR_TIM7EN          (1 << 5)
#define RCC_APB1ENR_USART2EN        (1 << 17)
#define RCC_APB1ENR_USART3EN        (1 << 18)

/* RCC->AHB1ENR */
#define RCC_AHB1ENR_GPIOAEN         (1 << 0)
#define RCC_AHB1ENR_GPIOBEN         (1 << 1)
#define RCC_AHB1ENR_GPIOCEN         (1 << 2)
#define RCC_AHB1ENR_GPIODEN         (1 << 3)
#define RCC_AHB1ENR_GPIOEEN         (1 << 4)
#define RCC_AHB1ENR_DMA1EN          (1 << 21)
#define RCC_AHB1ENR_DMA2EN          (1 << 22)

/* ============================================================
 * 第十九部分：TIM相关位定义
 * ============================================================ */
/* TIM_CR1 */
#define TIM_CR1_CEN                 (1 << 0)    /* 计数器使能 */
#define TIM_CR1_DIR                 (1 << 4)    /* 计数方向 (0=up, 1=down) */
#define TIM_CR1_CMS                 (0x03 << 5) /* 中心对齐模式 */
#define TIM_CR1_CMS_EDGE            0x00        /* 边沿对齐 */
#define TIM_CR1_CMS_MODE1           0x01        /* 中心对齐模式1: 下溢点更新 */
#define TIM_CR1_CMS_MODE2           0x02        /* 中心对齐模式2: 上溢点更新 */
#define TIM_CR1_CMS_MODE3           0x03        /* 中心对齐模式3: 上下溢都更新 */
#define TIM_CR1_ARPE                (1 << 7)    /* 自动重装载预装载使能 */

/* TIM_CR2 */
#define TIM_CR2_OIS1                (1 << 0)    /* 输出空闲状态1 */
#define TIM_CR2_OIS1N               (1 << 1)    /* 互补输出空闲状态1 */
#define TIM_CR2_OIS2                (1 << 2)
#define TIM_CR2_OIS2N               (1 << 3)
#define TIM_CR2_OIS3                (1 << 4)
#define TIM_CR2_OIS3N               (1 << 5)
#define TIM_CR2_MMS                 (0x07 << 4) /* 主模式选择 */
#define TIM_CR2_MMS_RESET           0x04        /* UR: 计数器复位 */

/* TIM_DIER */
#define TIM_DIER_UIE                (1 << 0)    /* 更新中断使能 */
#define TIM_DIER_UDE                (1 << 8)    /* 更新DMA请求使能 */
#define TIM_DIER_CC1IE              (1 << 1)    /* 捕获/比较1中断使能 */
#define TIM_DIER_CC2IE              (1 << 2)
#define TIM_DIER_CC3IE              (1 << 3)
#define TIM_DIER_CC4IE              (1 << 4)

/* TIM_SR */
#define TIM_SR_UIF                  (1 << 0)    /* 更新中断标志 */
#define TIM_SR_CC1IF                (1 << 1)
#define TIM_SR_CC2IF                (1 << 2)
#define TIM_SR_CC3IF                (1 << 3)
#define TIM_SR_CC4IF                (1 << 4)

/* TIM_EGR */
#define TIM_EGR_UG                  (1 << 0)    /* 产生更新事件 */

/* TIM_CCMR1 */
#define TIM_CCMR1_CC1S              (0x03 << 0) /* 捕获/比较1选择 */
#define TIM_CCMR1_CC1S_OUT          0x00        /* 输出模式 */
#define TIM_CCMR1_CC1S_IN_TI1       0x01        /* 输入: TI1 */
#define TIM_CCMR1_CC1S_IN_TI2       0x02        /* 输入: TI2 */
#define TIM_CCMR1_CC1S_IN_TRC       0x03        /* 输入: 编码器模式 */

#define TIM_CCMR1_OC1M              (0x07 << 4) /* 输出比较1模式 */
#define TIM_CCMR1_OC1M_PWM1         0x06        /* PWM模式1 */
#define TIM_CCMR1_OC1M_PWM2         0x07        /* PWM模式2 */

#define TIM_CCMR1_OC1PE              (1 << 3)    /* 输出比较1预装载使能 */
#define TIM_CCMR1_OC1FE              (1 << 2)    /* 输出比较1快速使能 */

/* TIM_CCMR2 */
#define TIM_CCMR2_CC3S              (0x03 << 0)
#define TIM_CCMR2_CC4S              (0x03 << 8)
#define TIM_CCMR2_OC3M              (0x07 << 4)
#define TIM_CCMR2_OC3PE              (1 << 3)
#define TIM_CCMR2_OC4M              (0x07 << 12)
#define TIM_CCMR2_OC4PE              (1 << 11)

/* TIM_CCER */
#define TIM_CCER_CC1E               (1 << 0)    /* 捕获/比较1输出使能 */
#define TIM_CCER_CC1P               (1 << 1)    /* 捕获/比较1输出极性 */
#define TIM_CCER_CC1NE              (1 << 2)    /* 捕获/比较1N输出使能 */
#define TIM_CCER_CC1NP              (1 << 3)    /* 捕获/比较1N输出极性 */
#define TIM_CCER_CC2E               (1 << 4)
#define TIM_CCER_CC2P               (1 << 5)
#define TIM_CCER_CC2NE              (1 << 6)
#define TIM_CCER_CC2NP              (1 << 7)
#define TIM_CCER_CC3E               (1 << 8)
#define TIM_CCER_CC3P               (1 << 9)
#define TIM_CCER_CC3NE              (1 << 10)
#define TIM_CCER_CC3NP              (1 << 11)
#define TIM_CCER_CC4E               (1 << 12)
#define TIM_CCER_CC4P               (1 << 13)
#define TIM_CCER_CC4NP              (1 << 15)

/* TIM_BDTR */
#define TIM_BDTR_MOE                (1 << 15)   /* 主输出使能 */
#define TIM_BDTR_AOE                (1 << 14)   /* 自动输出使能 */
#define TIM_BDTR_BKE                (1 << 12)   /* 刹车功能使能 */
#define TIM_BDTR_BKP                (1 << 13)   /* 刹车信号极性 */
#define TIM_BDTR_DTG                0xFF        /* 死区发生器 */
#define TIM_BDTR_DTG_Pos            0

/* TIM_SMCR */
#define TIM_SMCR_SMS                (0x07 << 0) /* 从模式选择 */
#define TIM_SMCR_SMS_DISABLED       0x00        /* 关闭从模式 */
#define TIM_SMCR_SMS_ENCODER_MODE1  0x01        /* 编码器模式1: TI1边沿计数 */
#define TIM_SMCR_SMS_ENCODER_MODE2  0x02        /* 编码器模式2: TI2边沿计数 */
#define TIM_SMCR_SMS_ENCODER_MODE3  0x03        /* 编码器模式3: TI1+TI2边沿计数 */
#define TIM_SMCR_TS                 (0x07 << 4) /* 触发选择 */

/* ============================================================
 * 第二十部分：ADC相关位定义
 * ============================================================ */
/* ADC_CR1 */
#define ADC_CR1_RES                 (0x03 << 24) /* 分辨率 */
#define ADC_CR1_RES_12BIT           0x00        /* 12位分辨率 */
#define ADC_CR1_RES_10BIT           0x01        /* 10位分辨率 */
#define ADC_CR1_RES_8BIT            0x02        /* 8位分辨率 */
#define ADC_CR1_RES_6BIT            0x03        /* 6位分辨率 */
#define ADC_CR1_SCAN                (1 << 8)    /* 扫描模式使能 */
#define ADC_CR1_EOSIE               (1 << 5)    /* 序列结束中断使能 */
#define ADC_CR1_AWDEN               (1 << 23)   /* 模拟看门狗使能 */

/* ADC_CR2 */
#define ADC_CR2_ADON                (1 << 0)    /* ADC使能 */
#define ADC_CR2_CONT                (1 << 1)    /* 连续转换模式 */
#define ADC_CR2_DMA                 (1 << 8)    /* DMA模式使能 */
#define ADC_CR2_DDS                 (1 << 9)    /* DMA禁用选择 */
#define ADC_CR2_EOCS                (1 << 10)   /* 序列结束选择 */
#define ADC_CR2_SWSTART             (1 << 30)   /* 开始转换规则通道 */

/* ADC_SQR1 */
#define ADC_SQR1_L                  (0x1F << 20) /* 规则序列长度 */
#define ADC_SQR1_SQ13               (0x1F << 0)  /* 规则序列13 */
#define ADC_SQR1_SQ14               (0x1F << 5)  /* 规则序列14 */
#define ADC_SQR1_SQ15               (0x1F << 10) /* 规则序列15 */
#define ADC_SQR1_SQ16               (0x1F << 15) /* 规则序列16 */

/* ADC_SQR2 */
#define ADC_SQR2_SQ7                (0x1F << 0)
#define ADC_SQR2_SQ8                (0x1F << 5)
#define ADC_SQR2_SQ9                (0x1F << 10)
#define ADC_SQR2_SQ10               (0x1F << 15)
#define ADC_SQR2_SQ11               (0x1F << 20)
#define ADC_SQR2_SQ12               (0x1F << 25)

/* ADC_SQR3 */
#define ADC_SQR3_SQ1                (0x1F << 0)  /* 规则序列1 */
#define ADC_SQR3_SQ2                (0x1F << 5)  /* 规则序列2 */
#define ADC_SQR3_SQ3                (0x1F << 10) /* 规则序列3 */
#define ADC_SQR3_SQ4                (0x1F << 15) /* 规则序列4 */
#define ADC_SQR3_SQ5                (0x1F << 20) /* 规则序列5 */
#define ADC_SQR3_SQ6                (0x1F << 25) /* 规则序列6 */

/* ADC_SMPR1 */
#define ADC_SMPR1_SMP18             (0x07 << 24) /* 通道18采样时间 */
#define ADC_SMPR1_SMP17             (0x07 << 21)
#define ADC_SMPR1_SMP16             (0x07 << 18)
#define ADC_SMPR1_SMP0               (0x07 << 0)

/* ADC_SMPR2 */
#define ADC_SMPR2_SMP9              (0x07 << 27)
#define ADC_SMPR2_SMP8              (0x07 << 24)

/* ADC_DR */
#define ADC_DR_ADC2DATA             (0xFFFF << 16) /* ADC2数据 (双ADC模式) */
#define ADC_DR_DATA                 0xFFFF        /* ADC1数据 */

/* ============================================================
 * 第二十一部分：USART相关位定义
 * ============================================================ */
/* USART_SR */
#define USART_SR_RXNE               (1 << 5)    /* 接收数据寄存器非空 */
#define USART_SR_TXE                (1 << 7)    /* 发送数据寄存器空 */
#define USART_SR_TC                 (1 << 6)    /* 传输完成 */
#define USART_SR_ORE                (1 << 3)    /* 过速错误 */
#define USART_SR_NE                 (1 << 2)    /* 噪声错误 */
#define USART_SR_FE                 (1 << 1)    /* 帧错误 */
#define USART_SR_PE                 (1 << 0)    /* 奇偶校验错误 */

/* USART_CR1 */
#define USART_CR1_UE                (1 << 13)   /* USART使能 */
#define USART_CR1_RE                (1 << 2)    /* 接收使能 */
#define USART_CR1_TE                (1 << 3)    /* 发送使能 */
#define USART_CR1_RXNEIE            (1 << 5)    /* RXNE中断使能 */
#define USART_CR1_TCIE              (1 << 6)    /* TC中断使能 */
#define USART_CR1_PCE                (1 << 10)   /* 奇偶校验控制使能 */
#define USART_CR1_PS                (1 << 9)    /* 奇偶校验选择 (0=偶,1=奇) */
#define USART_CR1_M                 (1 << 12)   /* 字长 (0=8位,1=9位) */

/* USART_CR2 */
#define USART_CR2_STOP              (0x03 << 12) /* 停止位 */
#define USART_CR2_STOP_1BIT         0x00        /* 1位停止位 */
#define USART_CR2_STOP_2BIT         0x02        /* 2位停止位 */

/* USART_CR3 */
#define USART_CR3_RTSE              (1 << 8)    /* RTS使能 */
#define USART_CR3_CTSE              (1 << 9)    /* CTS使能 */

/* ============================================================
 * 第二十二部分：DMA相关位定义
 * ============================================================ */
/* DMA_SxCR */
#define DMA_SxCR_EN                 (1 << 0)    /* 流使能 */
#define DMA_SxCR_DMEIE              (1 << 1)    /* 直接模式错误中断使能 */
#define DMA_SxCR_TEIE               (1 << 2)    /* 传输错误中断使能 */
#define DMA_SxCR_HTIE               (1 << 3)    /* 半传输中断使能 */
#define DMA_SxCR_TCIE               (1 << 4)    /* 传输完成中断使能 */
#define DMA_SxCR_PFCTRL             (1 << 5)    /* 外设流控制 */
#define DMA_SxCR_DIR                (0x03 << 6) /* 传输方向 */
#define DMA_SxCR_DIR_PER_TO_MEM     0x00        /* 外设到内存 */
#define DMA_SxCR_DIR_MEM_TO_PER     0x01        /* 内存到外设 */
#define DMA_SxCR_DIR_MEM_TO_MEM     0x02        /* 内存到内存 */
#define DMA_SxCR_CIRC               (1 << 8)    /* 循环模式使能 */
#define DMA_SxCR_PINC               (1 << 9)    /* 外设增量模式使能 */
#define DMA_SxCR_MINC               (1 << 10)   /* 内存增量模式使能 */
#define DMA_SxCR_PSIZE              (0x03 << 11) /* 外设数据大小 */
#define DMA_SxCR_PSIZE_8BIT         0x00
#define DMA_SxCR_PSIZE_16BIT        0x01
#define DMA_SxCR_PSIZE_32BIT        0x02
#define DMA_SxCR_MSIZE              (0x03 << 13) /* 内存数据大小 */
#define DMA_SxCR_MSIZE_8BIT         0x00
#define DMA_SxCR_MSIZE_16BIT        0x01
#define DMA_SxCR_MSIZE_32BIT        0x02
#define DMA_SxCR_PBURST             (0x03 << 16) /* 外设突发传输 */
#define DMA_SxCR_MBURST             (0x03 << 18) /* 内存突发传输 */
#define DMA_SxCR_CHSEL              (0x07 << 25) /* 通道选择 */

/* DMA_SxFCR */
#define DMA_SxFCR_FEIE              (1 << 7)    /* FIFO错误中断使能 */
#define DMA_SxFCR_FS                (0x07 << 3) /* FIFO状态 */
#define DMA_SxFCR_DMDIS            (1 << 2)    /* 直接模式禁用 */

/* DMA_LISR / DMA_HISR */
#define DMA_LISR_FEIF0              (1 << 0)
#define DMA_LISR_DMEIF0            (1 << 2)
#define DMA_LISR_TEIF0             (1 << 3)
#define DMA_LISR_HTIF0             (1 << 4)
#define DMA_LISR_TCIF0             (1 << 5)
#define DMA_LISR_FEIF1              (1 << 6)
#define DMA_LISR_DMEIF1            (1 << 8)
#define DMA_LISR_TEIF1             (1 << 9)
#define DMA_LISR_HTIF1             (1 << 10)
#define DMA_LISR_TCIF1             (1 << 11)

/* ============================================================
 * 第二十三部分：EXTI相关位定义
 * ============================================================ */
/* EXTI_IMR */
#define EXTI_IMR_MR0                (1 << 0)    /* 中断线0屏蔽 */
#define EXTI_IMR_MR1                (1 << 1)
#define EXTI_IMR_MR2                (1 << 2)
#define EXTI_IMR_MR3                (1 << 3)

/* EXTI_RTSR */
#define EXTI_RTSR_TR0               (1 << 0)    /* 上升沿触发使能 */
#define EXTI_RTSR_TR1               (1 << 1)
#define EXTI_RTSR_TR2               (1 << 2)
#define EXTI_RTSR_TR3               (1 << 3)

/* EXTI_FTSR */
#define EXTI_FTSR_TR0               (1 << 0)    /* 下降沿触发使能 */
#define EXTI_FTSR_TR1               (1 << 1)
#define EXTI_FTSR_TR2               (1 << 2)
#define EXTI_FTSR_TR3               (1 << 3)

/* EXTI_PR */
#define EXTI_PR_PR0                 (1 << 0)    /* 挂起标志位 */
#define EXTI_PR_PR1                 (1 << 1)
#define EXTI_PR_PR2                 (1 << 2)
#define EXTI_PR_PR3                 (1 << 3)

/* ============================================================
 * 第二十四部分：SysTick相关
 * ============================================================ */
/** @brief SysTick寄存器基地址 (SCB) */
#define BSP_SYSTICK_BASE            0xE000E010UL

typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} BSP_SysTick_TypeDef;

#define SysTick                     ((BSP_SysTick_TypeDef *) BSP_SYSTICK_BASE)

/* SysTick CTRL */
#define SysTick_CTRL_COUNTFLAG      (1 << 16)
#define SysTick_CTRL_CLKSOURCE      (1 << 2)
#define SysTick_CTRL_TICKINT        (1 << 1)
#define SysTick_CTRL_ENABLE         (1 << 0)

/* ============================================================
 * 第二十五部分：杂项定义
 * ============================================================ */
/** @brief 启动方式枚举 */
typedef enum {
    BSP_BOOT_NORMAL = 0,           /* 正常启动 */
    BSP_BOOT_AFTER_FAULT = 1,      /* 故障后重启 */
    BSP_BOOT_IAP = 2               /* IAP升级后重启 */
} BSP_BootMode_t;

/** @brief MCU唯一ID地址 (96位) */
#define BSP_UID_BASE                0x1FFF7A10UL

/** @brief Flash容量寄存器地址 */
#define BSP_FLASH_SIZE_BASE         0x1FFF7A22UL

/** @brief 获取MCU UID (3个字) */
#define BSP_GET_UID(n)              (*(volatile uint32_t *)(BSP_UID_BASE + (n) * 4))

/** @brief 获取Flash大小 (KB) */
#define BSP_GET_FLASH_SIZE_KB       (*(volatile uint16_t *)BSP_FLASH_SIZE_BASE)

#endif /* MCU_STM32F4 / STM32F4 */

/* ============================================================
 * 第二十六部分：其他平台条件编译（预留）
 * ============================================================ */
#if defined(MCU_DSP28335)
    /* DSP28335 硬件定义预留区域 */
    /* 后续扩展 */
#endif

#if defined(MCU_STM32F7)
    /* STM32F7 硬件定义预留区域 */
    /* 后续扩展 */
#endif

/* ============================================================
 * 文件结束
 * ============================================================ */
#endif /* __BSP_HW_CONFIG_H */

/**
 * @}
 */
