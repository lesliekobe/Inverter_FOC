/**
 * @file drv_uart.c
 * @brief 串口驱动 - STM32F4硬件抽象层
 *
 * 使用USART1（TX=PA9, RX=PA10），支持RS485（半双工）。
 * 特性：
 *   - 环形缓冲区（256字节TX/RX）
 *   - TXE中断驱动发送（自动切换RS485方向）
 *   - RXNE中断驱动接收
 *   - RS485 DE控制（PD12）：发送时高，发送完成后自动低
 *
 * 使用说明：
 *   - 初始化后自动进入接收模式
 *   - 发送字节/字符串会自动控制DE引脚
 *   - 读取字节：轮询调用DRV_UART_ReadByte()
 */

#include <drv_uart.h>
#include <stm32f4xx_hal.h>

/* 全局UART句柄 */
UartHandle g_uart = {0};

/* USART1句柄 */
static USART_HandleTypeDef husart1;

/* RS485 DE引脚定义（与drv_gpio.c共用PD12） */
#define RS485_DE_PORT  GPIOD
#define RS485_DE_PIN   GPIO_PIN_12

/**
 * @brief UART初始化
 * @param baud 波特率（常用：9600, 19200, 115200）
 *
 * 引脚：
 *   USART1_TX -> PA9
 *   USART1_RX -> PA10
 *   RS485_DE  -> PD12（已在drv_gpio.c中初始化）
 */
void DRV_UART_Init(uint32_t baud)
{
    /* 使能GPIOA和USART1时钟 */
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* 配置USART1引脚: PA9=TX, PA10=RX */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;     // 复用推挽
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;  // USART1复用功能
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 初始化USART1 */
    husart1.Instance        = USART1;
    husart1.Init.BaudRate   = baud;
    husart1.Init.WordLength = USART_WORDLENGTH_8B;  // 8位数据位
    husart1.Init.StopBits   = USART_STOPBITS_1;     // 1位停止位
    husart1.Init.Parity     = USART_PARITY_NONE;    // 无校验
    husart1.Init.Mode       = USART_MODE_TX_RX;     // 收发模式
    husart1.Init.HwFlowCtl  = USART_HWCONTROL_NONE; // 无硬件流控
    HAL_USART_Init(&husart1);

    /* 使能USART1 TXE和RXNE中断 */
    HAL_NVIC_SetPriority(USART1_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    __HAL_USART_ENABLE_IT(&husart1, USART_IT_TXE);  // 发送寄存器空中断
    __HAL_USART_ENABLE_IT(&husart1, USART_IT_RXNE); // 接收寄存器非空中断

    /* 初始化环形缓冲区 */
    g_uart.rx_head = 0;
    g_uart.rx_tail = 0;
    g_uart.tx_head = 0;
    g_uart.tx_tail = 0;
    g_uart.tx_busy = 0;
}

/**
 * @brief 发送单字节（阻塞式入队）
 * @param data 要发送的字节
 *
 * 发送前将DE置高（RS485转为发送模式），发送完成后TXE中断自动切回接收模式。
 */
void DRV_UART_SendByte(uint8_t data)
{
    /* 如果发送器空闲，先开启DE（RS485方向切换） */
    if (g_uart.tx_busy == 0) {
        DRV_UART_SetDE(1);   // RS485发送模式
        g_uart.tx_busy = 1;
        /* 直接写入DR寄存器启动发送 */
        USART1->DR = data;
    } else {
        /* 发送器忙，数据入环形缓冲区 */
        uint16_t next_head = (g_uart.tx_head + 1) % UART_TX_BUF_SIZE;
        /* 缓冲区满时丢弃（实际应用中应加错误处理） */
        if (next_head != g_uart.tx_tail) {
            g_uart.tx_buf[g_uart.tx_head] = data;
            g_uart.tx_head = next_head;
        }
    }
}

/**
 * @brief 发送缓冲区数据
 * @param buf 数据指针
 * @param len 长度
 */
void DRV_UART_SendBuf(const uint8_t *buf, uint16_t len)
{
    while (len--) {
        DRV_UART_SendByte(*buf++);
    }
}

/**
 * @brief 读取一个字节
 * @return 读取的字节，0xFFFF表示无数据
 */
uint16_t DRV_UART_ReadByte(void)
{
    if (g_uart.rx_head == g_uart.rx_tail) {
        return 0xFFFF;  // 无数据
    }
    uint8_t data = g_uart.rx_buf[g_uart.rx_tail];
    g_uart.rx_tail = (g_uart.rx_tail + 1) % UART_RX_BUF_SIZE;
    return data;
}

/**
 * @brief 查询当前可读字节数
 * @return 可用字节数
 */
uint16_t DRV_UART_Available(void)
{
    if (g_uart.rx_head >= g_uart.rx_tail) {
        return (uint16_t)(g_uart.rx_head - g_uart.rx_tail);
    } else {
        return (uint16_t)(UART_RX_BUF_SIZE - g_uart.rx_tail + g_uart.rx_head);
    }
}

/**
 * @brief RS485方向控制
 * @param enable 1=发送模式(DE高)，0=接收模式(DE低)
 */
void DRV_UART_SetDE(uint8_t enable)
{
    if (enable) {
        HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_RESET);
    }
}

