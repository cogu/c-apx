#ifndef APX_CLIENT_CONNECTION_H
#define APX_CLIENT_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>
#include "adt_hash.h"
#include "adt_bytearray.h"
#include "apx_fileManager.h"
#include "apx_nodeManager.h"
#include "msocket.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_client_tag;

typedef struct apx_clientConnection_tag
{
   apx_fileManager_t fileManager;
   msocket_t *msocket;
   bool isAcknowledgeSeen;
   adt_bytearray_t sendBuffer;
   uint8_t maxMsgHeaderSize;
   struct apx_client_tag *client;
}apx_clientConnection_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
int8_t apx_clientConnection_create(apx_clientConnection_t *self, msocket_t *msocket, struct apx_client_tag *client);
void apx_clientConnection_destroy(apx_clientConnection_t *self);
apx_clientConnection_t *apx_clientConnection_new(msocket_t *msocket, struct apx_client_tag *client);
void apx_clientConnection_delete(apx_clientConnection_t *self);
void apx_clientConnection_vdelete(void *arg);
void apx_clientConnection_start(apx_clientConnection_t *self);
int8_t apx_clientConnection_dataReceived(apx_clientConnection_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);


#endif //APX_CLIENT_CONNECTION_H
