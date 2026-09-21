/*****************************************************************************
* \file      socket_server_extension.h
* \author    Conny Gustafsson
* \date      2019-09-04
* \brief     APX socket server extension (TCP+UNIX)
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_SERVER_SOCKET_EXTENSION_H
#define APX_SERVER_SOCKET_EXTENSION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/server_extension.h"
#ifdef UNIT_TEST
#include "testsocket.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_SOCKET_SERVER_EXT_CFG_KEY "socket-server"
#define TCP_USER_PORT_BEGIN 1024
#define TCP_USER_PORT_END   49151

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_socketServerExtension_register(struct apx_server_tag *apx_server, dtl_dv_t *config);

#ifdef UNIT_TEST
void apx_socketServerExtension_accept_testsocket(testsocket_t *sock);
#endif

#endif //APX_SERVER_SOCKET_EXTENSION_H
