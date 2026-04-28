#include "Ina219.h"
#include "I2c.h"
#include "Msbase.h"
#include <stdio.h>

Ina219Data_t g_CarPower;
Ina219Data_t g_LightPower;

static int Ina219WriteHalfWord(uint8_t addr, uint8_t reg, uint16_t word)
{
    I2cBus->Start();
    I2cBus->SndByte(addr & ~0x01);
    I2C_ACK_CHECK;
    I2cBus->SndByte(reg);
    I2C_ACK_CHECK;
    I2cBus->SndByte(word >> 8);
    I2C_ACK_CHECK;
    I2cBus->SndByte(word);
    I2C_ACK_CHECK;
    I2cBus->Stop();
    return 0;
}

static int Ina219ReadHalfWord(uint8_t addr, uint8_t reg, uint16_t *word)
{
    I2cBus->Start();
    I2cBus->SndByte(addr & ~0x01);
    I2C_ACK_CHECK;
    I2cBus->SndByte(reg);
    I2C_ACK_CHECK;
    I2cBus->Start();
    I2cBus->SndByte(addr | 0x01);
    I2C_ACK_CHECK;
    *word = I2cBus->RcvByte();
    I2cBus->SndAck(0);
    *word <<= 8;
    *word += I2cBus->RcvByte();
    I2cBus->SndAck(1);
    I2cBus->Stop();
    return 0;
}

int Ina219Init(void)
{
    uint16_t cfg;

    INA219_CONFIG_16V(cfg);
    if (Ina219WriteHalfWord(INA219_LIGHT_ADDR, INA219_REG_CALIBRATION, INA219_CALIB_VALUE) < 0)
    {
        printf("Light Chip Calib Init Failed\r\n");
        return -1;
    }
    Delay_Ms(10);

    INA219_CONFIG_32V(cfg);
    if (Ina219WriteHalfWord(INA219_CAR_ADDR, INA219_REG_CALIBRATION, INA219_CALIB_VALUE) < 0)
    {
        printf("Car Chip Calib Init Failed\r\n");
        return -1;
    }
    if (Ina219WriteHalfWord(INA219_LIGHT_ADDR, INA219_REG_CONFIG, cfg) < 0)
    {
        printf("Light Chip Config Init Failed\r\n");
        return -1;
    }
    if (Ina219WriteHalfWord(INA219_CAR_ADDR, INA219_REG_CONFIG, cfg) < 0)
    {
        printf("Car Chip Config Init Failed\r\n");
        return -1;
    }

    printf("INA219 Init ok\r\n");
    return 0;
}

int Ina219GetLightData(float *vol, float *elec, float *pow)
{
    if (!vol || !elec || !pow) return -1;
    uint16_t vRaw, iRaw, pRaw;
    if (Ina219ReadHalfWord(INA219_LIGHT_ADDR, INA219_REG_BUS_VOLTAGE, &vRaw)) return -1;
    if (Ina219ReadHalfWord(INA219_LIGHT_ADDR, INA219_REG_CURRENT_DATA, &iRaw)) return -1;
    if (Ina219ReadHalfWord(INA219_LIGHT_ADDR, INA219_REG_POWER_DATA, &pRaw)) return -1;
    *vol  = INA219_TO_VOLTAGE(vRaw);
    *elec = INA219_TO_ELECTRICITY(iRaw);
    *pow  = INA219_TO_POWER(pRaw);
    return 0;
}

int Ina219GetCarData(float *vol, float *elec, float *pow)
{
    if (!vol || !elec || !pow) return -1;
    uint16_t vRaw, iRaw, pRaw;
    if (Ina219ReadHalfWord(INA219_CAR_ADDR, INA219_REG_BUS_VOLTAGE, &vRaw)) return -1;
    if (Ina219ReadHalfWord(INA219_CAR_ADDR, INA219_REG_CURRENT_DATA, &iRaw)) return -1;
    if (Ina219ReadHalfWord(INA219_CAR_ADDR, INA219_REG_POWER_DATA, &pRaw)) return -1;
    *vol  = INA219_TO_VOLTAGE(vRaw);
    *elec = INA219_TO_ELECTRICITY(iRaw);
    *pow  = INA219_TO_POWER(pRaw);
    return 0;
}
