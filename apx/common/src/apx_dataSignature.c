/*****************************************************************************
* \file      apx_dataSignature.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     Data Signature (DSG) container and parser logic
*
* Copyright (c) 2017-2019 Conny Gustafsson
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
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include "apx_cfg.h"
#include "apx_dataSignature.h"
#include "bstr.h"
#include "adt_ary.h"
#include "adt_hash.h"
#include "adt_str.h"
#include "apx_types.h"
#include "apx_datatype.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t parseDataSignature(apx_dataSignature_t *self, const uint8_t *dsg);
static const uint8_t *parseDataElement(apx_dataSignature_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement);
static const uint8_t *parseName(apx_dataSignature_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement);
static const uint8_t *parseType(apx_dataSignature_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement);
static const uint8_t *parseArrayLength(apx_dataSignature_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement);
static const uint8_t *parseLimit(apx_dataSignature_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement);
static const uint8_t *parseTypeReference(apx_dataSignature_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement);
static void clearError(apx_dataSignature_t *self);
static void setError(apx_dataSignature_t *self, apx_error_t errorCode);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_dataSignature_t *apx_dataSignature_new(const char *dsg, apx_error_t *errorCode)
{
   apx_dataSignature_t *self = (apx_dataSignature_t*) malloc(sizeof(apx_dataSignature_t));
   if(self != 0)
   {
      apx_error_t err = apx_dataSignature_create(self,dsg);
      if (err != APX_NO_ERROR)
      {
         free(self);
         self=0;
      }
      if (errorCode != 0)
      {
         *errorCode = err;
      }
   }
   else
   {
      if (errorCode != 0)
      {
         *errorCode = APX_MEM_ERROR;
      }
   }
   return self;
}

void apx_dataSignature_delete(apx_dataSignature_t *self)
{
   if(self != 0)
   {
      apx_dataSignature_destroy(self);
      free(self);
   }
}

void apx_dataSignature_vdelete(void *arg)
{
   apx_dataSignature_delete((apx_dataSignature_t*) arg);
}

apx_error_t apx_dataSignature_create(apx_dataSignature_t *self, const char *dsg)
{
   if (self != 0)
   {
      if (dsg != 0)
      {
         self->raw=STRDUP(dsg);
         if (self->raw == 0)
         {
            errno = ENOMEM;
            return -1;
         }
      }
      else
      {
         self->raw = 0;
      }
      if (self->raw != 0)
      {
         apx_error_t err;
         self->dataElement=apx_dataElement_new(APX_BASE_TYPE_NONE,0);
         err = parseDataSignature(self,(const uint8_t*) self->raw);
         if (err != APX_NO_ERROR)
         {
            apx_dataElement_delete(self->dataElement);
            if (self->raw)
            {
               free(self->raw);
            }
            return err;
         }
      }
      else
      {
         self->dataElement=0;
      }
      self->dsgType = APX_DSG_TYPE_SENDER_RECEIVER; //No support for client/server yet
      self->derived = NULL;
      self->lastError = APX_NO_ERROR;
   }
   return APX_NO_ERROR;
}

void apx_dataSignature_destroy(apx_dataSignature_t *self)
{
   if (self != 0)
   {
      if (self->dataElement != 0)
      {
         apx_dataElement_delete(self->dataElement);
      }
      if (self->raw != 0)
      {
         free(self->raw);
      }
   }
}

apx_error_t apx_dataSignature_calcPackLen(apx_dataSignature_t *self, apx_size_t *packLen)
{
   if (self != 0)
   {
      if (self->dsgType == APX_DSG_TYPE_SENDER_RECEIVER)
      {
         return apx_dataElement_calcPackLen(self->dataElement, packLen);
      }
      else
      {
         return APX_NOT_IMPLEMENTED_ERROR;
      }
   }
   return APX_INVALID_ATTRIBUTE_ERROR;
}

apx_error_t apx_dataSignature_resolveTypes(apx_dataSignature_t *self, struct adt_ary_tag *typeList, struct adt_hash_tag *typeMap)
{
   if (self != 0)
   {
      if (self->dataElement->baseType == APX_BASE_TYPE_REF_ID)
      {
         if (typeList != 0)
         {
            if (self->dataElement->typeRef.id < adt_ary_length(typeList) )
            {
               apx_dataElement_setTypeReferencePtr(self->dataElement, (apx_datatype_t*) adt_ary_value(typeList, self->dataElement->typeRef.id));
               //TODO: Implement support for recursive type lookups here, possibly with circular redundancy check
               return APX_NO_ERROR;
            }
         }
         return APX_INVALID_TYPE_REF_ERROR;
      }
      else if (self->dataElement->baseType == APX_BASE_TYPE_REF_NAME)
      {
         if (typeMap != 0)
         {
            void **ppVal = adt_hash_get(typeMap, self->dataElement->typeRef.name);
            if (ppVal != 0)
            {
               apx_dataElement_setTypeReferencePtr(self->dataElement, (apx_datatype_t*) *ppVal);
               //TODO: Implement support for recursive type lookups here, possibly with circular redundancy check
               return APX_NO_ERROR;
            }
         }
         return APX_INVALID_TYPE_REF_ERROR;
      }
      else
      {
         //MISRA
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

const char *apx_dataSignature_getDerivedString(apx_dataSignature_t *self)
{
   if ( (self != 0) && (self->dataElement != 0) )
   {
      apx_dataElement_t *dataElement = self->dataElement;
      if (dataElement->baseType != APX_BASE_TYPE_NONE)
      {
         if (dataElement->baseType < APX_BASE_TYPE_RECORD)
         {
            return self->raw;
         }
         else if (dataElement->baseType == APX_BASE_TYPE_RECORD)
         {
            int32_t i;
            int32_t numChildElements = apx_dataElement_getNumChild(dataElement);
            for(i=0;i<numChildElements;i++)
            {
               apx_dataElement_t *childElement = apx_dataElement_getChildAt(dataElement, i);
               assert(childElement != 0);
               if ( childElement->baseType == APX_BASE_TYPE_REF_PTR)
               {
                  setError(self, APX_NOT_IMPLEMENTED_ERROR);
                  return (const char*) 0;
               }
            }
            return self->raw;
         }
         else if (dataElement->baseType == APX_BASE_TYPE_REF_PTR)
         {
            apx_dataSignature_t *referencedDsg;
            apx_datatype_t *dataType = dataElement->typeRef.ptr;
            referencedDsg = dataType->dataSignature;
            return apx_dataSignature_getDerivedString(referencedDsg);
         }
         else
         {
            setError(self, APX_DATA_SIGNATURE_ERROR);
            return (const char*) 0;
         }
      }
   }
   setError(self, APX_INVALID_ARGUMENT_ERROR);
   return (const char*) 0;
}

apx_dataElement_t *apx_dataSignature_getDerivedDataElement(apx_dataSignature_t *self)
{
   if ( (self != 0) && (self->dataElement != 0) )
   {
      apx_dataElement_t *dataElement = self->dataElement;
      if (dataElement->baseType != APX_BASE_TYPE_NONE)
      {
         if (dataElement->baseType == APX_BASE_TYPE_REF_PTR)
         {
            apx_datatype_t *dataType = dataElement->typeRef.ptr;
            return apx_dataSignature_getDerivedDataElement(dataType->dataSignature);
         }
         else
         {
            return dataElement;
         }
      }
   }
   return (apx_dataElement_t*) 0;
}

apx_error_t apx_dataSignature_getLastError(apx_dataSignature_t *self)
{
   if (self != 0)
   {
      return self->lastError;
   }
   return APX_INVALID_ARGUMENT_ERROR;

}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
/**
 * returns 0 on success, -1 on error
 */
