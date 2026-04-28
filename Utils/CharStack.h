#ifndef __CHAR_STACK_H_
#define __CHAR_STACK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *Name;
    size_t StackSize;
    size_t Sp;
    uint8_t *Stack;
    uint8_t *String;
} CharStack_t;

typedef CharStack_t *String_t;

CharStack_t *CharStackCreate(size_t size, const char *name);
int CharStackPush(CharStack_t *stack, uint8_t data);
int CharStackPull(CharStack_t *stack, uint8_t *data);
int CharStackDelete(CharStack_t *stack);

String_t StringCreate(unsigned slen, const char *name);
char StringGetEndChar(String_t str);
char *StringGetString(String_t str);
int StringAddChar(String_t str, const char ch);
int StringAddString(String_t str, const char *ch, size_t nbest);
int StringDelChar(String_t str, char *ch);
void StringClean(String_t str);
int StringGetLen(String_t str);
int StringDelEndNChar(String_t str, unsigned len, char *del_data);
void StringFree(String_t str);
int StringDelFrontChar(String_t str);
int StringAddFrontChar(String_t str, char ch);
int StringInsertString(String_t str, size_t index, const char *pstr);
int StringDelIndexChar(String_t str, size_t index, char *data);
void StringAddStringFormat(String_t str, const char *format, ...);
int StringFilterKeepNumber(String_t str);
int StringFilterKeepAlpha(String_t str);
int StringFilterKeepList(String_t str, const char *keep_list, size_t list_len);
int StringFilterDelList(String_t str, const char *del_list, size_t list_len);

#ifdef __cplusplus
}
#endif

#endif
