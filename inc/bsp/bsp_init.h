/**
 ******************************************************************************
 * @file    bsp_init.h
 * @brief   BSP层公共接口声明 - 三相逆变器FOC项目
 * @version V1.0.0
 * @date    2024-01-01
 * @note    BSP层初始化、时钟、GPIO、ADC、PWM、UART等外设初始化接口
 *          以及BSP层公共函数声明
 ******************************************************************************
 */

#ifndef __BSP_INIT_H
#define __BSP_INIT_H

/* ============================================================
 * 第一部分：包含头文件
 * ============================================================ */
#include "global_def.h"
#include "global_ctx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * 第二部分：BSP层函数声明
 * ============================================================ */

/**
 * @brief   BSP层初始化总入口
 * @note    按顺序调用各个子模块初始化函数:
 *          1. 系统时钟配置 (PLL 168MHz)
 *          2. GPIO初始化
 *          3. ADC初始化
 *          4. PWM初始化
 *          5. 编码器接口初始化
 *          6. UART初始化
 *          7. NVIC中断优先级配置
 *          8. 看门狗初始化
 * @retval  None
 */
void BSP_Init(void);

/**
 * @brief   系统时钟配置函数
 * @note    配置PLL: HSE=8MHz -> SYSCLK=168MHz
 *          AHB = 168MHz, APB1 = 42MHz, APB2 = 84MHz
 * @retval  None
 */
void BSP_SystemClock_Config(void);

/**
 * @brief   GPIO初始化函数
 * @note    初始化所有GPIO端口:
 *          - GPIOA: PWM输出(PA7/8/9/10)、调试串口(PA2/3)、编码器(PA6/7)
 *          - GPIOB: PWM下桥(PB0/1)、编码器(PB6/7/8)
 *          - GPIOC: 数字输入(PC0~7)、数字输出(PC8~15)
 *          - GPIOD: RS485方向控制(PD12)、LED(PD0/1/2)
 * @retval  None
 */
void BSP_GPIO_Init(void);

/**
 * @brief   ADC初始化函数
 * @note    配置ADC1多通道扫描模式，用于采样:
 *          - 通道0(PA0): Ia 电流
 *          - 通道1(PA1): Ib 电流
 *          - 通道2(PA2): 直流母线电压 Udc
 *          - 通道3(PA3): NTC 温度
 *          使用DMA循环模式，12位分辨率，采样时间15周期
 * @retval  None
 */
void BSP_ADC_Init(void);

/**
 * @brief   PWM初始化函数
 * @note    配置TIM1为三相PWM输出 (高级定时器):
 *          - 中心对齐模式1 (对称PWM，减少谐波)
 *          - 互补输出，带死区插入 (防止上下桥臂直通)
 *          - 频率由参数freq_hz指定，默认10kHz
 *          - 占空比初始为0 (安全停机状态)
 * @param   freq_hz: PWM载波频率，单位Hz
 * @retval  None
 */
void BSP_PWM_Init(uint32_t freq_hz);

/**
 * @brief   编码器接口初始化函数
 * @note    配置TIM4为ABZ编码器接口:
 *          - TI1FP1 = A相 (PB6)
 *          - TI2FP2 = B相 (PB7)
 *          - TI3FP3 = Z相 (PB8, 零位脉冲)
 *          - 编码器模式3 (TI1和TI2边沿都计数，4倍频)
 *          - 支持Z相零位脉冲中断
 * @retval  None
 */
void BSP_TIM_Encoder_Init(void);

/**
 * @brief   UART初始化函数
 * @note    配置USART1用于Modbus RTU通信:
 *          - 波特率可配置 (默认115200)
 *          - 8位数据位，无奇偶校验，1位停止位 (8N1)
 *          - 接收/发送中断使能
 *          - RS485模式: PD12作为DE方向控制
 * @param   baud: 波特率，如115200UL
 * @retval  None
 */
void BSP_UART_Init(uint32_t baud);

/**
 * @brief   NVIC中断优先级配置函数
 * @note    配置各外设中断优先级:
 *          - TIM1_UP (PWM周期中断): 优先级1 (最高)
 *          - ADC (采样完成中断): 优先级2
 *          - USART1 (Modbus接收): 优先级5
 *          - EXTI (外部中断): 优先级6
 * @retval  None
 */
void BSP_NVIC_Config(void);

/**
 * @brief   PWM故障安全输出控制
 * @note    当检测到严重故障时，强制关闭所有PWM输出
 *          将TIM1所有通道设置为复位状态 (低电平)
 *          这是最后一道安全防线
 * @param   enable: 0=关闭PWM输出(故障安全), 1=使能PWM输出
 * @retval  None
 */
