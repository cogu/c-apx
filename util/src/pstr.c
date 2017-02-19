#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include "pstr.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif



/**************** Private Function Declarations *******************/


/**************** Private Variable Declarations *******************/


/****************** Public Function Definitions *******************/

/**
 * copies strings like strdup, but instead of searching for NULL-terminator it uses the memory between two provided pointers pBegin and pEnd.
 * Obviously pEnd must be greater than pBegin.
 * returns uint8_t* but can easily be cast to char* by user
 */
uint8_t *pstr_make(const uint8_t *pBegin, const uint8_t *pEnd){
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
 * like pstr_make but in addition it adds optional arguments startOffset and endOffset.
 * startOffset is the number of extra bytes to add before string copy (before pBegin).
 * endOffset is the number of extra bytes to add after string copy (after pEnd)
 */
uint8_t *pstr_make_x(const uint8_t *pBegin, const uint8_t *pEnd, uint16_t startOffset, uint16_t endOffset){
   if( (pBegin != 0) && (pEnd != 0) && (pBegin)){
      uint8_t *str;
      uint32_t allocLen;
      uint32_t strLen = (uint32_t) (pEnd-pBegin);
      allocLen = strLen+startOffset+endOffset+1;
      str = (uint8_t*) malloc(allocLen);
      if(str != 0){
         memcpy(str+startOffset,pBegin,strLen);
         str[allocLen-1]=(uint8_t)0;
      }
      return str;
   }
   return 0;
}


/***************** Private Function Definitions *******************/
