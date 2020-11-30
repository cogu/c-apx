/*****************************************************************************
* \file      vm_deserializer.c
* \author    Conny Gustafsson
* \date      2019-10-03
* \brief     APX port data deserializer
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "apx/vm_deserializer.h"
#include "apx/vm_defs.h"
#include "pack.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_vmReadState_setRecordKey_cstr(apx_vmReadState_t *self, const char *key, bool isLastElement);
static apx_error_t apx_vmReadState_initValue(apx_vmReadState_t *self, apx_valueType_t valueType, uint32_t arrayLen, apx_dynLenType_t dynLenType);
static apx_error_t apx_vmDeserializer_unpackDynArrayValue(apx_vmDeserializer_t *self, apx_dynLenType_t dynLenType);
static void apx_vmDeserializer_popState(apx_vmDeserializer_t *self);
static apx_error_t apx_vmDeserializer_unpackValueInternal(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType, apx_vmVariant_t variant);
static apx_error_t apx_vmDeserializer_unpackValueU8(apx_vmDeserializer_t *self, dtl_sv_t **sv);
static apx_error_t apx_vmDeserializer_unpackValueU16(apx_vmDeserializer_t *self, dtl_sv_t **sv);
static apx_error_t apx_vmDeserializer_unpackValueU32(apx_vmDeserializer_t *self, dtl_sv_t **sv);
static apx_error_t apx_vmDeserializer_unpackValueS8(apx_vmDeserializer_t *self, dtl_sv_t **sv);
static apx_error_t apx_vmDeserializer_unpackValueS16(apx_vmDeserializer_t *self, dtl_sv_t **sv);
static apx_error_t apx_vmDeserializer_unpackValueS32(apx_vmDeserializer_t *self, dtl_sv_t **sv);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

//apx_vmReadState_t API
void apx_vmReadState_create(apx_vmReadState_t *self)
{
   self->valueType = APX_VALUE_TYPE_NONE;
   self->value.dv = (dtl_dv_t*) 0;
   self->arrayIdx = 0u;
   self->arrayLen = 0u;
   self->maxArrayLen = 0u;
   self->dynLenType = APX_DYN_LEN_NONE;
   self->parent = (apx_vmReadState_t*) 0;
   self->recordKey = (adt_str_t*) 0;
   self->isLastElement = false;
}

void apx_vmReadState_destroy(apx_vmReadState_t *self)
{
   if (self != 0)
   {
      if (self->value.dv != 0)
      {
         dtl_dv_dec_ref(self->value.dv);
      }
      if (self->recordKey != 0)
      {
         adt_str_delete(self->recordKey);
      }
   }
}

apx_vmReadState_t* apx_vmReadState_new(void)
{
   apx_vmReadState_t *self = (apx_vmReadState_t*) malloc(sizeof(apx_vmReadState_t));
   if (self != 0)
   {
      apx_vmReadState_create(self);
   }
   return self;
}

void apx_vmReadState_delete(apx_vmReadState_t *self)
{
   if (self != 0)
   {
      apx_vmReadState_destroy(self);
      free(self);
   }
}

void apx_vmReadState_vdelete(void *arg)
{
   apx_vmReadState_delete((apx_vmReadState_t*) arg);
}

//apx_vmDeserializer_t API
void apx_vmDeserializer_create(apx_vmDeserializer_t *self)
{
   if (self != 0)
   {
     adt_stack_create(&self->stack, apx_vmReadState_vdelete);
     self->state = (apx_vmReadState_t*) 0;
     self->hasValidReadBuf = false;
     self->buf.pBegin = (const uint8_t*) 0;
     self->buf.pEnd = (const uint8_t*) 0;
     self->buf.pNext = (const uint8_t*) 0;
     self->buf.pAdjustedNext = (const uint8_t*) 0;
   }
}

void apx_vmDeserializer_destroy(apx_vmDeserializer_t *self)
{
   if (self != 0)
   {
     adt_stack_destroy(&self->stack);
     if (self->state != 0)
     {
        apx_vmReadState_delete(self->state);
     }
   }
}

apx_vmDeserializer_t* apx_vmDeserializer_new(void)
{
   apx_vmDeserializer_t *self = (apx_vmDeserializer_t*) malloc(sizeof(apx_vmDeserializer_t));
   if (self != 0)
   {
      apx_vmDeserializer_create(self);
   }
   return self;
}

void apx_vmDeserializer_delete(apx_vmDeserializer_t *self)
{
   if (self != 0)
   {
      apx_vmDeserializer_destroy(self);
      free(self);
   }
}

const uint8_t* apx_vmDeserializer_getReadPtr(apx_vmDeserializer_t *self)
{
   if ( (self != 0) && (self->hasValidReadBuf) )
   {
      return self->buf.pNext;
   }
   return (const uint8_t*) 0;
}

const uint8_t* apx_vmDeserializer_getAdjustedReadPtr(apx_vmDeserializer_t *self)
{
   if ( (self != 0) && (self->hasValidReadBuf) )
   {
      return self->buf.pAdjustedNext;
   }
   return (const uint8_t*) 0;
}

apx_error_t apx_vmDeserializer_begin(apx_vmDeserializer_t *self, const uint8_t *pData, uint32_t dataLen)
{
   if (self != 0 && (pData != 0) && (dataLen > 0))
   {
      self->buf.pBegin = pData;
      self->buf.pEnd = pData+dataLen;
      self->buf.pNext = pData;
      self->buf.pAdjustedNext = (uint8_t*) 0;
      self->hasValidReadBuf = true;
      if (self->state != 0)
      {
         apx_vmReadState_delete(self->state);
      }
      self->state = apx_vmReadState_new();
      if (self->state == 0)
      {
         return APX_MEM_ERROR;
      }
      return APX_NO_ERROR;
   }
   else
   {
      self->hasValidReadBuf = false;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

dtl_dv_t* apx_vmDeserializer_getValue(apx_vmDeserializer_t *self, bool autoIncrementRef)
{
   if (self != 0)
   {
      dtl_dv_t *dv = self->state->value.dv;
      if (dv != 0 && (autoIncrementRef) )
      {
         dtl_dv_inc_ref(dv);
      }
      return dv;
   }
   return (dtl_dv_t*) 0;

}

apx_size_t apx_vmDeserializer_getBytesRead(apx_vmDeserializer_t *self)
{
   apx_size_t retval = 0u;
   if (self != 0)
   {
      retval = (apx_size_t) (self->buf.pNext-self->buf.pBegin);
   }
   return retval;
}

//Low-level API

apx_error_t apx_vmDeserializer_unpackU8(apx_vmDeserializer_t *self, uint8_t *u8Value)
{
   if ( (self != 0) && (u8Value != 0) )
   {
      if (self->hasValidReadBuf)
      {
         if (self->buf.pNext < self->buf.pEnd)
         {
            *u8Value = *self->buf.pNext++;
            return APX_NO_ERROR;
         }
         else
         {
            return APX_BUFFER_BOUNDARY_ERROR;
         }
      }
      else
      {
         return APX_MISSING_BUFFER_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_unpackU16(apx_vmDeserializer_t *self, uint16_t *u16Value)
{
   if ( self != 0)
   {
      if (self->hasValidReadBuf)
      {
         if (self->buf.pNext+1u < self->buf.pEnd)
         {
            *u16Value = unpackU16LE(self->buf.pNext);
            return APX_NO_ERROR;
         }
         else
         {
            return APX_BUFFER_BOUNDARY_ERROR;
         }
      }
      else
      {
         return APX_MISSING_BUFFER_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_unpackU32(apx_vmDeserializer_t *self, uint32_t *u32Value)
{
   if ( (self != 0) && (u32Value != 0))
   {
      if (self->hasValidReadBuf)
      {
         if (self->buf.pNext+1u < self->buf.pEnd)
         {
            *u32Value = unpackU32LE(self->buf.pNext);
            return APX_NO_ERROR;
         }
         else
         {
            return APX_BUFFER_BOUNDARY_ERROR;
         }
      }
      else
      {
         return APX_MISSING_BUFFER_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_unpackS8(apx_vmDeserializer_t *self, int8_t *s8Value)
{
   if ( (self != 0) && (s8Value != 0) )
   {
      if (self->hasValidReadBuf)
      {
         if (self->buf.pNext < self->buf.pEnd)
         {
            *s8Value = (int8_t) *self->buf.pNext++;
            return APX_NO_ERROR;
         }
         else
         {
            return APX_BUFFER_BOUNDARY_ERROR;
         }
      }
      else
      {
         return APX_MISSING_BUFFER_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_unpackS16(apx_vmDeserializer_t *self, int16_t *s16Value)
{
   if ( self != 0)
   {
      if (self->hasValidReadBuf)
      {
         if (self->buf.pNext+1u < self->buf.pEnd)
         {
            *s16Value = (int16_t) unpackU16LE(self->buf.pNext);
            return APX_NO_ERROR;
         }
         else
         {
            return APX_BUFFER_BOUNDARY_ERROR;
         }
      }
      else
      {
         return APX_MISSING_BUFFER_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_unpackS32(apx_vmDeserializer_t *self, int32_t *s32Value)
{
   if ( (self != 0) && (s32Value != 0))
   {
      if (self->hasValidReadBuf)
      {
         if (self->buf.pNext+1u < self->buf.pEnd)
         {
            *s32Value = (int32_t) unpackU32LE(self->buf.pNext);
            return APX_NO_ERROR;
         }
         else
         {
            return APX_BUFFER_BOUNDARY_ERROR;
         }
      }
      else
      {
         return APX_MISSING_BUFFER_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


apx_error_t apx_vmDeserializer_unpackFixedStr(apx_vmDeserializer_t *self, adt_str_t *str, int32_t readLen)
{
   if ( (self != 0) && (str != 0) && (readLen > 0) )
   {
      if (self->hasValidReadBuf)
      {
         int32_t bytesNeeded = readLen;
         if (self->buf.pNext+readLen <= self->buf.pEnd)
         {
            adt_error_t result;
            adt_str_clear(str);
            result = adt_str_append_bstr(str, self->buf.pNext, self->buf.pNext+readLen);
            if (result == ADT_NO_ERROR)
            {
               self->buf.pNext+=bytesNeeded;
               return APX_NO_ERROR;
            }
            else
            {
               return (apx_error_t) result;
            }
         }
         else
         {
            return APX_BUFFER_BOUNDARY_ERROR;
         }
      }
      return APX_MISSING_BUFFER_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_unpackBytes(apx_vmDeserializer_t *self, adt_bytes_t **data, int32_t readLen)
{
   if ( (self != 0) && (data != 0) && (readLen > 0) )
   {
      if (self->hasValidReadBuf)
      {
         if (self->buf.pNext+readLen <= self->buf.pEnd)
         {
            *data = adt_bytes_new(self->buf.pNext, readLen);
            if (*data != 0)
            {
               self->buf.pNext+=readLen;
               return APX_NO_ERROR;
            }
            else
            {
               return APX_MEM_ERROR;
            }
         }
         else
         {
            return APX_BUFFER_BOUNDARY_ERROR;
         }
      }
      return APX_MISSING_BUFFER_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

//High-level API
apx_error_t apx_vmDeserializer_enterRecordValue(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType)
{
   if ( self != 0 )
   {
      if (self->state != 0)
      {
         return apx_vmReadState_initValue(self->state, maxArrayLen > 0u? APX_VALUE_TYPE_ARRAY : APX_VALUE_TYPE_RECORD, maxArrayLen, dynLenType);
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_selectRecordElement_cstr(apx_vmDeserializer_t *self, const char *key, bool isLastElement)
{
   if ( self != 0 )
   {
      if (self->state != 0)
      {
         apx_error_t rc;
         apx_vmReadState_t *childState = apx_vmReadState_new();
         if (self->state == 0)
         {
            return APX_MEM_ERROR;
         }
         rc = apx_vmReadState_setRecordKey_cstr(self->state, key, isLastElement);
         if (rc != APX_NO_ERROR)
         {
            apx_vmReadState_delete(childState);
            return rc;
         }
         adt_stack_push(&self->stack, (void*) self->state);
         childState->parent = self->state;
         self->state = childState;
         return APX_NO_ERROR;
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_unpackU8Value(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      return apx_vmDeserializer_unpackValueInternal(self, maxArrayLen, dynLenType, APX_VARIANT_U8);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_unpackU16Value(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      return apx_vmDeserializer_unpackValueInternal(self, maxArrayLen, dynLenType, APX_VARIANT_U16);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_unpackU32Value(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      return apx_vmDeserializer_unpackValueInternal(self, maxArrayLen, dynLenType, APX_VARIANT_U32);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_unpackS8Value(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      return apx_vmDeserializer_unpackValueInternal(self, maxArrayLen, dynLenType, APX_VARIANT_S8);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_unpackS16Value(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      return apx_vmDeserializer_unpackValueInternal(self, maxArrayLen, dynLenType, APX_VARIANT_S16);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_unpackS32Value(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      return apx_vmDeserializer_unpackValueInternal(self, maxArrayLen, dynLenType, APX_VARIANT_S32);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}



apx_error_t apx_vmDeserializer_unpackStrValue(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      const apx_size_t elemSize = UINT8_SIZE;
      apx_vmReadState_t *state = self->state;
      apx_error_t rc;
      if (state != 0)
      {
         rc = apx_vmReadState_initValue(state, APX_VALUE_TYPE_SCALAR, maxArrayLen, dynLenType);
         if (rc != APX_NO_ERROR)
         {
            return rc;
         }
         if ( state->valueType == APX_VALUE_TYPE_SCALAR )
         {
            adt_str_t str;
            apx_error_t rc;
            adt_str_create(&str);
            self->buf.pAdjustedNext = self->buf.pNext+elemSize*state->maxArrayLen;
            if (state->dynLenType == APX_DYN_LEN_NONE)
            {
               state->arrayLen = state->maxArrayLen;
            }
            else
            {
               rc = apx_vmDeserializer_unpackDynArrayValue(self, state->dynLenType);
               if (rc != APX_NO_ERROR)
               {
                  return rc;
               }
            }
            rc = apx_vmDeserializer_unpackFixedStr(self, &str, state->arrayLen);
            if (rc != APX_NO_ERROR)
            {
               return rc;
            }
            dtl_sv_set_str(state->value.sv, &str);
            adt_str_destroy(&str);
         }
         else
         {
            return APX_DV_TYPE_ERROR;
         }
         apx_vmDeserializer_popState(self);
         return APX_NO_ERROR;
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_unpackBytesValue(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      const apx_size_t elemSize = UINT8_SIZE;
      apx_vmReadState_t *state = self->state;
      apx_error_t rc;
      if (state != 0)
      {
         rc = apx_vmReadState_initValue(state, APX_VALUE_TYPE_SCALAR, maxArrayLen, dynLenType);
         if (rc != APX_NO_ERROR)
         {
            return rc;
         }
         if ( state->valueType == APX_VALUE_TYPE_SCALAR )
         {
            adt_bytes_t *bytes = 0;
            apx_error_t rc;
            self->buf.pAdjustedNext = self->buf.pNext+elemSize*state->maxArrayLen;
            if (state->dynLenType == APX_DYN_LEN_NONE)
            {
               state->arrayLen = state->maxArrayLen;
            }
            else
            {
               rc = apx_vmDeserializer_unpackDynArrayValue(self, state->dynLenType);
               if (rc != APX_NO_ERROR)
               {
                  return rc;
               }
            }
            rc = apx_vmDeserializer_unpackBytes(self, &bytes, state->arrayLen);
            if (rc != APX_NO_ERROR)
            {
               return rc;
            }
            assert(bytes != 0);
            dtl_sv_set_bytes(state->value.sv, bytes);
            adt_bytes_delete(bytes);
         }
         else
         {
            return APX_DV_TYPE_ERROR;
         }
         apx_vmDeserializer_popState(self);
         return APX_NO_ERROR;
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_vmReadState_t *apx_vmDeserializer_getState(apx_vmDeserializer_t *self)
{
   if (self != 0)
   {
      return self->state;
   }
   return (apx_vmReadState_t*) 0;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_vmReadState_initValue(apx_vmReadState_t *self, apx_valueType_t valueType, uint32_t arrayLen, apx_dynLenType_t dynLenType)
{
   if (self->valueType != APX_VALUE_TYPE_NONE)
   {
      dtl_dv_dec_ref(self->value.dv);
      self->value.dv = (dtl_dv_t*) 0;
   }
   self->maxArrayLen = arrayLen;
   self->dynLenType = dynLenType;
   switch(valueType)
   {
   case APX_VALUE_TYPE_NONE:
      break;
   case APX_VALUE_TYPE_SCALAR:
      self->valueType = APX_VALUE_TYPE_SCALAR;
      self->value.sv = dtl_sv_new();
      break;
   case APX_VALUE_TYPE_ARRAY:
      self->valueType = APX_VALUE_TYPE_ARRAY;
      self->value.av = dtl_av_new();
      break;
   case APX_VALUE_TYPE_RECORD:
      self->valueType = APX_VALUE_TYPE_RECORD;
      self->value.hv = dtl_hv_new();
      self->recordKey = adt_str_new();
      break;
   }
   return APX_NO_ERROR;
}

static apx_error_t apx_vmReadState_setRecordKey_cstr(apx_vmReadState_t *self, const char *key, bool isLastElement)
{
   if (self->recordKey != 0)
   {
      adt_error_t result = adt_str_set_cstr(self->recordKey, key);
      if (result != ADT_NO_ERROR)
      {
         return (apx_error_t) result;
      }
      self->isLastElement = isLastElement;
      return APX_NO_ERROR;
   }
   return APX_NULL_PTR_ERROR;
}

static apx_error_t apx_vmDeserializer_unpackDynArrayValue(apx_vmDeserializer_t *self, apx_dynLenType_t dynLenType)
{
   return APX_NOT_IMPLEMENTED_ERROR;
}

static void apx_vmDeserializer_popState(apx_vmDeserializer_t *self)
{
   if (adt_stack_size(&self->stack) > 0)
   {
      apx_vmReadState_t *childState = self->state;
      apx_vmReadState_t *state = adt_stack_top(&self->stack);
      adt_stack_pop(&self->stack);
      if (state->valueType == APX_VALUE_TYPE_RECORD)
      {
         const char *recordKey = adt_str_cstr(state->recordKey);
         if (recordKey != 0 && strlen(recordKey) > 0)
         {
            dtl_hv_set_cstr(state->value.hv, recordKey, childState->value.dv, true); //reference count +1
         }
      }
      self->state = state;
      apx_vmReadState_delete(childState); //reference count -1
   }
}

static apx_error_t apx_vmDeserializer_unpackValueInternal(apx_vmDeserializer_t *self, uint32_t maxArrayLen, apx_dynLenType_t dynLenType, apx_vmVariant_t variant)
{
   apx_size_t elemSize = 0;
   apx_vmReadState_t *state = self->state;

   switch(variant)
   {
   case APX_VARIANT_U8:
      elemSize = UINT8_SIZE;
      break;
   case APX_VARIANT_U16:
      elemSize = UINT16_SIZE;
      break;
   case APX_VARIANT_U32:
      elemSize = UINT32_SIZE;
      break;
   case APX_VARIANT_S8:
      elemSize = SINT8_SIZE;
      break;
   case APX_VARIANT_S16:
      elemSize = SINT16_SIZE;
      break;
   case APX_VARIANT_S32:
      elemSize = SINT32_SIZE;
      break;
   }
   if (elemSize == 0u)
   {
      return APX_UNSUPPORTED_ERROR;
   }


   if (state != 0)
   {
      apx_error_t rc = apx_vmReadState_initValue(state, maxArrayLen > 0u? APX_VALUE_TYPE_ARRAY : APX_VALUE_TYPE_SCALAR, maxArrayLen, dynLenType);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      if ( state->valueType == APX_VALUE_TYPE_SCALAR )
      {
         rc = APX_UNSUPPORTED_ERROR;
         switch(variant)
         {
         case APX_VARIANT_U8:
            rc = apx_vmDeserializer_unpackValueU8(self, &state->value.sv);
            break;
         case APX_VARIANT_U16:
            rc = apx_vmDeserializer_unpackValueU16(self, &state->value.sv);
            break;
         case APX_VARIANT_U32:
            rc = apx_vmDeserializer_unpackValueU32(self, &state->value.sv);
            break;
         case APX_VARIANT_S8:
            rc = apx_vmDeserializer_unpackValueS8(self, &state->value.sv);
            break;
         case APX_VARIANT_S16:
            rc = apx_vmDeserializer_unpackValueS16(self, &state->value.sv);
            break;
         case APX_VARIANT_S32:
            rc = apx_vmDeserializer_unpackValueS32(self, &state->value.sv);
            break;
         }

         if (rc != APX_NO_ERROR)
         {
            return rc;
         }
      }
      else if ( state->valueType == APX_VALUE_TYPE_ARRAY)
      {
         self->buf.pAdjustedNext = self->buf.pNext+elemSize*state->maxArrayLen;
         uint32_t i;
         if (state->dynLenType == APX_DYN_LEN_NONE)
         {
            state->arrayLen = state->maxArrayLen;
         }
         else
         {
            apx_error_t rc;
            rc = apx_vmDeserializer_unpackDynArrayValue(self, state->dynLenType);
            if (rc != APX_NO_ERROR)
            {
               return rc;
            }
         }
         for(i=0; i < state->arrayLen; i++)
         {
            dtl_sv_t *childValue = (dtl_sv_t*) 0;
            rc = APX_UNSUPPORTED_ERROR;

            switch(variant)
            {
            case APX_VARIANT_U8:
               rc = apx_vmDeserializer_unpackValueU8(self, &childValue);
               break;
            case APX_VARIANT_U16:
               rc = apx_vmDeserializer_unpackValueU16(self, &childValue);
               break;
            case APX_VARIANT_U32:
               rc = apx_vmDeserializer_unpackValueU32(self, &childValue);
               break;
            case APX_VARIANT_S8:
               rc = apx_vmDeserializer_unpackValueS8(self, &childValue);
               break;
            case APX_VARIANT_S16:
               rc = apx_vmDeserializer_unpackValueS16(self, &childValue);
               break;
            case APX_VARIANT_S32:
               rc = apx_vmDeserializer_unpackValueS32(self, &childValue);
               break;
            }

            if (rc != APX_NO_ERROR)
            {
               return rc;
            }
            assert(childValue != 0);
            dtl_av_push(state->value.av, (dtl_dv_t*) childValue, false);
         }
      }
      else
      {
         return APX_DV_TYPE_ERROR;
      }
      apx_vmDeserializer_popState(self);
      return APX_NO_ERROR;
   }
   return APX_NULL_PTR_ERROR;

}

static apx_error_t apx_vmDeserializer_unpackValueU8(apx_vmDeserializer_t *self, dtl_sv_t **sv)
{
   if (sv != 0)
   {
      uint8_t u8Value;
      apx_error_t rc = apx_vmDeserializer_unpackU8(self, &u8Value);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      if (*sv != 0)
      {
         dtl_sv_set_u32(*sv, (uint32_t) u8Value);
      }
      else
      {
         *sv = dtl_sv_make_u32((uint32_t) u8Value);
         if (*sv == 0)
         {
            return APX_MEM_ERROR;
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_vmDeserializer_unpackValueU16(apx_vmDeserializer_t *self, dtl_sv_t **sv)
{
   if (sv != 0)
   {
      uint16_t u16Value;
      apx_error_t rc = apx_vmDeserializer_unpackU16(self, &u16Value);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      if (*sv != 0)
      {
         dtl_sv_set_u32(*sv, (uint32_t) u16Value);
      }
      else
      {
         *sv = dtl_sv_make_u32((uint32_t) u16Value);
         if (*sv == 0)
         {
            return APX_MEM_ERROR;
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
static apx_error_t apx_vmDeserializer_unpackValueU32(apx_vmDeserializer_t *self, dtl_sv_t **sv)
{
   if (sv != 0)
   {
      uint32_t u32Value;
      apx_error_t rc = apx_vmDeserializer_unpackU32(self, &u32Value);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      if (*sv != 0)
      {
         dtl_sv_set_u32(*sv, u32Value);
      }
      else
      {
         *sv = dtl_sv_make_u32(u32Value);
         if (*sv == 0)
         {
            return APX_MEM_ERROR;
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_vmDeserializer_unpackValueS8(apx_vmDeserializer_t *self, dtl_sv_t **sv)
{
   if (sv != 0)
   {
      int8_t s8Value;
      apx_error_t rc = apx_vmDeserializer_unpackS8(self, &s8Value);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      if (*sv != 0)
      {
         dtl_sv_set_i32(*sv, (int32_t) s8Value);
      }
      else
      {
         *sv = dtl_sv_make_i32( (int32_t) s8Value);
         if (*sv == 0)
         {
            return APX_MEM_ERROR;
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_vmDeserializer_unpackValueS16(apx_vmDeserializer_t *self, dtl_sv_t **sv)
{
   if (sv != 0)
   {
      int16_t s16Value;
      apx_error_t rc = apx_vmDeserializer_unpackS16(self, &s16Value);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      if (*sv != 0)
      {
         dtl_sv_set_i32(*sv, (int32_t) s16Value);
      }
      else
      {
         *sv = dtl_sv_make_i32( (int32_t) s16Value);
         if (*sv == 0)
         {
            return APX_MEM_ERROR;
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_vmDeserializer_unpackValueS32(apx_vmDeserializer_t *self, dtl_sv_t **sv)
{
   if (sv != 0)
   {
      int32_t s32Value;
      apx_error_t rc = apx_vmDeserializer_unpackS32(self, &s32Value);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      if (*sv != 0)
      {
         dtl_sv_set_i32(*sv, s32Value);
      }
      else
      {
         *sv = dtl_sv_make_i32(s32Value);
         if (*sv == 0)
         {
            return APX_MEM_ERROR;
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
