//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
//#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "apx_attributeParser.h"
#include "bstr.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_ATTRIB_INIT    0
#define APX_ATTRIB_PARAM   1
#define APX_ATTRIB_QUEUE   2
//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
DYN_STATIC const uint8_t* apx_attributeParser_parseSingleAttribute(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_portAttributes_t *attr);
DYN_STATIC const uint8_t* apx_attributeParser_parseInitValue(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, dtl_dv_t **ppInitValue);
DYN_STATIC const uint8_t* apx_attributeParser_parseQueueLength(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_portAttributes_t *attr);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_attributeParser_create(apx_attributeParser_t *self)
{
   self->lastError = APX_NO_ERROR;
   self->lastErrorPos=-1;
   self->pErrorNext = 0;
}

void apx_attributeParser_destroy(apx_attributeParser_t *self)
{
   //nothing to do
}

/**
 * Convenience function for calling apx_attributeParser_parse.
 * returns true on success, false on failure
 */
bool apx_attributeParser_parseObject(apx_attributeParser_t *self, apx_portAttributes_t *attributeObject)
{
   if ( (self != 0 ) && (attributeObject != 0) && (attributeObject->rawValue != 0) )
   {
      const uint8_t *pBegin;
      const uint8_t *pEnd;
      const uint8_t *pResult;
      pBegin = (const uint8_t*) attributeObject->rawValue;
      pEnd = pBegin + strlen(attributeObject->rawValue);
      pResult = apx_attributeParser_parse(self, pBegin, pEnd, attributeObject);
      if ( pResult == pEnd)
      {
         //success only if entire string was parse.
         //if error occurs (i.e. this function returns false, get the error using the apx_attributeParser_getLastError function)
         return true;
      }
   }
   return false;
}

const uint8_t* apx_attributeParser_parse(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_portAttributes_t *attr)
{
   const uint8_t *pNext = pBegin;
   const uint8_t *pResult;
   if ((pEnd < pBegin) || (pBegin==0) || (pEnd == 0))
   {
      errno = EINVAL;
      return 0;
   }
   while(pNext < pEnd)
   {

      pResult = bstr_whilePredicate(pNext, pEnd, bstr_pred_isHorizontalSpace);
      if (pResult == pEnd)
      {
         break;
      }
      pNext = pResult;
      pResult = apx_attributeParser_parseSingleAttribute(self, pNext, pEnd, attr);
      if ( (pResult == 0) || (pResult == pNext) )
      {
         pNext = 0; //an error occured
         break;
      }
      pNext = pResult;
      if (pNext < pEnd)
      {
         pResult = bstr_whilePredicate(pNext, pEnd, bstr_pred_isHorizontalSpace);
         if (pResult == pEnd)
         {
            break;
         }
         pNext = pResult;
         if ( (char) *pNext ==','){
            pNext++;
         }
         else
         {
            self->lastError = APX_PARSE_ERROR;
            self->pErrorNext = pNext;
            return 0;
         }
      }
   }
   return pNext;
}

int32_t apx_attributeParser_getLastError(apx_attributeParser_t *self, const uint8_t **ppNext)
{
   if (self != 0)
   {
      if (ppNext != 0)
      {
         *ppNext = self->pErrorNext;
      }
      return self->lastError;
   }
   errno = EINVAL;
   return -1;
}



//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
/**
 * parses a single attribute
 * An valid APX attribute starts with either:
 * An equals sign (=): denotes the start of an init value
 * Letter P: Applies the parameter property to the port
 * Letter Q: Applies the queued property to the port
 */
