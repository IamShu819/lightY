/**
 * @file    syscalls.c
 * @brief   newlib-nano syscall stubs for bare-metal STM32H7
 */

#include "stm32h7xx_hal.h"
#include <sys/stat.h>
#include <errno.h>

extern UART_HandleTypeDef huart1;

int _write(int file, char *ptr, int len)
{
    (void)file;
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 200);
    return len;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

int _close(int file)
{
    (void)file;
    return -1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

void _exit(int status)
{
    (void)status;
    while (1);
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

int _getpid(void)
{
    return 1;
}

void *_sbrk(int incr)
{
    extern char _end;
    extern char __heap_end;
    static char *heap_end = &_end;
    char *prev = heap_end;

    if (heap_end + incr > &__heap_end)
    {
        errno = ENOMEM;
        return (void *)-1;
    }
    heap_end += incr;
    return prev;
}
