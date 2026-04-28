#ifndef __RGB_H_
#define __RGB_H_

#include "stm32h7xx_hal.h"

#define LED_OPEN         GPIO_PIN_RESET
#define LED_CLOSE        GPIO_PIN_SET
#define RGB_GPIO         GPIOA
#define RED_GPIO_PIN     GPIO_PIN_5
#define GREEN_GPIO_PIN   GPIO_PIN_6
#define BLUE_GPIO_PIN    GPIO_PIN_7

typedef enum {
    RgbBlack = 0,
    RgbRed   = RED_GPIO_PIN,
    RgbGreen = GREEN_GPIO_PIN,
    RgbBlue  = BLUE_GPIO_PIN,
    RgbYellow = (RED_GPIO_PIN | GREEN_GPIO_PIN),
    RgbPurple = (RED_GPIO_PIN | BLUE_GPIO_PIN),
    RgbCyan  = (GREEN_GPIO_PIN | BLUE_GPIO_PIN),
    RgbWhite = (RED_GPIO_PIN | GREEN_GPIO_PIN | BLUE_GPIO_PIN)
} RgbColor_t;

void RgbControl(RgbColor_t color);
RgbColor_t RgbGetColor(void);

#endif
