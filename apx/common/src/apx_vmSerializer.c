/*****************************************************************************
* \file      apx_vmSerializer.c
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "apx_vmSerializer.h"
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
static apx_error_t apx_vmWriteState_setValue(apx_vmWriteState_t *self, const dtl_dv_t *dv);
static void apx_vmSerializer_updateStateInfo(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType);
static apx_error_t apx_vmSerializer_packDynArrayHeader(apx_vmSerializer_t *self, apx_dynLenType_t dynLenType, uint32_t arrayLen);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
//apx_vmWriteState_t
void apx_vmWriteState_create(apx_vmWriteState_t *self)
{
   if (self != 0)
   {
      self->valueType = APX_VALUE_TYPE_NONE;
      self->arrayIdx = 0u;
      self->arrayLen = 0u;
      self->maxArrayLen = 0u;
      self->dynLenType = APX_DYN_LEN_NONE;
      self->parent = (apx_vmWriteState_t*) 0;

   }
}

void apx_vmWriteState_destroy(apx_vmWriteState_t *self)
{
   //Nothing yet to clean up
}

apx_vmWriteState_t* apx_vmWriteState_new(void)
{
   apx_vmWriteState_t *self = (apx_vmWriteState_t*) malloc(sizeof(apx_vmWriteState_t));
   if (self != 0)
   {
      apx_vmWriteState_create(self);
   }
   return self;
}

void apx_vmWriteState_delete(apx_vmWriteState_t *self)
{
   if (self != 0)
   {
      apx_vmWriteState_destroy(self);
      free(self);
   }
}

void apx_vmWriteState_vdelete(void *arg)
{
   apx_vmWriteState_delete( (apx_vmWriteState_t*) arg );
}

//apx_vmSerializer_t
void apx_vmSerializer_create(apx_vmSerializer_t *self)
{
   if (self != 0)
   {
     adt_stack_create(&self->stack, apx_vmWriteState_vdelete);
     self->state = (apx_vmWriteState_t*) 0;
     self->hasValidWriteBuf = false;
     self->buf.pBegin = (uint8_t*) 0;
     self->buf.pEnd = (uint8_t*) 0;
     self->buf.pNext = (uint8_t*) 0;
     self->buf.pAdjustedNext = (uint8_t*) 0;
   }
}

void apx_vmSerializer_destroy(apx_vmSerializer_t *self)
{
   if (self != 0)
   {
     adt_stack_destroy(&self->stack);
     if (self->state != 0)
     {
        apx_vmWriteState_delete(self->state);
     }
   }
}

apx_vmSerializer_t* apx_vmSerializer_new(void)
{
   apx_vmSerializer_t *self = (apx_vmSerializer_t*) malloc(sizeof(apx_vmSerializer_t));
   if (self != 0)
   {
      apx_vmSerializer_create(self);
   }
   return self;
}

void apx_vmSerializer_delete(apx_vmSerializer_t *self)
{
   if (self != 0)
   {
      apx_vmSerializer_destroy(self);
      free(self);
   }
}

uint8_t* apx_vmSerializer_getWritePtr(apx_vmSerializer_t *self)
{
   if ( (self != 0) && (self->hasValidWriteBuf) )
   {
      return self->buf.pNext;
   }
   return (uint8_t*) 0;
}

uint8_t* apx_vmSerializer_getAdjustedWritePtr(apx_vmSerializer_t *self)
{
   if ( (self != 0) && (self->state != 0))
   {
      return self->buf.pAdjustedNext;
   }
   return (uint8_t*) 0;
}

apx_error_t apx_vmSerializer_begin(apx_vmSerializer_t *self, uint8_t *pData, uint32_t dataLen)
{
   if (self != 0 && (pData != 0) && (dataLen > 0))
   {
      self->buf.pBegin = pData;
      self->buf.pEnd = pData+dataLen;
      self->buf.pNext = pData;
      self->buf.pAdjustedNext = (uint8_t*) 0;
      self->hasValidWriteBuf = true;
      if (self->state != 0)
      {
         apx_vmWriteState_delete(self->state);
      }
      self->state = apx_vmWriteState_new();
      if (self->state == 0)
      {
         return APX_MEM_ERROR;
      }
      return APX_NO_ERROR;
   }
   else
   {
      self->hasValidWriteBuf = false;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmSerializer_setValue(apx_vmSerializer_t *self, const dtl_dv_t *dv)
{
   if ( (self != 0) && (dv != 0) )
   {
      if (self->state != 0)
      {
         return apx_vmWriteState_setValue(self->state, dv);
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmSerializer_packU8(apx_vmSerializer_t *self, uint8_t u8Value)
{
   if ( self != 0)
   {
      if (self->hasValidWriteBuf)
      {
         if (self->buf.pNext < self->buf.pEnd)
         {
            *self->buf.pNext++ = u8Value;
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

apx_error_t apx_vmSerializer_packU16(apx_vmSerializer_t *self, uint16_t u16Value)
{
   if ( self != 0)
   {
      if (self->hasValidWriteBuf)
      {
         if (self->buf.pNext+1u < self->buf.pEnd)
         {
            packU16LE(self->buf.pNext, u16Value);
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

apx_error_t apx_vmSerializer_packU32(apx_vmSerializer_t *self, uint32_t u32Value)
{
   if ( self != 0)
   {
      if (self->hasValidWriteBuf)
      {
         if (self->buf.pNext+3u < self->buf.pEnd)
         {
            packU32LE(self->buf.pNext, u32Value);
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

apx_error_t apx_vmSerializer_packS8(apx_vmSerializer_t *self, int8_t s8Value);
apx_error_t apx_vmSerializer_packS16(apx_vmSerializer_t *self, int16_t s16Value);
apx_error_t apx_vmSerializer_packS32(apx_vmSerializer_t *self, int32_t s32Value);
apx_error_t apx_vmSerializer_packFixedStr(apx_vmSerializer_t *self, const adt_str_t *str, int32_t writeLen)
{
   if ( (self != 0) && (str != 0) && (writeLen > 0) )
   {
      if (self->hasValidWriteBuf)
      {
         int32_t bytesNeeded = adt_str_size(str);
         if ( bytesNeeded > writeLen)
         {
            return APX_LENGTH_ERROR;
         }
         if (self->buf.pNext+bytesNeeded <= self->buf.pEnd)
         {
            memcpy(self->buf.pNext, adt_str_cstr((adt_str_t*) str), bytesNeeded);
            self->buf.pNext+=bytesNeeded;
            writeLen-=bytesNeeded;
            while(writeLen > 0)
            {
               *self->buf.pNext++ = 0u;
               writeLen--;
            }
            return APX_NO_ERROR;
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

apx_error_t apx_vmSerializer_packBytes(apx_vmSerializer_t *self, const adt_bytes_t *bytes)
{
   if ( (self != 0) && (bytes != 0) )
   {
      if (self->hasValidWriteBuf)
      {
         uint32_t bytesNeeded = adt_bytes_length(bytes);
         if (self->buf.pNext+bytesNeeded <= self->buf.pEnd)
         {
            memcpy(self->buf.pNext, adt_bytes_data(bytes), bytesNeeded);
            self->buf.pNext+=bytesNeeded;
            return APX_NO_ERROR;
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



apx_error_t apx_vmSerializer_packValueAsU8(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      const apx_size_t elemSize = UINT8_SIZE;
      apx_vmWriteState_t *state = self->state;
      if (state != 0)
      {
         apx_vmSerializer_updateStateInfo(self, arrayLen, dynLenType);
         if ( (state->valueType == APX_VALUE_TYPE_SCALAR) && (state->maxArrayLen == 0u) )
         {
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
         }
         else if ( state->valueType == APX_VALUE_TYPE_ARRAY)
         {
            state->arrayLen = dtl_av_length(state->value.av);
            if ( ( (state->dynLenType != APX_DYN_LEN_NONE) && (state->arrayLen <= state->maxArrayLen) ) ||
                 ( (state->dynLenType == APX_DYN_LEN_NONE) && (state->arrayLen == state->maxArrayLen) ) )
            {
               uint32_t i;
               if (state->dynLenType != APX_DYN_LEN_NONE)
               {
                  apx_error_t rc;
                  self->buf.pAdjustedNext = self->buf.pNext+elemSize*state->maxArrayLen;
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

apx_error_t apx_vmSerializer_packValueAsU32(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      const apx_size_t elemSize = UINT32_SIZE;
      apx_vmWriteState_t *state = self->state;
      if (state != 0)
      {
         apx_vmSerializer_updateStateInfo(self, arrayLen, dynLenType);
         if ( (state->valueType == APX_VALUE_TYPE_SCALAR) && (state->maxArrayLen == 0u) )
         {
            bool valueOk = false;
            uint32_t u32Value = dtl_sv_to_u32(state->value.sv, &valueOk);
            if (valueOk)
            {
               apx_error_t rc = apx_vmSerializer_packU32(self, u32Value);
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
         }
         else if ( state->valueType == APX_VALUE_TYPE_ARRAY)
         {
            state->arrayLen = dtl_av_length(state->value.av);
            if ( ( (state->dynLenType != APX_DYN_LEN_NONE) && (state->arrayLen <= state->maxArrayLen) ) ||
                 ( (state->dynLenType == APX_DYN_LEN_NONE) && (state->arrayLen == state->maxArrayLen) ) )
            {
               uint32_t i;
               if (state->dynLenType != APX_DYN_LEN_NONE)
               {
                  apx_error_t rc;
                  self->buf.pAdjustedNext = self->buf.pNext+elemSize*state->maxArrayLen;
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
                        apx_error_t rc = apx_vmSerializer_packU32(self, u32Value);
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

void apx_vmSerializer_adjustWritePtr(apx_vmSerializer_t *self)
{

}


apx_error_t apx_vmSerializer_packValueAsFixedStr(apx_vmSerializer_t *self, int32_t writeLen, bool autoPopState)
{
   if (self != 0)
   {
      if (self->state != 0)
      {
         if (self->state->valueType == APX_VALUE_TYPE_SCALAR)
         {
            adt_str_t *str = dtl_sv_to_str(self->state->value.sv);
            if (str != 0)
            {
               apx_error_t result = apx_vmSerializer_packFixedStr(self, str, writeLen);
               if ( (result == APX_NO_ERROR) && (autoPopState) )
               {
                  result = apx_vmSerializer_pop(self);
               }
               adt_str_delete(str);
               return result;
            }
            return APX_VALUE_ERROR;
         }
         return APX_DV_TYPE_ERROR;
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmSerializer_packValueAsBytes(apx_vmSerializer_t *self, bool autoPopState)
{
   if (self != 0)
   {
      if (self->state != 0)
      {
         if (self->state->valueType == APX_VALUE_TYPE_SCALAR)
         {
            const adt_bytes_t *bytes = dtl_sv_get_bytes(self->state->value.sv);
            if (bytes != 0)
            {
               apx_error_t result = apx_vmSerializer_packBytes(self, bytes);
               if ( (result == APX_NO_ERROR) && (autoPopState) )
               {
                  result = apx_vmSerializer_pop(self);
               }
               return result;
            }
            return APX_VALUE_ERROR;
         }
         return APX_DV_TYPE_ERROR;
      }
      return APX_NULL_PTR_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


apx_error_t apx_vmSerializer_recordSelect_cstr(apx_vmSerializer_t *self, const char *key)
{
   if ( (self != 0) && (key != 0) )
   {
      if(self->state->valueType == APX_VALUE_TYPE_RECORD)
      {
         const dtl_dv_t *childValue = dtl_hv_get_cstr(self->state->value.hv, key);
         if (childValue == 0)
         {
            return APX_NOT_FOUND_ERROR;
         }
         else
         {
            apx_vmWriteState_t *childState = apx_vmWriteState_new();
            if (childState == 0)
            {
               return APX_MEM_ERROR;
            }
            childState->parent = self->state;
            adt_stack_push(&self->stack, self->state);
            self->state = childState;
            return apx_vmWriteState_setValue(self->state, childValue);
         }
      }
      return APX_DV_TYPE_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmSerializer_pop(apx_vmSerializer_t *self)
{
   if (self != 0)
   {
      if (adt_stack_size(&self->stack) > 0)
      {
         apx_vmWriteState_delete(self->state);
         self->state = adt_stack_top(&self->stack);
         adt_stack_pop(&self->stack);
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_size_t apx_vmSerializer_getBytesWritten(apx_vmSerializer_t *self)
{
   apx_size_t retval = 0u;
   if ( (self != 0) && (self->hasValidWriteBuf))
   {
      retval = (apx_size_t) (self->buf.pNext-self->buf.pBegin);
   }
   return retval;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_vmWriteState_setValue(apx_vmWriteState_t *self, const dtl_dv_t *dv)
{
   switch(dtl_dv_type(dv))
   {
   case DTL_DV_SCALAR:
      self->valueType = APX_VALUE_TYPE_SCALAR;
      self->value.sv = (dtl_sv_t*) dv;
      break;
   case DTL_DV_ARRAY:
      self->valueType = APX_VALUE_TYPE_ARRAY;
      self->value.av = (dtl_av_t*) dv;
      break;
   case DTL_DV_HASH:
      self->valueType = APX_VALUE_TYPE_RECORD;
      self->value.hv = (dtl_hv_t*) dv;
      break;
   default:
      return APX_INVALID_ARGUMENT_ERROR;
   }
   return APX_NO_ERROR;
}

static void apx_vmSerializer_updateStateInfo(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType)
{
   self->state->maxArrayLen = arrayLen;
   self->state->dynLenType = dynLenType;
}

static apx_error_t apx_vmSerializer_packDynArrayHeader(apx_vmSerializer_t *self, apx_dynLenType_t dynLenType, uint32_t arrayLen)
{
   switch(dynLenType)
   {
   case APX_DYN_LEN_NONE:
      return APX_NO_ERROR;
   case APX_DYN_LEN_U8:
      return apx_vmSerializer_packU8(self, (uint8_t) arrayLen);
   case APX_DYN_LEN_U16:
      return apx_vmSerializer_packU16(self, (uint16_t) arrayLen);
   case APX_DYN_LEN_U32:
      return apx_vmSerializer_packU32(self, arrayLen);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
