/*****************************************************************************
* \file      server_text_log.h
* \author    Conny Gustafsson
* \date      2019-09-12
* \brief     Server text log
*
* Copyright (c) 2019-2020 Conny Gustafsson
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

typedef struct apx_serverTextLog_tag
{
   apx_textLogBase_t base;
   struct apx_server_tag *server;
} apx_serverTextLog_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_serverTextLog_create(apx_serverTextLog_t *self, struct apx_server_tag *server);
void apx_serverTextLog_destroy(apx_serverTextLog_t *self);
void apx_serverTextLog_vdestroy(void *arg);
apx_serverTextLog_t *apx_serverTextLog_new(struct apx_server_tag *server);
void apx_serverTextLog_delete(apx_serverTextLog_t *self);

void apx_serverTextLog_enableFile(apx_serverTextLog_t *self, const char *path);
void apx_serverTextLog_enableStdOut(apx_serverTextLog_t *self);
void apx_serverTextLog_enableSysLog(apx_serverTextLog_t *self, const char *label);
void apx_serverTextLog_closeAll(apx_serverTextLog_t *self);

//Virtual functions
void apx_serverTextLog_virtual_on_protocol_header_accepted(void* arg, struct apx_connectionBase_tag* connection);
void apx_serverTextLog_virtual_on_file_published(void* arg, struct apx_connectionBase_tag* connection, const struct rmf_fileInfo_tag* file_info);
void apx_serverTextLog_virtual_on_file_revoked(void* arg, struct apx_connectionBase_tag* connection, const struct rmf_fileInfo_tag* file_info);


#endif //APX_SERVER_TEXT_LOG_H
