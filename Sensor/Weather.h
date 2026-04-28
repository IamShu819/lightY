#ifndef __WEATHER_H_
#define __WEATHER_H_

#include "stm32h7xx_hal.h"
#include "SensorCommon.h"

#define WEATHER_MODBUS_ADDR     0x01
#define WEATHER_MODBUS_BAUD     4800

#define WEATHER_REG_HUMI    500
#define WEATHER_REG_TEMP    501
#define WEATHER_REG_PM25    503
#define WEATHER_REG_PM10    504
#define WEATHER_REG_LUXH    506
#define WEATHER_REG_LUXL    507

typedef struct {
    float Humi;
    float Temp;
    uint16_t PM25;
    uint16_t PM10;
    uint32_t Lux;
} WeatherData_t;

int WeatherUpdate(void);
WeatherData_t *WeatherGetData(void);
SensorStatus_t *WeatherGetStatus(void);

#endif
