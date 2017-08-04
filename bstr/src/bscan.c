#include "bscan.h"
#include <ctype.h>
#include <errno.h>

/**************** Private Function Declarations *******************/


/**************** Private Variable Declarations *******************/


/****************** Public Function Definitions *******************/
/**
 * scans for \par val between \par pBegin and \par pEnd.
 * On success it returns the pointer to \par val.
 * On failure it returns \par pBegin if not found or NULL if invalid arguments was given
 */
const uint8_t *bscan_searchVal(const uint8_t *pBegin, const uint8_t *pEnd, uint8_t val){
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
const uint8_t *bscan_matchPair(const uint8_t *pBegin, const uint8_t *pEnd, uint8_t left, uint8_t right, uint8_t escapeChar){
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
const uint8_t *bscan_matchStr(const uint8_t *pBegin, const uint8_t *pEnd,const uint8_t *pStrBegin, const uint8_t *pStrEnd)
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

const uint8_t *tscan_digit(const uint8_t *pBegin, const uint8_t *pEnd)
{
   const uint8_t *pNext = pBegin;
   while (pNext < pEnd)
   {
      int c = (int) *pNext;
      if (!isdigit(c)){
         break;
      }
      pNext++;
   }
   return pNext;
}

const uint8_t *bscan_toLong(const uint8_t *pBegin, const uint8_t *pEnd,long *data)
{
   const uint8_t *pResult=tscan_digit(pBegin,pEnd);
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

const uint8_t *bscan_toUnsignedLong(const uint8_t *pBegin, const uint8_t *pEnd, unsigned long *data)
{
   const uint8_t *pResult=tscan_digit(pBegin,pEnd);
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

/**
 * searches for next line ending '\n'. returns where it encountered the line ending
 */
const uint8_t *bscan_line(const uint8_t *pBegin, const uint8_t *pEnd)
{
   return bscan_searchVal(pBegin, pEnd, (uint8_t) '\n');
}

const uint8_t *bscan_whilePredicate(const uint8_t *pBegin, const uint8_t *pEnd, int (*pred_func)(int c) )
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
int bscan_pred_isHorizontalSpace(int c)
{
   if ( (c == (int) '\t') || (c == (int) ' ') )
   {
      return (int) 1;
   }
   return (int) 0;
}


/***************** Private Function Definitions *******************/
