#include "Tool.h"
#include <stdlib.h>
#include <string.h>

static bool VariantTypeIsRight(VariantDef *variant)
{
    return ((variant->Type >= VARIANT_TYPE_SECTION_LEFT) &&
            (variant->Type <= VARIANT_TYPE_SECTION_RIGHT));
}

VariantStatusTypeDef VariantCreateString(VariantDef *variant, const char *str)
{
    if (!variant) return STATUS_ERROR;
    variant->Data.s = (char *)calloc(sizeof(char), strlen(str) + 1);
    if (!variant->Data.s) return STATUS_ERROR;
    strcpy(variant->Data.s, str);
    variant->Type = TYPE_STRING;
    return STATUS_NORMAL;
}

VariantStatusTypeDef VariantTypeAssist(VariantDef *variant)
{
    return (VariantTypeIsRight(variant) == true) ? STATUS_NORMAL : STATUS_ERROR;
}

VariantStatusTypeDef VariantConvertCheck(VariantDef *variant)
{
    if (!variant) return STATUS_ERROR;
    if (STATUS_ERROR == VariantTypeAssist(variant)) return STATUS_ERROR;
    if (variant->Type == TYPE_STRING && variant->Data.s)
    {
        free(variant->Data.s);
        variant->Data.s = NULL;
    }
    return STATUS_NORMAL;
}

VariantStatusTypeDef VariantToInt(VariantDef *variant, int value)
{
    if (STATUS_ERROR == VariantConvertCheck(variant)) return STATUS_ERROR;
    variant->Type = TYPE_INT;
    variant->Data.i = value;
    return STATUS_NORMAL;
}

VariantStatusTypeDef VariantToUint(VariantDef *variant, unsigned int value)
{
    if (STATUS_ERROR == VariantConvertCheck(variant)) return STATUS_ERROR;
    variant->Type = TYPE_UINT;
    variant->Data.ui = value;
    return STATUS_NORMAL;
}

VariantStatusTypeDef VariantToFloat(VariantDef *variant, float value)
{
    if (STATUS_ERROR == VariantConvertCheck(variant)) return STATUS_ERROR;
    variant->Type = TYPE_FLOAT;
    variant->Data.f = value;
    return STATUS_NORMAL;
}

VariantStatusTypeDef VariantToString(VariantDef *variant, const char *pValue)
{
    if (!pValue || STATUS_ERROR == VariantConvertCheck(variant)) return STATUS_ERROR;
    return VariantCreateString(variant, pValue);
}

VariantStatusTypeDef VariantGetTypeData(VariantDef *variant, void *pValue)
{
    if (!pValue || STATUS_ERROR == VariantTypeAssist(variant)) return STATUS_ERROR;
    switch (variant->Type)
    {
        case TYPE_UNINIT: return STATUS_ERROR;
        case TYPE_INT:    *(int *)pValue = variant->Data.i; break;
        case TYPE_UINT:   *(unsigned int *)pValue = variant->Data.ui; break;
        case TYPE_FLOAT:  *(float *)pValue = variant->Data.f; break;
        case TYPE_STRING: *(char **)pValue = variant->Data.s; break;
    }
    return STATUS_NORMAL;
}

bool MsgIsUint(VariantDef *msg)   { return (msg->Type == TYPE_UINT); }
bool MsgIsInt(VariantDef *msg)    { return (msg->Type == TYPE_INT); }
bool MsgIsFloat(VariantDef *msg)  { return (msg->Type == TYPE_FLOAT); }
bool MsgIsString(VariantDef *msg) { return (msg->Type == TYPE_STRING); }

const char *MsgGetString(VariantDef *msg)
{
    if (msg->Type == TYPE_STRING) return msg->Data.s;
    return NULL;
}

int MsgGetInt(VariantDef *msg)
{
    if (msg->Type == TYPE_INT)    return msg->Data.i;
    if (msg->Type == TYPE_UINT)   return (int)msg->Data.ui;
    if (msg->Type == TYPE_FLOAT)  return (int)msg->Data.f;
    return 0xFFFFFFFF;
}

int MsgGetUint(VariantDef *msg)
{
    if (msg->Type == TYPE_INT)    return (unsigned)msg->Data.i;
    if (msg->Type == TYPE_UINT)   return msg->Data.ui;
    if (msg->Type == TYPE_FLOAT)  return (unsigned)(int)msg->Data.f;
    return 0xFFFFFFFF;
}

int MsgGetFloat(VariantDef *msg)
{
    if (msg->Type == TYPE_INT)    return (float)msg->Data.i;
    if (msg->Type == TYPE_UINT)   return (float)(unsigned)msg->Data.ui;
    if (msg->Type == TYPE_FLOAT)  return msg->Data.f;
    return 0xFFFFFFFF;
}

int MsgReleaseString(VariantDef *msg)
{
    if (MsgIsString(msg))
    {
        if (msg->Data.s)
        {
            free((void *)msg->Data.s);
            msg->Data.s = NULL;
        }
        return 0;
    }
    return -1;
}