void BSP_FAULT_PWM_Output(uint8_t enable);

/**
 * @brief   GPIO引脚写操作
 * @note    封装GPIO写操作，支持多引脚同时写入
 * @param   port: GPIO端口基地址
 * @param   pin:  GPIO引脚号 (0-15)
 * @param   state: 引脚状态 (0=低, 1=高)
 * @retval  None
 */
void BSP_GPIO_WritePin(BSP_GPIO_TypeDef *port, uint16_t pin, uint8_t state);

/**
 * @brief   GPIO引脚读操作
 * @note    读取指定GPIO引脚的电平状态
 * @param   port: GPIO端口基地址
 * @param   pin:  GPIO引脚号 (0-15)
 * @retval  引脚状态 (0=低, 1=高)
 */
uint8_t BSP_GPIO_ReadPin(BSP_GPIO_TypeDef *port, uint16_t pin);

/**
 * @brief   GPIO端口批量写操作
 * @note    一次性写入整个端口的16位数据
 * @param   port:  GPIO端口基地址
 * @param   value: 写入的16位数据
 * @retval  None
 */
void BSP_GPIO_WritePort(BSP_GPIO_TypeDef *port, uint16_t value);

/**
 * @brief   GPIO端口读操作
 * @note    读取整个GPIO端口的16位输入数据
 * @param   port: GPIO端口基地址
 * @retval  端口输入数据 (16位)
 */
uint16_t BSP_GPIO_ReadPort(BSP_GPIO_TypeDef *port);

/**
 * @brief   获取系统主频
 * @note    返回当前系统时钟频率，单位Hz
 *          在PLL配置好后调用，返回168000000
 * @retval  系统时钟频率 (Hz)
 */
uint32_t BSP_GetSystemClock(void);

/**
 * @brief   微秒延时函数
 * @note    基于SysTick的微秒级延时
 *          精度不如定时器，但实现简单
 * @param   us: 延时微秒数
 * @retval  None
 */
void BSP_Delay_US(uint32_t us);

/**
 * @brief   毫秒延时函数
 * @note    基于SysTick的毫秒级延时
 * @param   ms: 延时毫秒数
 * @retval  None
 */
void BSP_Delay_MS(uint32_t ms);

/**
 * @brief   获取当前SysTick值
 * @note    返回当前SysTick计数器的值
 *          可用于测量时间间隔
 * @retval  SysTick当前计数值
 */
uint32_t BSP_GetSysTick(void);

/**
 * @brief   独立看门狗初始化
 * @note    初始化IWDG，设置超时时间
 *          超时后会自动复位系统
 * @param   timeout_ms: 看门狗超时时间，单位ms
 * @retval  None
 */
void BSP_IWDG_Init(uint32_t timeout_ms);

/**
 * @brief   独立看门狗喂狗
 * @note    周期性调用，防止系统复位
 *          建议在主循环或空闲任务中调用
 * @retval  None
 */
void BSP_IWDG_Feed(void);

/**
 * @brief   PWM占空比设置函数
 * @note    设置三相PWM的占空比
 *          值为0-65535，对应0%-100%
 *          中心对齐模式下，CCR值决定PWM占空比
 * @param   uh/ul: U相上下桥臂占空比 (0-65535)
 * @param   vh/vl: V相上下桥臂占空比 (0-65535)
 * @param   wh/wl: W相上下桥臂占空比 (0-65535)
 * @retval  None
 */
void BSP_PWM_SetDuty(uint16_t uh, uint16_t ul,
                     uint16_t vh, uint16_t vl,
                     uint16_t wh, uint16_t wl);

/**
 * @brief   获取ADC采样值
 * @note    获取指定通道的ADC原始采样值
 *          原始值为0-4095 (12位ADC)
 * @param   channel: ADC通道号 (0-3对应Ia/Ib/Udc/NTC)
 * @retval  ADC原始采样值 (0-4095)
 */
uint16_t BSP_ADC_GetValue(uint8_t channel);

/**
 * @brief   获取编码器计数值
 * @note    返回TIM4编码器接口的当前计数值
 *          可正可负，表示相对位置
 * @retval  编码器计数值 (16位有符号)
 */
int16_t BSP_Encoder_GetCount(void);

/**
 * @brief   获取编码器速度 (RPM)
 * @note    根据单位时间内的计数差计算电机转速
 *          需要知道编码器线数和电机极对数
 * @retval  电机转速 (RPM)
 */
float BSP_Encoder_GetSpeed_RPM(void);