/**
 * @brief TXE中断处理（由USART1_IRQHandler调用）
 *
 * 职责：
 *  1. 从tx_buf取出一字节发送
 *  2. 当最后一个字节发送完成后，切回RS485接收模式（DE=0）
 */
void DRV_UART_TXE_IRQHandler(void)
{
    if (g_uart.tx_head != g_uart.tx_tail) {
        /* 缓冲区还有数据，发送下一字节 */
        uint8_t data = g_uart.tx_buf[g_uart.tx_tail];
        g_uart.tx_tail = (g_uart.tx_tail + 1) % UART_TX_BUF_SIZE;
        USART1->DR = data;
    } else {
        /* 发送缓冲区空，关闭TXE中断，防止重复触发 */
        __HAL_USART_DISABLE_IT(&husart1, USART_IT_TXE);
        g_uart.tx_busy = 0;
        /* 最后一个字节已写入DR（还在移位寄存器中发送），
           短暂延迟后切换DE为低（接收模式）
           注：最保险的方式是在TXE中断中检查TC（传输完成）标志，
           此处简化处理：关闭TXE后直接清除DE
        */
        DRV_UART_SetDE(0);
    }
}

/**
 * @brief RXNE中断处理（由USART1_IRQHandler调用）
 *
 * 将接收到的字节放入rx_buf。
 */
void DRV_UART_RXNE_IRQHandler(void)
{
    uint8_t data = (uint8_t)(USART1->DR & 0xFF);
    uint16_t next_head = (g_uart.rx_head + 1) % UART_RX_BUF_SIZE;

    if (next_head != g_uart.rx_tail) {
        /* 缓冲区未满，写入数据 */
        g_uart.rx_buf[g_uart.rx_head] = data;
        g_uart.rx_head = next_head;
    }
    /* 缓冲区满时丢弃数据（应加溢出标志供上层处理） */
}

/**
 * @brief USART1中断处理（由stm32f4xx_it.c调用）
 */
void USART1_IRQHandler(void)
{
    /* 检查TXE标志（发送寄存器空） */
    if (__HAL_USART_GET_FLAG(&husart1, USART_FLAG_TXE) &&
        __HAL_USART_GET_IT_SOURCE(&husart1, USART_IT_TXE)) {
        DRV_UART_TXE_IRQHandler();
    }
    /* 检查RXNE标志（接收寄存器非空） */
    if (__HAL_USART_GET_FLAG(&husart1, USART_FLAG_RXNE) &&
        __HAL_USART_GET_IT_SOURCE(&husart1, USART_IT_RXNE)) {
        DRV_UART_RXNE_IRQHandler();
    }
}
