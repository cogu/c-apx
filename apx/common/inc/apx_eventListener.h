/*****************************************************************************
* \file      apx_eventListener.h
* \author    Conny Gustafsson
* \date      2018-05-01
* \brief     Interface for internal event listerner
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
#ifndef APX_EVENT_LISTENER_H
#define APX_EVENT_LISTENER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <apx_types.h>

//forward declarations
struct apx_file2_tag;
struct apx_fileManager_tag;
struct apx_nodeData_tag;
struct apx_serverConnectionBase_tag;
struct apx_clientConnectionBase_tag;
struct apx_portConnectionTable_tag;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

typedef struct apx_clientEventListener_tag
{
   void *arg;
   void (*clientConnected)(void *arg, struct apx_clientConnectionBase_tag *clientConnection);            //type 1 event
   void (*clientDisconnected)(void *arg, struct apx_clientConnectionBase_tag *clientConnection);         //type 1 event
   void (*nodeCompleted)(void *arg, struct apx_nodeData_tag *nodeData);                                  //type 2 event
   void (*logEvent)(void *arg, apx_logLevel_t level, const char *label, const char *msg);                //type 2 event
}apx_clientEventListener_t;

typedef struct apx_serverEventListener_tag
{
   void *arg;
   void (*serverConnected)(void *arg, struct apx_serverConnectionBase_tag *connection);                  //type 1 event
   void (*serverDisconnected)(void *arg, struct apx_serverConnectionBase_tag *connection);               //type 1 event
   void (*nodeCompleted)(void *arg, struct apx_nodeData_tag *nodeData);                                  //type 2 event
   void (*logEvent)(void *arg, apx_logLevel_t level, const char *label, const char *msg);                //type 2 event
}apx_serverEventListener_t;

typedef void (apx_eventListener_fileManagerEventFunc_t)(void *arg, struct apx_fileManager_tag *fileManager);
typedef void (apx_eventListener_fileManagerFileEventFunc_t)(void *arg, struct apx_fileManager_tag *fileManager, struct apx_file2_tag *file);
typedef struct apx_fileManagerEventListener_tag
{
   void *arg;
   apx_eventListener_fileManagerEventFunc_t *managerStart;    //type 2 event
   apx_eventListener_fileManagerEventFunc_t *managerStop;     //type 2 event
   apx_eventListener_fileManagerEventFunc_t *headerComplete;  //type 2 event
   apx_eventListener_fileManagerFileEventFunc_t *fileCreate;  //type 2 event
   apx_eventListener_fileManagerFileEventFunc_t *fileRevoke;  //type 2 event
   apx_eventListener_fileManagerFileEventFunc_t *fileOpen;    //type 2 event
   apx_eventListener_fileManagerFileEventFunc_t *fileClose;   //type 2 event
} apx_fileManagerEventListener_t;

typedef void (apx_eventListener_nodeDataWriteFunc_t)(void *arg, struct apx_nodeData_tag *nodeData, uint32_t offset, uint32_t len);
typedef void (apx_eventListener_nodeDataFunc_t)(void *arg, struct apx_nodeData_tag *nodeData);
typedef void (apx_eventListener_nodeDataPortConnectFunc_t)(void *arg, struct apx_nodeData_tag *nodeData, struct apx_portConnectionTable_tag *connectionTable);

typedef struct apx_nodeDataEventListener_tag
{
   void *arg; //user argument
   apx_eventListener_nodeDataWriteFunc_t *definitionDataWritten;          //type 1 event
   apx_eventListener_nodeDataWriteFunc_t *inPortDataWritten;              //type 1 event
   apx_eventListener_nodeDataWriteFunc_t *outPortDataWritten;             //type 1 event
   apx_eventListener_nodeDataFunc_t *nodeComplete;                        //type 2 event
   apx_eventListener_nodeDataPortConnectFunc_t *requirePortsConnected;    //type 2 event
   apx_eventListener_nodeDataPortConnectFunc_t *providePortsConnected;    //type 2 event
   apx_eventListener_nodeDataPortConnectFunc_t *requirePortsDisconnected; //type 2 event
   apx_eventListener_nodeDataPortConnectFunc_t *providePortsDisconnected; //type 2 event
} apx_nodeDataEventListener_t;


typedef void (apx_eventListener_fileWriteNotifyFunc_t)(void *arg, struct apx_file2_tag *file, uint32_t offset, uint32_t len);

typedef struct apx_fileEventListener_tag
{
   void *arg; //user argument
   apx_eventListener_fileWriteNotifyFunc_t *writeNotify; //type 1 event
} apx_fileEventListener_t;



//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

apx_clientEventListener_t *apx_clientEventListener_clone(apx_clientEventListener_t *other);
void apx_clientEventListener_delete(apx_clientEventListener_t *self);
void apx_clientEventListener_vdelete(void *arg);

apx_serverEventListener_t *apx_serverEventListener_clone(apx_serverEventListener_t *other);
void apx_serverEventListener_delete(apx_serverEventListener_t *self);
void apx_serverEventListener_vdelete(void *arg);

apx_fileManagerEventListener_t *apx_fileManagerEventListener_clone(apx_fileManagerEventListener_t *other);
void apx_fileManagerEventListener_delete(apx_fileManagerEventListener_t *self);
void apx_fileManagerEventListener_vdelete(void *arg);

apx_nodeDataEventListener_t *apx_nodeDataEventListener_clone(apx_nodeDataEventListener_t *other);
void apx_nodeDataEventListener_delete(apx_nodeDataEventListener_t *self);
void apx_nodeDataEventListener_vdelete(void *arg);

apx_fileEventListener_t *apx_fileEventListener_clone(apx_fileEventListener_t *other);
void apx_fileEventListener_delete(apx_fileEventListener_t *self);
void apx_fileEventListener_vdelete(void *arg);


#endif //APX_EVENT_LISTENER_H
