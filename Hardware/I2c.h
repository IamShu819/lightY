#ifndef __I2C_H_
#define __I2C_H_

#include "stm32h7xx_hal.h"

#define I2C_GPIO        GPIOD
#define SCL_GPIO_PIN    GPIO_PIN_11
#define SDA_GPIO_PIN    GPIO_PIN_10

void I2cDelay_Us(uint16_t us);

#define SCL_H   {(I2C_GPIO->BSRR = SCL_GPIO_PIN);          I2cDelay_Us(I2cBus->DelayUs);}
#define SCL_L   {(I2C_GPIO->BSRR = (SCL_GPIO_PIN << 16U)); I2cDelay_Us(I2cBus->DelayUs);}
#define SDA_H   {(I2C_GPIO->BSRR = SDA_GPIO_PIN);                      }
#define SDA_L   {(I2C_GPIO->BSRR = (SDA_GPIO_PIN << 16U));             }
#define SDA_R   ((I2C_GPIO->IDR & SDA_GPIO_PIN) ? 1 : 0)

#define I2C_ACK_TIMEOUT_US  1000

#define I2C_ACK_CHECK do { \
    if (I2cBus->RcvAck()) { I2cBus->Stop(); return -1; } \
} while (0)

typedef struct {
    void (*Reset)(void);
    void (*FreqInit)(float freq);
    void (*Start)(void);
    void (*Stop)(void);
    void (*SndByte)(uint8_t byte);
    void (*SndAck)(uint8_t ack);
    uint8_t (*RcvByte)(void);
    uint8_t (*RcvAck)(void);
    uint16_t DelayUs;
} I2cBasicTools_t;

typedef struct {
    int (*WriteData)(uint8_t devAddr, uint8_t reg, uint8_t *data, uint8_t len);
    int (*ReadData)(uint8_t devAddr, uint8_t startReg, uint8_t *data, uint8_t len);
} I2cStdTools_t;

extern I2cBasicTools_t *I2cBus;
extern I2cStdTools_t *I2cStd;

#endif
