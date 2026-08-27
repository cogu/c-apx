/*****************************************************************************
* \file      stream.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX input stream
*
* Copyright (c) 2017-2020 Conny Gustafsson
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

