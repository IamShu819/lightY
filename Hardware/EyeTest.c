#include "EyeTest.h"
#include "Debug.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart6;
extern DMA_HandleTypeDef hdma_usart6_tx;

#define EYE_BUF_SIZE  256

static uint8_t g_eye_dma_buffer[EYE_BUF_SIZE];

static void EyeGenerateLfsrPattern(void)
{
    uint8_t lfsr = 0x01;
    for (int i = 0; i < EYE_BUF_SIZE; i++)
    {
        uint8_t bit = ((lfsr >> 6) ^ (lfsr >> 5)) & 1;
        lfsr = (lfsr << 1) | bit;
        g_eye_dma_buffer[i] = lfsr;
    }
}

void EyeTestStart(void)
{
    HAL_UART_DeInit(&huart6);

    huart6.Instance          = USART6;
    huart6.Init.BaudRate     = 15000000;
    huart6.Init.WordLength   = UART_WORDLENGTH_8B;
    huart6.Init.StopBits     = UART_STOPBITS_1;
    huart6.Init.Parity       = UART_PARITY_NONE;
    huart6.Init.Mode         = UART_MODE_TX;
    huart6.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart6.Init.OverSampling = UART_OVERSAMPLING_8;
    huart6.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart6.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart6.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart6) != HAL_OK)
    {
        printf("EyeTest: UART init failed\r\n");
        return;
    }

    EyeGenerateLfsrPattern();

    if (HAL_UART_Transmit_DMA(&huart6, g_eye_dma_buffer, EYE_BUF_SIZE) != HAL_OK)
    {
        printf("EyeTest: DMA start failed\r\n");
        return;
    }

    printf("EyeTest: USART6 15Mbps DMA started\r\n");
}
