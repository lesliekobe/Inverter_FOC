#ifndef __DRV_UART_H
#define __DRV_UART_H

#include <stdint.h>

#define UART_RX_BUF_SIZE  256
#define UART_TX_BUF_SIZE  256

/* UART环形缓冲区句柄 */
typedef struct {
    uint8_t  rx_buf[UART_RX_BUF_SIZE];  // 接收缓冲区
    uint8_t  tx_buf[UART_TX_BUF_SIZE];  // 发送缓冲区
    uint16_t rx_head;   // 接收写指针（ISR写，主程序读）
    uint16_t rx_tail;   // 接收读指针（主程序读）
    uint16_t tx_head;   // 发送写指针（主程序写，ISR读）
    uint16_t tx_tail;   // 发送读指针（ISR读）
    uint8_t  tx_busy;   // 发送忙标志（1=正在发送）
} UartHandle;

extern UartHandle g_uart;

void DRV_UART_Init(uint32_t baud);
void DRV_UART_SendByte(uint8_t data);
void DRV_UART_SendBuf(const uint8_t *buf, uint16_t len);
uint16_t DRV_UART_ReadByte(void);   // 返回0xFFFF表示无数据
uint16_t DRV_UART_Available(void);
void DRV_UART_SetDE(uint8_t enable);   // RS485方向控制
void DRV_UART_TXE_IRQHandler(void);     // TXE中断处理（由USART1_IRQHandler调用）
void DRV_UART_RXNE_IRQHandler(void);    // RXNE中断处理（由USART1_IRQHandler调用）

#endif
