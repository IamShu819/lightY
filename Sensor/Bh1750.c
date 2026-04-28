#include "Bh1750.h"
#include "I2c.h"
#include <stdio.h>

static int Bh1750DrvStart(void)
{
    I2cBus->Start();
    I2cBus->SndByte(BH1750_ADDR);
    I2C_ACK_CHECK;
    I2cBus->SndByte(0x10);
    I2C_ACK_CHECK;
    I2cBus->Stop();
    return 0;
}

static int Bh1750DrvRead(float *lux)
{
    uint8_t buf[2];
    I2cBus->Start();
    I2cBus->SndByte(BH1750_ADDR | 0x01);
    I2C_ACK_CHECK;
    buf[0] = I2cBus->RcvByte();
    I2cBus->SndAck(0);
    buf[1] = I2cBus->RcvByte();
    I2cBus->SndAck(1);
    I2cBus->Stop();

    uint16_t raw = ((uint16_t)buf[0] << 8) | buf[1];
    *lux = (float)raw / 1.2f;
    return 0;
}

int Bh1750Start(void)
{
    if (Bh1750DrvStart() < 0)
    {
        printf("BH1750 start failed\r\n");
        return -1;
    }
    return 0;
}

float Bh1750Read(void)
{
    float lux;
    if (Bh1750DrvRead(&lux) < 0)
    {
        printf("BH1750 read failed\r\n");
        return -1.0f;
    }
    return lux;
}
