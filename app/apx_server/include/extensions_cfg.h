/*****************************************************************************
* \file      extensions_cfg.h
* \author    Conny Gustafsson
* \date      2026-08-28
* \brief     Extension configuration and static registry
*
* Copyright (c) 2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef EXTENSIONS_CFG_H
#define EXTENSIONS_CFG_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/server.h"
#include "dtl_type.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef apx_error_t (*apx_extension_register_fn)(struct apx_server_tag *apx_server, dtl_dv_t *config);

typedef struct apx_server_extension_entry_tag
{
   const char *name;
   apx_extension_register_fn register_fn;
} apx_server_extension_entry_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

const apx_server_extension_entry_t* apx_server_get_registered_extensions(void);
apx_error_t register_apx_server_extensions(struct apx_server_tag *server, dtl_hv_t *extensions_config);

#endif //EXTENSIONS_CFG_H
