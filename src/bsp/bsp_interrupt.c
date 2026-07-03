/**
 ******************************************************************************
 * @file    bsp_interrupt.c
 * @brief   BSP层中断服务程序 - 三相逆变器FOC项目
 * @version V1.0.0
 * @date    2024-01-01
 * @note    STM32F4系列中断处理函数
 *          包含:
 *          - TIM1_UP_IRQHandler: PWM周期中断 (主控制循环)
 *          - ADC_IRQHandler: ADC采样完成中断 (备用方案)
 *          - USART1_IRQHandler: USART1接收中断 (Modbus通信)
 *          - EXTI0-3_IRQHandler: 外部中断 (数字输入/故障检测)
 *
 * 使用说明:
 * - TIM1_UP在每个PWM周期触发 (10kHz)，执行电流环/速度环控制
 * - USART1中断用于Modbus RTU协议栈的字节接收
 * - EXTI中断用于数字输入的去抖检测和外部故障快速响应
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
 * 第二部分：外部函数声明（来自MC层和DRV层）
 * ============================================================ */

/**
 * @brief 外部函数声明 - 来自MC_FOC.c或MC_VF.c
 * @note  这些函数由具体控制算法实现，BSP层负责调用
 */

/**
 * @brief   电流环控制函数 (FOC模式)
 * @note    在TIM1_UP_IRQHandler中被调用
 *          包含: Clarke变换, Park变换, PI调节, SVPWM, 反Park变换
 * @retval  None
 */
extern void MC_FOC_CurrentLoop(void);

/**
 * @brief   V/F控制函数 (V/F模式)
 * @note    在TIM1_UP_IRQHandler中被调用 (当run_mode=VF模式时)
 *          包含: V/F曲线计算, 空间矢量调制, 输出
 * @retval  None
 */
extern void MC_VF_Control(void);

/**
 * @brief   速度环控制函数
 * @note    在速度环周期内被调用 (如10ms或1ms)
 *          计算q轴电流给定值
 * @retval  None
 */
extern void MC_SpeedLoop(void);

/**
 * @brief 外部函数声明 - 来自DRV层
 */

/**
 * @brief   Modbus接收中断处理函数
 * @note    在USART1_IRQHandler中被调用
 *          处理Modbus RTU字节接收，更新接收状态机
 * @param   byte: 接收到的字节
 * @retval  None
 */
extern void Modbus_ReceiveISR(uint8_t byte);

/**
 * @brief   Modbus发送完成回调
 * @note    USART1 TXE中断中调用，用于RS485方向控制
 * @retval  None
 */
extern void Modbus_TxComplete(void);

/**
 * @brief 外部函数声明 - 来自PROT层（保护模块）
 */

/**
 * @brief   保护检查函数
 * @note    在每次主循环中调用，检查所有保护条件
 *          如果触发保护，设置故障标志并停止PWM
 * @retval  None
 */
extern void PROT_Check(void);

/**
 * @brief   过流保护快速响应
 * @note    在ADC采样完成后立即调用（硬件过流时）
 *          比主循环更快速响应短路故障
 * @param   iu: U相电流 (A)
 * @param   iv: V相电流 (A)
 * @retval  0=正常, 1=故障触发
 */
extern uint8_t PROT_FastOverCurrent(uint16_t iu, uint16_t iv);

/* ============================================================
 * 第三部分：全局变量（中断与主程序共享）
 * ============================================================ */

/**
 * @brief   主循环运行标志
 * @note    1=主循环正在运行, 0=主循环停止
 *          用于协调启动/停止序列
 */
volatile uint8_t g_MainLoopRunning = 0;

/**
 * @brief   当前运行模式
 * @note    0=停止, 1=FOC, 2=V/F等
 *          由MC层设置，BSP层可读取
 */
volatile uint8_t g_CurrentRunMode = 0;

/**
 * @brief   PWM周期计数器
 * @note    用于分频: g_PwmCounter % N 可实现N分频
 *          如: %1000 = 10Hz (10kHz PWM时)
 */
volatile uint32_t g_PwmCounter = 0;

/**
 * @brief   速度环周期计数器
 * @note    用于速度环分频
 *          10kHz / 1000 = 10Hz (100ms周期)
 */
