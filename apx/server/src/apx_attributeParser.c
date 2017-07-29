//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
//#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "apx_attributeParser.h"
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
   self->errorCode = -1;
}

void apx_attributeParser_destroy(apx_attributeParser_t *self)
{

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
   bool isNegative=false;

   dtl_sv_t *initValue = dtl_sv_new();
   if (initValue == 0)
   {
      return 0;
   }
   if(pNext < pEnd)
   {
      char c = (char) *pNext;
      if (isdigit(c) != 0)
      {
         unsigned long value;
         pResult = scan_toUnsignedLong(pNext, pEnd, &value);
         if ( (pResult == 0) || (pResult == pNext))
         {
            dtl_sv_delete(initValue);
            return pResult;
         }
         dtl_sv_set_u32(initValue, (uint32_t) value);
         pNext = pResult;
      }
      else if(c=='-')
      {
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
                  dtl_sv_delete(initValue);
                  return pResult;
               }
               dtl_sv_set_i32(initValue, (uint32_t) -value);
               pNext = pResult;
            }
         }
      }
   }
   if (ppInitValue != 0)
   {
      *ppInitValue = (dtl_dv_t*) initValue;
   }
   else
   {
      dtl_sv_delete(initValue);
   }
   return pResult;
}
