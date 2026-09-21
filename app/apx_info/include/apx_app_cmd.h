/*****************************************************************************
* \file      apx_app_cmd.h
* \author    Conny Gustafsson
* \date      2021-01-01
* \brief     Program action definitions
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_APP_COMMAND_H
#define APX_APP_COMMAND_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef uint8_t apx_app_cmd_t;
#define APX_APP_CMD_NONE        ((apx_app_cmd_t) 0u)
#define APX_APP_CMD_CLIENTS     ((apx_app_cmd_t) 1u)

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

#endif //APX_APP_COMMAND_H
