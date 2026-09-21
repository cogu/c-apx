/*****************************************************************************
* \file      server_text_log.h
* \author    Conny Gustafsson
* \date      2019-09-12
* \brief     Server text log
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_SERVER_TEXT_LOG_H
#define APX_SERVER_TEXT_LOG_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/connection_base.h"
#include "apx/file_info.h"
#include "apx/extension/text_log_base.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_server_tag;

typedef struct apx_server_text_log_tag
{
   apx_text_log_base_t base;
   struct apx_server_tag *server;
} apx_server_text_log_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_serverTextLog_create(apx_server_text_log_t *self, struct apx_server_tag *server);
void apx_serverTextLog_destroy(apx_server_text_log_t *self);
void apx_serverTextLog_vdestroy(void *arg);
apx_server_text_log_t *apx_serverTextLog_new(struct apx_server_tag *server);
void apx_serverTextLog_delete(apx_server_text_log_t *self);

void apx_serverTextLog_enableFile(apx_server_text_log_t *self, const char *path);
void apx_serverTextLog_enableStdOut(apx_server_text_log_t *self);
void apx_serverTextLog_enableSysLog(apx_server_text_log_t *self, const char *label);
void apx_serverTextLog_closeAll(apx_server_text_log_t *self);

//Virtual functions
void apx_serverTextLog_virtual_on_protocol_header_accepted(void* arg, struct apx_connection_base_tag* connection);
void apx_serverTextLog_virtual_on_file_published(void* arg, struct apx_connection_base_tag* connection, const struct rmf_file_info_tag* file_info);
void apx_serverTextLog_virtual_on_file_revoked(void* arg, struct apx_connection_base_tag* connection, const struct rmf_file_info_tag* file_info);


#endif //APX_SERVER_TEXT_LOG_H
