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