volatile uint32_t g_SpeedLoopCounter = 0;

/**
 * @brief   编码器Z相脉冲计数
 * @note    在Z相中断中递增
 *          用于计算电机转过的圈数
 */
volatile uint32_t g_EncoderZPulseCount = 0;

/**
 * @brief   串口接收字节计数
 * @note    用于调试和通信统计
 */
volatile uint32_t g_UartRxCount = 0;

/**
 * @brief   上一次ADC采样时间戳
 * @note    用于计算采样周期
 */
volatile uint32_t g_LastAdcTick = 0;

/**
 * @brief   电流采样完成标志
 * @note    ADC DMA传输完成后置1，主循环读取后清0
 */
volatile uint8_t g_AdcConversionDone = 0;

/**
 * @brief   看门狗计数器
 * @note    每次主循环递增，达到阈值后喂狗
 */
volatile uint32_t g_IwdgCounter = 0;

/**
 * @brief   系统运行时间 (秒)
 * @note    每秒递增，用于运行时间统计
 */
volatile uint32_t g_RunTimeSecond = 0;
volatile uint32_t g_LastRunTimeTick = 0;

/* ============================================================
 * 第四部分：TIM1_UP_IRQHandler - PWM周期中断 (主控制循环)
 * ============================================================ */

/**
 * @brief   TIM1更新中断处理函数
 * @brief   这是整个FOC控制的核心中断，10kHz频率触发
 *
 * @note    中断每10kHz触发一次，执行:
 *          1. 清除更新标志
 *          2. 读取ADC采样值 (Ia, Ib, Udc, NTC)
 *          3. 调用FOC电流环或V/F控制
 *          4. 保护检查
 *          5. 分频计数 (速度环10Hz)
 *
 *          注意: 此函数执行时间应 < 50us (10kHz周期的50%)
 *          所有复杂计算应放在主循环中执行
 */
void TIM1_UP_IRQHandler(void)
{
    /* 清除TIM1更新中断标志 */
    TIM1->SR = (uint16_t)(~TIM_SR_UIF);

    /* ===== 读取ADC采样值 ===== */
    /* DMA缓冲区: [Ia, Ib, Udc, NTC, Ia, Ib, ...] */
    uint16_t adc_ia = bsp_adc_dma_buf[0];  /* U相电流 */
    uint16_t adc_ib = bsp_adc_dma_buf[1];  /* V相电流 */
    uint16_t adc_udc = bsp_adc_dma_buf[2]; /* 母线电压 */
    uint16_t adc_ntc = bsp_adc_dma_buf[3]; /* 温度 */

    /* ===== 快速过流检查 (硬件保护) ===== */
    /* 如果过流，跳过控制计算，直接进入故障处理 */
    /* 这里仅做快速检查，真正的过流保护由PROT模块执行 */
    if (PROT_FastOverCurrent(adc_ia, adc_ib) == 1) {
        /* 严重过流: 关闭PWM输出 */
        BSP_FAULT_PWM_Output(0);
        return;
    }

    /* ===== 保护检查 ===== */
    /* 检查过压/欠压/过温等条件 */
    PROT_Check();

    /* ===== 执行控制算法 ===== */
    if (g_MainLoopRunning) {
        /* 根据运行模式选择控制算法 */
        if (g_CurrentRunMode == RUN_MODE_FOC) {
            /* FOC电流环控制 */
            /* 10kHz周期，由具体FOC算法实现 */
            MC_FOC_CurrentLoop();
        } else if (g_CurrentRunMode == RUN_MODE_VF_OPEN_LOOP ||
                   g_CurrentRunMode == RUN_MODE_VF_CLOSED_LOOP) {
            /* V/F控制 */
            MC_VF_Control();
        }
        /* 其他模式可继续添加 */
    } else {
        /* 停机状态: PWM输出安全状态 (CCR=0) */
        TIM1->CCR1 = 0;
        TIM1->CCR2 = 0;
        TIM1->CCR3 = 0;
    }

    /* ===== 分频计数器 ===== */
    g_PwmCounter++;

    /* 速度环分频: 10kHz / 1000 = 10Hz (100ms周期) */
    g_SpeedLoopCounter++;
    if (g_SpeedLoopCounter >= 1000) {
        g_SpeedLoopCounter = 0;
        /* 速度环在主循环中处理，此处仅设置标志 */
    }

    /* 1秒定时器 */
    g_LastAdcTick++;
    if (g_LastAdcTick >= 10000) {  /* 10kHz * 1s = 10000 */
        g_LastAdcTick = 0;
        g_RunTimeSecond++;
    }

    /* ===== 看门狗喂狗 ===== */
    g_IwdgCounter++;
    if (g_IwdgCounter >= 100) {  /* 每100个PWM周期喂一次 */
        g_IwdgCounter = 0;
        BSP_IWDG_Feed();
    }
}

