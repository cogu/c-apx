/*****************************************************************************
* \file      server_extension.h
* \author    Conny Gustafsson
* \date      2019-09-05
* \brief     APX server extension data structure
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

typedef struct apx_serverExtensionHandler_tag
{
   apx_error_t (*init)(struct apx_server_tag *apx_server, dtl_dv_t *config);
   void (*shutdown)(void);
} apx_serverExtensionHandler_t;

typedef struct apx_serverExtension_tag
{
   apx_serverExtensionHandler_t handler;
   dtl_dv_t *config;
   char *name;
} apx_serverExtension_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_serverExtension_create(apx_serverExtension_t *self, const char *name, const apx_serverExtensionHandler_t *handler, dtl_dv_t *config);
void apx_serverExtension_destroy(apx_serverExtension_t *self);
apx_serverExtension_t* apx_serverExtension_new(const char *name, const apx_serverExtensionHandler_t *handler, dtl_dv_t *config);
void apx_serverExtension_delete(apx_serverExtension_t *self);
void apx_serverExtension_vdelete(void *arg);

#endif //APX_SERVER_EXTENTION_H
