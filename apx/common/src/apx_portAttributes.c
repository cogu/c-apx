//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <assert.h>
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif
#include "apx_portAttributes.h"



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int8_t apx_portAttributes_create(apx_portAttributes_t *self, const char *attributeString)
{
   if (self != 0)
   {
      self->isFinalized = false;
      self->isParameter = false;
      self->isQueued = false;
      self->queueLen = -1;
      self->initValue = 0;
      self->rawValue = 0;
      if (attributeString != 0)
      {
         self->rawValue = STRDUP(attributeString);
         if (self->rawValue == 0)
         {
            errno = ENOMEM;
            return -1;
         }
      }
      return 0;
   }
   errno = EINVAL;
   return -1;
}

void apx_portAttributes_destroy(apx_portAttributes_t *self)
{
   if (self != 0)
   {
      if (self->rawValue != 0)
      {
         free(self->rawValue);
      }
      if (self->initValue != 0)
      {
         dtl_dv_delete(self->initValue);
      }
   }
}

apx_portAttributes_t* apx_portAttributes_new(const char *attr)
{
   apx_portAttributes_t *self = 0;
   self = (apx_portAttributes_t*) malloc(sizeof(apx_portAttributes_t));
   if (self != 0)
   {
      int8_t result = apx_portAttributes_create(self, attr);
      if (result < 0)
      {
         free(self);
         return (apx_portAttributes_t*) 0;
      }
   }
   else
   {
      errno = ENOMEM;
   }
   return self;
}

void apx_portAttributes_delete(apx_portAttributes_t *self)
{
   if (self != 0)
   {
      apx_portAttributes_destroy(self);
      free(self);
   }
}

void apx_portAttributes_vdelete(void *arg)
{
   apx_portAttributes_delete((apx_portAttributes_t*) arg);
}

void apx_portAttributes_clearInitValue(apx_portAttributes_t *self)
{
   if ( (self != 0) && (self->initValue != 0) )
   {
      dtl_dv_delete(self->initValue);
      self->initValue = 0;
   }
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


