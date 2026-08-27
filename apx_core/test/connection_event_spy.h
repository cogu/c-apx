#ifndef APX_FILEMANAGER_EVENT_LISTENER_SPY_H
#define APX_FILEMANAGER_EVENT_LISTENER_SPY_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/event_listener.h"
#include "apx/file_info.h"
#include "apx/connection_base.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_connectionEventSpy_tag
{
   int32_t headerAcceptedCount;
   int32_t fileCreateCount;
   apx_connectionBase_t *lastConnection;
   apx_fileInfo_t *lastFileInfo;
}apx_connectionEventSpy_t;
//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_connectionEventSpy_create(apx_connectionEventSpy_t *self);
void apx_connectionEventSpy_destroy(apx_connectionEventSpy_t *self);
void apx_connectionEventSpy_register(apx_connectionEventSpy_t *self, apx_connectionBase_t *connection);

void apx_connectionEventSpy_headerAccepted(void *arg, apx_connectionBase_t *connection);
void apx_connectionEventSpy_fileCreate(void *arg, apx_connectionBase_t *connection, const apx_fileInfo_t *fileInfo);



#endif //APX_FILEMANAGER_EVENT_LISTENER_SPY_H
