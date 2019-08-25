/*****************************************************************************
* \file      apx_vm.c
* \author    Conny Gustafsson
* \date      2019-02-24
* \brief     APX virtual machine (implements v2 of APX byte code language)
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
#include "apx_vm.h"
#include "pack.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_vm_prepareForPackInstruction(apx_vm_t *self);
static apx_error_t apx_vm_runPackProg(apx_vm_t *self);
static apx_error_t apx_vm_executePackInstruction(apx_vm_t *self, uint8_t variant);
static apx_error_t apx_vm_executeArrayInstruction(apx_vm_t *self, uint8_t variant, bool isDynamicArray);
//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_vm_create(apx_vm_t *self)
{
   if (self != 0)
   {
      apx_vmSerializer_create(&self->serializer);
      self->codeBegin = (uint8_t*) 0;
      self->codeEnd = (uint8_t*) 0;
      self->codeNext = (uint8_t*) 0;
      self->dataSize = 0u;
      self->progType = 0u;
      self->expectedCode = APX_OPCODE_INVALID;
      self->arrayLen = 0u;
      self->isArray = false;
      self->isDynamicArray = false;
   }
}

void apx_vm_destroy(apx_vm_t *self)
{
   if (self != 0)
   {
      apx_vmSerializer_destroy(&self->serializer);
   }
}

apx_vm_t* apx_vm_new(void)
{
   apx_vm_t *self = (apx_vm_t*) malloc(sizeof(apx_vm_t));
   if (self != 0)
   {
      apx_vm_create(self);
   }
   return self;
}

void apx_vm_delete(apx_vm_t *self)
{
   if (self != 0)
   {
      apx_vm_destroy(self);
      free(self);
   }
}

/**
 * Accepts a byte-code program by parsing the program header to see if its valid.
 * Returns APX_NO_ERROR on success
 */
