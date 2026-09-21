/*****************************************************************************
* \file      server_extension.h
* \author    Conny Gustafsson
* \date      2019-09-05
* \brief     APX server extension data structure
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_SERVER_EXTENTION_H
#define APX_SERVER_EXTENTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "dtl_type.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_server_tag;

typedef struct apx_server_extension_handler_tag
{
   apx_error_t (*init)(struct apx_server_tag *apx_server, dtl_dv_t *config);
   void (*shutdown)(void);
} apx_server_extension_handler_t;

typedef struct apx_server_extension_tag
{
   apx_server_extension_handler_t handler;
   dtl_dv_t *config;
   char *name;
} apx_server_extension_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_serverExtension_create(apx_server_extension_t *self, const char *name, const apx_server_extension_handler_t *handler, dtl_dv_t *config);
void apx_serverExtension_destroy(apx_server_extension_t *self);
apx_server_extension_t* apx_serverExtension_new(const char *name, const apx_server_extension_handler_t *handler, dtl_dv_t *config);
void apx_serverExtension_delete(apx_server_extension_t *self);
void apx_serverExtension_vdelete(void *arg);

#endif //APX_SERVER_EXTENTION_H
