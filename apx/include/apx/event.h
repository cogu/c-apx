/*****************************************************************************
* \file      apx_event.h
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
#ifndef APX_EVENT_H
#define APX_EVENT_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "adt_str.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_connectionBase_tag;
struct rmf_fileInfo_tag;

typedef struct apx_event_tag
{
   apx_eventId_t ev_type;
   uint16_t flags;
   void *callback; //callback function
   uint32_t data1; //generic uint32 value
   uint32_t data2; //generic uint32 value
   void *data3;    //generic void* pointer value
   void *data4;    //generic void* pointer value
   void *data5;    //generic void* pointer value
} apx_event_t;

#define APX_EVENT_SIZE sizeof(apx_event_t)

#define APX_EVENT_FLAG_FILE_MANAGER_EVENT    0x01
#define APX_EVENT_FLAG_REMOTE_ADDRESS        0x02

//APX Log event
#define APX_EVENT_LOG_WRITE                0 //data1: logLevel (0-3) data3: char[16] label (strong), data4: adt_str_t *msg (strong), 

//APX connection events
#define APX_EVENT_PROTOCOL_HEADER_ACCEPTED 1 //data3: apx_connectionBase_t* connection (weak)
#define APX_EVENT_REMOTE_FILE_PUBLISHED    2 //data3: apx_connectionBase_t* connection (weak), data4: rmf_fileInfo_t* connection (strong)


typedef void (apx_eventHandlerFunc_t)(void *arg, apx_event_t *event);
//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

void apx_event_pack_log_write(apx_event_t* event, apx_logLevel_t level, char* label, adt_str_t* msg);
void apx_event_unpack_log_write(apx_event_t const* event, apx_logLevel_t* level, char** label, adt_str_t** msg);
void apx_event_pack_protocol_header_accepted(apx_event_t* event, struct apx_connectionBase_tag* connection);
void apx_event_unpack_protocol_header_accepted(apx_event_t const* event, struct apx_connectionBase_tag** connection);
void apx_event_pack_remote_file_published(apx_event_t* event, struct apx_connectionBase_tag* connection, struct rmf_fileInfo_tag* file_info);
void apx_event_unpack_remote_file_published(apx_event_t const* event, struct apx_connectionBase_tag** connection, struct rmf_fileInfo_tag** file_info);

#endif //APX_EVENT_H
