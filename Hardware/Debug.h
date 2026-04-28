#ifndef __DEBUG_H_
#define __DEBUG_H_

#include "stm32h7xx_hal.h"
#include <stdio.h>

#define PSERIAL_BUF_SIZE    256

int pSerial(UART_HandleTypeDef *huart, const char *format, ...);

extern UART_HandleTypeDef huart4;
#define pScreen(__FORMAT__, ...)  pSerial(&huart4, "debug.txt+=\"" __FORMAT__ "\r\n\"\xff\xff\xff", ##__VA_ARGS__)
#define pReport(__FORMAT__, ...)  pSerial(&huart4, "report.txt+=\"" __FORMAT__ "\r\n\"\xff\xff\xff", ##__VA_ARGS__)

void DebugInit(void);

#define LOG_BASE(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#define ERROR_LOG(fmt, ...) LOG_BASE("\r\n[ERROR]:" fmt "\r\n", ##__VA_ARGS__)
#define DEBUG_LOG(fmt, ...) LOG_BASE("\r\n[INFO]:" fmt "\r\n", ##__VA_ARGS__)

#endif
