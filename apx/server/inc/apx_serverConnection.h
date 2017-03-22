#ifndef APX_SERVER_CONNECTION_H
#define APX_SERVER_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#if defined(_MSC_PLATFORM_TOOLSET) && (_MSC_PLATFORM_TOOLSET<=110)
#include "msc_bool.h"
#else
#include <stdbool.h>
#endif
#include "adt_bytearray.h"
#include "apx_fileManager.h"
#include "apx_nodeManager.h"
#ifdef _MSC_VER
#include <Windows.h>
#endif
#include "msocket.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
struct apx_server_tag;

typedef struct apx_serverConnection_tag
{
   apx_fileManager_t fileManager;
   msocket_t *msocket;
   struct apx_server_tag *server;
   bool isGreetingParsed;
   adt_bytearray_t sendBuffer;
   uint8_t numHeaderMaxLen;
}apx_serverConnection_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
int8_t apx_serverConnection_create(apx_serverConnection_t *self, msocket_t *msocket, struct apx_server_tag *server);
void apx_serverConnection_destroy(apx_serverConnection_t *self);
apx_serverConnection_t *apx_serverConnection_new(msocket_t *msocket, struct apx_server_tag *server);
void apx_serverConnection_delete(apx_serverConnection_t *self);
void apx_serverConnection_vdelete(void *arg);

void apx_serverConnection_attachNodeManager(apx_serverConnection_t *self, apx_nodeManager_t *nodeManager);
void apx_serverConnection_detachNodeManager(apx_serverConnection_t *self, apx_nodeManager_t *nodeManager);
void apx_serverConnection_start(apx_serverConnection_t *self);

int8_t apx_serverConnection_dataReceived(apx_serverConnection_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);

#endif //APX_SERVER_CONNECTION_H
