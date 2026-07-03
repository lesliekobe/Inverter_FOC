#ifndef __HMI_KEY_H
#define __HMI_KEY_H

#include <stdint.h>

typedef enum {
    KEY_NONE = 0,
    KEY_RUN, KEY_STOP, KEY_UP, KEY_DOWN,
    KEY_LEFT, KEY_RIGHT, KEY_ENTER, KEY_ESC, KEY_PROG, KEY_RESET
} KeyCode;

typedef struct {
    KeyCode key;
    uint8_t state;      /* 0=idle, 1=short, 2=long */
    uint16_t hold_time;
    uint8_t repeat_cnt;
} KeyState;

extern KeyState g_key;

void HMI_KEY_Init(void);
KeyCode HMI_KEY_Scan(void);
uint8_t HMI_KEY_IsPressed(KeyCode k);
uint8_t HMI_KEY_IsLongPress(KeyCode k);

#endif
