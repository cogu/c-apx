/*****************************************************************************
* \file      apx_util.c
* \author    Conny Gustafsson
* \date      2020-02-17
* \brief     Various APX-related utility functions
*
* Copyright (c) 2020 Conny Gustafsson
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
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "apx_util.h"

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
   if ( (file != 0) && (dataBuf != 0) && (maxColumns > 0) && (dataSize > 0) )
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
   if ( (text != 0) && (name != 0) )
   {

      adt_str_t *parsed_name = 0;
      unsigned long parsed_port = 0u;
      char *cstr_result = strchr(text, '/');
      if (cstr_result != 0 )
      {
         retval = APX_RESOURCE_TYPE_FILE;
         parsed_name = adt_str_new_cstr(text);
      }
      else
      {
         bool isValid;
         char *parse_end = 0;
         const char *str_end;
         char *port_begin = strrchr(text, ':'); //TODO: This check needs to be improved for IPV6 support
         str_end = text + strlen(text);
         if (port_begin == 0)
         {
            port_begin = (char*) str_end;
         }
         else
         {
            parsed_port = strtoul(port_begin+1, &parse_end, 10);
            if (parse_end == 0 )
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


      if (parsed_name != 0)
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
      if ( (parsed_name == 0) && (retval != APX_RESOURCE_TYPE_UNKNOWN) && (retval != APX_RESOURCE_TYPE_ERROR) )
      {
         //Something has gone wrong when allocating memory for the string parsed_name
         retval = APX_RESOURCE_TYPE_ERROR;
      }


      if ( (retval != APX_RESOURCE_TYPE_ERROR) && (port != 0) )
      {
         *port = (uint16_t) parsed_port;
      }
   }
   return retval;
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
   assert((pBegin != 0) && (pEnd != 0) && (pBegin <= pEnd));
   for(c=*pNext; pNext<pEnd; c=*(++pNext))
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
   assert((pBegin != 0) && (pEnd != 0) && (pBegin <= pEnd));
   if (pBegin==pEnd)
   {
      //empty string
      return true;
   }
   for(c=*pNext; pNext<pEnd; c=*(++pNext))
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
