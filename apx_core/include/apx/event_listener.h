/*****************************************************************************
* \file      event_listener.h
* \author    Conny Gustafsson
* \date      2020-01-03
* \brief     Event listener API
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_EVENT_LISTENER_H
#define APX_EVENT_LISTENER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"

//forward declarations
struct apx_serverConnection_tag;
struct apx_clientConnection_tag;
struct apx_portConnectionTable_tag;
struct rmf_fileInfo_tag;
struct apx_fileInfo_tag;
struct apx_file_tag;
struct apx_connectionBase_tag;
struct apx_nodeInstance_tag;
struct apx_portInstance_tag;



//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//Client/Server typedefs
typedef void (apx_clientConnectionEventFunc_t)(void* arg, struct apx_clientConnection_tag* connection);
typedef void (apx_serverConnectionEventFunc_t)(void* arg, struct apx_serverConnection_tag* connection);
typedef void (apx_serverLogWriteEventFunc_t)(void* arg, apx_logLevel_t level, const char* label, const char* msg);

//Connection typedefs
typedef void (apx_protocolHeaderAcceptedFunc_t)(void* arg, struct apx_connectionBase_tag* connection);
typedef void (apx_portDataWriteFunc_t)(void* arg, struct apx_portInstance_tag* port_instance, uint8_t const* data, apx_size_t size);
typedef void (apx_fileEventFunc_t)(void* arg, struct apx_connectionBase_tag* connection, const struct rmf_fileInfo_tag* file_info);

typedef struct apx_clientEventListener_tag
{
   void *arg;
   apx_clientConnectionEventFunc_t* connected;
   apx_clientConnectionEventFunc_t* disconnected;
   apx_portDataWriteFunc_t* require_port_write;
} apx_clientEventListener_t;

typedef struct apx_serverEventListener_tag
{
   void *arg;
   apx_serverConnectionEventFunc_t* new_connection;
   apx_serverConnectionEventFunc_t* connection_closed;
   apx_serverLogWriteEventFunc_t* server_write_log;
} apx_serverEventListener_t;

typedef struct apx_serverConnectionEventListener_tag
{
   void *arg;
   apx_protocolHeaderAcceptedFunc_t* protocol_header_accepted;
   apx_fileEventFunc_t* file_published;
   apx_fileEventFunc_t* file_revoked;
} apx_serverConnectionEventListener_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_clientEventListener_t *apx_clientEventListener_clone(apx_clientEventListener_t *other);
void apx_clientEventListener_delete(apx_clientEventListener_t *self);
void apx_clientEventListener_vdelete(void *arg);

apx_serverEventListener_t *apx_serverEventListener_clone(apx_serverEventListener_t *other);
void apx_serverEventListener_delete(apx_serverEventListener_t *self);
void apx_serverEventListener_vdelete(void *arg);

apx_serverConnectionEventListener_t *apx_connectionEventListener_clone(apx_serverConnectionEventListener_t *other);
void apx_connectionEventListener_delete(apx_serverConnectionEventListener_t *self);
void apx_connectionEventListener_vdelete(void *arg);



#endif //APX_EVENT_LISTENER_H
