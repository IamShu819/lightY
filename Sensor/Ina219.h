#ifndef __INA219_H_
#define __INA219_H_

#include "stm32h7xx_hal.h"

#define INA219_LIGHT_ADDR       (0x40 << 1)
#define INA219_CAR_ADDR         (0x45 << 1)

#define INA219_REG_CONFIG        0x00
#define INA219_REG_SHUNT_VOLTAGE 0x01
#define INA219_REG_BUS_VOLTAGE   0x02
#define INA219_REG_POWER_DATA    0x03
#define INA219_REG_CURRENT_DATA  0x04
#define INA219_REG_CALIBRATION   0x05

#define INA219_CALIB_VALUE       0x346E

#define INA219_CONFIG_16V(cfg)  do { (cfg) = 0x37FF; (cfg) &= ~(1 << 13); } while(0)
#define INA219_CONFIG_32V(cfg)  do { (cfg) = 0x37FF; (cfg) |=  (1 << 13); } while(0)

#define INA219_TO_ELECTRICITY(raw)  ((float)((int16_t)(raw)) * 0.00030518f)
#define INA219_TO_POWER(raw)        ((float)(raw) * 0.0061036f)
#define INA219_TO_VOLTAGE(raw)      (((raw) >> 3) * 0.004f)

typedef struct {
    float Voltage;
    float Elec;
    float Pow;
} Ina219Data_t;

extern Ina219Data_t g_CarPower;
extern Ina219Data_t g_LightPower;

int Ina219Init(void);
int Ina219GetLightData(float *vol, float *elec, float *pow);
int Ina219GetCarData(float *vol, float *elec, float *pow);

#endif
