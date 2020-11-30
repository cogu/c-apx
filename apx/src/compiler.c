/*****************************************************************************
* \file      compiler.c
* \author    Conny Gustafsson
* \date      2019-01-03
* \brief     APX bytecode compiler
*
* Copyright (c) 2019-2020 Conny Gustafsson
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
#include "apx/compiler.h"
#include "pack.h"
#include <malloc.h>
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#else
#define vfree free
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static uint8_t apx_compiler_encodeArrayInstruction(apx_compiler_t *self,  uint32_t arrayLen, bool isDynamic, uint8_t *packLen);
static uint8_t apx_compiler_encodeRecordSelectInstruction(bool isLastField);
static apx_error_t apx_compiler_encodeProgramHeader(apx_compiler_t *self, uint8_t majorVersion, uint8_t minorVersion, apx_size_t dataSize, uint8_t programType);
static apx_error_t apx_compiler_compileDataElement(apx_compiler_t *self, apx_dataElement_t *dataElement, uint8_t opcode);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_compiler_create(apx_compiler_t *self)
{
   if (self != 0)
   {
      self->hasHeader = false;
      self->program = (adt_bytearray_t*) 0;
      adt_stack_create(&self->offsetStack, vfree);
      self->dataOffset = (apx_size_t*) malloc(sizeof(apx_size_t));
      if (self->dataOffset != 0)
      {
         *self->dataOffset = 0u;
      }
   }
}

void apx_compiler_destroy(apx_compiler_t *self)
{
   if (self != 0)
   {
      adt_stack_destroy(&self->offsetStack);
      if (self->dataOffset != 0)
      {
         free(self->dataOffset);
      }
   }
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

void apx_compiler_begin(apx_compiler_t *self, adt_bytearray_t *buffer)
{
   if ( (self != 0) && (buffer != 0) )
   {
      self->program = buffer;
      self->hasHeader = false;
      *self->dataOffset = 0;
   }
}

/**
 * Same as apx_compiler_begin but also appends a pack program header at the start
 */
