#ifndef __WIND_H_
#define __WIND_H_

#include "stm32h7xx_hal.h"
#include "SensorCommon.h"

#define WIND_MODBUS_ADDR    0x02
#define WIND_MODBUS_BAUD    9600

#define WIND_REG_SPEED      0x0000
#define WIND_REG_DIRECTION  0x0001

typedef struct {
    float Speed;
    float Dir;
} WindData_t;

int WindUpdate(void);
WindData_t *WindGetData(void);
SensorStatus_t *WindGetStatus(void);

#endif