static apx_error_t parseDataSignature(apx_dataSignature_t *self, const uint8_t *dsg)
{
   const uint8_t *pNext=dsg;
   const uint8_t *pEnd=pNext+strlen((const char*)dsg);
   const uint8_t *pResult;
   char c = (char) *pNext;
   clearError(self);

   if (c =='{')
   {
      const uint8_t *pRecordBegin = pNext;
      const uint8_t *pRecordEnd = bstr_match_pair(pRecordBegin,pEnd,'{','}','\\');
      if (pRecordEnd > pRecordBegin)
      {
         apx_dataElement_create(self->dataElement,APX_BASE_TYPE_RECORD,0);

         pNext = pRecordBegin+1; //point to the first character after '{'
         while (pNext<pRecordEnd)
         {
            apx_dataElement_t *pChildElement;

            pChildElement = apx_dataElement_new(APX_BASE_TYPE_NONE,0);

            pResult = parseDataElement(self, pNext, pRecordEnd, pChildElement);
            if (pResult > pNext)
            {
               adt_ary_push(self->dataElement->childElements,pChildElement);
               pNext=pResult;
            }
            else
            {
               apx_dataElement_delete(pChildElement);
               apx_dataElement_delete(self->dataElement);
               self->dataElement=0;
               return apx_dataSignature_getLastError(self);
            }
         }
      }
      else
      {
         return APX_UNMATCHED_BRACE_ERROR;
      }
   }
   else
   {
      pResult = parseDataElement(self, pNext,pEnd,self->dataElement);
      if (pResult <= pNext)
      {
         return apx_dataSignature_getLastError(self);
      }
   }
   return 0;
}