apx_error_t apx_vm_setProgram(apx_vm_t *self, apx_program_t *program)
{
   if (self != 0)
   {
      uint8_t majorVersion = 0u;
      uint8_t minorVersion = 0u;
      apx_error_t rc;
      uint32_t programLength = adt_bytearray_length(program);
      if (programLength < APX_VM_HEADER_SIZE)
      {
         return APX_LENGTH_ERROR;
      }
      rc = apx_vm_parsePackHeader(program, &majorVersion, &minorVersion, &self->progType, &self->dataSize);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      if( (majorVersion == APX_VM_MAJOR_VERSION) && (minorVersion == APX_VM_MINOR_VERSION) )
      {
         self->codeBegin = adt_bytearray_data(program);
         self->codeEnd = self->codeBegin+programLength;
         self->codeNext = self->codeBegin + APX_VM_HEADER_SIZE;
      }
      else
      {
         return APX_UNSUPPORTED_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

uint8_t apx_vm_getProgType(apx_vm_t *self)
{
   if (self != 0)
   {
      return self->progType;
   }
   return 0u;
}

apx_size_t apx_vm_getDataSize(apx_vm_t *self)
{
   if (self != 0)
   {
      return self->dataSize;
   }
   return 0u;
}

apx_error_t apx_vm_setWriteBuffer(apx_vm_t *self, uint8_t *buffer, uint32_t bufSize)
{
   if (self != 0)
   {
      return apx_vmSerializer_begin(&self->serializer, buffer, bufSize);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serialize(apx_vm_t *self, const dtl_dv_t *dv)
{
   if (self != 0)
   {
      if ( (self->codeNext == 0) || (self->codeNext >= self->codeEnd) || (self->progType != APX_VM_HEADER_PACK_PROG))
      {
         return APX_INVALID_PROGRAM_ERROR;
      }
      else
      {
         apx_error_t rc = apx_vmSerializer_setValue(&self->serializer, dv);
         if (rc == APX_NO_ERROR)
         {
            return apx_vm_runPackProg(self);
         }
         else
         {
            return rc;
         }
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_size_t apx_vm_getBytesWritten(apx_vm_t *self)
{
   apx_size_t retval = 0u;
   if (self != 0)
   {
      retval = apx_vmSerializer_getBytesWritten(&self->serializer);
   }
   return retval;
}


//stateless functions
apx_error_t apx_vm_parsePackHeader(adt_bytearray_t *program, uint8_t *majorVersion, uint8_t *minorVersion, uint8_t *progType, apx_size_t *dataSize)
{
   if ( (program != 0) && (majorVersion != 0) && (minorVersion != 0) && (dataSize != 0) )
   {
      if (adt_bytearray_length(program) >= APX_VM_HEADER_SIZE)
      {
         const uint8_t *pNext = adt_bytearray_data(program);
         if (*pNext++ != APX_VM_MAGIC_NUMBER)
         {
            return APX_UNEXPECTED_DATA_ERROR;
         }
         *majorVersion = *pNext++;
         *minorVersion = *pNext++;
         *progType = *pNext++;
         *dataSize = (apx_size_t) unpackLE(pNext, UINT32_SIZE);
         return APX_NO_ERROR;
      }
      else
      {
         return APX_LENGTH_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_decodeInstruction(uint8_t instruction, uint8_t *opcode, uint8_t *variant, uint8_t *flags)
{
   if ( (opcode != 0) && (variant != 0) && (flags != 0) )
   {
      *opcode = instruction & APX_INST_OPCODE_MASK;
      *variant = (instruction >> APX_INST_VARIANT_SHIFT) & APX_INST_VARIANT_MASK;
      *flags = (instruction >> APX_INST_FLAG_SHIFT) & APX_INST_FLAG_MASK;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void apx_vm_prepareForPackInstruction(apx_vm_t *self)
{
   self->arrayLen = 0u;
   self->isArray = false;
   self->isDynamicArray = false;
}

static apx_error_t apx_vm_runPackProg(apx_vm_t *self)
{
   apx_error_t retval = APX_NO_ERROR;
   self->expectedCode = APX_OPCODE_PACK;
   while(self->codeNext < self->codeEnd)
   {
      apx_error_t rc;
      uint8_t opcode, variant, flags;
      uint8_t instruction = *self->codeNext++;
      (void) apx_vm_decodeInstruction(instruction, &opcode, &variant, &flags);
      if (opcode != self->expectedCode)
      {
         retval = APX_INVALID_STATE_ERROR;
         break;
      }
      switch(opcode)
      {
      case APX_OPCODE_PACK:
         apx_vm_prepareForPackInstruction(self);
         if (flags & APX_ARRAY_FLAG)
         {
            if (self->codeNext < self->codeEnd)
            {
               uint8_t opcode2, variant2, flags2;
               instruction = *self->codeNext++;
               (void) apx_vm_decodeInstruction(instruction, &opcode2, &variant2, &flags2);
               if (opcode2 != APX_OPCODE_ARRAY)
               {
                  rc = APX_INVALID_INSTRUCTION_ERROR;
                  break;
               }
               rc = apx_vm_executeArrayInstruction(self, variant2, flags2 & APX_DYN_ARRAY_FLAG);
               if (rc != APX_NO_ERROR)
               {
                  break;
               }
            }
            else
            {
               rc = APX_INVALID_PROGRAM_ERROR;
               break;
            }
         }
         rc = apx_vm_executePackInstruction(self, variant);
         break;
      case APX_OPCODE_DATA_CTRL:
         rc = APX_NOT_IMPLEMENTED_ERROR;
         break;
      case APX_OPCODE_FLOW_CTRL:
         rc = APX_NOT_IMPLEMENTED_ERROR;
         break;
      }
      if (rc != APX_NO_ERROR)
      {
         retval = rc;
         break;
      }
   }
   return retval;
}

static apx_error_t apx_vm_executePackInstruction(apx_vm_t *self, uint8_t variant)
{
   switch(variant)
   {
   case APX_VARIANT_U8:
      return apx_vmSerializer_packValueAsU8(&self->serializer, self->arrayLen, self->isDynamicArray);
   }
   return APX_NOT_IMPLEMENTED_ERROR;
}

static apx_error_t apx_vm_executeArrayInstruction(apx_vm_t *self, uint8_t variant, bool isDynamicArray)
{
   uint8_t valueSize = 0;
   switch(variant)
   {
   case APX_VARIANT_U8:
      valueSize = UINT8_SIZE;
      break;
   case APX_VARIANT_U16:
      valueSize = UINT16_SIZE;
      break;
   case APX_VARIANT_U32:
      valueSize = UINT32_SIZE;
      break;
   default:
      return APX_INVALID_INSTRUCTION_ERROR;
   }
   if (self->codeNext+valueSize <= self->codeEnd)
   {
      self->arrayLen = unpackLE(self->codeNext, valueSize);
      self->codeNext+=valueSize;
   }
   else
   {
      return APX_INVALID_INSTRUCTION_ERROR;
   }
   return APX_NO_ERROR;
}