/**
 * @brief   复位编码器计数器
 * @note    将TIM4编码器计数清零
 *          通常在找到Z相零位脉冲后调用
 * @retval  None
 */
void BSP_Encoder_Reset(void);

/**
 * @brief   软复位系统
 * @note    触发MCU软件复位
 *          相当于按复位键
 * @retval  None
 */
void BSP_SystemReset(void);

/**
 * @brief   获取MCU芯片唯一ID
 * @note    读取96位的MCU唯一标识符
 *          可用于产品序列号或加密
 * @param   idx: 0/1/2 (取96位ID的任意32位)
 * @retval  UID的32位字
 */
uint32_t BSP_GetUID(uint8_t idx);

/**
 * @brief   获取芯片Flash大小
 * @note    返回芯片Flash存储空间大小
 * @retval  Flash大小 (KB)
 */
uint16_t BSP_GetFlashSize(void);

/**
 * @brief   使能外设时钟
 * @note    统一的外设时钟使能接口
 *          支持AHB1/APB1/APB2上的各种外设
 * @param   periph: 外设类型 (见BSP_Periph_t)
 * @retval  None
 */
typedef enum {
    BSP_PERIPH_GPIOA = 0,
    BSP_PERIPH_GPIOB = 1,
    BSP_PERIPH_GPIOC = 2,
    BSP_PERIPH_GPIOD = 3,
    BSP_PERIPH_GPIOE = 4,
    BSP_PERIPH_TIM1  = 10,
    BSP_PERIPH_TIM2  = 11,
    BSP_PERIPH_TIM3  = 12,
    BSP_PERIPH_TIM4  = 13,
    BSP_PERIPH_TIM5  = 14,
    BSP_PERIPH_ADC1  = 20,
    BSP_PERIPH_ADC2  = 21,
    BSP_PERIPH_USART1 = 30,
    BSP_PERIPH_USART2 = 31,
    BSP_PERIPH_USART3 = 32,
    BSP_PERIPH_DMA1  = 40,
    BSP_PERIPH_DMA2  = 41,
    BSP_PERIPH_RCC   = 50
} BSP_Periph_t;

void BSP_EnablePeriphClock(BSP_Periph_t periph);

/**
 * @brief   禁用外设时钟
 * @note    关闭指定外设的时钟，用于省电
 * @param   periph: 外设类型
 * @retval  None
 */
void BSP_DisablePeriphClock(BSP_Periph_t periph);

/* ============================================================
 * 第三部分：BSP层调试与诊断函数
 * ============================================================ */

/**
 * @brief   BSP层自检函数
 * @note    在系统启动时检测关键外设是否正常
 *          检测GPIO、ADC、PWM等硬件状态
 * @retval  0=自检通过, 负值=自检失败
 */
int BSP_SelfTest(void);

/**
 * @brief   打印BSP层版本信息
 * @note    通过调试串口输出BSP版本和编译日期
 * @retval  None
 */
void BSP_PrintVersion(void);

/**
 * @brief   打印引脚配置信息
 * @note    调试用，输出所有GPIO引脚的当前配置
 * @retval  None
 */
void BSP_PrintPinConfig(void);

/* ============================================================
 * 第四部分：DMA缓冲区定义（供外部引用）
 * ============================================================ */

/**
 * @brief   ADC DMA循环缓冲区
 * @note    存放ADC DMA传输的数据
 *          大小为ADC_DMA_BUF_SIZE * ADC通道数
 *          每通道数据依次存放: [ch0, ch1, ch2, ch3, ch0, ch1, ...]
 */
extern uint16_t bsp_adc_dma_buf[];

/**
 * @brief   ADC DMA缓冲区大小
 * @note    定义在bsp_init.c中
 */
extern uint32_t bsp_adc_dma_buf_size;

/**
 * @brief   校准后的ADC零点偏移量
 * @note    用于消除电流传感器的零点偏置
 */
extern int16_t bsp_adc_offset_ia;
extern int16_t bsp_adc_offset_ib;
extern int16_t bsp_adc_offset_vdc;

/* ============================================================
 * 第五部分：函数属性声明
 * ============================================================ */

/** @brief 标记函数为弱符号 (可被用户覆盖) */
#define BSP_WEAK    __attribute__((weak))

/** @brief 标记函数为必须内联 */
#define BSP_INLINE  __attribute__((always_inline))

/** @brief 标记函数为IRET域 (中断处理函数) */
#define BSP_IRT     __attribute__((interrupt))

#ifdef __cplusplus
}
#endif

/* ============================================================
 * 文件结束
 * ============================================================ */
#endif /* __BSP_INIT_H */

/**
 * @}
 */