/* ============================================================
 * 第五部分：ADC_IRQHandler - ADC中断处理
 * ============================================================ */

/**
 * @brief   ADC中断处理函数
 * @note    当不使用DMA时使用此中断
 *          读取ADC转换结果，存储到全局变量
 *          然后调用保护检查
 *
 *          注意: 本项目默认使用DMA模式，此中断可能不启用
 */
void ADC_IRQHandler(void)
{
    /* 检查EOC标志 */
    if (ADC1->SR & (1 << 1)) {  /* EOC: 转换完成 */
        /* 读取ADC数据 */
        uint16_t adc_value = (uint16_t)(ADC1->DR & 0xFFFF);

        /* 更新到全局变量 (如果需要) */
        /* 实际使用DMA时，数据自动传输到bsp_adc_dma_buf */

        /* 清除EOC标志 */
        ADC1->SR = (uint16_t)(~(1 << 1));
    }

    /* 检查模拟看门狗标志 (AWD) */
    if (ADC1->SR & (1 << 0)) {  /* AWD: 模拟看门狗 */
        /* 采样值超出设定范围，触发报警 */
        /* 可在这里添加额外的保护逻辑 */

        /* 清除AWD标志 */
        ADC1->SR = (uint16_t)(~(1 << 0));
    }
}

/* ============================================================
 * 第六部分：USART1_IRQHandler - USART1接收中断
 * ============================================================ */

/**
 * @brief   USART1中断处理函数
 * @note    处理Modbus RTU接收和发送完成中断:
 *          - RXNE: 接收数据寄存器非空，读出数据
 *          - TC: 传输完成，用于RS485方向切换
 *
 *          Modbus RTU接收协议:
 *          1. 字节到达 -> 读取DR -> 调用Modbus_ReceiveISR()
 *          2. Modbus协议栈处理字节流
 *          3. 帧接收完成后处理Modbus请求
 */
void USART1_IRQHandler(void)
{
    /* ===== 接收中断处理 ===== */
    if (USART1->SR & USART_SR_RXNE) {
        /* 接收数据寄存器非空 */
        uint8_t received_byte = (uint8_t)(USART1->DR & 0xFF);

        /* 统计接收字节数 */
        g_UartRxCount++;

        /* 调用Modbus接收处理函数 */
        Modbus_ReceiveISR(received_byte);

        /* 清除RXNE标志 (读取DR自动清除) */
    }

    /* ===== 发送完成中断处理 (用于RS485方向切换) ===== */
    if (USART1->SR & USART_SR_TC) {
        /* 传输完成 */
        /* TC标志在TXE之后设置 */

        /* 清除TC标志 */
        USART1->SR = (uint16_t)(~(USART_SR_TC));

        /* RS485发送完成后，切换为接收模式 */
        BSP_GPIO_WritePin(GPIOD, BSP_MODBUS_DE_PIN, 0);  /* DE=0, 接收模式 */

        /* 调用Modbus发送完成回调 */
        Modbus_TxComplete();
    }

    /* ===== 发送中断处理 ===== */
    if (USART1->SR & USART_SR_TXE) {
        /* 发送数据寄存器空，可以继续发送 */
        /* 本项目中发送由DMA或主循环触发，此处预留 */
    }

    /* ===== 错误处理 ===== */
    if (USART1->SR & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) {
        /* 发生错误: ORE(过速), NE(噪声), FE(帧错误), PE(奇偶校验) */
        /* 读取DR清除错误标志 */
        volatile uint8_t dummy = (uint8_t)(USART1->DR);

        /* 可选: 记录错误日志 */
#ifdef ENABLE_DEBUG
        uint32_t sr = USART1->SR;
        printf("[UART] ERR: SR=0x%08lX, byte=0x%02X\r\n", sr, dummy);
#endif

        /* 清除错误标志 */
        USART1->SR = 0;
    }
}

