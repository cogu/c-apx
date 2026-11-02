/*****************************************************************************
 * \file      server_text_log_extension.h
 * \author    Conny Gustafsson
 * \date      2019-09-08
 * \brief     Text log extension for APX server
 *
 * Copyright (c) 2019-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_SERVER_TEXT_LOG_EXTENSION_H
#define APX_SERVER_TEXT_LOG_EXTENSION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/server_extension.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_SERVER_TEXTLOG_CFG_KEY "textlog"
//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_server_text_log_extension_register(struct apx_server_tag *apx_server, dtl_dv_t *config);
const char *const *apx_server_text_log_extension_config_keys(void);
void apx_server_text_log_extension_check_config(dtl_hv_t *cfg, apx_config_warning_cb cb, void *arg);

#endif // APX_SERVER_TEXT_LOG_EXTENSION_H
