#ifndef APX_TEST_SERVER_H
#define APX_TEST_SERVER_H
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#if defined(_MSC_PLATFORM_TOOLSET) && (_MSC_PLATFORM_TOOLSET<=110)
#include "msc_bool.h"
#else
#include <stdbool.h>
#endif
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#endif
#include "osmacro.h"
#include "testsocket.h"
#include "apx_nodeManager.h"
#include "apx_serverConnection.h"
#include "apx_router.h"
#include "adt_list.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_testServer_tag
{
   adt_list_t connections; //linked list of strong references to apx_serverConnection_t
   apx_nodeManager_t nodeManager; //the server has a single instance of the node manager, all connections interface with this object
   apx_router_t router; //this component handles all routing tables within the server
}apx_testServer_t;

//////////////////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_testServer_create(apx_testServer_t *self);
void apx_testServer_destroy(apx_testServer_t *self);
void apx_testServer_accept(apx_testServer_t *self, testsocket_t *socket);

#endif //APX_TEST_SERVER_H
