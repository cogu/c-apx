/*****************************************************************************
* \file      server_cfg.h
* \author    Conny Gustafsson
* \date      2026-08-28
* \brief     APX Server configuration loader (Directory & File)
*
* Copyright (c) 2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_SERVER_CFG_H
#define APX_SERVER_CFG_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "dtl_type.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

/**
 * Loads server configuration from a path (JSON file or directory containing server.json).
 *
 * If path is a directory:
 *   - Resolves <path>/server.json (or fallback <path>/apx_server.json)
 *
 * If path is a file:
 *   - Reads the specified JSON configuration file
 *
 * The JSON configuration file contains:
 *   - "apx-server": Server daemon settings (optional)
 *   - "<name>-extension": Extension settings
 *
 * Caller takes ownership of the returned dtl_hv_t pointer and should release it with dtl_dec_ref().
 */
apx_error_t apx_server_load_config(const char *path, dtl_hv_t **config);

#endif //APX_SERVER_CFG_H
