#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include "bstr.h"

#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif



/**************** Private Function Declarations *******************/


/**************** Private Variable Declarations *******************/
const int ASCIIHexToInt[256] =
{
    // ASCII
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};


/****************** Public Function Definitions *******************/

/**
 * copies strings like strdup, but instead of searching for NULL-terminator it uses the memory between two provided pointers pBegin and pEnd.
 * Obviously pEnd must be greater than pBegin.
 * returns uint8_t* but can easily be cast to char* by user
 */
uint8_t *bstr_make(const uint8_t *pBegin, const uint8_t *pEnd){
   if( (pBegin != 0) && (pEnd != 0) && (pBegin<pEnd)){
      uint32_t len = (uint32_t) (pEnd-pBegin);
      uint8_t *str = (uint8_t*) malloc(len+1);
      if(str != 0){
         memcpy(str,pBegin,len);
         str[len]=0;
      }
      return str;
   }
   return 0;
}


/**
 * like bstr_make but in addition it adds optional arguments startOffset and endOffset.
 * beginOffset is the number of extra bytes to add before string copy (before pBegin).
 * endOffset is the number of extra bytes to add after string copy (after pEnd)
 */
uint8_t *bstr_make_x(const uint8_t *pBegin, const uint8_t *pEnd, uint16_t beginOffset, uint16_t endOffset){
   if( (pBegin != 0) && (pEnd != 0) && (pBegin)){
      uint8_t *str;
      uint32_t allocLen;
      uint32_t strLen = (uint32_t) (pEnd-pBegin);
      allocLen = strLen+beginOffset+endOffset+1;
      str = (uint8_t*) malloc(allocLen);
      if(str != 0){
         memcpy(str+beginOffset,pBegin,strLen);
         str[allocLen-1]=(uint8_t)0;
      }
      return str;
   }
   return 0;
}

/**
 * scans for \par val between \par pBegin and \par pEnd.
 * On success it returns the pointer to \par val.
 * On failure it returns \par pBegin if not found or NULL if invalid arguments was given
 */
const uint8_t *bstr_searchVal(const uint8_t *pBegin, const uint8_t *pEnd, uint8_t val){
   const uint8_t *pNext = pBegin;
   if (pNext > pEnd)
   {
      return 0; //invalid arguments
   }
   while(pNext < pEnd){
      uint8_t c = *pNext;
      if(c == val){
         return pNext;
      }
      pNext++;
   }
   return pBegin; //val was not found before pEnd was reached
}

/**
 * scans for matching \par left and \par right characters in a string. Used for matching '(' with ')', '[' with, ']' etc.
 * On Success it returns the pointer to \par right.
 * On failure it returns \par pBegin if the scan reached \par pEnd before \par right was found.
 * If it cannot even match \par left on the first character of \par pBegin it returns NULL.
 */
const uint8_t *bstr_matchPair(const uint8_t *pBegin, const uint8_t *pEnd, uint8_t left, uint8_t right, uint8_t escapeChar){
   const uint8_t *pNext = pBegin;
   uint32_t innerLevelCount=0;
   if (pNext < pEnd){
      if (*pNext == left){
         pNext++;
         if (escapeChar != 0){
            uint8_t isEscape = 0;
            while (pNext < pEnd){
               uint8_t c = *pNext;
               if (isEscape != 0){
                  //ignore this char
                  pNext++;
                  isEscape = 0;
                  continue;
               }
               else {
                  if ( c == escapeChar ){
                     isEscape = 1;
                  }
                  else if (c == right){
                     if (innerLevelCount == 0) {
                        return pNext;
                     }
                     else {
                        innerLevelCount--;
                     }
                  }
                  else if ( c == left )
                  {
                     innerLevelCount++;
                  }
               }
               pNext++;
            }
         }
         else{
            while (pNext < pEnd) {
               uint8_t c = *pNext;
               if (c == right){
                  if (innerLevelCount == 0) {
                     return pNext;
                  }
                  else {
                     innerLevelCount--;
                  }
               }
               else if (c == left)
               {
                  innerLevelCount++;
               }
               pNext++;
            }
         }
      }
      else
      {
         return 0; //string does not start with \par left character
      }
   }
   return pBegin;
}

