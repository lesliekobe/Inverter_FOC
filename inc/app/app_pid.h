#ifndef __APP_PID_H
#define __APP_PID_H

#include <stdint.h>

typedef struct {
    float kp;
    float ki;
    float kd;
    float out_max;
    float out_min;
    float integral;
    float prev_err;
    float output;
    uint8_t enable;
} PidCtrl;

void APP_PID_Init(PidCtrl *p);
void APP_PID_SetGains(PidCtrl *p, float kp, float ki, float kd);
float APP_PID_Calc(PidCtrl *p, float setpoint, float feedback, float dt);
void APP_PID_Reset(PidCtrl *p);

#endif
