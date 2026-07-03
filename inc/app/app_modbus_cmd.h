#ifndef __APP_MODBUS_CMD_H
#define __APP_MODBUS_CMD_H

#include <stdint.h>

typedef struct {
    uint8_t addr;
    uint16_t *reg_map;
} ModbusHandle;

extern ModbusHandle g_modbus;

void APP_MODBUS_Init(void);
uint16_t APP_MODBUS_Process(uint8_t *frame, uint16_t len);
void APP_MODBUS_ProcessRx(void);

#endif
