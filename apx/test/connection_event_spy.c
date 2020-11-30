//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <stdio.h>
#include "connection_event_spy.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


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
void apx_connectionEventSpy_create(apx_connectionEventSpy_t *self)
{
   if (self != 0)
   {
      self->headerAcceptedCount = 0;
      self->fileCreateCount = 0;
      self->lastConnection = (apx_connectionBase_t*) 0;
      self->lastFileInfo = (apx_fileInfo_t*) 0;
   }
}

void apx_connectionEventSpy_destroy(apx_connectionEventSpy_t *self)
{
   if (self != 0)
   {
      if (self->lastFileInfo != 0)
      {
         apx_fileInfo_delete(self->lastFileInfo);
      }
   }
}

void apx_connectionEventSpy_register(apx_connectionEventSpy_t *self, apx_connectionBase_t *connection)
{
   if ((self != 0) && (connection != 0) )
   {
      apx_connectionEventListener_t handler;
      memset(&handler, 0, sizeof(handler));
      handler.arg = (void*) self;
      handler.headerAccepted2 = apx_connectionEventSpy_headerAccepted;
      handler.fileCreate2 = apx_connectionEventSpy_fileCreate;
      (void)apx_connectionBase_registerEventListener(connection, &handler);
   }
}

void apx_connectionEventSpy_headerAccepted(void *arg, apx_connectionBase_t *connection)
{
   apx_connectionEventSpy_t *self = (apx_connectionEventSpy_t*) arg;
   if ( (self != 0) && (connection != 0) )
   {

      self->headerAcceptedCount++;
      self->lastConnection = connection;
   }
}

void apx_connectionEventSpy_fileCreate(void *arg, apx_connectionBase_t *connection, const apx_fileInfo_t *fileInfo)
{
   apx_connectionEventSpy_t *self = (apx_connectionEventSpy_t*) arg;
   if ( (self != 0) && (connection != 0) && (fileInfo != 0))
   {
      self->fileCreateCount++;
      self->lastConnection = connection;
      if (self->lastFileInfo != 0)
      {
         apx_fileInfo_delete(self->lastFileInfo);
      }
      self->lastFileInfo = apx_fileInfo_clone(fileInfo);
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


