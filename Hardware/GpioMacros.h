#ifndef __GPIO_MACROS_H
#define __GPIO_MACROS_H

#include "stm32h7xx_hal.h"

#define GPIO_PIN_RESET_EX(port, pin)  HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET)
#define GPIO_PIN_SET_EX(port, pin)    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET)
#define GPIO_PIN_GET_EX(port, pin)    ((port->IDR & pin) ? 1 : 0)

#define KEY_PC13    GPIO_PIN_GET_EX(GPIOC, GPIO_PIN_13)
#define KEY_PC14    GPIO_PIN_GET_EX(GPIOC, GPIO_PIN_14)
#define KeyDown(k)  (!(k))

/* Relay ports */
#define WC_LED_GPIO     GPIOE
#define WC_GPIO_PIN     GPIO_PIN_0
#define LED_GPIO_PIN    GPIO_PIN_1

#define WC_ENABLE       GPIO_PIN_SET_EX(WC_LED_GPIO, WC_GPIO_PIN)
#define WC_DISABLE      GPIO_PIN_RESET_EX(WC_LED_GPIO, WC_GPIO_PIN)
#define WC_STATE        GPIO_PIN_GET_EX(WC_LED_GPIO, WC_GPIO_PIN)

#define LED_ENABLE      GPIO_PIN_SET_EX(WC_LED_GPIO, LED_GPIO_PIN)
#define LED_DISABLE     GPIO_PIN_RESET_EX(WC_LED_GPIO, LED_GPIO_PIN)
#define LED_STATE       GPIO_PIN_GET_EX(WC_LED_GPIO, LED_GPIO_PIN)

#endif