static const uint8_t *parseDataElement(apx_dataSignature_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement)
{
   const uint8_t *pNext=pBegin;
   //optional name
   pNext = parseName(self, pNext,pEnd,pDataElement);
   if (pNext == 0)
   {
      return NULL;
   }
   //mandatory type byte
   pNext = parseType(self, pNext,pEnd,pDataElement);
   if (pNext == 0)
   {
      return NULL;
   }
   if (pDataElement->baseType==APX_BASE_TYPE_REF_ID)
   {
      pNext = parseTypeReference(self, pNext,pEnd,pDataElement);
      if (pNext == 0)
      {
         return NULL;
      }
   }
   else
   {
      //optional array length [arrayLen]
      pNext = parseArrayLength(self, pNext,pEnd,pDataElement);
      if (pNext == 0)
      {
         return NULL;
      }
      //optional range limit (min,max)
      pNext = parseLimit(self, pNext,pEnd,pDataElement);
      if (pNext == 0)
      {
         return NULL;
      }
   }
   return pNext;
}


static const uint8_t *parseName(apx_dataSignature_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement)
{
   const uint8_t *pNext=pBegin;
   if ( (pBegin == 0) || (pEnd == 0) || (pDataElement == 0) || (pEnd < pBegin) )
   {
      errno = EINVAL;
      return 0;
   }
   if (pNext < pEnd)
   {
      char c = (char) *pNext;
      if(c == '\"')
      {
         const uint8_t *pResult;
         pResult=bstr_match_pair(pNext,pEnd,'"','"','\\');
         if (pResult>pNext)
         {
            uint32_t nameLen=(uint32_t) (pResult-pNext-1);
            assert(*pResult=='"');
            if (nameLen<APX_MAX_NAME_LEN)
            {
               pDataElement->name = (char*) bstr_make_cstr(pNext+1,pResult); //copy string start from the first byte after left '"' until (but not including) the right '"'
            }
            pNext=pResult+1;
         }
         else
         {
            return 0;
         }
      }
   }
   return pNext;
}


static const uint8_t *parseType(apx_dataSignature_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement)
{   
   const uint8_t *pNext;
   char c;
   (void)pEnd;
   if ( (pBegin == 0) || (pEnd == 0) || (pDataElement == 0) || (pEnd < pBegin) )
   {
      errno = EINVAL;
      return 0;
   }
   pNext =pBegin;
   c = (char) *pNext++;
   //check for name (name is optional)
   switch(c)
   {
   case 'C':
      pDataElement->baseType=APX_BASE_TYPE_UINT8;
      break;
   case 'S':
      pDataElement->baseType=APX_BASE_TYPE_UINT16;
      break;
   case 'L':
      pDataElement->baseType=APX_BASE_TYPE_UINT32;
      break;
   case 'T':
      pDataElement->baseType=APX_BASE_TYPE_REF_ID; //assume type ID reference until we have parsed further into the string
      break;
   case 'a':
      pDataElement->baseType=APX_BASE_TYPE_STRING;
      break;
   case 'c':
      pDataElement->baseType=APX_BASE_TYPE_SINT8;
      break;
   case 's':
      pDataElement->baseType=APX_BASE_TYPE_SINT16;
      break;
   case 'l':
      pDataElement->baseType=APX_BASE_TYPE_SINT32;
      break;
   case '{':
      apx_dataElement_initRecordType(pDataElement);
      //TODO: implement child record parsing here
      return NULL;
   default:
      setError(self, APX_ELEMENT_TYPE_ERROR);
      return NULL;
   }
   return pNext;
}

static const uint8_t *parseArrayLength(apx_dataSignature_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement)
{
   const uint8_t *pNext=pBegin;
   if ( (pBegin == 0) || (pEnd == 0) || (pDataElement == 0) || (pEnd < pBegin) )
   {
      errno = EINVAL;
      return 0;
   }
   if (pNext < pEnd)
   {
      char c = (char) *pNext;
      if(c == '[')
      {
         bool isHandled = false;
         const uint8_t *pResult;
         pResult=bstr_match_pair(pNext,pEnd,'[',']','\\');
         if (pResult>pNext)
         {
            long value;
            const uint8_t *pNumStart = pNext+1;
            const uint8_t *pNumResult = bstr_to_long(pNumStart, pResult, &value);
            if (pNumResult > pNumStart)
            {
               pDataElement->arrayLen = (uint32_t) value;
               isHandled = true;
            }
            else if (pNumResult == pNumStart)
            {
               if ( ((pResult-pNumStart) == 1) && (*pNumStart == '*') )
               {
                  apx_dataElement_setDynamicArray(pDataElement);
                  isHandled = true;
               }
            }
            if(isHandled)
            {
               pNext=pResult+1;
            }
            else
            {
               pNext = (const uint8_t*) 0;
            }
         }
         else
         {
            pNext = (const uint8_t*) 0;
         }
      }
   }
   return pNext;
}