apx_error_t apx_compiler_begin_packProgram(apx_compiler_t *self, adt_bytearray_t *buffer)
{
   if ( (self != 0) && (buffer != 0) )
   {
      apx_compiler_begin(self, buffer);
      return apx_compiler_encodePackProgramHeader(self, APX_VM_MAJOR_VERSION, APX_VM_MINOR_VERSION, 0u);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_compiler_begin_unpackProgram(apx_compiler_t *self, adt_bytearray_t *buffer)
{
   if ( (self != 0) && (buffer != 0) )
   {
      apx_compiler_begin(self, buffer);
      return apx_compiler_encodeUnpackProgramHeader(self, APX_VM_MAJOR_VERSION, APX_VM_MINOR_VERSION, 0u);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_compiler_end(apx_compiler_t *self)
{
   if ( (self != 0) && (self->program != 0))
   {
      if (self->hasHeader)
      {
         uint8_t *code = adt_bytearray_data(self->program);
         assert(adt_bytearray_length(self->program) >= APX_VM_HEADER_SIZE);
         packLE(&code[APX_VM_HEADER_DATA_OFFSET], *self->dataOffset, UINT32_SIZE);
      }
      self->program = (adt_bytearray_t*) 0;
   }
}


apx_error_t apx_compiler_compilePackDataElement(apx_compiler_t *self, apx_dataElement_t *dataElement)
{
   return apx_compiler_compileDataElement(self, dataElement, APX_OPCODE_PACK);
}


apx_error_t apx_compiler_compileUnpackDataElement(apx_compiler_t *self, apx_dataElement_t *dataElement)
{
   return apx_compiler_compileDataElement(self, dataElement, APX_OPCODE_UNPACK);
}


uint8_t apx_compiler_encodeInstruction(uint8_t opcode, uint8_t variant, uint8_t flags)
{
   uint8_t result = (opcode & APX_INST_OPCODE_MASK) | ( (variant & APX_INST_VARIANT_MASK) << APX_INST_VARIANT_SHIFT);
   if (flags != 0)
   {
      result |= (flags & APX_INST_FLAG_MASK) << APX_INST_FLAG_SHIFT;
   }
   return result;
}


apx_error_t apx_compiler_encodePackProgramHeader(apx_compiler_t *self, uint8_t majorVersion, uint8_t minorVersion, apx_size_t dataSize)
{
   return apx_compiler_encodeProgramHeader(self, majorVersion, minorVersion, dataSize, APX_VM_HEADER_PACK_PROG);
}

apx_error_t apx_compiler_encodeUnpackProgramHeader(apx_compiler_t *self, uint8_t majorVersion, uint8_t minorVersion, apx_size_t dataSize)
{
   return apx_compiler_encodeProgramHeader(self, majorVersion, minorVersion, dataSize, APX_VM_HEADER_UNPACK_PROG);
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static uint8_t apx_compiler_encodeArrayInstruction(apx_compiler_t *self,  uint32_t arrayLen, bool isDynamic, uint8_t *packLen)
{
   const uint8_t opcode = APX_OPCODE_ARRAY;
   uint8_t variant;
   uint8_t flag = isDynamic? APX_DYN_ARRAY_FLAG : 0;
   if (arrayLen <= UINT8_MAX)
   {
      variant = APX_VARIANT_U8;
      *packLen = UINT8_SIZE;
   }
   else if (arrayLen <= UINT16_MAX)
   {
      variant = APX_VARIANT_U16;
      *packLen = UINT16_SIZE;
   }
   else if (arrayLen <= UINT32_MAX)
   {
      variant = APX_VARIANT_U32;
      *packLen = UINT32_SIZE;
   }
   else
   {
      return APX_OPCODE_INVALID;
   }
   return apx_compiler_encodeInstruction(opcode, variant, flag);
}

static uint8_t apx_compiler_encodeRecordSelectInstruction(bool isLastField)
{
   const uint8_t opcode = APX_OPCODE_DATA_CTRL;
   const uint8_t variant = APX_VARIANT_RECORD_SELECT;
   uint8_t flag = isLastField? APX_LAST_FIELD_FLAG : 0u;
   return apx_compiler_encodeInstruction(opcode, variant, flag);
}

static apx_error_t apx_compiler_encodeProgramHeader(apx_compiler_t *self, uint8_t majorVersion, uint8_t minorVersion, apx_size_t dataSize, uint8_t programType)
{
   if (self != 0)
   {
      if (self->program != 0)
      {
         uint8_t instruction[APX_VM_HEADER_SIZE] = {APX_VM_MAGIC_NUMBER, APX_VM_MAJOR_VERSION, APX_VM_MINOR_VERSION, programType, 0, 0, 0, 0};
         packLE(&instruction[4], dataSize, UINT32_SIZE);
         adt_bytearray_append(self->program, &instruction[0], (uint32_t) APX_VM_HEADER_SIZE);
         self->hasHeader = true;
         return APX_NO_ERROR;
      }
      else
      {
         return APX_MISSING_BUFFER_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_compiler_compileDataElement(apx_compiler_t *self, apx_dataElement_t *dataElement, uint8_t opcode)
{
   if ( (self != 0) && (dataElement != 0) )
   {
      apx_error_t retval = APX_NO_ERROR;
      uint8_t variant = 0;
      uint8_t flags = 0;
      apx_size_t elemSize = 0u;
      uint8_t arrayPackLen = 0u;
      uint32_t arrayLen;
      bool isDynamicArray;
      arrayLen = apx_dataElement_getArrayLen(dataElement);
      isDynamicArray = apx_dataElement_isDynamicArray(dataElement);

      if (self->program == 0)
      {
         return APX_MISSING_BUFFER_ERROR;
      }

      if (dataElement->arrayLen > 0)
      {
         flags |= APX_ARRAY_FLAG;
      }
      switch(dataElement->baseType)
      {
      case APX_BASE_TYPE_NONE:
         retval = APX_ELEMENT_TYPE_ERROR;
         break;
      case APX_BASE_TYPE_UINT8:
         variant = APX_VARIANT_U8;
         elemSize = UINT8_SIZE;
         break;
      case APX_BASE_TYPE_UINT16:
         variant = APX_VARIANT_U16;
         elemSize = UINT16_SIZE;
         break;
      case APX_BASE_TYPE_UINT32:
         variant = APX_VARIANT_U32;
         elemSize = UINT32_SIZE;
         break;
      case APX_BASE_TYPE_SINT8:
         variant = APX_VARIANT_S8;
         elemSize = UINT8_SIZE;
         break;
      case APX_BASE_TYPE_SINT16:
         variant = APX_VARIANT_S16;
         elemSize = UINT16_SIZE;
         break;
      case APX_BASE_TYPE_SINT32:
         variant = APX_VARIANT_S32;
         elemSize = UINT32_SIZE;
         break;
      case APX_BASE_TYPE_RECORD:
         variant = APX_VARIANT_RECORD;
         break;
      case APX_BASE_TYPE_STRING:
         variant = APX_VARIANT_STR;
         elemSize = UINT8_SIZE;
         break;
      default:
         retval = APX_NOT_IMPLEMENTED_ERROR;
         break;
      }
      if (retval == APX_NO_ERROR)
      {
         uint8_t instruction = apx_compiler_encodeInstruction(opcode, variant, flags);
         adt_bytearray_push(self->program, instruction);
         if (arrayLen > 0u)
         {
            instruction = apx_compiler_encodeArrayInstruction(self, arrayLen, isDynamicArray, &arrayPackLen);
            if (instruction != APX_OPCODE_INVALID)
            {
               uint8_t tmp[UINT32_SIZE];
               adt_bytearray_push(self->program, instruction);
               assert(arrayPackLen<=UINT32_SIZE);
               packLE(&tmp[0], arrayLen, arrayPackLen);
               adt_bytearray_append(self->program, &tmp[0], (uint32_t) arrayPackLen);
            }
            else
            {
               retval = APX_LENGTH_ERROR;
            }
         }

      }
      if (variant == APX_VARIANT_RECORD)
      {
         adt_stack_push(&self->offsetStack, (void*) self->dataOffset);
         self->dataOffset = (apx_size_t*) malloc(sizeof(apx_size_t));
         if (self->dataOffset != 0)
         {
            *self->dataOffset = 0u;
            int32_t i;
            int32_t end = adt_ary_length(dataElement->childElements);
            for(i=0; i<end; i++)
            {
               apx_dataElement_t *childElement = (apx_dataElement_t*) adt_ary_value(dataElement->childElements,i);
               assert(childElement != 0);
               if (childElement->name != 0)
               {
                  uint8_t instruction = apx_compiler_encodeRecordSelectInstruction(i==end-1);
                  if (instruction != APX_OPCODE_INVALID)
                  {

                     size_t len = strlen(childElement->name);
                     adt_bytearray_push(self->program, instruction);
                     if (len > 0)
                     {
                        adt_bytearray_append(self->program, (const uint8_t*) childElement->name, len);
                     }
                     adt_bytearray_push(self->program, 0u); //Null-terminator
                  }
                  else
                  {
                     break;
                  }
               }
               else
               {
                  retval = APX_NAME_MISSING_ERROR;
                  break;
               }
               retval = apx_compiler_compileDataElement(self, childElement, opcode);
               if (retval != APX_NO_ERROR)
               {
                  break;
               }
            }
            elemSize = *self->dataOffset;
            free(self->dataOffset);
            self->dataOffset = (apx_size_t*) adt_stack_top(&self->offsetStack);
            adt_stack_pop(&self->offsetStack);
         }
         else
         {
            retval = APX_MEM_ERROR;
         }
      }
      if (retval == APX_NO_ERROR)
      {
         if (elemSize > 0u)
         {
            if ( (arrayLen > 0u) )
            {
               *self->dataOffset += (elemSize*dataElement->arrayLen);
               if (isDynamicArray)
               {
                  *self->dataOffset += arrayPackLen;
               }
            }
            else
            {
               *self->dataOffset += elemSize;
            }
         }
         else
         {
            retval = APX_ELEMENT_TYPE_ERROR;
         }
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
