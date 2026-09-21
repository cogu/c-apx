/*****************************************************************************
* \file      client_internal.h
* \author    Conny Gustafsson
* \date      2019-01-15
* \brief     Private API used internally by c-apx
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_CLIENT_INTERNAL_H
#define APX_CLIENT_INTERNAL_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/client.h"
#include "apx/node_instance.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
//Client internal API (do not call as end-user)
void apx_client_internal_connect_notification(apx_client_t *self, apx_client_connection_t *connection);
void apx_client_internal_disconnect_notification(apx_client_t *self, apx_client_connection_t *connection);
void apx_client_internal_require_port_write_notification(apx_client_t *self, apx_client_connection_t *connection, apx_port_instance_t *port_instance, const uint8_t *data, apx_size_t size);


#endif //APX_CLIENT_INTERNAL_H
