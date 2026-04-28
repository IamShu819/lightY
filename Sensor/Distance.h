#ifndef __DISTANCE_H_
#define __DISTANCE_H_

#include "stm32h7xx_hal.h"
#include "SensorCommon.h"

#define DISTANCE_REG_REAL_VALUE     0x0001
#define DISTANCE_REG_FILTER_VALUE   0x0002
#define DISTANCE_REG_DEV_ADDR       0x0003

typedef enum {
    DisFront = 0,
    DisRear,
    DisLeft,
    DisRight,
    DisCount
} DisChannel_t;

typedef struct {
    uint8_t ModbusAddr;
    uint16_t Data;
    uint32_t BaudRate;
    SensorStatus_t Status;
} DisDev_t;

void DistanceInit(void);
DisDev_t *DistanceGetPoint(DisChannel_t ch);
void DistanceUpdate(void);

#endif
