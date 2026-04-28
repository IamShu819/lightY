#ifndef __BH1750_H_
#define __BH1750_H_

#include "stm32h7xx_hal.h"

#define BH1750_ADDR     0x46

int Bh1750Start(void);
float Bh1750Read(void);

#endif