DYN_STATIC const uint8_t* apx_attributeParser_parseSingleAttribute(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_portAttributes_t *attr)
{
   char c;
   const uint8_t *pNext = pBegin;
   const uint8_t *pResult = 0;
   uint8_t attribType;
   if (pNext < pEnd)
   {
      c = (char) *pNext;
      switch(c)
      {
      case '=':
         attribType = APX_ATTRIB_INIT;
         break;
      case 'P':
         attribType = APX_ATTRIB_PARAM;
         break;
      case 'Q':
         attribType = APX_ATTRIB_QUEUE;
         break;
      default:
         self->lastError = APX_PARSE_ERROR;
         self->pErrorNext = pNext;
         return 0;
      }
      pNext++;
      switch(attribType)
      {
      case APX_ATTRIB_INIT:
         apx_portAttributes_clearInitValue(attr);
         pResult = apx_attributeParser_parseInitValue(self, pNext, pEnd, &attr->initValue);
         if ( (pResult == 0) || (pResult == pNext) )
         {
            self->lastError = APX_PARSE_ERROR;
            self->pErrorNext = pNext;
            return 0;
         }
         pNext = pResult;
         break;
      case APX_ATTRIB_PARAM:
         attr->isParameter = true;
         break;
      case APX_ATTRIB_QUEUE:
         attr->isQueued = true;
         pResult = apx_attributeParser_parseQueueLength(self, pNext, pEnd, attr);
         if ( (pResult == 0) || (pResult == pNext) )
         {
            self->lastError = APX_PARSE_ERROR;
            self->pErrorNext = pNext;
            return 0;
         }
         pNext = pResult;
         break;
      }
   }
   return pNext;
}

DYN_STATIC const uint8_t* apx_attributeParser_parseInitValue(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, dtl_dv_t **ppInitValue)
{
   if ( (self != 0) && (ppInitValue != 0) && (pBegin != 0) && (pEnd != 0) && (pBegin <= pEnd) )
   {
      const uint8_t *pResult;
      const uint8_t *pNext = pBegin;
      dtl_dv_t *initValueInternal = (dtl_dv_t*) 0;

      if(pNext < pEnd)
      {
         char c = (char) *pNext;
         if (isdigit(c) != 0)
         {
            uint8_t base = 10;
            //integer, check to see if the string starts with "0x" in which case we shall interpret the string as hex
            if ( (pNext+1 < pEnd) && (pNext[0] == '0') && (pNext[1] == 'x'))
            {
               pNext+=2;
               base = 16;
               if (pNext >= pEnd)
               {
                  self->lastError = APX_PARSE_ERROR;
                  self->pErrorNext = pNext;
                  return 0;
               }
            }
            unsigned long value;
            dtl_sv_t *sv = dtl_sv_new();
            if (sv == 0)
            {
               self->lastError = APX_MEM_ERROR;
               self->pErrorNext = pNext;
               return 0;
            }
            pResult = bstr_toUnsignedLong(pNext, pEnd, base, &value);
            if ( (pResult == 0) || (pResult == pNext))
            {
               dtl_sv_delete(sv);
               return pResult;
            }
            dtl_sv_set_u32(sv, (uint32_t) value);
            pNext = pResult;
            initValueInternal = (dtl_dv_t*) sv;

         }
         else if(c=='-')
         {
            //negative integer
            dtl_sv_t *sv = dtl_sv_new();
            if (sv == 0)
            {
               self->lastError = APX_MEM_ERROR;
               self->pErrorNext = pNext;
               return 0;
            }
            ++pNext;
            if(pNext < pEnd)
            {
               c = (char) *pNext;
               if (isdigit(c) != 0)
               {
                  long value;
                  pResult = bstr_toLong(pNext, pEnd, &value);
                  if ( (pResult == 0) || (pResult == pNext))
                  {
                     dtl_sv_delete(sv);
                     self->lastError = APX_PARSE_ERROR;
                     self->pErrorNext = pNext;
                     return 0;
                  }
                  dtl_sv_set_i32(sv, (uint32_t) -value);
                  pNext = pResult;
                  initValueInternal = (dtl_dv_t*) sv;
               }
               else
               {
                  dtl_sv_delete(sv);
                  self->lastError = APX_PARSE_ERROR;
                  self->pErrorNext = pNext;
                  return 0;
               }
            }
         }
         else if(c=='{')
         {
            //array literal
            const uint8_t *pMark1;
            const uint8_t *pMark2;
            dtl_av_t *av = dtl_av_new();
            if (av == 0)
            {
               self->lastError = APX_MEM_ERROR;
               self->pErrorNext = pNext;
               return 0;
            }
            pResult = bstr_matchPair(pNext, pEnd, '{', '}', 0);
            if ( (pResult == 0) || (pResult == pNext) )
            {
               //failed to parse
               dtl_av_delete(av);
               self->lastError = APX_PARSE_ERROR;
               self->pErrorNext = pNext;
               return 0;
            }
            assert(*pResult == '}');
            pMark1 = pEnd; //remember where old pEnd was
            pMark2 = pResult+1; //remember where old pResult was (and skip the '}' character)
            pEnd = pResult; //set new pEnd pointer
            pNext++;
            while(pNext < pEnd)
            {
               dtl_dv_t *dv = 0;
               //search for optional white-space followed by a value followed by optional whitespace followed by optional comma
               pResult = bstr_whilePredicate(pNext, pEnd, bstr_pred_isHorizontalSpace);
               if (pResult == pEnd)
               {
                  break;
               }
               pNext = pResult;
               pResult = apx_attributeParser_parseInitValue(self, pNext, pEnd, &dv);
               if ( (pResult == 0) || (pResult == pNext) )
               {
                  //failed to parse
                  dtl_av_delete(av);
                  self->lastError = APX_PARSE_ERROR;
                  self->pErrorNext = pNext;
                  return 0;
               }
               if (dv != 0)
               {
                  dtl_av_push(av, dv);
               }
               pNext = pResult;
               pResult = bstr_whilePredicate(pNext, pEnd, bstr_pred_isHorizontalSpace);
               if (pResult == pEnd)
               {
                  break;
               }
               pNext = pResult;
               c = (char) *pNext;
               if (c==','){
                  pNext++;
               }
               else
               {
                  dtl_av_delete(av);
                  self->lastError = APX_PARSE_ERROR;
                  self->pErrorNext = pNext;
                  return 0;
               }
            }
            pEnd = pMark1;
            pResult = pMark2;
            initValueInternal = (dtl_dv_t*) av;
         }
         else if(c == '"')
         {
            //string literal
            dtl_sv_t *sv = dtl_sv_new();
            if (sv == 0)
            {
               self->lastError = APX_MEM_ERROR;
               self->pErrorNext = pNext;
               return 0;
            }
            pResult = bstr_matchPair(pNext, pEnd, '"', '"', '\\');
            if ( (pResult == 0) || (pResult == pNext) )
            {
               //failed to parse
               dtl_sv_delete(sv);
               self->lastError = APX_PARSE_ERROR;
               self->pErrorNext = pNext;
               return 0;
            }
            assert(*pResult == '"');
            dtl_sv_set_bstr(sv, (const char*)pNext+1, (const char*) pResult);
            pResult++; //move cursor past the '"' character
            initValueInternal = (dtl_dv_t*) sv;
         }
         else
         {
            //failed to parse
            self->lastError = APX_PARSE_ERROR;
            self->pErrorNext = pNext;
            return 0;
         }
      }
      if (initValueInternal != 0)
      {
         if (ppInitValue != 0)
         {
            *ppInitValue = (dtl_dv_t*) initValueInternal;
         }
         else
         {
            dtl_dv_delete(initValueInternal);
         }
      }
      return pResult;
   }
   errno = EINVAL;
   return 0;
}

