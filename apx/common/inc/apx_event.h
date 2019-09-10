/*****************************************************************************
* \file      apx_event.h
* \author    Conny Gustafsson
* \date      2018-10-15
* \brief     Maps all APX event listeners event data into a common data structure
*
* Copyright (c) 2018 Conny Gustafsson
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
#include "apx_types.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_event_tag
{
   apx_eventId_t evType;
   uint16_t evFlags;
   void *evData1;    //generic void* pointer value
   void *evData2;    //generic void* pointer value
   void *evData3;    //generic void* pointer value
   uint32_t evData4; //generic uint32 value
   uint32_t evData5; //generic uint32 value
} apx_event_t;

#define APX_EVENT_SIZE sizeof(apx_event_t)

#define APX_EVENT_FLAG_FILE_MANAGER_EVENT    0x01
#define APX_EVENT_FLAG_REMOTE_ADDRESS        0x02

//APX Log event
#define APX_EVENT_LOG_EVENT                0 //evData1: char[16] label, evData2: adt_str_t *msg, evData4: logLevel (0-3)

//APX connection events
#define APX_EVENT_SERVER_CONNECTED         1 //evData1: apx_serverConnectionBase_t *connection
#define APX_EVENT_SERVER_DISCONNECTED      2 //evData1: apx_serverConnectionBase_t *connection
#define APX_EVENT_CLIENT_CONNECTED         3 //evData1: apx_clientConnectionBase_T *connection
#define APX_EVENT_CLIENT_DISCONNECTED      4 //evData1: apx_clientConnectionBase_T *connection

//APX file manager events
#define APX_EVENT_FM_PRE_START             5 //evData1: apx_fileManager_t *fileManager
#define APX_EVENT_FM_POST_STOP             6 //evData1: apx_fileManager_t *fileManager
#define APX_EVENT_FM_HEADER_COMPLETE       7 //evData1: apx_fileManager_t *fileManager
#define APX_EVENT_FM_FILE_CREATED          8 //evData1: apx_fileManager_t *fileManager, evData2: apx_file2_t *file, evData3: const void *caller
#define APX_EVENT_FM_FILE_REVOKED          9 //evData1: apx_fileManager_t *fileManager, evData2: apx_file2_t *file, evData3: const void *caller
#define APX_EVENT_FM_FILE_OPENED           10 //evData1: apx_fileManager_t *fileManager, evData2: apx_file2_t *file, evData3: const void *caller
#define APX_EVENT_FM_FILE_CLOSED           11 //evData1: apx_fileManager_t *fileManager, evData2: apx_file2_t *file, evData3: const void *caller

//APX node events
#define APX_EVENT_REQUIRE_PORT_CONNECT     13 //evData1: apx_nodeData_t *nodeData (weak), evData2: apx_portConnectionTable_t *connections (strong)
#define APX_EVENT_PROVIDE_PORT_CONNECT     14 //evData1: apx_nodeData_t *nodeData (weak), evData2: apx_portConnectionTable_t *connections (strong)
#define APX_EVENT_REQUIRE_PORT_DISCONNECT  15 //evData1: apx_nodeData_t *nodeData (weak), evData2: apx_portConnectionTable_t *connections (strong)
#define APX_EVENT_PROVIDE_PORT_DISCONNECT  16 //evData1: apx_nodeData_t *nodeData (weak), evData2: apx_portConnectionTable_t *connections (strong)
#define APX_EVENT_NODE_COMPLETE            17 //evData1:*arg, evData2:*nodeData
#define APX_EVENT_NODE_DEFINITION_WRITE    18 //evFlag: APX_EVENT_FLAG_REMOTE_ADDRESS?, evData1:*arg, evData2:*nodeData, evData4: offset, evData5: len
#define APX_EVENT_NODE_INDATA_WRITE        19 //evFlag: APX_EVENT_FLAG_REMOTE_ADDRESS?, evData1:*arg, evData2:*nodeData, evData4: offset, evData5: len
#define APX_EVENT_NODE_OUTATA_WRITE        20 //evFlag: APX_EVENT_FLAG_REMOTE_ADDRESS?, evData1:*arg, evData2:*nodeData, evData4: offset, evData5: len



typedef void (apx_eventHandlerFunc_t)(void *arg, apx_event_t *event);
//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_serverConnectionBase_tag;
struct apx_clientConnectionBase_tag;

void apx_event_create_serverConnected(apx_event_t *event, struct apx_serverConnectionBase_tag *connection);
void apx_event_create_serverDisconnected(apx_event_t *event, struct apx_serverConnectionBase_tag *connection);
void apx_event_create_clientConnected(apx_event_t *event, struct apx_clientConnectionBase_tag *connection);
void apx_event_create_clientDisconnected(apx_event_t *event, struct apx_clientConnectionBase_tag *connection);



#endif //APX_EVENT_H
