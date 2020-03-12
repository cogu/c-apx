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
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "apx_vm.h"
#include "apx_vmSerializer.h"
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
static void apx_vm_prepareForPackUnpackInstruction(apx_vm_t *self);
static apx_error_t apx_vm_execProg(apx_vm_t *self);
static apx_error_t apx_vm_executePackInstruction(apx_vm_t *self, uint8_t variant);
static apx_error_t apx_vm_executeUnpackInstruction(apx_vm_t *self, uint8_t variant);
static apx_error_t apx_vm_executeArrayInstruction(apx_vm_t *self, uint8_t variant, bool isDynamicArray);
static apx_error_t apx_vm_executeDataControlInstruction(apx_vm_t *self, uint8_t variant, bool isLastElement);
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
      apx_vmDeserializer_create(&self->deserializer);
      self->progBegin = (uint8_t*) 0;
      self->progEnd = (uint8_t*) 0;
      self->progNext = (uint8_t*) 0;
      self->progDataSize = 0u;
      self->progType = 0u;
      self->expectedNext = APX_OPCODE_INVALID;
      self->arrayLen = 0u;
      self->isArray = false;
      self->dynLenType = APX_DYN_LEN_NONE;
   }
}

void apx_vm_destroy(apx_vm_t *self)
{
   if (self != 0)
   {
      apx_vmSerializer_destroy(&self->serializer);
      apx_vmDeserializer_destroy(&self->deserializer);
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
apx_error_t apx_vm_selectProgram(apx_vm_t *self, const adt_bytes_t *program)
{
   if ( (self != 0) && (program != 0) )
   {
      uint8_t majorVersion = 0u;
      uint8_t minorVersion = 0u;
      apx_error_t rc;
      uint32_t programLength = adt_bytes_length(program);
      if (programLength < APX_VM_HEADER_SIZE)
      {
         return APX_LENGTH_ERROR;
      }
      rc = apx_vm_decodeProgramHeader(program, &majorVersion, &minorVersion, &self->progType, &self->progDataSize);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      if( (majorVersion == APX_VM_MAJOR_VERSION) && (minorVersion == APX_VM_MINOR_VERSION) )
      {
         self->progBegin = adt_bytes_constData(program);
         self->progEnd = self->progBegin+programLength;
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

apx_size_t apx_vm_getProgDataSize(apx_vm_t *self)
{
   if (self != 0)
   {
      return self->progDataSize;
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

apx_error_t apx_vm_setReadBuffer(apx_vm_t *self, const uint8_t *buffer, uint32_t bufSize)
{
   if (self != 0)
   {
      return apx_vmDeserializer_begin(&self->deserializer, buffer, bufSize);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_packValue(apx_vm_t *self, const dtl_dv_t *dv)
{
   if (self != 0)
   {
      self->progNext = self->progBegin + APX_VM_HEADER_SIZE;
      if ( (self->progNext == 0) || (self->progNext >= self->progEnd) || (self->progType != APX_VM_HEADER_PACK_PROG))
      {
         return APX_INVALID_PROGRAM_ERROR;
      }
      else
      {
         apx_error_t rc = apx_vmSerializer_setValue(&self->serializer, dv);
         if (rc == APX_NO_ERROR)
         {
            return apx_vm_execProg(self);
         }
         else
         {
            return rc;
         }
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_unpackValue(apx_vm_t *self, dtl_dv_t **dv)
{
   if ( (self != 0) && (dv != 0) )
   {
      self->progNext = self->progBegin + APX_VM_HEADER_SIZE;
      if ( (self->progNext == 0) || (self->progNext >= self->progEnd) || (self->progType != APX_VM_HEADER_UNPACK_PROG))
      {
         return APX_INVALID_PROGRAM_ERROR;
      }
      else
      {
         apx_error_t rc = apx_vm_execProg(self);
         if (rc == APX_NO_ERROR)
         {
            *dv = apx_vmDeserializer_getValue(&self->deserializer, true);
         }
         else
         {
            *dv = (dtl_dv_t*) 0;
         }
         return rc;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_writeNullValue(apx_vm_t *self)
{
   if (self != 0)
   {
      if ( (self->progDataSize > 0) && (self->dynLenType == APX_DYN_LEN_NONE) )
      {
         return apx_vmSerializer_packNull(&self->serializer, self->progDataSize);
      }
      return APX_VALUE_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

#ifdef UNIT_TEST
apx_size_t apx_vm_getBytesWritten(apx_vm_t *self)
{
   apx_size_t retval = 0u;
   if (self != 0)
   {
      retval = apx_vmSerializer_getBytesWritten(&self->serializer);
   }
   return retval;
}

apx_size_t apx_vm_getBytesRead(apx_vm_t *self)
{
   apx_size_t retval = 0u;
   if (self != 0)
   {
      retval = apx_vmDeserializer_getBytesRead(&self->deserializer);
   }
   return retval;
}
#endif

//state-less functions
apx_error_t apx_vm_decodeProgramHeader(const adt_bytes_t *program, uint8_t *majorVersion, uint8_t *minorVersion, uint8_t *progType, apx_size_t *dataSize)
{
   if ( (program != 0) && (majorVersion != 0) && (minorVersion != 0) && (dataSize != 0) )
   {
      if (adt_bytes_length(program) >= APX_VM_HEADER_SIZE)
      {
         const uint8_t *pNext = adt_bytes_constData(program);
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

apx_error_t apx_vm_decodeProgramDataProps(const adt_bytes_t *program, apx_size_t *dataSize, uint8_t *dataFlags)
{
   uint8_t majorVersion;
   uint8_t minorVersion;
   uint8_t progType;
   (void) dataFlags;
   return apx_vm_decodeProgramHeader(program, &majorVersion, &minorVersion, &progType, dataSize);
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
static void apx_vm_prepareForPackUnpackInstruction(apx_vm_t *self)
{
   self->arrayLen = 0u;
   self->isArray = false;
   self->dynLenType = APX_DYN_LEN_NONE;
}

static apx_error_t apx_vm_execProg(apx_vm_t *self)
{
   apx_error_t retval = APX_NO_ERROR;
   assert( (self->progType == APX_VM_HEADER_PACK_PROG) || (self->progType == APX_VM_HEADER_UNPACK_PROG));
   self->expectedNext = (self->progType == APX_VM_HEADER_PACK_PROG)? APX_OPCODE_PACK : APX_OPCODE_UNPACK;
   while(self->progNext < self->progEnd)
   {
      apx_error_t rc;
      uint8_t opcode, variant, flags;
      uint8_t instruction = *self->progNext++;

      (void) apx_vm_decodeInstruction(instruction, &opcode, &variant, &flags);
      if (opcode != self->expectedNext)
      {
         retval = APX_INVALID_STATE_ERROR;
         break;
      }
      switch(opcode)
      {
      case APX_OPCODE_PACK:
         apx_vm_prepareForPackUnpackInstruction(self);
         if (flags & APX_ARRAY_FLAG)
         {
            if (self->progNext < self->progEnd)
            {
               uint8_t opcode2, variant2, flags2;
               instruction = *self->progNext++;
               (void) apx_vm_decodeInstruction(instruction, &opcode2, &variant2, &flags2);
               if (opcode2 != APX_OPCODE_ARRAY)
               {
                  rc = APX_INVALID_INSTRUCTION_ERROR;
                  break;
               }
               rc = apx_vm_executeArrayInstruction(self, variant2, (flags2 & APX_DYN_ARRAY_FLAG) != 0);
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
      case APX_OPCODE_UNPACK:
         apx_vm_prepareForPackUnpackInstruction(self);
         if (flags & APX_ARRAY_FLAG)
         {
            if (self->progNext < self->progEnd)
            {
               uint8_t opcode2, variant2, flags2;
               instruction = *self->progNext++;
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
         rc = apx_vm_executeUnpackInstruction(self, variant);
         break;
      case APX_OPCODE_DATA_CTRL:
         rc = apx_vm_executeDataControlInstruction(self, variant, (flags & APX_LAST_FIELD_FLAG) != 0 );
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
   apx_error_t rc;
   switch(variant)
   {
   case APX_VARIANT_U8:
      rc =  apx_vmSerializer_packValueAsU8(&self->serializer, self->arrayLen, self->dynLenType);
      break;
   case APX_VARIANT_U16:
      rc = apx_vmSerializer_packValueAsU16(&self->serializer, self->arrayLen, self->dynLenType);
      break;
   case APX_VARIANT_U32:
      rc = apx_vmSerializer_packValueAsU32(&self->serializer, self->arrayLen, self->dynLenType);
      break;
   case APX_VARIANT_S8:
      rc = apx_vmSerializer_packValueAsS8(&self->serializer, self->arrayLen, self->dynLenType);
      break;
   case APX_VARIANT_S16:
      rc = apx_vmSerializer_packValueAsS16(&self->serializer, self->arrayLen, self->dynLenType);
      break;
   case APX_VARIANT_S32:
      rc = apx_vmSerializer_packValueAsS32(&self->serializer, self->arrayLen, self->dynLenType);
      break;
   case APX_VARIANT_RECORD:
      rc = apx_vmSerializer_enterRecordValue(&self->serializer, self->arrayLen, self->dynLenType);
      self->expectedNext = APX_OPCODE_DATA_CTRL;
      return rc; //Return early here since we already set expectedNexts
   default:
      rc = APX_NOT_IMPLEMENTED_ERROR;
   }
   if (rc != APX_NO_ERROR)
   {
      return rc;
   }
   else
   {
      apx_vmWriteState_t *state = apx_vmSerializer_getState(&self->serializer);
      if ( (state->valueType == APX_VALUE_TYPE_RECORD) )
      {
         if (!state->isLastElement)
         {
            self->expectedNext = APX_OPCODE_DATA_CTRL;
         }
         else
         {
            self->expectedNext = APX_OPCODE_INVALID;
         }
      }
   }
   return rc;
}

static apx_error_t apx_vm_executeUnpackInstruction(apx_vm_t *self, uint8_t variant)
{
   apx_error_t rc;
   switch(variant)
   {
   case APX_VARIANT_U8:
      rc =  apx_vmDeserializer_unpackU8Value(&self->deserializer, self->arrayLen, self->dynLenType);
      break;
   case APX_VARIANT_U16:
      rc = apx_vmDeserializer_unpackU16Value(&self->deserializer, self->arrayLen, self->dynLenType);
      break;
   case APX_VARIANT_U32:
      rc = apx_vmDeserializer_unpackU32Value(&self->deserializer, self->arrayLen, self->dynLenType);
      break;
   case APX_VARIANT_S8:
      rc = apx_vmDeserializer_unpackS8Value(&self->deserializer, self->arrayLen, self->dynLenType);
      break;
   case APX_VARIANT_S16:
      rc = apx_vmDeserializer_unpackS16Value(&self->deserializer, self->arrayLen, self->dynLenType);
      break;
   case APX_VARIANT_S32:
      rc = apx_vmDeserializer_unpackS32Value(&self->deserializer, self->arrayLen, self->dynLenType);
      break;
   case APX_VARIANT_RECORD:
      rc = apx_vmDeserializer_enterRecordValue(&self->deserializer, self->arrayLen, self->dynLenType);
      self->expectedNext = APX_OPCODE_DATA_CTRL;
      return rc; //Return early here since we already set expectedNexts
   default:
      rc = APX_NOT_IMPLEMENTED_ERROR;
   }
   if (rc != APX_NO_ERROR)
   {
      return rc;
   }
   else
   {
      apx_vmReadState_t *state = apx_vmDeserializer_getState(&self->deserializer);
      if ( (state->valueType == APX_VALUE_TYPE_RECORD) )
      {
         if (!state->isLastElement)
         {
            self->expectedNext = APX_OPCODE_DATA_CTRL;
         }
         else
         {
            self->expectedNext = APX_OPCODE_INVALID;
         }
      }
   }
   return rc;
}

static apx_error_t apx_vm_executeArrayInstruction(apx_vm_t *self, uint8_t variant, bool isDynamicArray)
{
   uint8_t valueSize = 0;
   apx_dynLenType_t dynLenType = APX_DYN_LEN_NONE;
   switch(variant)
   {
   case APX_VARIANT_U8:
      valueSize = UINT8_SIZE;
      dynLenType = APX_DYN_LEN_U8;
      break;
   case APX_VARIANT_U16:
      valueSize = UINT16_SIZE;
      dynLenType = APX_DYN_LEN_U16;
      break;
   case APX_VARIANT_U32:
      valueSize = UINT32_SIZE;
      dynLenType = APX_DYN_LEN_U32;
      break;
   default:
      return APX_INVALID_INSTRUCTION_ERROR;
   }
   if (self->progNext+valueSize <= self->progEnd)
   {
      self->arrayLen = unpackLE(self->progNext, valueSize);
      self->dynLenType = isDynamicArray? dynLenType : APX_DYN_LEN_NONE;
      self->progNext+=valueSize;
   }
   else
   {
      return APX_INVALID_INSTRUCTION_ERROR;
   }
   return APX_NO_ERROR;
}

static apx_error_t apx_vm_executeDataControlInstruction(apx_vm_t *self, uint8_t variant, bool isLastElement)
{
   apx_error_t retval = APX_NO_ERROR;
   assert(self != 0);
   if (variant == APX_VARIANT_RECORD_SELECT)
   {
      assert(self->progNext != 0);
      assert(self->progEnd != 0);
      if (self->progNext < self->progEnd)
      {
         size_t nameSize;
         const char *elementName = (const char*) self->progNext;
         nameSize = strlen(elementName);
         if (nameSize > 0)
         {
            if (self->progType == APX_VM_HEADER_PACK_PROG)
            {
               retval = apx_vmSerializer_selectRecordElement_cstr(&self->serializer, elementName, isLastElement);
            }
            else
            {
               retval = apx_vmDeserializer_selectRecordElement_cstr(&self->deserializer, elementName, isLastElement);
            }
            if (retval == APX_NO_ERROR)
            {
               self->progNext += (nameSize + 1); //Skip one past the null-byte
               self->expectedNext = (self->progType == APX_VM_HEADER_PACK_PROG)? APX_OPCODE_PACK : APX_OPCODE_UNPACK;
            }
         }
         else
         {
            retval = APX_INVALID_NAME_ERROR;
         }
      }
      else
      {
         retval = APX_INVALID_INSTRUCTION_ERROR;
      }
   }
   else
   {
      retval = APX_INVALID_INSTRUCTION_ERROR;
   }
   return retval;
}
