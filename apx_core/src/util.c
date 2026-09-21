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
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "apx/util.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define ASCII_ZERO 0x30
#define MAX_PORT_NUMBER 65535

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static bool apx_util_verifyIPV4Address(const char *pBegin, const char *pEnd);
static bool apx_util_verify_name(const char *pBegin, const char *pEnd);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_fprint_hex_bytes(FILE *file, int32_t maxColumns, const uint8_t *dataBuf, apx_size_t dataSize)
{
   if ( (file != NULL) && (dataBuf != NULL) && (maxColumns > 0) && (dataSize > 0) )
   {
      int32_t column;
      apx_size_t byteCount = 0u;
      const uint8_t *p = dataBuf;
      while(byteCount < dataSize)
      {
         for (column = 0; column < maxColumns; column++)
         {
            if (byteCount >= dataSize)
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

/**
 * Parses a string and tries to guess whether it is a file path, IP address or name (for example "localhost").
 *
 * - If the string contains a slash it assumes it is a file path.
 * - If the string is just alpha-numerical letters (potentially separated by dots) it assumes it is a name.
 * - If the string seems to be a IP address it parses it as an IP address
 * - IPV6 address support is not yet implemented (Maybe later).
 *
 * A parsed name will be allocated and assigned to the "name" parameter. The caller is responsible for disposing its memory.
 *
 * Additionally, if the string (the text argument) ends with the ":\d+" pattern as in ":8080" it parses the number as
 * a port number. If no port number is present the port parameter will be assigned to 0.
 *
 * The port argument can be NULL meaning its optional to use in call.
 *
 * Note:
 * Given these rules, if you want to refer to a UNIX socket name in current directory you must start with "./".
 *   Example:
 *    Socket name in current directory is "test.socket".
 *    Then you must type "./test.socket" to get it identified as a file resource by this function.
 *
 * Returns the resource type (integer) which is the best guess this function can make.
 * In case of parse failure the value APX_RESOURCE_TYPE_ERROR will be returned.
 */

apx_resource_type_t apx_parse_resource_name(const char *text, adt_str_t **name, uint16_t *port)
{
   apx_resource_type_t retval = APX_RESOURCE_TYPE_UNKNOWN;
   if ( (text != NULL) && (name != NULL) )
   {

      adt_str_t *parsed_name = NULL;
      unsigned long parsed_port = 0u;
      char *cstr_result = strchr(text, '/');
      if (cstr_result != NULL )
      {
         retval = APX_RESOURCE_TYPE_FILE;
         parsed_name = adt_str_new_cstr(text);
      }
      else
      {
         bool isValid;
         char *parse_end = NULL;
         const char *str_end;
         char *port_begin = strrchr(text, ':'); //TODO: This check needs to be improved for IPV6 support
         str_end = text + strlen(text);
         if (port_begin == NULL)
         {
            port_begin = (char*) str_end;
         }
         else
         {
            parsed_port = strtoul(port_begin+1, &parse_end, 10);
            if (parse_end == NULL )
            {
               parsed_port = 0u;
               retval = APX_RESOURCE_TYPE_ERROR;
            }
         }
         if (retval != APX_RESOURCE_TYPE_ERROR)
         {
            isValid = apx_util_verifyIPV4Address(text, port_begin);
            if (isValid)
            {
               parsed_name = adt_str_new_bstr((const uint8_t*) text, (const uint8_t*) port_begin);
               retval = APX_RESOURCE_TYPE_IPV4;
            }
            else
            {
               //TODO: check for IPV6 address here
               isValid = apx_util_verify_name(text, port_begin);
               if (isValid)
               {
                  parsed_name = adt_str_new_bstr((const uint8_t*) text, (const uint8_t*) port_begin);
                  retval = APX_RESOURCE_TYPE_NAME;
               }
               else
               {
                  retval = APX_RESOURCE_TYPE_ERROR;
               }
            }
         }
      }

      if ( parsed_port > MAX_PORT_NUMBER )
      {
         retval = APX_RESOURCE_TYPE_ERROR;
      }


      if (parsed_name != NULL)
      {
         if ( (retval != APX_RESOURCE_TYPE_UNKNOWN) && (retval != APX_RESOURCE_TYPE_ERROR) )
         {
            *name = parsed_name;
         }
         else
         {
            adt_str_delete(parsed_name);
         }
      }
      if ( (parsed_name == NULL) && (retval != APX_RESOURCE_TYPE_UNKNOWN) && (retval != APX_RESOURCE_TYPE_ERROR) )
      {
         //Something has gone wrong when allocating memory for the string parsed_name
         retval = APX_RESOURCE_TYPE_ERROR;
      }


      if ( (retval != APX_RESOURCE_TYPE_ERROR) && (port != NULL) )
      {
         *port = (uint16_t) parsed_port;
      }
   }
   return retval;
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

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static bool apx_util_verifyIPV4Address(const char *pBegin, const char *pEnd)
{
   const char *pNext = pBegin;
   int c;
   const int number_base = 10;
   int number_in_group = 0;
   int group_length = 0;
   int group_count = 1u; //verifies that we have exactly 4 groups of numbers separated by '.'
   assert((pBegin != NULL) && (pEnd != NULL) && (pBegin <= pEnd));
   for(c = (unsigned char)*pNext; pNext < pEnd; c = (unsigned char)*(++pNext))
   {
      if (c == '.')
      {
         if (group_length > 0)
         {
            ++group_count;
            number_in_group=0u;
            group_length = 0;
         }
         else
         {
            //This happens when user enters two consecutive dots without a number in between.
            return false;
         }
      }
      else if (isdigit(c))
      {
         group_length++;
         number_in_group = number_in_group*number_base + (c-ASCII_ZERO);
         if (number_in_group > 255)
         {
            //invalid IP number in group
            return false;
         }
      }
      else
      {
         return false;
      }
   }
   if (group_count == 4)
   {
      return true;
   }
   return false;
}

/**
 * Verifies that given bounded text string contains a name (such as "localhost") or is a computer name (such as DNS name).
 * First character must not be a digit (otherwise it can get confused with an IP number)
 */
static bool apx_util_verify_name(const char *pBegin, const char *pEnd)
{
   bool first = true;
   const char *pNext = pBegin;
   int c;
   assert((pBegin != NULL) && (pEnd != NULL) && (pBegin <= pEnd));
   if (pBegin==pEnd)
   {
      //empty string
      return true;
   }
   for(c = (unsigned char)*pNext; pNext < pEnd; c = (unsigned char)*(++pNext))
   {
      if (first)
      {
         first = false;
         if ( (c != '.') && (c != '_') && (c != '-') && (c!= '~') && !isalpha(c))
         {
            return false;
         }
      }
      else
      {
         if ( (c != '.') && (c != '_') && (c != '-') && (c!= '~') && !isalnum(c))
         {
            return false;
         }
      }
   }
   return true;
}
