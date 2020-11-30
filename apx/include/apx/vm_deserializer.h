/*****************************************************************************
* \file      vm_deserializer.h
* \author    Conny Gustafsson
* \date      2019-10-03
* \brief     APX port data deserializer
*
* Copyright (c) 2019-2020 Conny Gustafsson
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
#ifndef APX_VM_DESERIALIZER_H
#define APX_VM_DESERIALIZER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_stack.h"
#include "dtl_type.h"
#include "adt_str.h"
#include "apx/error.h"
#include "apx/vm_defs.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

typedef struct apx_vmReadState_tag
{
   union value_tag
   {
      dtl_dv_t *dv;
      dtl_sv_t *sv;
      dtl_av_t *av;
      dtl_hv_t *hv;
   } value;
   struct apx_vmReadState_tag *parent;
   apx_valueType_t valueType;
   uint32_t arrayIdx; //array index
   uint32_t arrayLen; //array length of current object
   uint32_t maxArrayLen; //maximum array length of current object. This is only applicable for dynamic arrays
   apx_dynLenType_t dynLenType;
   adt_str_t *recordKey; //currently selected record key
   bool isLastElement;
} apx_vmReadState_t;

typedef struct apx_vmReadBuf_tag
{
   const uint8_t *pBegin;
   const uint8_t *pEnd;
   const uint8_t *pNext;
   const uint8_t *pAdjustedNext;
}apx_vmReadBuf_t;

typedef struct apx_vmDeserializer_tag
{
   adt_stack_t stack; //stack containing strong references to apx_vmReadState_t
   apx_vmReadState_t *state; //current inner state
   bool hasValidReadBuf;
   apx_vmReadBuf_t buf;
}apx_vmDeserializer_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//apx_vmReadState_t API
void apx_vmReadState_create(apx_vmReadState_t *self);
void apx_vmReadState_destroy(apx_vmReadState_t *self);
apx_vmReadState_t* apx_vmReadState_new(void);
void apx_vmReadState_delete(apx_vmReadState_t *self);
void apx_vmReadState_vdelete(void *arg);


//apx_vmDeserializer_t API
void apx_vmDeserializer_create(apx_vmDeserializer_t *self);
void apx_vmDeserializer_destroy(apx_vmDeserializer_t *self);
apx_vmDeserializer_t* apx_vmDeserializer_new(void);
void apx_vmDeserializer_delete(apx_vmDeserializer_t *self);

const uint8_t* apx_vmDeserializer_getReadPtr(apx_vmDeserializer_t *self);
const uint8_t* apx_vmDeserializer_getAdjustedReadPtr(apx_vmDeserializer_t *self);
apx_error_t apx_vmDeserializer_begin(apx_vmDeserializer_t *self, const uint8_t *pData, uint32_t dataLen);
dtl_dv_t* apx_vmDeserializer_getValue(apx_vmDeserializer_t *self, bool autoIncrementRef);

apx_size_t apx_vmDeserializer_getBytesRead(apx_vmDeserializer_t *self);

//low-level API
apx_error_t apx_vmDeserializer_unpackU8(apx_vmDeserializer_t *self, uint8_t *u8Value);
apx_error_t apx_vmDeserializer_unpackU16(apx_vmDeserializer_t *self, uint16_t *u16Value);
apx_error_t apx_vmDeserializer_unpackU32(apx_vmDeserializer_t *self, uint32_t *u32Value);
apx_error_t apx_vmDeserializer_unpackS8(apx_vmDeserializer_t *self, int8_t *s8Value);
apx_error_t apx_vmDeserializer_unpackS16(apx_vmDeserializer_t *self, int16_t *s16Value);
apx_error_t apx_vmDeserializer_unpackS32(apx_vmDeserializer_t *self, int32_t *s32Value);
apx_error_t apx_vmDeserializer_unpackFixedStr(apx_vmDeserializer_t *self, adt_str_t *str, int32_t readLen);
apx_error_t apx_vmDeserializer_unpackBytes(apx_vmDeserializer_t *self, adt_bytes_t **data, int32_t readLen);

//high-level API
apx_error_t apx_vmDeserializer_enterRecordValue(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType);
apx_error_t apx_vmDeserializer_selectRecordElement_cstr(apx_vmDeserializer_t *self, const char *key, bool isLastElement);
apx_error_t apx_vmDeserializer_unpackU8Value(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType);
apx_error_t apx_vmDeserializer_unpackU16Value(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType);
apx_error_t apx_vmDeserializer_unpackU32Value(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType);
apx_error_t apx_vmDeserializer_unpackS8Value(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType);
apx_error_t apx_vmDeserializer_unpackS16Value(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType);
apx_error_t apx_vmDeserializer_unpackS32Value(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType);
apx_error_t apx_vmDeserializer_unpackStrValue(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType);
apx_error_t apx_vmDeserializer_unpackBytesValue(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType);

apx_vmReadState_t *apx_vmDeserializer_getState(apx_vmDeserializer_t *self);


#endif //APX_VM_DESERIALIZER_H
