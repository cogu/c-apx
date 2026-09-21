/*****************************************************************************
* \file      observed_connection.h
* \author    Conny Gustafsson
* \date      2021-04-10
* \brief     Current connection state seen by a server monitor
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_OBSERVED_CONNECTION_H
#define APX_OBSERVED_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "adt_str.h"
#include "adt_list.h"
#include "apx/server_connection.h"
#include "apx/extension/observed_file.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

typedef struct apx_observed_connection_tag
{
   apx_server_connection_t* server_connection;
   apx_connection_id_t connection_id;
   apx_connection_type_t connection_type;
   apx_connection_state_t connection_state;
   adt_str_t* tag;
   adt_list_t file_list; //strong references to apx_observed_file_t
} apx_observed_connection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////


void apx_observed_connection_create(apx_observed_connection_t* self, apx_server_connection_t* server_connection);
void apx_observed_connection_destroy(apx_observed_connection_t* self);
apx_observed_connection_t* apx_observed_connection_new(apx_server_connection_t* server_connection);
void apx_observed_connection_delete(apx_observed_connection_t* self);
void apx_observed_connection_vdelete(void* arg);
char const* apx_observed_connection_tag(apx_observed_connection_t* const self);
apx_connection_id_t apx_observed_connection_connection_id(apx_observed_connection_t* const self);
apx_connection_type_t apx_observed_connection_get_connection_type(apx_observed_connection_t* const self);
void apx_observed_connection_set_connection_type(apx_observed_connection_t* self, apx_connection_type_t connection_type);
apx_connection_state_t apx_observed_connection_get_connection_state(apx_observed_connection_t* const self);
void apx_observed_connection_set_connection_state(apx_observed_connection_t* self, apx_connection_state_t connection_state);

#endif //APX_OBSERVED_CONNECTION_H