DYN_STATIC const uint8_t* apx_attributeParser_parseQueueLength(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_portAttributes_t *attr)
{
   if ( (self != 0) && (attr != 0) && (pBegin != 0) && (pEnd != 0) && (pBegin <= pEnd) )
   {
      const uint8_t *pNext = pBegin;
      if (pBegin < pEnd)
      {
         const uint8_t *pResult;
         const uint8_t *pMark;
         pResult = bstr_matchPair(pNext, pEnd, '[', ']', 0);
         if ( (pResult == 0) || (pResult == pNext) )
         {
            self->lastError = APX_PARSE_ERROR;
            self->pErrorNext = pNext;
            return 0;
         }
         assert(*pResult == ']');
         pMark = pResult+1;
         pNext++;
         //OK, now parse whatever is inside the brackets
         if (pNext < pResult)
         {
            long value;
            pResult = bstr_toLong(pNext, pResult, &value);
            if ( (pResult == 0) || (pResult == pNext) )
            {
               self->lastError = APX_PARSE_ERROR;
               self->pErrorNext = pNext;
               return 0;
            }
            //check validity of queue length value
            if (value > 0)
            {
               attr->queueLen = (int32_t) value;
            }
            else
            {
               self->lastError = APX_VALUE_ERROR;
               self->pErrorNext = pNext;
               return 0;
            }
         }
         else
         {
            self->lastError = APX_PARSE_ERROR;
            self->pErrorNext = pNext;
            return 0;
         }
         return pMark;
      }
   }
   errno = EINVAL;
   return 0;
}

