#include "Rgb.h"

void RgbControl(RgbColor_t color)
{
    HAL_GPIO_WritePin(RGB_GPIO, RgbWhite, LED_CLOSE);
    if (color == RgbBlack) return;
    HAL_GPIO_WritePin(RGB_GPIO, (uint16_t)color, LED_OPEN);
}

RgbColor_t RgbGetColor(void)
{
    RgbColor_t rgb = RgbBlack;
    if (HAL_GPIO_ReadPin(RGB_GPIO, RED_GPIO_PIN)   == GPIO_PIN_RESET) rgb = (RgbColor_t)(rgb | RED_GPIO_PIN);
    if (HAL_GPIO_ReadPin(RGB_GPIO, GREEN_GPIO_PIN) == GPIO_PIN_RESET) rgb = (RgbColor_t)(rgb | GREEN_GPIO_PIN);
    if (HAL_GPIO_ReadPin(RGB_GPIO, BLUE_GPIO_PIN)  == GPIO_PIN_RESET) rgb = (RgbColor_t)(rgb | BLUE_GPIO_PIN);
    return rgb;
}
