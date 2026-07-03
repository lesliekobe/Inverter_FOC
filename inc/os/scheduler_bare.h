#ifndef __SCHEDULER_BARE_H
#define __SCHEDULER_BARE_H

#include <stdint.h>

void Scheduler_Init(void);
void Scheduler_Run(void);
void Scheduler_TickISR(void);
uint32_t Scheduler_GetTick(void);

#endif
