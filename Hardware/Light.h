#ifndef __LIGHT_H_
#define __LIGHT_H_

#include "stm32h7xx_hal.h"
#include <stdint.h>

#define LIGHT_MODE_AUTO     0
#define LIGHT_MODE_MANUAL   1

#define LIGHT_BRIGHT_MIN    0
#define LIGHT_BRIGHT_MAX    1000

#define LIGHT_CLOSE_LUX     280
#define LIGHT_OPEN_LUX      250

void LightInit(void);
void LightSetBright(int target);
int  LightGetBright(void);
void LightEnable(void);
void LightDisable(void);
uint8_t LightGetState(void);
void LightAutoModeHandle(void);
void LightSetCompare(uint16_t compare);
void LightModeAuto(void);
void LightModeManual(void);
int  LightGetMode(void);

#endif
