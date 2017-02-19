#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <malloc.h>
#include "apx_datatype.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


/**************** Private Function Declarations *******************/


/**************** Private Variable Declarations *******************/


/****************** Public Function Definitions *******************/
apx_datatype_t* apx_datatype_new(const char *name, const char *dsg, const char *attr)
{
   apx_datatype_t *self = (apx_datatype_t*) malloc(sizeof(apx_datatype_t));
   if(self != 0)
   {
      int8_t result = apx_datatype_create(self,name,dsg,attr);
      if (result != 0)
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

void apx_datatype_delete(apx_datatype_t *self)
{
   if(self != 0){
      apx_datatype_destroy(self);
      free(self);
   }
}

void apx_datatype_vdelete(void *arg)
{
   apx_datatype_delete((apx_datatype_t*) arg);
}

int8_t apx_datatype_create(apx_datatype_t *self, const char *name, const char *dsg, const char *attr)
{
   if (self != 0)
   {
      uint32_t nameLen;
      uint32_t dsgLen;
      uint32_t attrLen;
      uint32_t numNullChars=0;
      char *pNext;
      char *pEnd;

      nameLen = (name==0)? 0 : (uint32_t) strlen(name);
      dsgLen  = (dsg==0)?  0 : (uint32_t) strlen(dsg);
      attrLen = (attr==0)? 0 : (uint32_t) strlen(attr);
      if (nameLen > 0)
      {
         numNullChars++;
      }
      if (dsgLen > 0)
      {
         numNullChars++;
      }
      if (attrLen > 0)
      {
         numNullChars++;
      }
      self->allocLen=nameLen+dsgLen+attrLen+numNullChars;
      self->pAlloc=(char*) malloc(self->allocLen);
      if (self->pAlloc==0)
      {
         errno = ENOMEM;
         return -1;
      }
      pNext=self->pAlloc;
      pEnd=self->pAlloc+self->allocLen;
      if (nameLen > 0)
      {
         self->name=pNext;
         memcpy(pNext,name,nameLen);
         pNext+=nameLen;
         *pNext++='\0';
      }
      if (dsgLen > 0)
      {
         self->dsg=pNext;
         memcpy(pNext,dsg,dsgLen);
         pNext+=dsgLen;
         *pNext++='\0';
      }
      if (attrLen > 0)
      {
         self->attr=pNext;
         memcpy(pNext,attr,attrLen);
         pNext+=attrLen;
         *pNext++='\0';
      }
      assert(pNext==pEnd); //check post-conditions
   }
   return 0;
}

void apx_datatype_destroy(apx_datatype_t *self)
{
   if ( (self!=0) && (self->pAlloc != 0) )
   {
      free(self->pAlloc);
      self->pAlloc=0;
   }
}


/***************** Private Function Definitions *******************/

