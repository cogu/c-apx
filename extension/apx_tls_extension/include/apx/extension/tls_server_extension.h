/*****************************************************************************
* \file      tls_server_extension.h
* \author    Conny Gustafsson
* \date      2026-09-27
* \brief     APX TLS server extension
*
* Copyright (c) 2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_TLS_SERVER_EXTENSION_H
#define APX_TLS_SERVER_EXTENSION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/server_extension.h"
#include "dtl_type.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_TLS_SERVER_EXT_CFG_KEY "tls-server"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_tls_server_extension_register(struct apx_server_tag *apx_server, dtl_dv_t *config);

#endif //APX_TLS_SERVER_EXTENSION_H
