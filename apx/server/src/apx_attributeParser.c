//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
//#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "apx_attributeParser.h"
#include "apx_error.h"
#include "scan.h"
#include <ctype.h>
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
DYN_STATIC const uint8_t* apx_attributeParser_parseSingleAttribute(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd);
DYN_STATIC const uint8_t* apx_attributeParser_parseInitValue(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, dtl_dv_t **ppInitValue);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_attributeParser_create(apx_attributeParser_t *self)
{
   adt_stack_create(&self->stack, dtl_dv_vdelete);
   self->lastError = APX_NO_ERROR;
   self->lastErrorPos=-1;
   self->pErrorNext = 0;
}

void apx_attributeParser_destroy(apx_attributeParser_t *self)
{
   adt_stack_destroy(&self->stack);
}

const uint8_t* apx_attributeParser_parse(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd)
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

      pResult = scan_whilePredicate(pNext, pEnd, pred_isHorizontalSpace);
      if (pResult == pEnd)
      {
         break;
      }
      pNext = pResult;
      pResult = apx_attributeParser_parseSingleAttribute(self, pNext, pEnd);
      if ( (pResult == 0) || (pResult == pNext) )
      {
         break; //an error occured
      }
      pNext = pResult;
   }
   return pNext;
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
DYN_STATIC const uint8_t* apx_attributeParser_parseSingleAttribute(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd)
{
   char c;
   const uint8_t *pNext = pBegin;
   const uint8_t *pResult;
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
         return 0;
      }
      pNext++;
      switch(attribType)
      {
      case APX_ATTRIB_INIT:
         break;
      case APX_ATTRIB_PARAM:
         break;
      case APX_ATTRIB_QUEUE:
         break;
      }
   }
   //return pNext;
   return 0;
}

DYN_STATIC const uint8_t* apx_attributeParser_parseInitValue(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, dtl_dv_t **ppInitValue)
{
   const uint8_t *pResult;
   const uint8_t *pNext = pBegin;
   dtl_dv_t *initValueInternal = (dtl_dv_t*) 0;

   if(pNext < pEnd)
   {
      char c = (char) *pNext;
      if (isdigit(c) != 0)
      {
         unsigned long value;
         dtl_sv_t *sv = dtl_sv_new();
         if (sv == 0)
         {
            self->lastError = APX_MEM_ERROR;
            self->pErrorNext = pNext;
            return 0;
         }
         pResult = scan_toUnsignedLong(pNext, pEnd, &value);
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
               pResult = scan_toLong(pNext, pEnd, &value);
               if ( (pResult == 0) || (pResult == pNext))
               {
                  dtl_sv_delete(sv);
                  return pResult;
               }
               dtl_sv_set_i32(sv, (uint32_t) -value);
               pNext = pResult;
               initValueInternal = (dtl_dv_t*) sv;
            }
         }
      }
      else if(c=='{')
      {
         const uint8_t *pMark1;
         const uint8_t *pMark2;
         dtl_av_t *av = dtl_av_new();
         if (av == 0)
         {
            self->lastError = APX_MEM_ERROR;
            self->pErrorNext = pNext;
            return 0;
         }
         pResult = scan_matchPair(pNext, pEnd, '{', '}', 0);
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
            pResult = scan_whilePredicate(pNext, pEnd, pred_isHorizontalSpace);
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
            pResult = scan_whilePredicate(pNext, pEnd, pred_isHorizontalSpace);
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
