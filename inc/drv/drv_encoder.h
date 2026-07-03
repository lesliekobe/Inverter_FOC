#ifndef __DRV_ENCODER_H
#define __DRV_ENCODER_H

#include <stdint.h>

/* 编码器句柄 */
typedef struct {
    int32_t  position;      // 当前机械位置（计数单位）
    int16_t  speed_rpm;     // 滤波后的转速（机械RPM）
    uint8_t  dir;           // 方向：0=CW（顺时针），1=CCW（逆时针）
    uint8_t  index_found;   // Z脉冲找到标志
} EncoderHandle;

extern EncoderHandle g_encoder;

void DRV_ENCODER_Init(void);
void DRV_ENCODER_Reset(void);
int16_t DRV_ENCODER_GetSpeedRPM(void);  // 获取机械转速（RPM）
void DRV_ENCODER_TIM_IRQHandler(void);   // TIM3更新中断处理

#endif
