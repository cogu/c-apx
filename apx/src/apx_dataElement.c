/*****************************************************************************
* \file      apx_dataElement.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     Data element data structure
*
* Copyright (c) 2017-2020 Conny Gustafsson
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
#include <errno.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "apx_dataElement.h"
#include "apx_error.h"
#include "apx_types.h"
#include "apx_dataType.h"
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
static apx_error_t apx_dataElement_calcDynLenType(apx_dataElement_t *self);
static dtl_dv_t *apx_dataElement_makeU32InitValueFromDynamicValue(apx_dataElement_t *self, dtl_dv_t *dv, apx_error_t *errorCode);
static dtl_dv_t *apx_dataElement_makeS32InitValueFromDynamicValue(apx_dataElement_t *self, dtl_dv_t *dv, apx_error_t *errorCode);
static dtl_dv_t *apx_dataElement_makeStringInitValueFromDynamicValue(apx_dataElement_t *self, dtl_dv_t *dv, apx_error_t *errorCode);
static dtl_dv_t *apx_dataElement_makeHashInitValueFromDynamicValue(apx_dataElement_t *self, dtl_dv_t *dv, apx_error_t *errorCode);
static dtl_dv_t *apx_dataElement_makeHashInitValueFromArray(apx_dataElement_t *self, dtl_av_t *av, apx_error_t *errorCode);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_dataElement_t *apx_dataElement_new(int8_t baseType, const char *name)
{
   apx_dataElement_t *self = (apx_dataElement_t*) malloc(sizeof(apx_dataElement_t));
   if(self != 0)
   {
      int8_t result = apx_dataElement_create(self,baseType,name);
      if (result<0)
      {
         free(self);
         self=0;
      }
   }
   else
   {
      errno = ENOMEM;
   }
   return self;
}

void apx_dataElement_delete(apx_dataElement_t *self)
{
   if(self != 0)
   {
      apx_dataElement_destroy(self);
      free(self);
   }
}

void apx_dataElement_vdelete(void *arg)
{
   apx_dataElement_delete((apx_dataElement_t*) arg);
}

int8_t apx_dataElement_create(apx_dataElement_t *self, int8_t baseType, const char *name)
{
   if (self != 0)
   {
      if (name != 0)
      {
         self->name=STRDUP(name);
         if (self->name == 0)
         {
            errno = ENOMEM;
            return -1;
         }
      }
      else
      {
         self->name = 0;
      }
      self->baseType=baseType;
      if (baseType == APX_BASE_TYPE_RECORD)
      {
         self->childElements = adt_ary_new(apx_dataElement_vdelete);
      }
      else
      {
         self->childElements = (adt_ary_t*) 0;
      }
      self->arrayLen = 0;
      self->lowerLimit.s32 = 0;
      self->upperLimit.s32 = 0;
      self->packLen = 0;
      self->isDynamicArray = false;
      self->dynLenType = APX_DYN_LEN_NONE;
      if (baseType == APX_BASE_TYPE_REF_NAME)
      {
         self->typeRef.name = 0;
      }
      else
      {
         self->typeRef.id = 0;
      }
   }
   return 0;
}

void apx_dataElement_destroy(apx_dataElement_t *self)
{
   if (self != 0)
   {
      if (self->name != 0)
      {
         free(self->name);
      }
      if (self->childElements != 0)
      {
         adt_ary_delete(self->childElements);
      }
      if ( (self->baseType == APX_BASE_TYPE_REF_NAME) && (self->typeRef.name != 0) )
      {
         free(self->typeRef.name);
      }
   }
}

void apx_dataElement_initRecordType(apx_dataElement_t *self)
{
   self->baseType=APX_BASE_TYPE_RECORD;
   if (self->childElements != 0)
   {
      adt_ary_delete(self->childElements);
   }
   self->childElements = adt_ary_new(apx_dataElement_vdelete);
}


apx_error_t apx_dataElement_setArrayLen(apx_dataElement_t *self, uint32_t arrayLen)
{
   apx_error_t retval = APX_NO_ERROR;
   if (self != 0)
   {
      self->arrayLen = arrayLen;
      if (self->isDynamicArray)
      {
         retval = apx_dataElement_calcDynLenType(self);
      }
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

uint32_t apx_dataElement_getArrayLen(apx_dataElement_t *self)
{
   if (self != 0)
   {
      return self->arrayLen;
   }
   return 0;
}

apx_error_t apx_dataElement_setDynamicArray(apx_dataElement_t *self)
{
   apx_error_t retval = APX_NO_ERROR;
   if (self != 0)
   {
      self->isDynamicArray = true;
      retval = apx_dataElement_calcDynLenType(self);
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

bool apx_dataElement_isDynamicArray(apx_dataElement_t *self)
{
   if (self != 0)
   {
      return self->dynLenType != APX_DYN_LEN_NONE;
   }
   return false;
}

apx_dynLenType_t apx_dataElement_getDynLenType(apx_dataElement_t *self)
{
   if (self != 0)
   {
      return self->dynLenType;
   }
   return APX_DYN_LEN_NONE;
}


void apx_dataElement_setTypeReferenceId(apx_dataElement_t *self, int32_t typeId)
{
   if (self != 0)
   {
      if ( (self->baseType == APX_BASE_TYPE_REF_NAME) && (self->typeRef.name != 0) )
      {
         free(self->typeRef.name);
      }
      if (self->baseType != APX_BASE_TYPE_REF_ID)
      {
         self->baseType = APX_BASE_TYPE_REF_ID;
      }
      self->typeRef.id = typeId;
   }
}

int32_t apx_dataElement_getTypeReferenceId(apx_dataElement_t *self)
{
   if ( (self != 0) && (self->baseType == APX_BASE_TYPE_REF_ID))
   {
      return self->typeRef.id;
   }
   return -1;
}

void apx_dataElement_setTypeReferenceName(apx_dataElement_t *self, const char *typeName)
{
   if (self != 0)
   {
      if ( (self->baseType == APX_BASE_TYPE_REF_NAME) && (self->typeRef.name != 0) )
      {
         free(self->typeRef.name);
      }
      if (self->baseType != APX_BASE_TYPE_REF_NAME)
      {
         self->baseType = APX_BASE_TYPE_REF_NAME;
      }
      self->typeRef.name = STRDUP(typeName);
   }
}

const char *apx_dataElement_getTypeReferenceName(apx_dataElement_t *self)
{
   if ( (self != 0) && (self->baseType == APX_BASE_TYPE_REF_NAME) )
   {
      return self->typeRef.name;
   }
   return (const char*) 0;
}

void apx_dataElement_setTypeReferencePtr(apx_dataElement_t *self, struct apx_datatype_tag *ptr)
{
   if (self != 0)
   {
      if ( (self->baseType == APX_BASE_TYPE_REF_NAME) && (self->typeRef.name != 0) )
      {
         free(self->typeRef.name);
      }
      if (self->baseType != APX_BASE_TYPE_REF_PTR)
      {
         self->baseType = APX_BASE_TYPE_REF_PTR;
      }
      self->typeRef.ptr = ptr;
   }
}

apx_datatype_t *apx_dataElement_getTypeReferencePtr(apx_dataElement_t *self)
{
   if ( (self != 0) && (self->baseType == APX_BASE_TYPE_REF_PTR) )
   {
      return self->typeRef.ptr;
   }
   return (apx_datatype_t*) 0;
}

apx_error_t apx_dataElement_calcPackLen(apx_dataElement_t *self, apx_size_t *packLen)
{
   apx_error_t retval = APX_NO_ERROR;
   if (self != 0)
   {
      self->packLen=0;
      if (self->baseType == APX_BASE_TYPE_RECORD)
      {
         int32_t i;
         int32_t end = adt_ary_length(self->childElements);
         for(i=0;i<end;i++)
         {
            apx_size_t childPackLen = 0;
            apx_dataElement_t *pChildElement = (apx_dataElement_t*) adt_ary_value(self->childElements,i);
            assert (pChildElement != 0);
            retval = apx_dataElement_calcPackLen(pChildElement, &childPackLen);
            self->packLen+=childPackLen;
            if (retval != APX_NO_ERROR)
            {
               break;
            }
         }
      }
      else
      {
         apx_size_t elemLen = 0;
         apx_error_t rc = APX_NO_ERROR;
         switch(self->baseType)
         {
         case APX_BASE_TYPE_NONE:
            break;
         case APX_BASE_TYPE_UINT8:
            elemLen = (uint32_t) sizeof(uint8_t);
            break;
         case APX_BASE_TYPE_UINT16:
            elemLen = (uint32_t) sizeof(uint16_t);
            break;
         case APX_BASE_TYPE_UINT32:
            elemLen = (uint32_t) sizeof(uint32_t);
            break;
         case APX_BASE_TYPE_SINT8:
            elemLen = (uint32_t) sizeof(int8_t);
            break;
         case APX_BASE_TYPE_SINT16:
            elemLen = (uint32_t) sizeof(int16_t);
            break;
         case APX_BASE_TYPE_SINT32:
            elemLen = (uint32_t) sizeof(int32_t);
            break;
         case APX_BASE_TYPE_STRING:
            elemLen = (uint32_t) sizeof(uint8_t);
            break;
         case APX_BASE_TYPE_REF_PTR:
            rc = apx_datatype_calcPackLen(self->typeRef.ptr, &elemLen);
            break;
         default:
            break;
         }
         if ( (elemLen > 0) && (rc == APX_NO_ERROR) )
         {
            if (self->arrayLen > 0)
            {
               if (self->dynLenType != APX_DYN_LEN_NONE)
               {
                  if (self->dynLenType == APX_DYN_LEN_U8)
                  {
                     self->packLen+=UINT8_SIZE;
                  }
                  else if (self->dynLenType == APX_DYN_LEN_U16)
                  {
                     self->packLen+=UINT16_SIZE;
                  }
                  else
                  {
                     self->packLen+=UINT32_SIZE;
                  }
               }
               self->packLen+=elemLen*self->arrayLen;
            }
            else
            {
               self->packLen+=elemLen;
            }
         }
         else
         {
            retval = rc;
         }
      }
      if ( (retval == APX_NO_ERROR) && (packLen != 0) )
      {
         *packLen = self->packLen;
      }
   }
   return retval;
}

apx_size_t apx_dataElement_getPackLen(apx_dataElement_t *self)
{
   if (self != 0)
   {
      return self->packLen;
   }
   return 0u;
}

apx_error_t apx_dataElement_getLastError(apx_dataElement_t *self)
{
   if (self != 0)
   {
      return self->lastError;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_dataElement_appendChild(apx_dataElement_t *self, apx_dataElement_t *child)
{
   if( (self != 0) && (child != 0) && (self->baseType == APX_BASE_TYPE_RECORD))
   {
      if (self->childElements == 0)
      {
         self->childElements = adt_ary_new(apx_dataElement_vdelete);
      }
      adt_ary_push(self->childElements, child);
   }
}

int32_t apx_dataElement_getNumChild(apx_dataElement_t *self)
{
   if ( (self != 0) && (self->childElements != 0) )
   {
      return adt_ary_length(self->childElements);
   }
   return -1;
}

apx_dataElement_t *apx_dataElement_getChildAt(apx_dataElement_t *self, int32_t index)
{
   if ( (self != 0) && (self->childElements != 0) )
   {
      void **ptr = adt_ary_get(self->childElements, index);
      if (ptr != 0)
      {
         return (apx_dataElement_t*) *ptr;
      }
   }
   return 0;
}

dtl_dv_t *apx_dataElement_makeProperInitValueFromDynamicValue(apx_dataElement_t *self, dtl_dv_t *dv, apx_error_t *errorCode)
{
   dtl_dv_t *initValue = (dtl_dv_t*) 0;
   if ( (self != 0) && (dv != 0) && (errorCode != 0))
   {
      switch(self->baseType)
      {
      case APX_BASE_TYPE_UINT8:
         return apx_dataElement_makeU32InitValueFromDynamicValue(self, dv, errorCode);
      case APX_BASE_TYPE_UINT16:
         return apx_dataElement_makeU32InitValueFromDynamicValue(self, dv, errorCode);
      case APX_BASE_TYPE_UINT32:
         return apx_dataElement_makeU32InitValueFromDynamicValue(self, dv, errorCode);
      case APX_BASE_TYPE_SINT8:
         return apx_dataElement_makeS32InitValueFromDynamicValue(self, dv, errorCode);
      case APX_BASE_TYPE_SINT16:
         return apx_dataElement_makeS32InitValueFromDynamicValue(self, dv, errorCode);
      case APX_BASE_TYPE_SINT32:
         return apx_dataElement_makeS32InitValueFromDynamicValue(self, dv, errorCode);
      case APX_BASE_TYPE_STRING:
         return apx_dataElement_makeStringInitValueFromDynamicValue(self, dv, errorCode);
      case APX_BASE_TYPE_RECORD:
         return apx_dataElement_makeHashInitValueFromDynamicValue(self, dv, errorCode);
      case APX_BASE_TYPE_REF_ID:
         *errorCode = APX_ELEMENT_TYPE_ERROR;
         break;
      case APX_BASE_TYPE_REF_NAME:
         *errorCode = APX_ELEMENT_TYPE_ERROR;
         break;
      case APX_BASE_TYPE_REF_PTR:
         *errorCode = APX_ELEMENT_TYPE_ERROR;
         break;
      default:
         *errorCode = APX_NOT_IMPLEMENTED_ERROR;
      }
   }
   return initValue;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_dataElement_calcDynLenType(apx_dataElement_t *self)
{
   apx_error_t retval = APX_NO_ERROR;
   if (self->arrayLen == 0u)
   {
      self->dynLenType=APX_DYN_LEN_NONE;
   }
   if (self->arrayLen <= UINT8_MAX)
   {
      self->dynLenType=APX_DYN_LEN_U8;
   }
   else if (self->arrayLen <= UINT16_MAX)
   {
      self->dynLenType=APX_DYN_LEN_U16;
   }
   else if (self->arrayLen <= UINT32_MAX)
   {
      self->dynLenType=APX_DYN_LEN_U32;
   }
   else
   {
      retval = APX_LENGTH_ERROR;
   }
   return retval;
}

static dtl_dv_t *apx_dataElement_makeU32InitValueFromDynamicValue(apx_dataElement_t *self, dtl_dv_t *dv, apx_error_t *errorCode)
{
   dtl_dv_t *retval = (dtl_dv_t*) 0;
   if ( (self != 0) && (dv != 0) && (errorCode != 0) )
   {
      dtl_sv_t *sv_retval = (dtl_sv_t*) 0;
      dtl_av_t *av_retval = (dtl_av_t*) 0;
      *errorCode = APX_NO_ERROR;
      bool isOk;
      uint32_t u32Value;
      if ( (self->arrayLen == 0) && (dtl_dv_type((dtl_dv_t*) dv) == DTL_DV_SCALAR) )
      {
         dtl_sv_t *sv = (dtl_sv_t*) dv;
         u32Value = dtl_sv_to_u32( sv, &isOk);
         if (isOk)
         {
            sv_retval = dtl_sv_make_u32(u32Value);
            assert(sv_retval != 0);
            retval = (dtl_dv_t*) sv_retval;
            *errorCode = APX_NO_ERROR;
         }
         else
         {
            *errorCode = APX_INIT_VALUE_ERROR;
         }
      }
      else if ( (self->arrayLen > 0) && (dtl_dv_type((dtl_dv_t*) dv) == DTL_DV_ARRAY) )
      {
         dtl_av_t *av = (dtl_av_t*) dv;
         int32_t arrayLen = dtl_av_length(av);
         if ( ((int32_t) self->arrayLen) != arrayLen)
         {
            *errorCode = APX_LENGTH_ERROR;
         }
         else
         {
            av_retval = dtl_av_new();
            if (av_retval == 0)
            {
               *errorCode = APX_MEM_ERROR;
            }
            else
            {
               apx_dataElement_t childElement;
               int32_t i;
               int8_t i8Result;
               i8Result = apx_dataElement_create(&childElement, self->baseType, (const char*) 0);
               assert(i8Result == 0);
               for(i=0; i < arrayLen; i++)
               {
                  apx_error_t childResult = APX_NO_ERROR;
                  dtl_dv_t *childValue;
                  dtl_dv_t *createdValue = (dtl_dv_t*) 0;
                  childValue = dtl_av_value(av, i);
                  if (childValue != 0)
                  {
                     createdValue = apx_dataElement_makeProperInitValueFromDynamicValue(&childElement, childValue, &childResult);
                  }
                  if ( (childValue == 0) || (createdValue == 0) )
                  {
                     *errorCode = childResult;
                     dtl_dec_ref(av_retval);
                     av_retval = (dtl_av_t*) 0;
                     break;
                  }
                  dtl_av_push(av_retval, createdValue, false);
               }
               apx_dataElement_destroy(&childElement);
            }
            retval = (dtl_dv_t*) av_retval;
         }
      }
      else
      {
         *errorCode = APX_INIT_VALUE_ERROR;
      }
   }
   return (dtl_dv_t*) retval;
}

static dtl_dv_t *apx_dataElement_makeS32InitValueFromDynamicValue(apx_dataElement_t *self, dtl_dv_t *dv, apx_error_t *errorCode)
{
   dtl_dv_t *retval = (dtl_dv_t*) 0;
   if ( (self != 0) && (dv != 0) && (errorCode != 0) )
   {
      dtl_sv_t *sv_retval = (dtl_sv_t*) 0;
      dtl_av_t *av_retval = (dtl_av_t*) 0;
      *errorCode = APX_NO_ERROR;
      bool isOk;
      int32_t s32Value;
      if ( (self->arrayLen == 0) && (dtl_dv_type((dtl_dv_t*) dv) == DTL_DV_SCALAR) )
      {
         dtl_sv_t *sv = (dtl_sv_t*) dv;
         s32Value = dtl_sv_to_i32( sv, &isOk);
         if (isOk)
         {
            sv_retval = dtl_sv_make_i32(s32Value);
            assert(sv_retval != 0);
            retval = (dtl_dv_t*) sv_retval;
            *errorCode = APX_NO_ERROR;
         }
         else
         {
            *errorCode = APX_INIT_VALUE_ERROR;
         }
      }
      else if ( (self->arrayLen > 0) && (dtl_dv_type((dtl_dv_t*) dv) == DTL_DV_ARRAY) )
      {
         dtl_av_t *av = (dtl_av_t*) dv;
         int32_t arrayLen = dtl_av_length(av);
         if ( ((int32_t) self->arrayLen) != arrayLen)
         {
            *errorCode = APX_LENGTH_ERROR;
         }
         else
         {
            av_retval = dtl_av_new();
            if (av_retval == 0)
            {
               *errorCode = APX_MEM_ERROR;
            }
            else
            {
               apx_dataElement_t childElement;
               int32_t i;
               int8_t i8Result;
               i8Result = apx_dataElement_create(&childElement, self->baseType, (const char*) 0);
               assert(i8Result == 0);
               for(i=0; i < arrayLen; i++)
               {
                  apx_error_t childResult = APX_NO_ERROR;
                  dtl_dv_t *childValue;
                  dtl_dv_t *createdValue = (dtl_dv_t*) 0;
                  childValue = dtl_av_value(av, i);
                  if (childValue != 0)
                  {
                     createdValue = apx_dataElement_makeProperInitValueFromDynamicValue(&childElement, childValue, &childResult);
                  }
                  if ( (childValue == 0) || (createdValue == 0) )
                  {
                     *errorCode = childResult;
                     dtl_dec_ref(av_retval);
                     av_retval = (dtl_av_t*) 0;
                     break;
                  }
                  dtl_av_push(av_retval, createdValue, false);
               }
               apx_dataElement_destroy(&childElement);
            }
            retval = (dtl_dv_t*) av_retval;
         }

      }
      else
      {
         *errorCode = APX_ELEMENT_TYPE_ERROR;
      }
   }
   return (dtl_dv_t*) retval;
}

static dtl_dv_t *apx_dataElement_makeStringInitValueFromDynamicValue(apx_dataElement_t *self, dtl_dv_t *dv, apx_error_t *errorCode)
{
   dtl_dv_t *retval = (dtl_dv_t*) 0;
   if ( (self != 0) && (dv != 0) && (errorCode != 0) )
   {
      if ( (dtl_dv_type(dv) == DTL_DV_SCALAR) && (dtl_sv_type((dtl_sv_t*) dv) == DTL_SV_STR) )
      {
         adt_str_t *tmp = dtl_sv_to_str((dtl_sv_t*) dv);
         assert(tmp != 0);
         retval = (dtl_dv_t*) dtl_sv_make_str(tmp);
         adt_str_delete(tmp);
      }
      else
      {
         *errorCode = APX_INIT_VALUE_ERROR;
      }
   }
   return retval;
}

static dtl_dv_t *apx_dataElement_makeHashInitValueFromDynamicValue(apx_dataElement_t *self, dtl_dv_t *dv, apx_error_t *errorCode)
{
   dtl_dv_t *retval = (dtl_dv_t*) 0;
   if ( (self != 0) && (dv != 0) && (errorCode != 0) )
   {
      if ( dtl_dv_type(dv) == DTL_DV_ARRAY )
      {
         retval = apx_dataElement_makeHashInitValueFromArray(self, (dtl_av_t*) dv, errorCode);
      }
      else
      {
         *errorCode = APX_INIT_VALUE_ERROR;
      }
   }
   return retval;
}

static dtl_dv_t *apx_dataElement_makeHashInitValueFromArray(apx_dataElement_t *self, dtl_av_t *av, apx_error_t *errorCode)
{
   dtl_hv_t *hv = (dtl_hv_t*) 0;
   if ( (self != 0) && (av != 0) && (errorCode != 0) )
   {
      *errorCode = APX_NO_ERROR;
      if (self->baseType == APX_BASE_TYPE_RECORD)
      {
         assert(self->childElements != 0);
         hv = dtl_hv_new();
         if (hv != 0)
         {
            int32_t numRecordElements;
            int32_t numArrayElements;
            numRecordElements = adt_ary_length(self->childElements);
            numArrayElements = dtl_av_length(av);
            if (numRecordElements == numArrayElements)
            {
               int32_t i;

               for(i=0; i< numRecordElements; i++)
               {
                  apx_error_t childResult = APX_NO_ERROR;
                  dtl_dv_t *initValue = (dtl_dv_t*) 0;
                  dtl_dv_t *arrayElement = dtl_av_value(av, i);
                  apx_dataElement_t *childElement = (apx_dataElement_t*) adt_ary_value(self->childElements, i);
                  assert(arrayElement != 0);
                  assert(childElement != 0);
                  assert(childElement->name != 0);
                  initValue = apx_dataElement_makeProperInitValueFromDynamicValue(childElement, arrayElement, &childResult);
                  if (childResult != APX_NO_ERROR)
                  {
                     *errorCode = childResult;
                     break;
                  }
                  else
                  {
                     assert(initValue != 0);
                     dtl_hv_set_cstr(hv, childElement->name, initValue, false);
                  }
               }
            }
            else
            {
               *errorCode = APX_LENGTH_ERROR;
            }
         }
         else
         {
            *errorCode = APX_MEM_ERROR;
         }
      }
      else
      {
         *errorCode = APX_ELEMENT_TYPE_ERROR;
      }
   }
   if (hv != 0)
   {
      assert(errorCode != 0);
      if (*errorCode != APX_NO_ERROR)
      {
         dtl_dec_ref(hv);
         hv = (dtl_hv_t*) 0;
      }
   }
   return (dtl_dv_t*) hv;
}
