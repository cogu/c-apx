#include <errno.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "apx_dataElement.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

#ifdef _MSC_VER
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif


/**************** Private Function Declarations *******************/


/**************** Private Variable Declarations *******************/


/****************** Public Function Definitions *******************/
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
      self->arrayLen=0;
      self->min.s32=0;
      self->max.s32=0;
      self->packLen=0;
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
   }
}

void apx_dataElement_initRecord(apx_dataElement_t *self)
{
   self->baseType=APX_BASE_TYPE_RECORD;
   self->childElements = adt_ary_new(apx_dataElement_vdelete);
}



/***************** Private Function Definitions *******************/