/* ============================================================
 * 第七部分：EXTI0-3_IRQHandler - 外部中断处理
 * ============================================================ */

/**
 * @brief   EXTI0中断处理函数
 * @note    PC0触发: 启动按钮 (DI0)
 *          外部按钮采用上拉，按下时低电平
 *          所以配置为下降沿触发
 */
void EXTI0_IRQHandler(void)
{
    /* 检查EXTI0挂起标志 */
    if (EXTI->PR & EXTI_PR_PR0) {
        /* 清除EXTI0挂起标志 (写1清零) */
        EXTI->PR = EXTI_PR_PR0;

        /* 启动按钮处理 */
        /* 实际启动逻辑由MC层状态机处理，这里仅设置标志 */
        /* g_StartButtonPressed = 1; */

#ifdef ENABLE_DEBUG
        printf("[EXTI0] Start Button Pressed\r\n");
#endif
    }
}

/**
 * @brief   EXTI1中断处理函数
 * @note    PC1触发: 停止按钮 (DI1)
 */
void EXTI1_IRQHandler(void)
{
    /* 检查EXTI1挂起标志 */
    if (EXTI->PR & EXTI_PR_PR1) {
        /* 清除EXTI1挂起标志 */
        EXTI->PR = EXTI_PR_PR1;

        /* 停止按钮处理 */
        /* g_StopButtonPressed = 1; */

#ifdef ENABLE_DEBUG
        printf("[EXTI1] Stop Button Pressed\r\n");
#endif
    }
}

/**
 * @brief   EXTI2中断处理函数
 * @note    PC2触发: 正反转选择 (DI2)
 */
void EXTI2_IRQHandler(void)
{
    /* 检查EXTI2挂起标志 */
    if (EXTI->PR & EXTI_PR_PR2) {
        /* 清除EXTI2挂起标志 */
        EXTI->PR = EXTI_PR_PR2;

        /* 读取当前方向状态 */
        uint8_t dir = BSP_GPIO_ReadPin(BSP_DI_REVERSE_PORT, BSP_DI_REVERSE_PIN);

        /* 方向切换处理 */
        /* g_DirectionRequested = dir; */

#ifdef ENABLE_DEBUG
        printf("[EXTI2] Direction Toggle: %s\r\n", dir ? "REVERSE" : "FORWARD");
#endif
    }
}

/**
 * @brief   EXTI3中断处理函数
 * @note    PC3触发: 外部故障输入 (DI3)
 *          外部故障是硬件保护信号，必须快速响应
 */
void EXTI3_IRQHandler(void)
{
    /* 检查EXTI3挂起标志 */
    if (EXTI->PR & EXTI_PR_PR3) {
        /* 清除EXTI3挂起标志 */
        EXTI->PR = EXTI_PR_PR3;

        /* 外部故障是紧急事件，立即关闭PWM */
        BSP_FAULT_PWM_Output(0);

        /* 设置全局故障标志 */
        /* g_ExternalFault = 1; */

#ifdef ENABLE_DEBUG
        printf("[EXTI3] EXTERNAL FAULT DETECTED!\r\n");
#endif
    }
}

/* ============================================================
 * 第八部分：DMA2_Stream0_IRQHandler - ADC DMA中断
 * ============================================================ */

/**
 * @brief   DMA2 Stream0中断处理函数
 * @note    ADC1 DMA传输完成触发
 *          在循环模式下，DMA自动重新开始，无需额外处理
 *          此中断可用于监控DMA传输状态
 */
