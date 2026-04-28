#ifndef __SENSOR_RETRY_H_
#define __SENSOR_RETRY_H_

#include "stm32h7xx_hal.h"
#include "SensorCommon.h"

#define SENSOR_RETRY_MAX    8

typedef struct {
    uint8_t ModbusAddr;
    uint32_t ModbusBaudRate;
    uint16_t TestReadReg;
    SensorStatus_t *Status;
    uint8_t Active;
} SensorRetryEntry_t;

int SensorRetryRegister(uint8_t addr, uint32_t baud, uint16_t reg, SensorStatus_t *status);
void SensorRetryPoll(void);

#endif
