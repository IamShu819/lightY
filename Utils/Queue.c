#include "Queue.h"
#include <stdlib.h>
#include <string.h>

int QueueIsEmpty(Queue_t *queue)
{
    return (queue->Front == queue->Rear);
}

int QueueIsFull(Queue_t *queue)
{
    return (((queue->Front + 1) % queue->Size) == queue->Rear);
}

Queue_t *QueueCreate(uint32_t size)
{
    Queue_t *queue = (Queue_t *)malloc(sizeof(Queue_t) + sizeof(uint8_t) * size);
    if (!queue) return NULL;

    queue->Size = size;
    queue->Front = 0;
    queue->Rear = 0;

    return queue;
}

int QueueDelete(Queue_t *queue)
{
    if (!queue) return -1;
    free(queue);
    return 0;
}

int QueueEnqueue(Queue_t *queue, uint8_t data)
{
    if (QueueIsFull(queue)) return -1;
    queue->Data[queue->Front] = data;
    queue->Front = (queue->Front + 1) % queue->Size;
    return 0;
}

int QueueDequeue(Queue_t *queue, uint8_t *data)
{
    if (QueueIsEmpty(queue)) return -1;
    *data = queue->Data[queue->Rear];
    queue->Rear = (queue->Rear + 1) % queue->Size;
    return 0;
}

void QueueClear(Queue_t *queue)
{
    if (!queue) return;
    queue->Front = 0;
    queue->Rear = 0;
}

int QueueEnString(Queue_t *queue, const char *str)
{
    if (!queue || !str) return -1;
    while (*str)
    {
        if (QueueEnqueue(queue, (uint8_t)*str++) < 0) return -1;
    }
    return 0;
}

uint32_t QueueCount(Queue_t *queue)
{
    if (queue->Front >= queue->Rear)
        return queue->Front - queue->Rear;
    else
        return queue->Size - queue->Rear + queue->Front;
}