static const uint8_t *parseLimit(apx_dataSignature_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement)
{
   const uint8_t *pNext;
   (void) pDataElement;
   if ( (pBegin == 0) || (pEnd == 0) || (pDataElement == 0) || (pEnd < pBegin) )
   {
      errno = EINVAL;
      return 0;
   }
   pNext =pBegin;
   if (pNext < pEnd)
   {
      char c = (char) *pNext;
      if(c == '(')
      {
         const uint8_t *pParenBegin = pNext; //'('
         const uint8_t *pParenEnd; //')'
         pParenEnd=bstr_match_pair(pParenBegin,pEnd,'(',')','\\');
         if (pParenEnd>pParenBegin)
         {
            const uint8_t *pMid;
            pParenBegin++; //move past the first '('
            pMid = bstr_search_val(pParenBegin,pParenEnd,',');
            if (pMid > pParenBegin)
            {
               long min;
               long max;
               uint8_t isParseOK = 0;
               const uint8_t *pResult;
               pResult = bstr_to_long(pParenBegin,pMid,&min);
               if (pResult > pParenBegin)
               {
                  pResult = bstr_to_long(pMid+1,pParenEnd,&max);
                  if (pResult > pMid+1)
                  {
                     isParseOK = 1;
                  }
               }

               if (isParseOK != 0)
               {
                  pNext=pParenEnd+1;
                  switch(pDataElement->baseType)
                  {
                  case APX_BASE_TYPE_NONE:
                     break;
                  case APX_BASE_TYPE_UINT8:
                  case APX_BASE_TYPE_UINT16:
                  case APX_BASE_TYPE_UINT32:
                     pDataElement->lowerLimit.u32 = (uint32_t) min; //TODO: implement support for unsigned parsing of min/max
                     pDataElement->upperLimit.u32 = (uint32_t) max;
                     break;
                  case APX_BASE_TYPE_SINT8:
                  case APX_BASE_TYPE_SINT16:
                  case APX_BASE_TYPE_SINT32:
                     pDataElement->lowerLimit.s32 = min;
                     pDataElement->upperLimit.s32 = max;
                     break;
                  case APX_BASE_TYPE_STRING:
                     break;
                  default:
                     break;
                  }
               }
            }
         }
      }
   }
   return pNext;
}

static const uint8_t *parseTypeReference(apx_dataSignature_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement)
{
   const uint8_t *pNext=pBegin;
   if ( (pBegin == 0) || (pEnd == 0) || (pDataElement == 0) || (pEnd < pBegin) )
   {
      setError(self, APX_INVALID_ARGUMENT_ERROR);
      return NULL;
   }
   if (pNext < pEnd)
   {
      char c = (char) *pNext;
      if(c == '[')
      {
         const uint8_t *pResult;
         pResult=bstr_match_pair(pNext,pEnd,'[',']','\\');
         if (pResult>pNext)
         {
            const uint8_t *pInner = pNext+1;
            if ( (pInner < pResult) && (pInner[0] == '"') )
            {
               const uint8_t *pInnerEnd = bstr_match_pair(pInner, pResult,'"','"','\\');
               if (pInnerEnd > pInner)
               {
                  uint8_t *tmp = (uint8_t*) bstr_make_cstr(pInner+1, pInnerEnd);
                  if (tmp != 0)
                  {
                     apx_dataElement_setTypeReferenceName(pDataElement, (const char*) tmp);
                     free(tmp);
                  }
                  else
                  {
                     setError(self, APX_MEM_ERROR);
                     return NULL;
                  }
               }
               else
               {
                  setError(self, APX_UNMATCHED_STRING_ERROR);
                  return NULL;
               }
            }
            else
            {
               long value;
               const uint8_t *pResult2 = bstr_to_long(pInner, pResult, &value);
               if ( (pInner < pResult) && (pResult2 == pResult) )
               {
                  apx_dataElement_setTypeReferenceId(pDataElement, (int32_t) value);
               }
               else
               {
                  setError(self, APX_INVALID_TYPE_REF_ERROR);
                  return NULL;
               }
            }
            pNext=pResult+1;
         }
         else
         {
            setError(self, APX_UNMATCHED_BRACKET_ERROR);
            return NULL;
         }
      }
      else
      {
         setError(self, APX_EXPECTED_BRACKET_ERROR);
         return NULL;
      }
   }
   return pNext;
}

static void clearError(apx_dataSignature_t *self)
{
   self->lastError = APX_NO_ERROR;
}

static void setError(apx_dataSignature_t *self, apx_error_t errorCode)
{
   self->lastError = errorCode;
}




