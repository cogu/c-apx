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
void apx_event_create_serverConnected(apx_event_t *event, struct apx_serverConnectionBase_tag *connection)
{
   memset(event, 0, APX_EVENT_SIZE);
   event->evType = APX_EVENT_SERVER_CONNECTED;
   event->evData1 = (void*) connection;
}

void apx_event_create_serverDisconnected(apx_event_t *event, struct apx_serverConnectionBase_tag *connection)
{
   memset(event, 0, APX_EVENT_SIZE);
   event->evType = APX_EVENT_SERVER_DISCONNECTED;
   event->evData1 = (void*) connection;
}

void apx_event_create_clientConnected(apx_event_t *event, struct apx_clientConnectionBase_tag *connection)
{
   memset(event, 0, APX_EVENT_SIZE);
   event->evType = APX_EVENT_CLIENT_CONNECTED;
   event->evData1 = (void*) connection;
}

void apx_event_create_clientDisconnected(apx_event_t *event, struct apx_clientConnectionBase_tag *connection)
{
   memset(event, 0, APX_EVENT_SIZE);
   event->evType = APX_EVENT_CLIENT_DISCONNECTED;
   event->evData1 = (void*) connection;
}

void apx_event_fillRemoteFileHeaderComplete(apx_event_t *event, struct apx_connectionBase_tag *connection)
{
   if (event != 0)
   {
      memset(event, 0, APX_EVENT_SIZE);
      event->evType = APX_EVENT_RMF_HEADER_ACCEPTED;
      event->evData1 = (void*) connection;
   }
}

void apx_event_fillFileCreatedEvent(apx_event_t *event, struct apx_connectionBase_tag *connection, struct apx_fileInfo_tag *fileInfo)
{
   if (event != 0)
   {
      if (event != 0)
      {
         memset(event, 0, APX_EVENT_SIZE);
         event->evType = APX_EVENT_FILE_CREATED;
         event->evData1 = (void*) connection;
         event->evData2 = (void*) fileInfo;
      }
   }
}

void apx_event_createHeaderAccepted(apx_event_t *event, struct apx_connectionBase_tag *connection)
{
   if (event != 0)
   {
      memset(event, 0, APX_EVENT_SIZE);
      event->evType = APX_EVENT_RMF_HEADER_ACCEPTED;
      event->evData1 = (void*) connection;
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


