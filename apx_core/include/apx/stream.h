/*****************************************************************************
* \file      stream.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX input stream
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_STREAM2_H
#define APX_STREAM2_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "adt_bytearray.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_istream_handler_tag
{
   void* arg;
   apx_error_t (*open)(void* arg);
   apx_error_t (*close)(void* arg);
   apx_error_t (*new_line)(void* arg, const char* begin, const char* end);
} apx_istream_handler_t;

typedef struct apx_istream_t
{
   apx_istream_handler_t handler;
   adt_bytearray_t buf;
   apx_error_t last_error;
} apx_istream_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

void apx_istream_create(apx_istream_t *self);
void apx_istream_destroy(apx_istream_t *self);
void apx_istream_reset(apx_istream_t *self);
void apx_istream_set_handler(apx_istream_t* self, apx_istream_handler_t const* handler);

void apx_istream_open(apx_istream_t *self);
void apx_istream_close(apx_istream_t *self);
void apx_istream_write(apx_istream_t *self, uint8_t const* chunk, uint32_t chunk_len);

void apx_istream_vopen(void *arg);
void apx_istream_vwrite(void *arg, uint8_t const* chunk, uint32_t chunk_len);
void apx_istream_vclose(void *arg);

#endif //APX_STREAM2_H

