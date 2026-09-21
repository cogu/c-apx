/*****************************************************************************
* \file      json_server.h
* \author    Conny Gustafsson
* \date      2020-03-07
* \brief     Server that listens for messages forwarded by apx_control application
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef MESSAGE_SERVER_H
#define MESSAGE_SERVER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/error.h"
#include "json_server_connection.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t json_server_init(struct apx_connection_tag *apx_connection, uint8_t address_family);
apx_error_t json_server_start_unix(const char *socket_path);
apx_error_t json_server_start_tcp(const char *bind_address, uint16_t port);
void json_server_cleanup_connection(json_server_connection_t *connection);
void json_server_shutdown(void);

#endif //MESSAGE_SERVER_H
