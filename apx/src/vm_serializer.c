/*****************************************************************************
* \file      apx_vmSerializer.c
* \author    Conny Gustafsson
* \date      2019-08-11
* \brief     APX port data serializer
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

static apx_error_t apx_vmSerializer_packValueInternal(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType, apx_vmVariant_t variant);
static apx_error_t apx_vmSerializer_packValueU8(apx_vmSerializer_t *self, const dtl_sv_t *sv);
static apx_error_t apx_vmSerializer_packValueU16(apx_vmSerializer_t *self, const dtl_sv_t *sv);
static apx_error_t apx_vmSerializer_packValueU32(apx_vmSerializer_t *self, const dtl_sv_t *sv);
static apx_error_t apx_vmSerializer_packValueS8(apx_vmSerializer_t *self, const dtl_sv_t *sv);
static apx_error_t apx_vmSerializer_packValueS16(apx_vmSerializer_t *self, const dtl_sv_t *sv);
static apx_error_t apx_vmSerializer_packValueS32(apx_vmSerializer_t *self, const dtl_sv_t *sv);

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
      self->recordKey = (adt_str_t*) 0;
      self->isLastElement = false;
   }
}

void apx_vmWriteState_destroy(apx_vmWriteState_t *self)
{
   if (self != 0)
   {
      if(self->recordKey != 0)
      {
         adt_str_delete(self->recordKey);
      }
   }
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

apx_error_t apx_vmWriteState_setRecordKey_cstr(apx_vmWriteState_t *self, const char *key, bool isLastElement)
{
   if (self != 0)
   {
      adt_error_t rc;
      if (self->recordKey == 0)
      {
         self->recordKey = adt_str_new();
         if (self->recordKey == 0)
         {
            return APX_MEM_ERROR;
         }
      }
      rc = adt_str_set_cstr(self->recordKey, key);
      if (rc == ADT_MEM_ERROR)
      {
         return APX_MEM_ERROR;
      }
      else if (rc != ADT_NO_ERROR)
      {
         return APX_GENERIC_ERROR;
      }
      self->isLastElement = isLastElement;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
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
      if (self->state != (apx_vmWriteState_t*) 0)
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

apx_error_t apx_vmSerializer_packS8(apx_vmSerializer_t *self, int8_t s8Value)
{
   if ( self != 0)
   {
      if (self->hasValidWriteBuf)
      {
         if (self->buf.pNext < self->buf.pEnd)
         {
            *self->buf.pNext++ = (uint8_t) s8Value;
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

apx_error_t apx_vmSerializer_packS16(apx_vmSerializer_t *self, int16_t s16Value)
{
   if ( self != 0)
   {
      if (self->hasValidWriteBuf)
      {
         if (self->buf.pNext+1u < self->buf.pEnd)
         {
            packU16LE(self->buf.pNext, (uint32_t) s16Value);
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

apx_error_t apx_vmSerializer_packS32(apx_vmSerializer_t *self, int32_t s32Value)
{
   if ( self != 0)
   {
      if (self->hasValidWriteBuf)
      {
         if (self->buf.pNext+3u < self->buf.pEnd)
         {
            packU32LE(self->buf.pNext, (uint32_t) s32Value);
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
            memcpy(self->buf.pNext, adt_bytes_constData(bytes), bytesNeeded);
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
      return apx_vmSerializer_packValueInternal(self, arrayLen, dynLenType, APX_VARIANT_U8);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmSerializer_packValueAsU16(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      return apx_vmSerializer_packValueInternal(self, arrayLen, dynLenType, APX_VARIANT_U16);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


apx_error_t apx_vmSerializer_packValueAsU32(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      return apx_vmSerializer_packValueInternal(self, arrayLen, dynLenType, APX_VARIANT_U32);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmSerializer_packValueAsS8(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      return apx_vmSerializer_packValueInternal(self, arrayLen, dynLenType, APX_VARIANT_S8);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmSerializer_packValueAsS16(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      return apx_vmSerializer_packValueInternal(self, arrayLen, dynLenType, APX_VARIANT_S16);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmSerializer_packValueAsS32(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      return apx_vmSerializer_packValueInternal(self, arrayLen, dynLenType, APX_VARIANT_S32);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vmSerializer_packValueAsString(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType)
{
   return apx_vmSerializer_packValueInternal(self, arrayLen, dynLenType, APX_VARIANT_STR);
}



void apx_vmSerializer_adjustWritePtr(apx_vmSerializer_t *self)
{
   (void) self;
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
                  result = apx_vmSerializer_popState(self);
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

apx_error_t apx_vmSerializer_packNull(apx_vmSerializer_t *self, int32_t writeLen)
{
   if ( (self != 0) && (writeLen > 0) )
   {
      if (self->hasValidWriteBuf)
      {
         if (self->buf.pNext+writeLen <= self->buf.pEnd)
         {
            memset(self->buf.pNext, 0, writeLen);
            self->buf.pNext+=writeLen;
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

apx_error_t apx_vmSerializer_enterRecordValue(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType)
{
   if (self != 0)
   {
      if (self->hasValidWriteBuf)
      {
         if ( (arrayLen != 0) || (dynLenType != APX_DYN_LEN_NONE) )
         {
            return APX_NOT_IMPLEMENTED_ERROR;
         }
         else
         {
            //Not an array, verify that selected value is a record
            if (self->state->valueType != APX_VALUE_TYPE_RECORD)
            {
               return APX_VALUE_ERROR;
            }
         }

         return APX_NO_ERROR;
      }
      return APX_MISSING_BUFFER_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


apx_error_t apx_vmSerializer_selectRecordElement_cstr(apx_vmSerializer_t *self, const char *key, bool isLastElement)
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
            apx_error_t rc;
            apx_vmWriteState_t *childState;
            rc = apx_vmWriteState_setRecordKey_cstr(self->state, key, isLastElement);
            if (rc != APX_NO_ERROR)
            {
               return rc;
            }
            childState = apx_vmWriteState_new();
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

apx_error_t apx_vmSerializer_popState(apx_vmSerializer_t *self)
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

apx_vmWriteState_t *apx_vmSerializer_getState(apx_vmSerializer_t *self)
{
   if (self != 0)
   {
      return self->state;
   }
   return (apx_vmWriteState_t*) 0;
}


#ifdef UNIT_TEST
apx_size_t apx_vmSerializer_getBytesWritten(apx_vmSerializer_t *self)
{
   apx_size_t retval = 0u;
   if ( (self != 0) && (self->hasValidWriteBuf))
   {
      retval = (apx_size_t) (self->buf.pNext-self->buf.pBegin);
   }
   return retval;
}

#endif

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

static apx_error_t apx_vmSerializer_packValueInternal(apx_vmSerializer_t *self, uint32_t arrayLen, apx_dynLenType_t dynLenType, apx_vmVariant_t variant)
{
   apx_size_t elemSize = 0u;
   apx_vmWriteState_t *state = self->state;
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
   case APX_VARIANT_STR:
      elemSize = UINT8_SIZE;
      break;
   }
   if (elemSize == 0u)
   {
      return APX_UNSUPPORTED_ERROR;
   }

   if (state != 0)
   {
      apx_vmSerializer_updateStateInfo(self, arrayLen, dynLenType);
      if ( (state->valueType == APX_VALUE_TYPE_SCALAR) && (state->maxArrayLen == 0u) )
      {
         apx_error_t rc =  APX_UNSUPPORTED_ERROR;
         switch(variant)
         {
         case APX_VARIANT_U8:
            rc = apx_vmSerializer_packValueU8(self, state->value.sv);
            break;
         case APX_VARIANT_U16:
            rc = apx_vmSerializer_packValueU16(self, state->value.sv);
            break;
         case APX_VARIANT_U32:
            rc = apx_vmSerializer_packValueU32(self, state->value.sv);
            break;
         case APX_VARIANT_S8:
            rc = apx_vmSerializer_packValueS8(self, state->value.sv);
            break;
         case APX_VARIANT_S16:
            rc = apx_vmSerializer_packValueS16(self, state->value.sv);
            break;
         case APX_VARIANT_S32:
            rc = apx_vmSerializer_packValueS32(self, state->value.sv);
            break;
         }
         if (rc != APX_NO_ERROR)
         {
            return rc;
         }
         return apx_vmSerializer_popState(self);
      }
      else if ( (state->valueType == APX_VALUE_TYPE_SCALAR) && (state->maxArrayLen >= 0u) )
      {
         apx_error_t rc =  APX_UNSUPPORTED_ERROR;
         if ( variant == APX_VARIANT_STR)
         {
            if (state->dynLenType == APX_DYN_LEN_NONE)
            {
               adt_str_t *str = dtl_sv_to_str(state->value.sv);
               if (str != 0)
               {
                  rc = apx_vmSerializer_packFixedStr(self, str, state->maxArrayLen);
                  adt_str_delete(str);
               }
               else
               {
                  rc = APX_VALUE_ERROR;
               }
            }
            else
            {
               rc = APX_NOT_IMPLEMENTED_ERROR;
            }
         }
         if (rc != APX_NO_ERROR)
         {
            return rc;
         }
         return apx_vmSerializer_popState(self);
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
                  apx_error_t rc =  APX_UNSUPPORTED_ERROR;
                  switch(variant)
                  {
                  case APX_VARIANT_U8:
                     rc = apx_vmSerializer_packValueU8(self, (dtl_sv_t*) childValue);
                     break;
                  case APX_VARIANT_U16:
                     rc = apx_vmSerializer_packValueU16(self, (dtl_sv_t*) childValue);
                     break;
                  case APX_VARIANT_U32:
                     rc = apx_vmSerializer_packValueU32(self, (dtl_sv_t*) childValue);
                     break;
                  case APX_VARIANT_S8:
                     rc = apx_vmSerializer_packValueS8(self, (dtl_sv_t*) childValue);
                     break;
                  case APX_VARIANT_S16:
                     rc = apx_vmSerializer_packValueS16(self, (dtl_sv_t*) childValue);
                     break;
                  case APX_VARIANT_S32:
                     rc = apx_vmSerializer_packValueS32(self, (dtl_sv_t*) childValue);
                     break;
                  }
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
            return apx_vmSerializer_popState(self);
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

static apx_error_t apx_vmSerializer_packValueU8(apx_vmSerializer_t *self, const dtl_sv_t *sv)
{
   {
      bool valueOk = false;
      uint8_t u8Value = (uint8_t) dtl_sv_to_u32(sv, &valueOk);
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
      return APX_NO_ERROR;
   }
}

static apx_error_t apx_vmSerializer_packValueU16(apx_vmSerializer_t *self, const dtl_sv_t *sv)
{
   bool valueOk = false;
   uint16_t u16Value = (uint16_t) dtl_sv_to_u32(sv, &valueOk);
   if (valueOk)
   {
      apx_error_t rc = apx_vmSerializer_packU16(self, u16Value);
      if ( rc != APX_NO_ERROR )
      {
         return rc;
      }
   }
   else
   {
      return APX_VALUE_ERROR;
   }
   return APX_NO_ERROR;
}

static apx_error_t apx_vmSerializer_packValueU32(apx_vmSerializer_t *self, const dtl_sv_t *sv)
{
   bool valueOk = false;
   uint32_t u32Value = dtl_sv_to_u32(sv, &valueOk);
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
   return APX_NO_ERROR;
}

static apx_error_t apx_vmSerializer_packValueS8(apx_vmSerializer_t *self, const dtl_sv_t *sv)
{
   bool valueOk = false;
   int8_t s8Value = (int8_t) dtl_sv_to_i32(sv, &valueOk);
   if (valueOk)
   {
      apx_error_t rc = apx_vmSerializer_packS8(self, s8Value);
      if ( rc != APX_NO_ERROR )
      {
         return rc;
      }
   }
   else
   {
      return APX_VALUE_ERROR;
   }
   return APX_NO_ERROR;
}

static apx_error_t apx_vmSerializer_packValueS16(apx_vmSerializer_t *self, const dtl_sv_t *sv)
{
   bool valueOk = false;
   int16_t s16Value = (int16_t) dtl_sv_to_i32(sv, &valueOk);
   if (valueOk)
   {
      apx_error_t rc = apx_vmSerializer_packS16(self, s16Value);
      if ( rc != APX_NO_ERROR )
      {
         return rc;
      }
   }
   else
   {
      return APX_VALUE_ERROR;
   }
   return APX_NO_ERROR;
}

static apx_error_t apx_vmSerializer_packValueS32(apx_vmSerializer_t *self, const dtl_sv_t *sv)
{
   bool valueOk = false;
   int32_t s32Value = dtl_sv_to_i32(sv, &valueOk);
   if (valueOk)
   {
      apx_error_t rc = apx_vmSerializer_packS32(self, s32Value);
      if ( rc != APX_NO_ERROR )
      {
         return rc;
      }
   }
   else
   {
      return APX_VALUE_ERROR;
   }
   return APX_NO_ERROR;
}
