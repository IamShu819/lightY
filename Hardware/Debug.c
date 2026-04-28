#include "Debug.h"
#include "nr_micro_shell.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart4;

void DebugInit(void)
{
    __HAL_UART_CLEAR_OREFLAG(&huart4);
    __HAL_UART_CLEAR_IT(&huart4, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart4, UART_IT_RXNE);
    __HAL_UART_CLEAR_IDLEFLAG(&huart4);
    __HAL_UART_ENABLE_IT(&huart4, UART_IT_IDLE);
    HAL_NVIC_SetPriority(UART4_IRQn, 4, 4);
    HAL_NVIC_EnableIRQ(UART4_IRQn);

    __HAL_UART_CLEAR_OREFLAG(&huart1);
    __HAL_UART_CLEAR_IT(&huart1, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 5);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    printf("Debug Init ok\r\n");
}

int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart1, (const uint8_t *)&ch, 1, 100);
    return ch;
}

int pSerial(UART_HandleTypeDef *huart, const char *format, ...)
{
    static char buf[PSERIAL_BUF_SIZE];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    if (len <= 0) return -1;
    if (len > (int)sizeof(buf)) len = sizeof(buf);
    HAL_UART_Transmit(huart, (uint8_t *)buf, len, 200);
    return 0;
}

void USART1_IRQHandler(void)
{
    uint8_t ch;
    __HAL_UART_CLEAR_OREFLAG(&huart1);
    if (__HAL_UART_GET_IT(&huart1, UART_IT_RXNE))
    {
        ch = (uint8_t)(huart1.Instance->RDR & 0xFF);
        shell(ch);
        __HAL_UART_CLEAR_IT(&huart1, UART_IT_RXNE);
    }
}
