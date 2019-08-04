/*****************************************************************************
* \file      apx_event.c
* \author    Conny Gustafsson
* \date      2018-12-16
* \brief     Description
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include "apx_event.h"


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

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


