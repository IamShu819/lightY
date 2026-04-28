#include "CharStack.h"
#include <stdarg.h>
#include <stdio.h>

CharStack_t *CharStackCreate(size_t size, const char *name)
{
    CharStack_t *stack = (CharStack_t *)malloc(sizeof(CharStack_t));
    if (!stack) return NULL;
    stack->StackSize = size;
    stack->Stack = (uint8_t *)calloc(sizeof(uint8_t), size + 1);
    if (!stack->Stack)
    {
        free(stack);
        return NULL;
    }
    stack->Name = name;
    stack->Sp = 0;
    stack->String = stack->Stack;
    return stack;
}

int CharStackPush(CharStack_t *stack, uint8_t data)
{
    if (stack->Sp >= (stack->StackSize - 1)) return -1;
    stack->Stack[stack->Sp++] = data;
    stack->Stack[stack->Sp] = '\0';
    return 0;
}

int CharStackPull(CharStack_t *stack, uint8_t *data)
{
    if (!stack->Sp) return -1;
    if (data != NULL)
        *data = stack->Stack[--stack->Sp];
    else
        stack->Sp--;
    stack->Stack[stack->Sp] = '\0';
    return 0;
}

int CharStackDelete(CharStack_t *stack)
{
    if (!stack) return -1;
    free(stack->Stack);
    free(stack);
    return 0;
}

String_t StringCreate(unsigned slen, const char *name)
{
    return CharStackCreate(slen, name);
}

char StringGetEndChar(String_t str)
{
    if (str->Sp > 0)
        return str->Stack[str->Sp - 1];
    else
        return '\0';
}

char *StringGetString(String_t str)
{
    return (char *)str->String;
}

int StringAddChar(String_t str, const char ch)
{
    return CharStackPush(str, ch);
}

int StringAddString(String_t str, const char *ch, size_t nbest)
{
    size_t count = 0;
    if (!ch) return -1;
    while (*ch)
    {
        if (StringAddChar(str, *ch++) < 0 && count > nbest)
            break;
        count++;
    }
    return count;
}

int StringDelChar(String_t str, char *ch)
{
    return CharStackPull(str, (uint8_t *)ch);
}

void StringClean(String_t str)
{
    memset(str->Stack, 0, str->Sp + 1);
    str->Sp = 0;
}

int StringGetLen(String_t str)
{
    return str->Sp;
}

int StringDelEndNChar(String_t str, unsigned len, char *del_data)
{
    if (!str) return -1;
    if (str->Sp < len)
    {
        len = str->Sp;
        if (del_data) memcpy(del_data, str->String, len);
        StringClean(str);
        if (del_data) del_data[len] = '\0';
        return len;
    }
    else
    {
        if (del_data) memcpy(del_data, &str->String[str->Sp - len], len);
        str->Sp -= len;
        str->String[str->Sp] = '\0';
        if (del_data) del_data[len] = '\0';
        return len;
    }
}

void StringFree(String_t str)
{
    CharStackDelete(str);
}

int StringDelFrontChar(String_t str)
{
    if (!str || str->Sp == 0) return -1;
    memmove(str->String, str->String + 1, str->Sp - 1);
    str->Sp--;
    str->String[str->Sp] = '\0';
    return 0;
}

int StringDelIndexChar(String_t str, size_t index, char *data)
{
    if (!str || index >= str->Sp) return -1;
    if (data) *data = str->String[index];
    memmove(str->String + index, str->String + index + 1, str->Sp - index - 1);
    str->Sp--;
    str->String[str->Sp] = '\0';
    return 0;
}

int StringAddFrontChar(String_t str, char ch)
{
    if (!str || str->Sp >= str->StackSize) return -1;
    memmove(str->String + 1, str->String, str->Sp);
    str->String[0] = ch;
    str->Sp++;
    str->String[str->Sp] = '\0';
    return 0;
}

int StringInsertString(String_t str, size_t index, const char *pstr)
{
    if (!pstr || !str || index >= str->StackSize) return -1;
    if (index >= str->Sp) index = str->Sp;

    size_t pstrlen = strlen(pstr);
    if (str->Sp + pstrlen > str->StackSize)
        pstrlen = str->StackSize - str->Sp;

    memmove(str->String + index + pstrlen, str->String + index, str->Sp - index);
    memcpy(str->String + index, pstr, pstrlen);
    str->Sp += pstrlen;
    str->String[str->Sp] = '\0';
    return pstrlen;
}

int StringFilterKeepNumber(String_t str)
{
    if (!str) return -1;
    size_t counter = 0;
    for (size_t i = 0; i < str->Sp; i++)
    {
        if (str->String[i] >= '0' && str->String[i] <= '9')
            str->String[counter++] = str->String[i];
    }
    str->Sp = counter;
    str->String[str->Sp] = '\0';
    return counter;
}

int StringFilterKeepAlpha(String_t str)
{
    if (!str) return -1;
    size_t counter = 0;
    for (size_t i = 0; i < str->Sp; i++)
    {
        char c = str->String[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
            str->String[counter++] = str->String[i];
    }
    str->Sp = counter;
    str->String[str->Sp] = '\0';
    return counter;
}

int StringFilterKeepList(String_t str, const char *keep_list, size_t list_len)
{
    if (!str) return -1;
    size_t counter = 0;
    for (size_t i = 0; i < str->Sp; i++)
    {
        for (size_t j = 0; j < list_len; j++)
        {
            if (str->String[i] == keep_list[j])
            {
                str->String[counter++] = str->String[i];
                break;
            }
        }
    }
    str->Sp = counter;
    str->String[str->Sp] = '\0';
    return counter;
}

int StringFilterDelList(String_t str, const char *del_list, size_t list_len)
{
    if (!str) return -1;
    size_t counter = 0;
    for (size_t i = 0; i < str->Sp; i++)
    {
        uint8_t keep = 1;
        for (size_t j = 0; j < list_len; j++)
        {
            if (str->String[i] == del_list[j])
            {
                keep = 0;
                break;
            }
        }
        if (keep)
            str->String[counter++] = str->String[i];
    }
    str->Sp = counter;
    str->String[str->Sp] = '\0';
    return counter;
}

void StringAddStringFormat(String_t str, const char *format, ...)
{
    char buf[str->StackSize - StringGetLen(str) + 1];
    va_list va;
    va_start(va, format);
    vsnprintf(buf, sizeof(buf), format, va);
    StringAddString(str, (const char *)buf, sizeof(buf));
    va_end(va);
}
