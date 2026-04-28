#include "Light.h"
#include "GpioMacros.h"
#include "Weather.h"
#include "Distance.h"
#include "Debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define LIGHT_TIM_CH    TIM_CHANNEL_1

extern TIM_HandleTypeDef htim4;

static volatile int TargetGear = 50;
static int CurrentGear = 0;
static uint8_t LightMode = LIGHT_MODE_AUTO;

void LightInit(void)
{
    __HAL_TIM_ENABLE_IT(&htim4, TIM_FLAG_UPDATE);
    HAL_NVIC_SetPriority(TIM4_IRQn, 3, 3);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
    HAL_TIM_PWM_Start(&htim4, LIGHT_TIM_CH);
    HAL_NVIC_SetPendingIRQ(TIM4_IRQn);
    printf("Light Init ok\r\n");
}

void LightEnable(void)
{
    LED_ENABLE;
}

void LightDisable(void)
{
    LED_DISABLE;
}

uint8_t LightGetState(void)
{
    return LED_STATE;
}

void LightSetCompare(uint16_t compare)
{
    __HAL_TIM_SetCompare(&htim4, LIGHT_TIM_CH, compare);
}

void LightModeAuto(void)
{
    LightMode = LIGHT_MODE_AUTO;
}

void LightModeManual(void)
{
    LightMode = LIGHT_MODE_MANUAL;
}

int LightGetMode(void)
{
    return LightMode;
}

/* Integer math: (Target * 950 + 500) / 1000 + 50 */
void LightSetBright(int target)
{
    CurrentGear = target;
    if (target == 0)
    {
        TargetGear = 0;
    }
    else
    {
        TargetGear = (target * 950 + 500) / 1000 + 50;
    }
    __HAL_TIM_ENABLE_IT(&htim4, TIM_FLAG_UPDATE);
}

int LightGetBright(void)
{
    return CurrentGear;
}

static void LightUpdate(void)
{
    int pulse = (int)__HAL_TIM_GetCompare(&htim4, LIGHT_TIM_CH);
    int target = TargetGear;

    if (pulse < target)
        pulse++;
    else if (pulse > target)
        pulse--;
    else
        __HAL_TIM_DISABLE_IT(&htim4, TIM_FLAG_UPDATE);

    LightSetCompare((uint16_t)pulse);
}

void TIM4_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&htim4, TIM_FLAG_UPDATE))
    {
        __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);
        LightUpdate();
    }
}

/* ---------- Auto-dim sub-functions ---------- */

static int LightLuxToBrightness(uint32_t lux)
{
    int y = (int)(-3.25f * lux + 1000.0f);
    if (y >= LIGHT_BRIGHT_MAX) return LIGHT_BRIGHT_MAX;
    if (y <  LIGHT_BRIGHT_MIN) return LIGHT_BRIGHT_MIN;
    return y;
}

static uint16_t LightGetMinDistance(void)
{
    uint16_t minDis = 3000;
    for (int i = 0; i < DisCount; i++)
    {
        DisDev_t *p = DistanceGetPoint((DisChannel_t)i);
        if (p && p->Status.State == SensorNormal && p->Status.FailedCount == 0 && p->Data < minDis)
        {
            minDis = p->Data;
        }
    }
    return minDis;
}

static bool LightDetectQuickMove(uint16_t currentDis[4])
{
    static uint16_t prevDis[4] = {3000, 3000, 3000, 3000};
    const float RATE_THRESH = 50.0f;
    bool detected = false;

    for (int i = 0; i < 4; i++)
    {
        DisDev_t *p = DistanceGetPoint((DisChannel_t)i);
        if (!p || p->Status.State != SensorNormal || p->Status.FailedCount != 0)
            continue;

        float rate = (float)p->Data - (float)prevDis[i];
        prevDis[i] = p->Data;
        if (fabsf(rate) >= RATE_THRESH)
            detected = true;
    }
    return detected;
}

static int LightDistanceToBrightness(uint16_t disMin)
{
    float ratio = 1.0f - (float)disMin / 3000.0f;
    int bright = LIGHT_BRIGHT_MIN + (int)(ratio * (LIGHT_BRIGHT_MAX - LIGHT_BRIGHT_MIN));
    bright = LIGHT_BRIGHT_MIN + (int)((bright - LIGHT_BRIGHT_MIN) * powf(ratio, 0.5f));

    if (bright > LIGHT_BRIGHT_MAX) bright = LIGHT_BRIGHT_MAX;
    if (bright < LIGHT_BRIGHT_MIN) bright = LIGHT_BRIGHT_MIN;
    return bright;
}

void LightAutoModeHandle(void)
{
    if (LightMode != LIGHT_MODE_AUTO) return;

    uint32_t lux = WeatherGetData()->Lux;

    if (WeatherGetStatus()->State != SensorNormal || WeatherGetStatus()->FailedCount != 0)
        return;

    if (lux >= LIGHT_CLOSE_LUX)
    {
        LightDisable();
        LightSetBright(0);
        return;
    }

    if (lux < LIGHT_OPEN_LUX)
        LightEnable();

    uint16_t currentDis[4];
    for (int i = 0; i < 4; i++)
    {
        DisDev_t *p = DistanceGetPoint((DisChannel_t)i);
        currentDis[i] = (p && p->Status.State == SensorNormal && p->Status.FailedCount == 0) ? p->Data : 3000;
    }

    if (LightDetectQuickMove(currentDis))
    {
        LightSetBright(LIGHT_BRIGHT_MAX);
        return;
    }

    uint16_t disMin = LightGetMinDistance();
    if (disMin < 2000)
    {
        LightSetBright(LightDistanceToBrightness(disMin));
        return;
    }

    LightSetBright(LightLuxToBrightness(lux));
}
