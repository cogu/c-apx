/*****************************************************************************
* \file      apx_port.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX port class
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
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include "apx_port.h"
#include "apx_error.h"
#include "apx_types.h"
#include "adt_ary.h"
#include "adt_hash.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_MAX_PORT_SIG_LEN 1024
#define APX_PORT_OVERHEAD_LEN 3 //Two "-characters plus 1 NULL-terminator

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_port_create(apx_port_t *self, apx_portType_t portType, const char *name, const char* dataSignature, const char *attributes, int32_t lineNumber){
   if( (self != 0) && (dataSignature != 0) ){
      apx_error_t error;
      self->name = (name != 0)? STRDUP(name) : 0;
      self->portType = portType;
      self->derivedPortSignature = (char*) 0;
      self->portId = -1;
      self->lineNumber = lineNumber;
      error = apx_dataSignature_create(&self->dataSignature, dataSignature);
      if (error != APX_NO_ERROR)
      {
         if (self->name != 0)
         {
            free(self->name);
         }
         return error;
      }
      if (attributes != 0)
      {
         self->portAttributes = apx_portAttributes_new(attributes);
      }
      else
      {
         self->portAttributes = (apx_portAttributes_t*) 0;
      }
      return APX_NO_ERROR;
   }
   errno = EINVAL;
   return -1;
}

void apx_port_destroy(apx_port_t *self){
   if(self != 0){
      if (self->name != 0)
      {
         free(self->name);
      }
      if (self->portAttributes != 0)
      {
         apx_portAttributes_delete(self->portAttributes);
      }
      if (self->derivedPortSignature != 0)
      {
         free(self->derivedPortSignature);
      }
      apx_dataSignature_destroy(&self->dataSignature);
   }
}

apx_port_t* apx_providePort_new(const char *name, const char* dataSignature, const char *attributes, int32_t lineNumber, apx_error_t *errorCode)
{
   apx_port_t *self = (apx_port_t*) malloc(sizeof(apx_port_t));
   if(self != 0)
   {
      apx_error_t result = apx_port_create(self, APX_PROVIDE_PORT, name, dataSignature, attributes, lineNumber);
      if (result != APX_NO_ERROR)
      {
         free(self);
         self = 0;
      }
      if (errorCode != 0)
      {
         *errorCode = result;
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

apx_port_t* apx_requirePort_new(const char *name, const char* dataSignature, const char *attributes, int32_t lineNumber, apx_error_t *errorCode)
{
   apx_port_t *self = malloc(sizeof(apx_port_t));
   if(self != 0)
   {
      apx_error_t result = apx_port_create(self, APX_REQUIRE_PORT, name, dataSignature, attributes, lineNumber);
      if (result != APX_NO_ERROR)
      {
         free(self);
         self = 0;
      }
      if (errorCode != 0)
      {
         *errorCode = result;
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

void apx_port_delete(apx_port_t *self)
{
   if (self != 0)
   {
      apx_port_destroy(self);
      free(self);
   }
}

void apx_port_vdelete(void *arg){
   apx_port_delete((apx_port_t*) arg);
}

apx_error_t apx_port_resolveTypes(apx_port_t *self, struct adt_ary_tag *typeList, struct adt_hash_tag *typeMap)
{
   if (self != 0)
   {
      return apx_dataSignature_resolveTypes(&self->dataSignature, typeList, typeMap);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/**
 * creates a port signature string for this port of the form:
 * "{port_name}"{dsg}
 * the string is stored in self->self->derivedPortSignature
 */
apx_error_t apx_port_updateDerivedPortSignature(apx_port_t *self)
{
   if (self != 0)
   {
      uint32_t namelen=0;
      uint32_t derivedDsgLen=0;
      const char *derivedDsg=0;


      if (self->derivedPortSignature != 0)
      {
         free(self->derivedPortSignature);
         self->derivedPortSignature = 0;
      }

      if (self->name != 0)
      {
         namelen = (uint32_t) strlen(self->name);
      }
      derivedDsg = apx_dataSignature_getDerivedString(&self->dataSignature);
      if (derivedDsg != 0)
      {
         derivedDsgLen = (uint32_t) strlen(derivedDsg);
         if ( (namelen > 0) && (derivedDsgLen > 0) )
         {
            uint32_t psgLen=namelen+derivedDsgLen+APX_PORT_OVERHEAD_LEN; //add 3 to fit null-terminator + 2 '"' characters
            self->derivedPortSignature = (char*) malloc(psgLen);
            if (self->derivedPortSignature != 0)
            {
               char *p = self->derivedPortSignature;
               *p++='"';
               memcpy(p,self->name,namelen); p+=namelen;
               *p++='"';
               memcpy(p, derivedDsg, derivedDsgLen); p+=derivedDsgLen;
               *p++='\0';
               assert(p == self->derivedPortSignature+psgLen);
               return APX_NO_ERROR;
            }
         }
         return APX_NOT_IMPLEMENTED_ERROR;
      }
   }
   else
   {
      return APX_DATA_SIGNATURE_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_port_updatePackLen(apx_port_t *self)
{
   if (self != 0)
   {
      if(self->dataSignature.dsgType == APX_DSG_TYPE_SENDER_RECEIVER)
      {
          int32_t result = apx_dataSignature_calcPackLen(&self->dataSignature);
          if (result < 0)
          {
             return APX_DATA_SIGNATURE_ERROR;
          }
          else
          {
             return APX_NO_ERROR;
          }
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

const char *apx_port_getDerivedPortSignature(apx_port_t *self)
{
   if (self != 0)
   {
      if (self->derivedPortSignature == 0)
      {
         apx_error_t result = apx_port_updateDerivedPortSignature(self);
         if (result != APX_NO_ERROR)
         {
            return (const char*) 0;
         }
      }
      assert(self->derivedPortSignature != 0);
      return self->derivedPortSignature;
   }
   return 0;
}


int32_t apx_port_getPackLen(apx_port_t *self)
{
   if (self != 0)
   {
      if(self->dataSignature.dsgType == APX_DSG_TYPE_SENDER_RECEIVER)
      {
         return apx_dataSignature_getPackLen(&self->dataSignature);
      }
   }
   return -1;
}


void apx_port_setPortId(apx_port_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId>=0) )
   {
      self->portId=portId;
   }
}

apx_portId_t  apx_port_getPortId(apx_port_t *self)
{
   if (self != 0)
   {
      return self->portId;
   }
   return -1;
}
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


