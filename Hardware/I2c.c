#include "I2c.h"
#include "Debug.h"
#include <stdio.h>

static uint32_t DwtCyclesPerUs;

static void I2cReset(void);
static void I2cFreqInit(float freq);
static void I2cStart(void);
static void I2cStop(void);
static void I2cSndByte(uint8_t byte);
static uint8_t I2cRcvByte(void);
static void I2cSndAck(uint8_t ack);
static uint8_t I2cRcvAck(void);
static int I2cWriteData(uint8_t devAddr, uint8_t reg, uint8_t *data, uint8_t len);
static int I2cReadData(uint8_t devAddr, uint8_t startReg, uint8_t *data, uint8_t len);

static I2cBasicTools_t I2cBasic = {
    .Reset    = I2cReset,
    .FreqInit = I2cFreqInit,
    .Start    = I2cStart,
    .Stop     = I2cStop,
    .SndByte  = I2cSndByte,
    .SndAck   = I2cSndAck,
    .RcvByte  = I2cRcvByte,
    .RcvAck   = I2cRcvAck,
    .DelayUs  = 5
};

static I2cStdTools_t I2cStdBasic = {
    .WriteData = I2cWriteData,
    .ReadData  = I2cReadData
};

I2cBasicTools_t *I2cBus = &I2cBasic;
I2cStdTools_t *I2cStd = &I2cStdBasic;

void I2cDelay_Us(uint16_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = (uint32_t)us * DwtCyclesPerUs;
    while ((DWT->CYCCNT - start) < ticks);
}

static void DwtInit(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR = 0xC5ACCE55;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DwtCyclesPerUs = SystemCoreClock / 1000000;
}

static void Sda(uint8_t state)
{
    if (state) { SDA_H; } else { SDA_L; }
}

static void Scl(uint8_t state)
{
    if (state) { SCL_H; } else { SCL_L; }
}

static void I2cReset(void)
{
    I2C_SCL_GPIO->ODR |= I2C_SCL_PIN;
    I2C_SDA_GPIO->ODR |= I2C_SDA_PIN;
}

static void I2cFreqInit(float freq)
{
    I2cBasic.DelayUs = (uint16_t)(500000.0f / freq);
    if (I2cBasic.DelayUs < 1) I2cBasic.DelayUs = 1;
}

static void I2cStart(void)
{
    Sda(1);
    Scl(1);
    Sda(0);
    Scl(0);
}

static void I2cStop(void)
{
    Sda(0);
    Scl(1);
    Sda(1);
}

static void I2cSndByte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        Sda(data & (0x80 >> i));
        Scl(1);
        Scl(0);
    }
}

static uint8_t I2cRcvByte(void)
{
    uint8_t data = 0;
    Sda(1);
    for (uint8_t i = 0; i < 8; i++)
    {
        Scl(1);
        data <<= 1;
        if (SDA_R) data |= 0x01;
        Scl(0);
    }
    return data;
}

static void I2cSndAck(uint8_t ack)
{
    Scl(0);
    Sda(ack);
    Scl(1);
    Scl(0);
}

static uint8_t I2cRcvAck(void)
{
    uint8_t ack;
    Sda(1);
    Scl(0);
    Scl(1);
    ack = SDA_R;
    Scl(0);
    return ack;
}

static int I2cWriteData(uint8_t devAddr, uint8_t reg, uint8_t *data, uint8_t len)
{
    I2cBus->Start();
    I2cBus->SndByte(devAddr & ~0x01);
    I2C_ACK_CHECK;
    I2cBus->SndByte(reg);
    I2C_ACK_CHECK;
    for (uint8_t i = 0; i < len; i++)
    {
        I2cBus->SndByte(data[i]);
        I2C_ACK_CHECK;
    }
    I2cBus->Stop();
    return 0;
}

static int I2cReadData(uint8_t devAddr, uint8_t startReg, uint8_t *data, uint8_t len)
{
    I2cBus->Start();
    I2cBus->SndByte(devAddr & ~0x01);
    I2C_ACK_CHECK;
    I2cBus->SndByte(startReg);
    I2C_ACK_CHECK;
    I2cBus->Start();
    I2cBus->SndByte(devAddr | 0x01);
    I2C_ACK_CHECK;
    for (uint8_t i = 0; i < len; i++)
    {
        data[i] = I2cBus->RcvByte();
        I2cBus->SndAck((i < (len - 1)) ? 0 : 1);
    }
    I2cBus->Stop();
    return 0;
}

void I2cInit(void)
{
    DwtInit();
    I2cBus->Reset();
    I2cBus->FreqInit(100000.0f);
}
