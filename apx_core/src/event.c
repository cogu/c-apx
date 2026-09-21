/*****************************************************************************
* \file      event.c
* \author    Conny Gustafsson
* \date      2018-10-15
* \brief     Maps all APX event listeners event data into a common data structure
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <assert.h>
#include "apx/event.h"
#include "apx/connection_base.h"
#include "apx/file_info.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_event_pack_log_write(apx_event_t* event, apx_log_level_t level, char* label, adt_str_t* msg)
{
   if ((event != NULL) && (label != NULL) && (msg != NULL))
   {
      event->ev_type = APX_EVENT_LOG_WRITE;
      event->data1 = (uint32_t)level;
      event->data3 = label;
      event->data4 = msg;
   }
}

void apx_event_unpack_log_write(apx_event_t const* event, apx_log_level_t* level, char** label, adt_str_t** msg)
{
   if ( (event != NULL) && (level != NULL) && (label != NULL) && (msg != NULL))
   {
      *level = (apx_log_level_t)event->data1;
      *label = (char*)event->data3;
      *msg = (adt_str_t*)event->data4;
   }
}

void apx_event_pack_protocol_header_accepted(apx_event_t* event, struct apx_connection_base_tag* connection)
{
   if (event != NULL)
   {
      memset(event, 0, APX_EVENT_SIZE);
      event->ev_type = APX_EVENT_PROTOCOL_HEADER_ACCEPTED;
      event->data3 = (void*) connection;
   }
}

void apx_event_unpack_protocol_header_accepted(apx_event_t const* event, struct apx_connection_base_tag** connection)
{
   if ((event != NULL) && (connection != NULL))
   {
      *connection = (apx_connection_base_t*)event->data3;
   }
}

void apx_event_pack_remote_file_published(apx_event_t* event, struct apx_connection_base_tag* connection, struct rmf_file_info_tag* file_info)
{
   if (event != NULL)
   {
      memset(event, 0, APX_EVENT_SIZE);
      event->ev_type = APX_EVENT_REMOTE_FILE_PUBLISHED;
      event->data3 = (void*)connection;
      event->data4 = (void*)file_info;
   }
}

void apx_event_unpack_remote_file_published(apx_event_t const* event, struct apx_connection_base_tag** connection, struct rmf_file_info_tag** file_info)
{
   if ((event != NULL) && (connection != NULL) && (file_info != NULL))
   {
      *connection = (apx_connection_base_t*)event->data3;
      *file_info = (rmf_file_info_t*)event->data4;
   }
}

void apx_event_destroy(apx_event_t* event, soa_t* allocator)
{
   if (event != NULL)
   {
      size_t label_size;
      char* label;
      adt_str_t* str;
      rmf_file_info_t* file_info = NULL;
      switch (event->ev_type)
      {
      case APX_EVENT_LOG_WRITE:
         label = (char*)event->data3;
         str = (adt_str_t*)event->data4;
         if ( (label != NULL) && (allocator != NULL) )
         {
            label_size = strlen(label);
            soa_free(allocator, label, label_size + 1);
         }
         adt_str_delete(str);
         break;
      case APX_EVENT_PROTOCOL_HEADER_ACCEPTED:
         //Weak references only
         break;
      case APX_EVENT_REMOTE_FILE_PUBLISHED:
         file_info = (rmf_file_info_t*)event->data4;
         rmf_fileInfo_delete(file_info);
         break;
      default:
         assert(0);
      }
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


