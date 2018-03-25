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
#ifdef UNIT_TEST
#include "testsocket.h"
#else
#include "msocket.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
struct apx_server_tag;
struct apx_testServer_tag;

typedef struct apx_serverConnection_tag
{
   apx_fileManager_t fileManager;
#ifdef UNIT_TEST
   testsocket_t *testsocket;
   struct apx_testServer_tag *server;
#else
   msocket_t *msocket;
   struct apx_server_tag *server;
#endif

   bool isGreetingParsed;
   int8_t debugMode;
   adt_bytearray_t sendBuffer;
   uint8_t numHeaderMaxLen;
}apx_serverConnection_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef UNIT_TEST
int8_t apx_serverConnection_create(apx_serverConnection_t *self, testsocket_t *socket, struct apx_testServer_tag *server);
#else
int8_t apx_serverConnection_create(apx_serverConnection_t *self, msocket_t *socket, struct apx_server_tag *server);
#endif
void apx_serverConnection_destroy(apx_serverConnection_t *self);
#ifdef UNIT_TEST
apx_serverConnection_t *apx_serverConnection_new(testsocket_t *socket, struct apx_testServer_tag *server);
#else
apx_serverConnection_t *apx_serverConnection_new(msocket_t *socket, struct apx_server_tag *server);
#endif
void apx_serverConnection_delete(apx_serverConnection_t *self);
void apx_serverConnection_vdelete(void *arg);

void apx_serverConnection_attachNodeManager(apx_serverConnection_t *self, apx_nodeManager_t *nodeManager);
void apx_serverConnection_detachNodeManager(apx_serverConnection_t *self, apx_nodeManager_t *nodeManager);
void apx_serverConnection_start(apx_serverConnection_t *self);
void apx_serverConnection_setDebugMode(apx_serverConnection_t *self, int8_t debugMode);

int8_t apx_serverConnection_dataReceived(apx_serverConnection_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);

#endif //APX_SERVER_CONNECTION_H
