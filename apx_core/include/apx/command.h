/*****************************************************************************
* \file      command.h
* \author    Conny Gustafsson
* \date      2021-01-21
* \brief     Command data structure
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
#define APX_CMD_CREATE_CONNECTION      ((apx_cmdType_t) 10u)



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
