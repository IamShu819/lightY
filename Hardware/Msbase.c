#include "Msbase.h"

static uint32_t DwtCyclesPerUs = 0;

static void DwtDelayInit(void)
{
    if (DwtCyclesPerUs != 0) return;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR = 0xC5ACCE55;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DwtCyclesPerUs = SystemCoreClock / 1000000;
}

void Delay_Us(uint16_t us)
{
    if (DwtCyclesPerUs == 0) DwtDelayInit();
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = (uint32_t)us * DwtCyclesPerUs;
    while ((DWT->CYCCNT - start) < ticks);
}

void Delay_Ms(uint32_t ms)
{
    while (ms--) Delay_Us(1000);
}
