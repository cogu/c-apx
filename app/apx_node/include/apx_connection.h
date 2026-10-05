/*****************************************************************************
 * \file      apx_connection.h
 * \author    Conny Gustafsson
 * \date      2020-04-14
 * \brief     APX client connection
 *
 * Copyright (c) 2020-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_CONNECTION_H
#define APX_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
#else
# include <pthread.h>
#endif
#include "adt_ary.h"
#include "adt_hash.h"
#include "adt_str.h"
#include "apx/client.h"
#include "osmacro.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_connection_tag
{
  apx_client_t *client;
  adt_hash_t provide_port_lookup_table; // Key is provide port name, value is port instance (apx_port_instance_t*) (weak
                                        // references)
  MUTEX_T mutex;
} apx_connection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_connection_create(apx_connection_t *self);
void apx_connection_destroy(apx_connection_t *self);
apx_connection_t *apx_connection_new(void);
void apx_connection_delete(apx_connection_t *self);

void apx_connection_disconnect(apx_connection_t *self);
apx_error_t apx_connection_attach_node(apx_connection_t *self, adt_str_t *apx_definition);
apx_error_t apx_connection_attach_node_signed(
  apx_connection_t *self, adt_str_t *apx_definition, rmf_signature_type_t sig_type, uint8_t const *sig_data);
int32_t apx_connection_get_last_error_line(apx_connection_t *self);
apx_error_t apx_connection_get_last_error(apx_connection_t *self);
char const *apx_connection_get_last_error_node(apx_connection_t *self);
apx_node_instance_t *apx_connection_get_last_attached_node(apx_connection_t *self);
struct msocket_tls_config_tag;

#ifndef _WIN32
apx_error_t apx_connection_connect_unix(apx_connection_t *self, const char *socket_path);
apx_error_t apx_connection_connect_vsock(apx_connection_t *self, uint32_t cid, uint32_t port);
#endif
apx_error_t apx_connection_connect_tcp(apx_connection_t *self, const char *address, uint16_t port);
apx_error_t apx_connection_connect_tls(
  apx_connection_t *self, const char *address, uint16_t port, const struct msocket_tls_config_tag *tls_config);
apx_error_t apx_connection_write_provide_port_data(
  apx_connection_t *self, const char *provide_port_name, dtl_dv_t *dv_value);

#endif // APX_CONNECTION_H