void DMA2_Stream0_IRQHandler(void)
{
    /* 检查传输完成标志 */
    if (DMA2->LISR & DMA_LISR_TCIF0) {
        /* 清除传输完成标志 */
        DMA2->LIFCR = DMA_LISR_TCIF0;

        /* DMA传输完成标志，可用于同步 */
        g_AdcConversionDone = 1;
    }

    /* 检查半传输标志 */
    if (DMA2->LISR & DMA_LISR_HTIF0) {
        /* 清除半传输标志 */
        DMA2->LIFCR = DMA_LISR_HTIF0;
    }

    /* 检查传输错误标志 */
    if (DMA2->LISR & DMA_LISR_TEIF0) {
        /* 清除传输错误标志 */
        DMA2->LIFCR = DMA_LISR_TEIF0;

#ifdef ENABLE_DEBUG
        printf("[DMA] ADC DMA Transfer Error!\r\n");
#endif
    }
}

/* ============================================================
 * 第九部分：TIM4_IRQHandler - 编码器Z相中断
 * ============================================================ */

/**
 * @brief   TIM4中断处理函数
 * @note    编码器Z相 (零位脉冲) 中断
 *          每转一次，用于:
 *          - 电机转数计数
 *          - 零点校准
 *          - 位置原点搜索
 */
void TIM4_IRQHandler(void)
{
    /* 检查CC3 (Z相) 中断标志 */
    if (TIM4->SR & TIM_SR_CC3IF) {
        /* 清除CC3中断标志 */
        TIM4->SR = (uint16_t)(~TIM_SR_CC3IF);

        /* Z相脉冲计数 */
        g_EncoderZPulseCount++;
        g_EncoderZFlag = 1;

        /* 可选: 复位计数器实现绝对位置 */
        /* TIM4->CNT = 0; */

#ifdef ENABLE_DEBUG
        /* 调试: 每转输出一次 */
        /* printf("[ENC] Z Pulse: %lu\r\n", g_EncoderZPulseCount); */
#endif
    }
}

/* ============================================================
 * 第十部分：NMI和HardFault中断
 * ============================================================ */

/**
 * @brief   NMI中断处理函数
 * @note    不可屏蔽中断，通常由时钟/看门狗故障触发
 */
void NMI_Handler(void)
{
#ifdef ENABLE_DEBUG
    printf("[FAULT] NMI Handler\r\n");
#endif
    /* 可记录故障日志 */
    /* 尝试系统恢复或停机 */
    while (1) {
        BSP_FAULT_PWM_Output(0);
    }
}

/**
 * @brief   硬件故障中断处理函数
 * @note    严重错误，通常是程序跑飞或内存越界
 */
void HardFault_Handler(void)
{
#ifdef ENABLE_DEBUG
    printf("[FAULT] HardFault Handler\r\n");
    printf("[FAULT] R0  = 0x%08lX\r\n", *(volatile uint32_t *)0xE000ED38UL);
    printf("[FAULT] R1  = 0x%08lX\r\n", *(volatile uint32_t *)0xE000ED3CUL);
    printf("[FAULT] R2  = 0x%08lX\r\n", *(volatile uint32_t *)0xE000ED40UL);
    printf("[FAULT] R3  = 0x%08lX\r\n", *(volatile uint32_t *)0xE000ED44UL);
    printf("[FAULT] R12 = 0x%08lX\r\n", *(volatile uint32_t *)0xE000ED34UL);
    printf("[FAULT] LR  = 0x%08lX\r\n", *(volatile uint32_t *)0xE000ED30UL);
    printf("[FAULT] PC  = 0x%08lX\r\n", *(volatile uint32_t *)0xE000ED2CUL);
    printf("[FAULT] PSR = 0x%08lX\r\n", *(volatile uint32_t *)0xE000ED38UL);
#endif

    /* 关闭PWM输出 */
    BSP_FAULT_PWM_Output(0);

    /* 记录故障信息到Flash或EEPROM */

    /* 尝试系统复位 */
    BSP_SystemReset();

    /* 如果复位失败，进入死循环 */
    while (1) {
        BSP_FAULT_PWM_Output(0);
    }
}

/**
 * @brief   内存管理故障中断
 */
void MemManage_Handler(void)
{
#ifdef ENABLE_DEBUG
    printf("[FAULT] MemManage Fault\r\n");
#endif
    BSP_FAULT_PWM_Output(0);
    BSP_SystemReset();
    while (1);
}

