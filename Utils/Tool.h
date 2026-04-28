#ifndef __TOOL_H_
#define __TOOL_H_

#include "stm32h7xx_hal.h"
#include <stdbool.h>

#define VARIANT_TYPE_SECTION_LEFT    TYPE_UNINIT
#define VARIANT_TYPE_SECTION_RIGHT   TYPE_STRING

typedef enum {
    TYPE_NONE  = -1,
    TYPE_UNINIT = 0,
    TYPE_INT    = 1,
    TYPE_UINT   = 2,
    TYPE_FLOAT  = 3,
    TYPE_STRING = 4
} VariantTypeDef;

typedef enum {
    STATUS_NORMAL,
    STATUS_ERROR = !STATUS_NORMAL
} VariantStatusTypeDef;

typedef struct {
    VariantTypeDef Type;
    union {
        int i;
        unsigned int ui;
        float f;
        char *s;
    } Data;
} VariantDef;

VariantStatusTypeDef VariantCreateString(VariantDef *variant, const char *str);
VariantStatusTypeDef VariantTypeAssist(VariantDef *variant);
VariantStatusTypeDef VariantConvertCheck(VariantDef *variant);
VariantStatusTypeDef VariantToInt(VariantDef *variant, int value);
VariantStatusTypeDef VariantToUint(VariantDef *variant, unsigned int value);
VariantStatusTypeDef VariantToFloat(VariantDef *variant, float value);
VariantStatusTypeDef VariantToString(VariantDef *variant, const char *pValue);
VariantStatusTypeDef VariantGetTypeData(VariantDef *variant, void *pValue);

bool MsgIsUint(VariantDef *msg);
bool MsgIsInt(VariantDef *msg);
bool MsgIsFloat(VariantDef *msg);
bool MsgIsString(VariantDef *msg);
const char *MsgGetString(VariantDef *msg);
int MsgGetInt(VariantDef *msg);
int MsgGetUint(VariantDef *msg);
int MsgGetFloat(VariantDef *msg);
int MsgReleaseString(VariantDef *msg);

#endif
