/*****************************************************************************
* \file      apx_clientEventListenerSpy.h
* \author    Conny Gustafsson
* \date      2020-01-30
* \brief     spy for apx_clientEventListener_t
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
#ifndef APX_CLIENT_EVENT_LISTENER_SPY_H
#define APX_CLIENT_EVENT_LISTENER_SPY_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "apx_eventListener.h"
#include "apx_client.h"


//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_clientEventListenerSpy_tag
{
   uint32_t connectCount;
   uint32_t disconnectCount;
   uint32_t headerAcceptedCount;
} apx_clientEventListenerSpy_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

void apx_clientEventListenerSpy_create(apx_clientEventListenerSpy_t *self);
void apx_clientEventListenerSpy_destroy(apx_clientEventListenerSpy_t *self);
apx_clientEventListenerSpy_t* apx_clientEventListenerSpy_new(void);
void apx_clientEventListenerSpy_delete(apx_clientEventListenerSpy_t *self);
void* apx_clientEventListenerSpy_register(apx_clientEventListenerSpy_t *self, apx_client_t *client);
uint32_t apx_clientEventListenerSpy_getConnectCount(apx_clientEventListenerSpy_t *self);
uint32_t apx_clientEventListenerSpy_getDisconnectCount(apx_clientEventListenerSpy_t *self);
uint32_t apx_clientEventListenerSpy_getHeaderAccepted(apx_clientEventListenerSpy_t *self);

#endif //APX_CLIENT_EVENT_LISTENER_SPY_H