/**
 * @brief   总线故障中断
 */
void BusFault_Handler(void)
{
#ifdef ENABLE_DEBUG
    printf("[FAULT] BusFault\r\n");
#endif
    BSP_FAULT_PWM_Output(0);
    BSP_SystemReset();
    while (1);
}

/**
 * @brief   用法故障中断
 */
void UsageFault_Handler(void)
{
#ifdef ENABLE_DEBUG
    printf("[FAULT] UsageFault\r\n");
#endif
    BSP_FAULT_PWM_Output(0);
    BSP_SystemReset();
    while (1);
}

/* ============================================================
 * 第十一部分：SysTick_Handler - 系统定时器中断
 * ============================================================ */

/**
 * @brief   SysTick中断处理函数
 * @note    1ms周期，用于:
 *          - 全局时间计数 (g_SysTickMs)
 *          - 软件定时器基准
 *          - 运行时间统计
 */
void SysTick_Handler(void)
{
    /* 递增1ms计数器 */
    g_SysTickMs++;

    /* 每秒统计 */
    if (g_SysTickMs - g_LastRunTimeTick >= 1000) {
        g_LastRunTimeTick = g_SysTickMs;
        g_RunTimeSecond++;
    }

    /* 可添加软件定时器处理 */
    /* ProcessSoftwareTimers(); */
}

/* ============================================================
 * 第十二部分：Dummy_IRQHandler - 虚假中断处理
 * ============================================================ */

/**
 * @brief   虚假中断处理函数
 * @note    用于处理未使用的外设中断
 *          进入此函数表明程序有问题，应检查中断配置
 */
void Dummy_IRQHandler(void)
{
#ifdef ENABLE_DEBUG
    printf("[WARN] Dummy IRQ Handler Called\r\n");
#endif
}

/* ============================================================
 * 第十三部分：中断使能/禁止函数
 * ============================================================ */

/**
 * @brief   全局中断使能
 */
void BSP_EnableInterrupts(void)
{
    __asm__ __volatile__("cpsie i");
}

/**
 * @brief   全局中断禁止
 */
void BSP_DisableInterrupts(void)
{
    __asm__ __volatile__("cpsid i");
}

/**
 * @brief   使能特定外设中断
 * @param   irq_number: 中断号 (0-98)
 */
void BSP_EnableIRQ(uint8_t irq_number)
{
    if (irq_number < 32) {
        *((volatile uint32_t *)0xE000E100UL) = (1UL << irq_number);
    } else if (irq_number < 64) {
        *((volatile uint32_t *)0xE000E104UL) = (1UL << (irq_number - 32));
    } else if (irq_number < 96) {
        *((volatile uint32_t *)0xE000E108UL) = (1UL << (irq_number - 64));
    }
}

/**
 * @brief   禁止特定外设中断
 * @param   irq_number: 中断号 (0-98)
 */
void BSP_DisableIRQ(uint8_t irq_number)
{
    if (irq_number < 32) {
        *((volatile uint32_t *)0xE000E180UL) = (1UL << irq_number);
    } else if (irq_number < 64) {
        *((volatile uint32_t *)0xE000E184UL) = (1UL << (irq_number - 32));
    } else if (irq_number < 96) {
        *((volatile uint32_t *)0xE000E188UL) = (1UL << (irq_number - 64));
    }
}

/* ============================================================
 * 第十四部分：中断优先级设置函数
 * ============================================================ */

/**
 * @brief   设置外设中断优先级
 * @param   irq_number: 中断号 (0-98)
 * @param   priority: 优先级 (0-255, 数值越小优先级越高)
 * @note    STM32F4使用8位优先级寄存器，此处取高4位
 */
void BSP_SetIRQPriority(uint8_t irq_number, uint8_t priority)
{
    volatile uint8_t *ip = (volatile uint8_t *)(0xE000ED00UL + 0x100UL + irq_number);
    *ip = (priority & 0xF0);  /* 高4位有效 */
}

/* ============================================================
 * 第十五部分：文件结束
 * ============================================================ */

/**
 * @}
 */

/**
 * @}
 */
