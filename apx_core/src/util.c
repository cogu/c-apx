/*****************************************************************************
* \file      util.c
* \author    Conny Gustafsson
* \date      2020-02-17
* \brief     Various APX-related utility functions
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "apx/util.h"


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_fprint_hex_bytes(FILE *file, int32_t max_columns, const uint8_t *data_buf, apx_size_t data_size)
{
   if ( (file != NULL) && (data_buf != NULL) && (max_columns > 0) && (data_size > 0) )
   {
      int32_t column;
      apx_size_t byteCount = 0u;
      const uint8_t *p = data_buf;
      while(byteCount < data_size)
      {
         for (column = 0; column < max_columns; column++)
         {
            if (byteCount >= data_size)
            {
               break;
            }
            if (column == 0)
            {
               fprintf(file, "%02X", (int) (*p++));
            }
            else
            {
               fprintf(file, " %02X",(int) (*p++));
            }
            byteCount++;
         }
         printf("\n");
      }
   }
}



apx_error_t convert_from_adt_to_apx_error(adt_error_t error_code)
{
   apx_error_t retval;
   switch (error_code)
   {
   case ADT_NO_ERROR:
      retval = APX_NO_ERROR;
      break;
   case ADT_INVALID_ARGUMENT_ERROR:
      retval = APX_INVALID_ARGUMENT_ERROR;
      break;
   case ADT_MEM_ERROR:
      retval = APX_MEM_ERROR;
      break;
   case ADT_INDEX_OUT_OF_BOUNDS_ERROR:
      retval = APX_INVALID_ARGUMENT_ERROR;
      break;
   case ADT_LENGTH_ERROR:
      retval = APX_LENGTH_ERROR;
      break;
   case ADT_NOT_IMPLEMENTED_ERROR:
      retval = APX_NOT_IMPLEMENTED_ERROR;
      break;
   default:
      retval = APX_INTERNAL_ERROR;
   }
   return retval;
}

const char *apx_strerror(apx_error_t error_code)
{
   switch (error_code)
   {
   case APX_NO_ERROR:
      return "No error";
   case APX_GENERIC_ERROR:
      return "Generic error";
   case APX_INVALID_ARGUMENT_ERROR:
      return "Invalid argument";
   case APX_MEM_ERROR:
      return "Out of memory";
   case APX_PARSE_ERROR:
      return "Parse error";
   case APX_DATA_SIGNATURE_ERROR:
      return "Data signature error";
   case APX_PORT_SIGNATURE_ERROR:
      return "Port signature error";
   case APX_INTERNAL_ERROR:
      return "Internal error";
   case APX_LENGTH_ERROR:
      return "Length error";
   case APX_ELEMENT_TYPE_ERROR:
      return "Element type error";
   case APX_UNSUPPORTED_ERROR:
      return "Unsupported feature";
   case APX_NOT_IMPLEMENTED_ERROR:
      return "Not implemented";
   case APX_NOT_FOUND_ERROR:
      return "Not found";
   case APX_UNMATCHED_BRACE_ERROR:
      return "Unmatched brace";
   case APX_UNMATCHED_BRACKET_ERROR:
      return "Unmatched bracket";
   case APX_UNMATCHED_STRING_ERROR:
      return "Unmatched string";
   case APX_INVALID_TYPE_REF_ERROR:
      return "Invalid type reference";
   case APX_EXPECTED_BRACKET_ERROR:
      return "Expected bracket";
   case APX_INVALID_ATTRIBUTE_ERROR:
      return "Invalid attribute";
   case APX_TOO_MANY_NODES_ERROR:
      return "Too many nodes";
   case APX_NODE_MISSING_ERROR:
      return "Node missing";
   case APX_NODE_ALREADY_EXISTS_ERROR:
      return "Node already exists";
   case APX_TYPE_ALREADY_EXIST_ERROR:
      return "Type already exists";
   case APX_PORT_ALREADY_EXIST_ERROR:
      return "Port already exists";
   case APX_FILE_ALREADY_EXISTS_ERROR:
      return "File already exists";
   case APX_MISSING_BUFFER_ERROR:
      return "Missing buffer";
   case APX_MISSING_FILE_ERROR:
      return "Missing file";
   case APX_NAME_MISSING_ERROR:
      return "Name missing";
   case APX_NAME_TOO_LONG_ERROR:
      return "Name too long";
   case APX_THREAD_CREATE_ERROR:
      return "Thread creation error";
   case APX_THREAD_JOIN_ERROR:
      return "Thread join error";
   case APX_THREAD_JOIN_TIMEOUT_ERROR:
      return "Thread join timeout";
   case APX_FILE_TOO_LARGE_ERROR:
      return "File too large";
   case APX_MSG_TOO_LARGE_ERROR:
      return "Message too large";
   case APX_CONNECTION_ERROR:
      return "Connection error";
   case APX_TRANSMIT_ERROR:
      return "Transmit error";
   case APX_NULL_PTR_ERROR:
      return "Null pointer error";
   case APX_BUFFER_BOUNDARY_ERROR:
      return "Buffer boundary error";
   case APX_BUFFER_FULL_ERROR:
      return "Buffer full";
   case APX_QUEUE_FULL_ERROR:
      return "Queue full";
   case APX_DATA_NOT_PROCESSED_ERROR:
      return "Data not processed";
   case APX_PACK_ERROR:
      return "Pack error";
   case APX_UNPACK_ERROR:
      return "Unpack error";
   case APX_READ_ERROR:
      return "Read error";
   case APX_INVALID_MSG_ERROR:
      return "Invalid message";
   case APX_UNEXPECTED_DATA_ERROR:
      return "Unexpected data";
   case APX_INVALID_PROGRAM_ERROR:
      return "Invalid program";
   case APX_INVALID_STATE_ERROR:
      return "Invalid state";
   case APX_INVALID_INSTRUCTION_ERROR:
      return "Invalid instruction";
   case APX_FILE_NOT_FOUND_ERROR:
      return "No such file or directory";
   case APX_MISSING_KEY_ERROR:
      return "Missing key";
   case APX_INVALID_OPEN_HANDLER_ERROR:
      return "Invalid open handler";
   case APX_INVALID_WRITE_HANDLER_ERROR:
      return "Invalid write handler";
   case APX_INVALID_WRITE_ERROR:
      return "Invalid write";
   case APX_INVALID_FILE_ERROR:
      return "Invalid file";
   case APX_INIT_VALUE_ERROR:
      return "Initial value error";
   case APX_INVALID_ADDRESS_ERROR:
      return "Invalid address";
   case APX_FILE_NOT_OPEN_ERROR:
      return "File not open";
   case APX_BUSY_ERROR:
      return "Resource busy";
   case APX_DATA_NOT_COMPLETE_ERROR:
      return "Data not complete";
   case APX_NOT_CONNECTED_ERROR:
      return "Not connected";
   case APX_INVALID_NAME_ERROR:
      return "Invalid name";
   case APX_INVALID_PORT_HANDLE_ERROR:
      return "Invalid port handle";
   case APX_STRAY_CHARACTERS_AFTER_PARSE_ERROR:
      return "Stray characters after parse";
   case APX_EMPTY_RECORD_ERROR:
      return "Empty record";
   case APX_INVALID_HEADER_ERROR:
      return "Invalid header";
   case APX_UNEXPECTED_END_ERROR:
      return "Unexpected end of data";
   case APX_VALUE_TYPE_ERROR:
      return "Invalid value type";
   case APX_VALUE_RANGE_ERROR:
      return "Value out of range";
   case APX_VALUE_CONVERSION_ERROR:
      return "Value conversion error";
   case APX_VALUE_LENGTH_ERROR:
      return "Value length error";
   case APX_NUMBER_TOO_LARGE_ERROR:
      return "Number too large";
   case APX_VERSION_ERROR:
      return "Version error";
   case APX_TOO_MANY_PORTS_ERROR:
      return "Too many ports";
   case APX_FILE_CREATE_ERROR:
      return "File creation error";
   case APX_TOO_MANY_REFERENCES_ERROR:
      return "Too many references";
   case APX_INDEX_ERROR:
      return "Index error";
   case APX_SEMAPHORE_ERROR:
      return "Semaphore error";
   case APX_NOT_A_DIRECTORY_ERROR:
      return "Not a directory";
   default:
      return "Unknown error";
   }
}
