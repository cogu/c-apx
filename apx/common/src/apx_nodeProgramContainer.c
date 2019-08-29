/*****************************************************************************
* \file      apx_nodeProgramContainer.c
* \author    Conny Gustafsson
* \date      2019-08-27
* \brief     Description
*
* Copyright (c) 2019 Conny Gustafsson
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
#include <assert.h>
#include <string.h>
#include "apx_compiler.h"
#include "apx_nodeProgramContainer.h"
#include "apx_compiler.h"
#include <malloc.h>
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_nodeProgramContainer_clearPackPrograms(apx_nodeProgramContainer_t *self);
static void apx_nodeProgramContainer_clearUnpackPrograms(apx_nodeProgramContainer_t *self);
//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_nodeProgramContainer_create(apx_nodeProgramContainer_t *self)
{
   if (self != 0)
   {
      self->requirePortUnpackPrograms = (adt_bytes_t**) 0;
      self->providePortUnpackPrograms = (adt_bytes_t**) 0;
      self->requirePortPackPrograms = (adt_bytes_t**) 0;
      self->providePortPackPrograms = (adt_bytes_t**) 0;
      self->numProvidePorts = 0;
      self->numRequirePorts = 0;
   }
}

void apx_nodeProgramContainer_destroy(apx_nodeProgramContainer_t *self)
{
   if(self != 0)
   {
      apx_nodeProgramContainer_clearPackPrograms(self);
      apx_nodeProgramContainer_clearUnpackPrograms(self);
   }
}

apx_nodeProgramContainer_t* apx_nodeProgramContainer_new(void)
{
   apx_nodeProgramContainer_t *self = (apx_nodeProgramContainer_t*) malloc(sizeof(apx_nodeProgramContainer_t));
   if (self != 0)
   {
      apx_nodeProgramContainer_create(self);
   }
   return self;
}

void apx_nodeProgramContainer_delete(apx_nodeProgramContainer_t *self)
{
   if (self != 0)
   {
      apx_nodeProgramContainer_destroy(self);
      free(self);
   }
}

apx_error_t apx_nodeProgramContainer_compilePackPrograms(apx_nodeProgramContainer_t *self, apx_node_t *node)
{
   apx_error_t retval = APX_NO_ERROR;
   if ( (self != 0) && (node != 0) )
   {
      int32_t portIndex;
      apx_compiler_t compiler;
      adt_bytearray_t program;
      self->numRequirePorts = apx_node_getNumRequirePorts(node);
      self->numProvidePorts = apx_node_getNumProvidePorts(node);
      apx_compiler_create(&compiler);
      if ( (self->requirePortPackPrograms != 0) || (self->providePortPackPrograms != 0) )
      {
         apx_nodeProgramContainer_clearPackPrograms(self);
      }
      adt_bytearray_create(&program, APX_PROGRAM_GROW_SIZE);

      if ( (retval == APX_NO_ERROR) && (self->numRequirePorts > 0) )
      {
         size_t numBytes = self->numRequirePorts * sizeof(adt_bytes_t*);
         self->requirePortPackPrograms = (adt_bytes_t**) malloc( numBytes );
         if (self->requirePortPackPrograms == 0)
         {
            retval = APX_MEM_ERROR;
         }
         else
         {
            //We need to NULLIFY this memory to prevent accidental free/delete during cleanup
            memset(self->requirePortPackPrograms, 0, numBytes);
         }
         if (retval == APX_NO_ERROR)
         {
            for(portIndex=0; portIndex < self->numRequirePorts; portIndex++)
            {
               apx_dataElement_t *dataElement;
               apx_error_t compilationResult;
               apx_port_t *port = apx_node_getRequirePort(node, portIndex);
               assert(port != 0);
               dataElement = apx_port_getDerivedDataElement(port);
               assert(dataElement != 0);

               adt_bytearray_clear(&program);
               apx_compiler_begin_packProgram(&compiler, &program);
               compilationResult = apx_compiler_compilePackDataElement(&compiler, dataElement);
               if (compilationResult == APX_NO_ERROR)
               {
                  self->requirePortPackPrograms[portIndex] = adt_bytearray_bytes(&program);
                  if (self->requirePortPackPrograms[portIndex] == 0)
                  {
                     retval = APX_MEM_ERROR;
                     break;
                  }
               }
               else
               {
                  retval = compilationResult;
                  break;
               }
            }
         }
      }
      if ( (retval == APX_NO_ERROR) && (self->numProvidePorts > 0) )
      {
         size_t numBytes = self->numProvidePorts * sizeof(adt_bytes_t*);
         self->providePortPackPrograms = (adt_bytes_t**) malloc( numBytes );
         if (self->providePortPackPrograms == 0)
         {
            retval = APX_MEM_ERROR;
         }
         else
         {
            //We need to NULLIFY this memory to prevent accidental free/delete during cleanup
            memset(self->providePortPackPrograms, 0, numBytes);
         }
         if (retval == APX_NO_ERROR)
         {
            for(portIndex=0; portIndex < self->numProvidePorts; portIndex++)
            {
               apx_dataElement_t *dataElement;
               apx_error_t compilationResult;
               apx_port_t *port = apx_node_getProvidePort(node, portIndex);
               assert(port != 0);
               dataElement = apx_port_getDerivedDataElement(port);
               assert(dataElement != 0);

               adt_bytearray_clear(&program);
               apx_compiler_begin_packProgram(&compiler, &program);
               compilationResult = apx_compiler_compilePackDataElement(&compiler, dataElement);
               if (compilationResult == APX_NO_ERROR)
               {
                  self->providePortPackPrograms[portIndex] = adt_bytearray_bytes(&program);
                  if (self->providePortPackPrograms[portIndex] == 0)
                  {
                     retval = APX_MEM_ERROR;
                     break;
                  }
               }
               else
               {
                  retval = compilationResult;
                  break;
               }
            }
         }
      }
      apx_compiler_destroy(&compiler);
      adt_bytearray_destroy(&program);
      if (retval != APX_NO_ERROR)
      {
         apx_nodeProgramContainer_clearPackPrograms(self);
      }
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

apx_error_t apx_nodeProgramContainer_compileUnpackPrograms(apx_nodeProgramContainer_t *self, apx_node_t *node)
{
   if ( (self != 0) && (node != 0) )
   {
      return APX_NOT_IMPLEMENTED_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

adt_bytes_t* apx_nodeProgramContainer_getRequirePortPackProgram(apx_nodeProgramContainer_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId < self->numRequirePorts) && (self->requirePortPackPrograms != 0) )
   {
      return self->requirePortPackPrograms[portId];
   }
   return (adt_bytes_t*) 0;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void apx_nodeProgramContainer_clearPackPrograms(apx_nodeProgramContainer_t *self)
{
   int32_t i;
   adt_bytes_t *program;
   if ( (self->numRequirePorts > 0) && ( self->requirePortPackPrograms != 0) )
   {
      for(i=0;i<self->numRequirePorts;i++)
      {
         program = self->requirePortPackPrograms[i];
         if (program != 0)
         {
            adt_bytes_delete(program);
         }
      }
      free(self->requirePortPackPrograms);
      self->requirePortPackPrograms = (adt_bytes_t**) 0;
   }
   if ( (self->numProvidePorts > 0) && ( self->providePortPackPrograms != 0) )
   {
      for(i=0;i<self->numProvidePorts;i++)
      {
         program = self->providePortPackPrograms[i];
         if (program != 0)
         {
            adt_bytes_delete(program);
         }
         free(self->providePortPackPrograms);
         self->providePortPackPrograms = (adt_bytes_t**) 0;
      }
   }
}

static void apx_nodeProgramContainer_clearUnpackPrograms(apx_nodeProgramContainer_t *self)
{
   int32_t i;
   adt_bytes_t *program;
   if ( (self->numRequirePorts > 0) && ( self->requirePortUnpackPrograms != 0) )
   {
      for(i=0;i<self->numRequirePorts;i++)
      {
         program = self->requirePortUnpackPrograms[i];
         if (program != 0)
         {
            adt_bytes_delete(program);
         }
      }
      free(self->requirePortUnpackPrograms);
      self->requirePortUnpackPrograms = (adt_bytes_t**) 0;

   }
   if ( (self->numProvidePorts > 0) && ( self->providePortUnpackPrograms != 0) )
   {
      for(i=0;i<self->numProvidePorts;i++)
      {
         program = self->providePortUnpackPrograms[i];
         if (program != 0)
         {
            adt_bytes_delete(program);
         }
      }
      free(self->providePortUnpackPrograms);
      self->providePortUnpackPrograms = (adt_bytes_t**) 0;
   }
}


