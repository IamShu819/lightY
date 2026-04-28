#ifndef __MODBUS_H_
#define __MODBUS_H_

#include "stm32h7xx_hal.h"
#include <string.h>
#include <stdio.h>

#define MODBUS_MAX_BUF      256
#define MODBUS_TIMEOUT_MS   350

typedef enum {
    ModbusIdle,
    ModbusBusy
} ModbusBusStatus_t;

typedef struct {
    uint16_t (*Crc16)(const uint8_t *buf, uint16_t len);
    int (*ReadHoldingRegs03)(uint8_t slaveAddr, uint16_t regAddr, uint16_t numRegs, uint16_t *respBuf, uint32_t baudRate);
    int (*WriteSingleRegister06)(uint8_t slaveAddr, uint16_t regAddr, uint16_t value, uint32_t baudRate);
    int (*WriteMultipleRegisters16)(uint8_t slaveAddr, uint16_t regAddr, uint16_t numRegs, uint16_t *values, uint32_t baudRate);
    void (*MasterInit)(void);
} ModbusRtu_t;

extern ModbusRtu_t *ModbusRtu;

HAL_StatusTypeDef ModbusSendFrame(uint8_t *frame, uint16_t len);
ModbusBusStatus_t ModbusGetBusStatus(void);

#endif
