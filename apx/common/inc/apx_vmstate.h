/*****************************************************************************
* \file      apx_vmInnerState.h
* \author    Conny Gustafsson
* \date      2019-03-05
* \brief     APX virtual machine state
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
#ifndef APX_VMSTATE_H
#define APX_VMSTATE_H

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
typedef struct apx_vmInnerState_tag
{
   union value_tag
   {
      int32_t  i32;
      uint32_t u32;
      int64_t  i64;
      uint64_t u64;
      bool bl;
      uint8_t *str;
      dtl_av_t *array;
      dtl_hv_t *record;
      const uint8_t *constStr;
      const dtl_av_t *constArray;
      const dtl_hv_t *constRecord;
   } value; //value currently being operated on
   apx_value_t valueType;
   adt_str_t *key; //current record key
   int32_t arrayIdx; //current array index
   int32_t arrayLen; //array index
}apx_vmInnerState_t;

typedef struct apx_vmReadData_tag
{
   const uint8_t *pBegin;
   const uint8_t *pEnd;
   const uint8_t *pNext;
}apx_vmReadData_t;

typedef struct apx_vmWriteData_tag
{
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pNext;
}apx_vmWriteData_t;

typedef struct apx_vmstate_tag
{
   adt_stack_t stack; //stack containing strong references to apx_vmInnerState_t
   apx_vmInnerState_t *inner; //current inner state
   bool hasValidWriteData;
   bool hasValidReadData;
   union {
      apx_vmWriteData_t write;
      apx_vmReadData_t read;
   } data;
}apx_vmstate_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_vmInnerState_create(apx_vmInnerState_t *self);
void apx_vmInnerState_destroy(apx_vmInnerState_t *self);
apx_vmInnerState_t* apx_vmInnerState_new(void);
void apx_vmInnerState_delete(apx_vmInnerState_t *self);
void apx_vmInnerState_vdelete(void *arg);
void apx_vmstate_create(apx_vmstate_t *self);
void apx_vmstate_destroy(apx_vmstate_t *self);
apx_vmstate_t* apx_vmstate_new(void);
void apx_vmstate_delete(apx_vmstate_t *self);

uint8_t* apx_vmstate_getWritePtr(apx_vmstate_t *self);

apx_error_t apx_vmstate_setWriteData(apx_vmstate_t *self, uint8_t *pData, uint32_t dataLen);
apx_error_t apx_vmstate_packU8(apx_vmstate_t *self, uint8_t u8Value);
apx_error_t apx_vmstate_packU16(apx_vmstate_t *self, uint16_t u16Value);
apx_error_t apx_vmstate_packU32(apx_vmstate_t *self, uint32_t u32Value);

//apx_error_t apx_vmstate_packU8U8DynArray(apx_vmstate_t *self, uint8_t *pArrayValues, uint8_t arrayLen);
//apx_error_t apx_vmstate_packU8U16DynArray(apx_vmstate_t *self, uint8_t *pArrayValues, uint16_t arrayLen);
//apx_error_t apx_vmstate_packU8U32DynArray(apx_vmstate_t *self, uint8_t *pArrayValues, uint32_t arrayLen);

apx_error_t apx_vmstate_packU8DynArray(apx_vmstate_t *self, uint8_t arrayLen);
apx_error_t apx_vmstate_packU16DynArray(apx_vmstate_t *self, uint16_t arrayLen);
apx_error_t apx_vmstate_packU32DynArray(apx_vmstate_t *self, uint32_t arrayLen);
apx_error_t apx_vmstate_setRecordValue(apx_vmstate_t *self, const dtl_hv_t *hv);
apx_error_t apx_vmstate_createRecordValue(apx_vmstate_t *self);
apx_error_t apx_vmstate_pop(apx_vmstate_t *self);

#endif //APX_VMSTATE_H
