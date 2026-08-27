/*****************************************************************************
* \file      server_cfg.h
* \author    Conny Gustafsson
* \date      2026-08-28
* \brief     APX Server configuration loader (Directory & File)
*
* Copyright (c) 2026 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
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
 * Loads server configuration from a path (directory or JSON file).
 *
 * If path is a directory:
 *   - Reads <path>/server.json (or <path>/apx_server.json) into *server_config
 *   - Reads individual extension JSON files (<path>/<key>.json) into *extensions_config
 *
 * If path is a file:
 *   - Reads the JSON file and extracts "server" and "extension" objects
 *
 * Caller takes ownership of the returned dtl_hv_t pointers and should release them with dtl_dec_ref().
 */
apx_error_t apx_server_load_config(const char *path, dtl_hv_t **server_config, dtl_hv_t **extensions_config);

#endif //APX_SERVER_CFG_H
