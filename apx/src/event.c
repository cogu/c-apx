/*****************************************************************************
* \file      apx_event.c
* \author    Conny Gustafsson
* \date      2018-10-15
* \brief     Maps all APX event listeners event data into a common data structure
*
* Copyright (c) 2018-2020 Conny Gustafsson
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
#include <string.h>
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

void apx_event_pack_log_write(apx_event_t* event, apx_logLevel_t level, char* label, adt_str_t* msg)
{
   if ((event != NULL) && (label != NULL) && (msg != NULL))
   {
      event->ev_type = APX_EVENT_LOG_WRITE;
      event->data1 = (uint32_t)level;
      event->data3 = label;
      event->data4 = msg;
   }
}

void apx_event_unpack_log_write(apx_event_t const* event, apx_logLevel_t* level, char** label, adt_str_t** msg)
{
   if ( (event != NULL) && (level != NULL) && (label != NULL) && (msg != NULL))
   {
      *level = (apx_logLevel_t)event->data1;
      *label = (char*)event->data4;
      *msg = (adt_str_t*)event->data3;      
   }
}

void apx_event_pack_protocol_header_accepted(apx_event_t* event, struct apx_connectionBase_tag* connection)
{
   if (event != 0)
   {
      memset(event, 0, APX_EVENT_SIZE);
      event->ev_type = APX_EVENT_PROTOCOL_HEADER_ACCEPTED;
      event->data3 = (void*) connection;
   }
}

void apx_event_unpack_protocol_header_accepted(apx_event_t const* event, struct apx_connectionBase_tag** connection)
{
   if ((event != NULL) && (connection != NULL))
   {
      *connection = (apx_connectionBase_t*)event->data3;
   }
}

void apx_event_pack_remote_file_published(apx_event_t* event, struct apx_connectionBase_tag* connection, struct rmf_fileInfo_tag* file_info)
{
   if (event != 0)
   {
      memset(event, 0, APX_EVENT_SIZE);
      event->ev_type = APX_EVENT_REMOTE_FILE_PUBLISHED;
      event->data3 = (void*)connection;
      event->data4 = (void*)file_info;
   }
}

void apx_event_unpack_remote_file_published(apx_event_t const* event, struct apx_connectionBase_tag** connection, struct rmf_fileInfo_tag** file_info)
{
   if ((event != NULL) && (connection != NULL) && (file_info != NULL))
   {
      *connection = (apx_connectionBase_t*)event->data3;
      *file_info = (rmf_fileInfo_t*)event->data4;
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


