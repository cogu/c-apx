/*****************************************************************************
* \file      client_event_listener_spy.h
* \author    Conny Gustafsson
* \date      2019-05-27
* \brief     Client event listener spy
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_CLIENT_EVENT_LISTENER_SPY_H
#define APX_CLIENT_EVENT_LISTENER_SPY_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/event_listener.h"
#include "apx/client.h"


//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_client_event_listener_spy_tag
{
   uint32_t connectCount;
   uint32_t disconnectCount;
   uint32_t headerAcceptedCount;
} apx_client_event_listener_spy_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

void apx_clientEventListenerSpy_create(apx_client_event_listener_spy_t *self);
void apx_clientEventListenerSpy_destroy(apx_client_event_listener_spy_t *self);
apx_client_event_listener_spy_t* apx_clientEventListenerSpy_new(void);
void apx_clientEventListenerSpy_delete(apx_client_event_listener_spy_t *self);
void* apx_clientEventListenerSpy_register(apx_client_event_listener_spy_t *self, apx_client_t *client);
uint32_t apx_clientEventListenerSpy_getConnectCount(apx_client_event_listener_spy_t *self);
uint32_t apx_clientEventListenerSpy_getDisconnectCount(apx_client_event_listener_spy_t *self);
uint32_t apx_clientEventListenerSpy_getHeaderAccepted(apx_client_event_listener_spy_t *self);

#endif //APX_CLIENT_EVENT_LISTENER_SPY_H
