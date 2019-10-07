/*****************************************************************************
* \file      apx_vmDeserializer.c
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
#include "apx_vmDeserializer.h"
#include "apx_vmdefs.h"
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
static apx_error_t apx_vmReadState_push(apx_vmReadState_t *self, apx_vmVariant_t variantType);
static apx_error_t apx_vmReadState_pop(apx_vmReadState_t *self, dtl_dv_t **dv);
static apx_error_t apx_vmReadState_recordSelect_cstr(apx_vmReadState_t *self, const char *key);
static void apx_vmReadState_setArrayInfo(apx_vmReadState_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType);

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

dtl_dv_t* apx_vmDeserializer_getValue(apx_vmDeserializer_t *self)
{
   if (self != 0)
   {
      return self->state->value.dv;
   }
   return (dtl_dv_t*) 0;
}

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

apx_error_t apx_vmDeserializer_push(apx_vmDeserializer_t *self, apx_vmVariant_t variantType)
{
   if ( self != 0 )
   {
      if (self->state != 0)
      {
         return apx_vmReadState_push(self->state, variantType);
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_pop(apx_vmDeserializer_t *self, dtl_dv_t **dv)
{
   if ( self != 0 )
   {
      if (self->state != 0)
      {
         return apx_vmReadState_pop(self->state, dv);
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_recordSelect_cstr(apx_vmDeserializer_t *self, const char *key)
{
   if ( self != 0 )
   {
      if (self->state != 0)
      {
         return apx_vmReadState_recordSelect_cstr(self->state, key);
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmDeserializer_unpackValueAsU8(apx_vmDeserializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      const apx_size_t elemSize = UINT8_SIZE;
      apx_vmReadState_t *state = self->state;
      apx_error_t rc;
      if (state != 0)
      {
         apx_vmReadState_setArrayInfo(state, arrayLen, dynLenType);
         if ( (state->valueType == APX_VALUE_TYPE_SCALAR) && (state->maxArrayLen == 0u) )
         {
#if 0
            bool valueOk = false;
            uint8_t u8Value = (uint8_t) dtl_sv_to_u32(state->value.sv, &valueOk);
            if (valueOk)
            {
               apx_error_t rc = apx_vmSerializer_packU8(self, u8Value);
               if ( rc != APX_NO_ERROR )
               {
                  return rc;
               }
            }
            else
            {
               return APX_VALUE_ERROR;
            }
            return apx_vmSerializer_pop(self);
#endif
            return APX_NOT_IMPLEMENTED_ERROR;
         }
         else if ( state->valueType == APX_VALUE_TYPE_ARRAY)
         {
#if 0
            state->arrayLen = dtl_av_length(state->value.av);
            if ( ( (state->dynLenType != APX_DYN_LEN_NONE) && (state->arrayLen <= state->maxArrayLen) ) ||
                 ( (state->dynLenType == APX_DYN_LEN_NONE) && (state->arrayLen == state->maxArrayLen) ) )
            {
               uint32_t i;
               if (state->dynLenType != APX_DYN_LEN_NONE)
               {
                  apx_error_t rc;
                  self->buf.adjustedNext = self->buf.pNext+elemSize*state->maxArrayLen;
                  rc = apx_vmSerializer_packDynArrayHeader(self, state->dynLenType, state->arrayLen);
                  if (rc != APX_NO_ERROR)
                  {
                     return rc;
                  }
               }
               for(i=0; i < state->arrayLen; i++)
               {
                  const dtl_dv_t *childValue = dtl_av_value(self->state->value.av, i);
                  if ( (childValue != 0) && (dtl_dv_type(childValue) == DTL_DV_SCALAR) )
                  {
                     bool valueOk = false;
                     uint32_t u32Value = dtl_sv_to_u32((dtl_sv_t*) childValue, &valueOk);
                     if (valueOk)
                     {
                        apx_error_t rc = apx_vmSerializer_packU8(self, (uint8_t) u32Value);
                        if (rc != APX_NO_ERROR)
                        {
                           return rc;
                        }
                     }
                     else
                     {
                        return APX_VALUE_ERROR;
                     }
                  }
                  else
                  {
                     return APX_VALUE_ERROR;
                  }
               }
               return apx_vmSerializer_pop(self);

            }
            else
            {
               return APX_LENGTH_ERROR;
            }
            #endif
            return APX_NOT_IMPLEMENTED_ERROR;
         }
         else if ( state->valueType == APX_VALUE_TYPE_RECORD)
         {
            if ( (state->recordKey == 0) || (adt_str_length(state->recordKey) == 0) )
            {
               return APX_MISSING_KEY_ERROR;
            }
            else
            {
               uint8_t u8Value;
               rc = apx_vmDeserializer_unpackU8(self, &u8Value);
               if (rc == APX_NO_ERROR)
               {
                  dtl_sv_t *sv = dtl_sv_make_u32((uint32_t) u8Value);
                  if (sv == 0)
                  {
                     return APX_MEM_ERROR;
                  }
                  dtl_hv_set_cstr(state->value.hv, adt_str_cstr(state->recordKey), (dtl_dv_t*) sv, false);
                  adt_str_clear(state->recordKey);
                  return APX_NO_ERROR;
               }
               else
               {
                  return rc;
               }
            }
         }
         else
         {
            return APX_DV_TYPE_ERROR;
         }
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;

}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_vmReadState_push(apx_vmReadState_t *self, apx_vmVariant_t variantType)
{
   if (variantType == APX_VARIANT_RECORD)
   {
      self->value.hv = dtl_hv_new();
      if (self->value.hv == 0)
      {
         return APX_MEM_ERROR;
      }
      self->recordKey = adt_str_new();
      if (self->recordKey == 0)
      {
         dtl_hv_delete(self->value.hv);
         return APX_MEM_ERROR;
      }
      self->valueType = APX_VALUE_TYPE_RECORD;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_vmReadState_pop(apx_vmReadState_t *self, dtl_dv_t **dv)
{
   if ( (self != 0) && (dv != 0) )
   {
      if (self->valueType != APX_VALUE_TYPE_NONE)
      {
         *dv = self->value.dv;
         dtl_dv_inc_ref(self->value.dv);
      }
      else
      {
         *dv = (dtl_dv_t*) 0;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_vmReadState_recordSelect_cstr(apx_vmReadState_t *self, const char *key)
{
   if (self->recordKey != 0)
   {
      adt_error_t result = adt_str_set_cstr(self->recordKey, key);
      if (result != ADT_NO_ERROR)
      {
         return (apx_error_t) result;
      }
      return APX_NO_ERROR;
   }
   return APX_NULL_PTR_ERROR;
}

static void apx_vmReadState_setArrayInfo(apx_vmReadState_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType)
{
   self->maxArrayLen = arrayLen;
   self->dynLenType = dynLenType;
}
