#ifndef __SENSOR_COMMON_H_
#define __SENSOR_COMMON_H_

#include "stm32h7xx_hal.h"

#define SENSOR_FAILED_THRESHOLD  4

typedef enum {
    SensorError  = 0,
    SensorNormal = 1
} SensorState_t;

typedef struct {
    uint8_t FailedCount;
    SensorState_t State;
} SensorStatus_t;

uint16_t SensorBaudRateToReg(uint32_t baud);
int SensorReadWithRetry(uint8_t addr, uint16_t reg, uint16_t *data, uint32_t baud);
void SensorCheckAndRegisterRetry(SensorStatus_t *status, uint8_t addr, uint32_t baud, uint16_t reg);
uint16_t SensorFilterEma(uint16_t newValue, uint16_t *filteredValue, uint8_t shift);

#endif
