/*****************************************************************************
* \file      command.h
* \author    Conny Gustafsson
* \date      2021-01-21
* \brief     Command data structure
*
* Copyright (c) 2021 Conny Gustafsson
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
#ifndef APX_COMMAND_H
#define APX_COMMAND_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include "apx/types.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

typedef uint8_t apx_cmdType_t;
#define APX_CMD_EXIT                   ((apx_cmdType_t) 0u)
#define APX_CMD_SEND_ACKNOWLEDGE       ((apx_cmdType_t) 1u)
#define APX_CMD_SEND_ERROR_CODE        ((apx_cmdType_t) 2u)
#define APX_CMD_PUBLISH_LOCAL_FILE     ((apx_cmdType_t) 3u)
#define APX_CMD_REVOKE_LOCAL_FILE      ((apx_cmdType_t) 4u)
#define APX_CMD_OPEN_REMOTE_FILE       ((apx_cmdType_t) 5u)
#define APX_CMD_CLOSE_REMOTE_FILE      ((apx_cmdType_t) 6u)
#define APX_CMD_SEND_LOCAL_CONST_DATA  ((apx_cmdType_t) 7u)
#define APX_CMD_SEND_LOCAL_DATA        ((apx_cmdType_t) 8u)
#define APX_CMD_SEND_HEADER_ACCEPTED   ((apx_cmdType_t) 9u)


typedef struct apx_command_tag
{
   apx_cmdType_t cmd_type;
   uint32_t data1; //generic uint32 value
   uint32_t data2; //generic uint32 value
   union msgData3_tag{
      void *ptr;                         //generic pointer value
      uint8_t data[APX_SMALL_DATA_SIZE]; //port data (when port data length is small)
   } data3;
   void *data4; //generic pointer value
} apx_command_t;


#define APX_COMMAND_SIZE sizeof(apx_command_t)


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_build_command_with_ptr(apx_command_t* self, apx_cmdType_t cmd_type, uint32_t d1, uint32_t d2, void* d3, void* d4);
void apx_build_command_with_data(apx_command_t* self, apx_cmdType_t cmd_type, uint32_t d1, uint32_t d2, uint8_t const* d3, void* d4);

#endif //APX_COMMAND_H
