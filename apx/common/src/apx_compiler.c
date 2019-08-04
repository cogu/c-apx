/*****************************************************************************
* \file      apx_compiler.c
* \author    Conny Gustafsson
* \date      2019-01-03
* \brief     APX bytecode compiler
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
#include "apx_compiler.h"
#include "apx_vmdefs.h"
#include "pack.h"
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
static void apx_compiler_appendPlaceHolderHeader(apx_compiler_t *self, uint8_t progType);
static void apx_compiler_appendPackDataElement(apx_compiler_t *self, apx_dataElement_t *dataElement);
static void apx_compiler_packSingleDataElement(apx_compiler_t *self, apx_dataElement_t *dataElement);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_compiler_create(apx_compiler_t *self)
{
   if (self != 0)
   {
      self->program = (adt_bytearray_t*) 0;
      self->dataSize = 0;
   }
}

void apx_compiler_destroy(apx_compiler_t *self)
{
   //nothing to do (yet)
}

apx_compiler_t* apx_compiler_new(void)
{
   apx_compiler_t *self = (apx_compiler_t*) malloc(sizeof(apx_compiler_t));
   if (self != 0)
   {
      apx_compiler_create(self);
   }
   return self;
}

void apx_compiler_delete(apx_compiler_t *self)
{
   if (self != 0)
   {
      apx_compiler_destroy(self);
      free(self);
   }
}

apx_error_t apx_compiler_compileRequirePort(apx_compiler_t *self, apx_node_t *node, apx_portId_t portId, apx_program_t *program)
{
   if ( (self != 0) && (node != 0) && (program != 0) && (portId>=0) )
   {
      int32_t numPorts = apx_node_getNumRequirePorts(node);
      if (portId >= numPorts)
      {
         return APX_INVALID_ARGUMENT_ERROR;
      }
      apx_compiler_appendPlaceHolderHeader(self, APX_OPCODE_UNPACK_PROG);
      self->program = program;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_compiler_compileProvidePort(apx_compiler_t *self, apx_node_t *node, apx_portId_t portId, apx_program_t *program)
{
   if ( (self != 0) && (node != 0) && (program != 0) && (portId>=0) )
   {
      apx_dataElement_t *dataElement;
      apx_port_t *port;
      int32_t numPorts = apx_node_getNumProvidePorts(node);
      if (portId >= numPorts)
      {
         return APX_INVALID_ARGUMENT_ERROR;
      }
      self->program = program;
      apx_compiler_appendPlaceHolderHeader(self, APX_OPCODE_PACK_PROG);
      port = apx_node_getProvidePort(node, portId);
      if (port != 0)
      {
         dataElement = apx_dataSignature_getDerivedDataElement(&port->dataSignature);
         if (dataElement != 0)
         {
            apx_compiler_appendPackDataElement(self, dataElement);
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void apx_compiler_appendPlaceHolderHeader(apx_compiler_t *self, uint8_t progType)
{
   if ( (self->program != 0) && ((progType == APX_OPCODE_PACK_PROG) || (progType == APX_OPCODE_UNPACK_PROG)) )
   {
      uint8_t instruction[APX_INST_PACK_PROG_SIZE] = {0, 0, 0, 0, 0, 0};
      instruction[0] = progType;
      packLE(&instruction[1], APX_VM_VERSION, sizeof(uint16_t));
      adt_bytearray_append(self->program, &instruction[0], (uint32_t) APX_INST_PACK_PROG_SIZE);
   }
}

static void apx_compiler_appendPackDataElement(apx_compiler_t *self, apx_dataElement_t *dataElement)
{
   if ( (self->program !=0 ) && (dataElement != 0) )
   {
      if (dataElement->arrayLen > 0)
      {
         //Not implemented
      }
      else
      {
         apx_compiler_packSingleDataElement(self, dataElement);
      }
   }
}

static void apx_compiler_packSingleDataElement(apx_compiler_t *self, apx_dataElement_t *dataElement)
{
   uint8_t instruction[APX_MAX_INST_PACK_SIZE];
   uint32_t instructionLen = 0u;
   apx_size_t elemSize = 0;
   switch(dataElement->baseType)
   {
   case APX_BASE_TYPE_NONE:
      break;
   case APX_BASE_TYPE_UINT8:
      instruction[0] = APX_OPCODE_PACK_U8;
      elemSize = UINT8_SIZE;
      instructionLen=APX_INST_PACK_U8_SIZE;
      break;
   default:
      break;
   }
   if ( (elemSize > 0) && (instructionLen > 0) && (instructionLen <= sizeof(instruction)) )
   {
      adt_bytearray_append(self->program, instruction, instructionLen);
      self->dataSize += elemSize;
   }
}
