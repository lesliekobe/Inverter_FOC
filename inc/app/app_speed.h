#ifndef __APP_SPEED_H
#define __APP_SPEED_H

#include <stdint.h>

typedef struct {
    float target_freq;
    float current_freq;
    float accel_rate;   /* Hz/s */
    float decel_rate;   /* Hz/s */
    uint8_t dir;
    uint8_t source;     /* 0=panel,1=AI,2=Modbus,3=MultiSpeed */
} AppSpeedHandle;

extern AppSpeedHandle g_speed;

void APP_Speed_Init(void);
void APP_Speed_SetTarget(float freq, uint8_t dir, uint8_t source);
void APP_Speed_SetRamp(float accel_s, float decel_s);
void APP_Speed_Task(void);
float APP_Speed_GetCurrentFreq(void);

#endif
