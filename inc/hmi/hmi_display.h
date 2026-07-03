#ifndef __HMI_DISP_H
#define __HMI_DISP_H

#include <stdint.h>

#define DISP_ROWS  4
#define DISP_COLS  20

typedef struct {
    char line[DISP_ROWS][DISP_COLS+1];
    uint8_t update_req;
    uint8_t page;
    uint16_t blink_tick;
} DispHandle;

extern DispHandle g_disp;

void HMI_DISP_Init(void);
void HMI_DISP_Update(void);
void HMI_DISP_SetPage(uint8_t page);
void HMI_DISP_Refresh(void);
const char* HMI_DISP_GetLine(uint8_t row);

#endif
