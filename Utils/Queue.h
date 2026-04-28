#ifndef __QUEUE_H_
#define __QUEUE_H_

#include "stm32h7xx_hal.h"

typedef struct {
    uint32_t Front;
    uint32_t Rear;
    uint32_t Size;
    uint8_t Data[];
} Queue_t;

int QueueIsEmpty(Queue_t *queue);
int QueueIsFull(Queue_t *queue);
Queue_t *QueueCreate(uint32_t size);
int QueueDelete(Queue_t *queue);
int QueueEnqueue(Queue_t *queue, uint8_t data);
int QueueDequeue(Queue_t *queue, uint8_t *data);
void QueueClear(Queue_t *queue);
int QueueEnString(Queue_t *queue, const char *str);
uint32_t QueueCount(Queue_t *queue);

#endif