/**
 * \brief compares characters in string bound by pStrBegin and pStrEnd in buffer bound by pBegin and pEnd
 * \param pBegin start of buffer
 * \param pEnd end of buffer
 * \param pStrBegin start of string to be matched
 * \param pStrEnd end of string to matched
 * \return On succes, pointer in buffer where the match stopped. On match failure it returns 0. If pEnd was reached before pStr was fully matched it returns pBegin.
 */
const uint8_t *bstr_matchStr(const uint8_t *pBegin, const uint8_t *pEnd,const uint8_t *pStrBegin, const uint8_t *pStrEnd)
{
   const uint8_t *pNext = pBegin;
   const uint8_t *pStrNext = pStrBegin;
   if ( (pBegin > pEnd) || (pStrBegin > pStrEnd) )
   {
      errno = EINVAL; //invalid arguments
      return 0;
   }
   while(pNext < pEnd){
      if (pStrNext < pStrEnd)
      {
         if (*pNext != *pStrNext)
         {
            return 0; //string did not match
         }
      }
      else
      {
         //All characters in pStr has been successfully matched
         return pNext; //pNext should point to pStrEnd at this point
      }
      pNext++;
      pStrNext++;
   }
   return pBegin; //reached pEnd before pStr was fully matched
}

const uint8_t *bstr_toLong(const uint8_t *pBegin, const uint8_t *pEnd,long *data)
{
   const uint8_t *pResult=bstr_whilePredicate(pBegin, pEnd, bstr_pred_isDigit);
   if (pResult > pBegin)
   {
      int radix=1;
      int i;
      int len=(pResult-pBegin);
      if (len == 0)
      {
         return pBegin;
      }
      for (i=1;i<len;i++)
      {
         radix*=10;
      }
      *data=0;
      for(i=0;i<len;i++)
      {
         *data+=(pBegin[i]-0x30)*radix;
         radix/=10;
      }
   }
   return pResult;
}

const uint8_t *bstr_toUnsignedLong(const uint8_t *pBegin, const uint8_t *pEnd, uint8_t base, unsigned long *data)
{
   const uint8_t *pResult;
   int (*pred_func)(int c) = 0;
   if ( (base == 0) || (base == 10) )
   {
      pred_func = bstr_pred_isDigit;
      base = 10;
   }
   else if (base == 16)
   {
      pred_func = bstr_pred_isHexDigit;
   }
   else
   {
      errno = EINVAL; //unsupported base
      return 0;
   }
   pResult=bstr_whilePredicate(pBegin, pEnd, pred_func);
   if (pResult > pBegin)
   {
      unsigned long radix=1;
      int i;
      int len=(pResult-pBegin);
      unsigned long tmp = 0;
      if (len == 0)
      {
         return pBegin;
      }
      for (i=1;i<len;i++)
      {
         radix*=base;
      }

      if (base == 16)
      {
         for(i=0;i<len;i++)
         {
            int tmp2 = ASCIIHexToInt[pBegin[i]];
            assert(tmp >= 0);
            tmp += tmp2*radix;
            radix/=base;
         }
      }
      else if ( (base == 8) || (base == 10) )
      {
         for(i=0;i<len;i++)
         {
            tmp += (pBegin[i]-0x30)*radix;
            radix/=base;
         }
      }
      else
      {
         assert(0);
      }
      *data = tmp;
   }
   return pResult;
}

/**
 * searches for next line ending '\n'. returns where it encountered the line ending
 */
const uint8_t *bstr_line(const uint8_t *pBegin, const uint8_t *pEnd)
{
   return bstr_searchVal(pBegin, pEnd, (uint8_t) '\n');
}

const uint8_t *bstr_whilePredicate(const uint8_t *pBegin, const uint8_t *pEnd, int (*pred_func)(int c) )
{
   const uint8_t *pNext = pBegin;
   while (pNext < pEnd)
   {
      int c = (int) *pNext;
      if (!pred_func(c)){
         break;
      }
      pNext++;
   }
   return pNext;
}

/**
 * returns true if v is either '\t' or ' '
 */
int bstr_pred_isHorizontalSpace(int c)
{
   return (c == (int) '\t') || (c == (int) ' ');
}

int bstr_pred_isDigit(int c)
{
   return (c >= '0') && (c <= '9');
}

int bstr_pred_isHexDigit(int c)
{
   return ((c >= '0') && (c <= '9') ) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F'));
}

/***************** Private Function Definitions *******************/
