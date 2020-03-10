/*****************************************************************************
* \file      apx_vmSerializer.h
* \author    Conny Gustafsson
* \date      2019-08-11
* \brief     APX port data serializer
*
* Copyright (c) 2019 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
#ifndef APX_VM_SERIALIZER_H
#define APX_VM_SERIALIZER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_stack.h"
#include "dtl_type.h"
#include "adt_str.h"
#include "apx_error.h"
#include "apx_vmdefs.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

typedef struct apx_vmWriteState_tag
{
   union const_value_tag
   {
      const dtl_sv_t *sv;
      const dtl_av_t *av;
      const dtl_hv_t *hv;
      const dtl_hv_t *dv;
   } value;
   struct apx_vmWriteState_tag *parent;
   apx_valueType_t valueType;
   uint32_t arrayIdx; //array index
   uint32_t arrayLen; //array length of current object
   uint32_t maxArrayLen; //maximum array length of current object. This is only applicable for dynamic arrays
   apx_dynLenType_t dynLenType;
} apx_vmWriteState_t;

typedef struct apx_vmWriteBuf_tag
{
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pNext;
   uint8_t *pAdjustedNext;
}apx_vmWriteBuf_t;

typedef struct apx_vmSerializer_tag
{
   adt_stack_t stack; //stack containing strong references to apx_vmWriteState_t
   apx_vmWriteState_t *state; //current inner state
   bool hasValidWriteBuf;
   apx_vmWriteBuf_t buf;
} apx_vmSerializer_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//apx_vmWriteState_t
void apx_vmWriteState_create(apx_vmWriteState_t *self);
void apx_vmWriteState_destroy(apx_vmWriteState_t *self);
apx_vmWriteState_t* apx_vmWriteState_new(void);
void apx_vmWriteState_delete(apx_vmWriteState_t *self);
void apx_vmWriteState_vdelete(void *arg);


//apx_vmSerializer_t
void apx_vmSerializer_create(apx_vmSerializer_t *self);
void apx_vmSerializer_destroy(apx_vmSerializer_t *self);
apx_vmSerializer_t* apx_vmSerializer_new(void);
void apx_vmSerializer_delete(apx_vmSerializer_t *self);

uint8_t* apx_vmSerializer_getWritePtr(apx_vmSerializer_t *self);
uint8_t* apx_vmSerializer_getAdjustedWritePtr(apx_vmSerializer_t *self);
apx_error_t apx_vmSerializer_begin(apx_vmSerializer_t *self, uint8_t *pData, uint32_t dataLen);
apx_error_t apx_vmSerializer_setValue(apx_vmSerializer_t *self, const dtl_dv_t *dv);
apx_error_t apx_vmSerializer_packU8(apx_vmSerializer_t *self, uint8_t u8Value);
apx_error_t apx_vmSerializer_packU16(apx_vmSerializer_t *self, uint16_t u16Value);
apx_error_t apx_vmSerializer_packU32(apx_vmSerializer_t *self, uint32_t u32Value);
apx_error_t apx_vmSerializer_packS8(apx_vmSerializer_t *self, int8_t s8Value);
apx_error_t apx_vmSerializer_packS16(apx_vmSerializer_t *self, int16_t s16Value);
apx_error_t apx_vmSerializer_packS32(apx_vmSerializer_t *self, int32_t s32Value);
apx_error_t apx_vmSerializer_packFixedStr(apx_vmSerializer_t *self, const adt_str_t *str, int32_t writeLen);
apx_error_t apx_vmSerializer_packBytes(apx_vmSerializer_t *self, const adt_bytes_t *bytes);
apx_error_t apx_vmSerializer_packNull(apx_vmSerializer_t *self, int32_t writeLen);



apx_error_t apx_vmSerializer_packValueAsU8(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType);
apx_error_t apx_vmSerializer_packValueAsU16(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType);
apx_error_t apx_vmSerializer_packValueAsU32(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType);
apx_error_t apx_vmSerializer_packValueAsS8(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType);
apx_error_t apx_vmSerializer_packValueAsS16(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType);
apx_error_t apx_vmSerializer_packValueAsS32(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType);
void apx_vmSerializer_adjustWritePtr(apx_vmSerializer_t *self);
apx_error_t apx_vmSerializer_packValueAsFixedStr(apx_vmSerializer_t *self, int32_t writeLen, bool autoPopState);
apx_error_t apx_vmSerializer_packValueAsBytes(apx_vmSerializer_t *self, bool autoPopState);

apx_error_t apx_vmSerializer_recordSelect_cstr(apx_vmSerializer_t *self, const char *key);
apx_error_t apx_vmSerializer_pop(apx_vmSerializer_t *self);

apx_size_t apx_vmSerializer_getBytesWritten(apx_vmSerializer_t *self);

#endif //APX_VM_SERIALIZER_H
