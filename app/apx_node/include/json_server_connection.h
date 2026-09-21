/*****************************************************************************
* \file      json_server_connection.h
* \author    Conny Gustafsson
* \date      2020-03-07
* \brief     Connection in the json server
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef MESSAGE_SERVER_CONNECTION_H
#define MESSAGE_SERVER_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "msocket.h"
//forward declarations
struct apx_connection_tag;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct json_server_connection_tag
{
   msocket_t *msocket; //Strong reference
   struct apx_connection_tag *apx_connection; //Weak reference
} json_server_connection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void json_server_connection_create(json_server_connection_t *self, msocket_t *msocket, struct apx_connection_tag *apx_connection);
void json_server_connection_destroy(json_server_connection_t *self);
json_server_connection_t *json_server_connection_new(msocket_t *msocket, struct apx_connection_tag *apx_connection);
void json_server_connection_delete(json_server_connection_t *self);
void json_server_connection_vdelete(void *arg);
void json_server_connection_start(json_server_connection_t *self);

#endif //MESSAGE_SERVER_CONNECTION_H
