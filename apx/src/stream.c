/*****************************************************************************
* \file      stream.c
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/stream.h"
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "bstr.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_headerLine_tag
{
   int16_t majorVersion;
   int16_t minorVersion;
} apx_headerLine_t;

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_istream_handler_open(apx_istream_handler_t const* handler);
static apx_error_t apx_istream_handler_close(apx_istream_handler_t const* handler);
static apx_error_t apx_istream_handler_new_line(apx_istream_handler_t const* handler, const char* begin, const char* end);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_istream_destroy(apx_istream_t* self);
void apx_istream_reset(apx_istream_t* self);

void apx_istream_open(apx_istream_t* self);
void apx_istream_close(apx_istream_t* self);
void apx_istream_write(apx_istream_t* self, uint8_t const* chunk, uint32_t chunk_len);

void apx_istream_vopen(void* arg);
void apx_istream_vwrite(void* arg, uint8_t const* chunk, uint32_t chunk_len);
void apx_istream_vclose(void* arg);


void apx_istream_create(apx_istream_t* self)
{
   if(self != NULL)
   {
      adt_bytearray_create(&self->buf, APX_BUF_GROW_SIZE);
      apx_istream_set_handler(self, NULL);
      self->last_error = APX_NO_ERROR;
   }
}

void apx_istream_destroy(apx_istream_t *self){
   if(self != 0)
   {
      adt_bytearray_destroy(&self->buf);
   }
}

void apx_istream_set_handler(apx_istream_t* self, apx_istream_handler_t const* handler)
{
   if (self != NULL)
   {
      if (handler != NULL)
      {
         memcpy(&self->handler, handler, sizeof(apx_istream_handler_t));
      }
      else
      {
         memset(&self->handler, 0u, sizeof(apx_istream_handler_t));
      }
   }
}

void apx_istream_reset(apx_istream_t* self)
{
   if (self != 0)
   {
      adt_bytearray_clear(&self->buf);
      self->last_error = APX_NO_ERROR;
   }
}

void apx_istream_open(apx_istream_t *self)
{
   if ( (self != NULL) && (self->last_error == APX_NO_ERROR) )
   {
      self->last_error = apx_istream_handler_open(&self->handler);
   }
}

void apx_istream_close(apx_istream_t* self)
{
   if (self != NULL && (self->last_error == APX_NO_ERROR))
   {
      const uint8_t* buffer_begin;
      const uint8_t* buffer_end;
      buffer_begin = adt_bytearray_data(&self->buf);
      buffer_end = buffer_begin + adt_bytearray_length(&self->buf);
      if ( (buffer_begin != NULL) && (buffer_end != NULL) && (buffer_end > buffer_begin))
      {
         self->last_error = apx_istream_handler_new_line(&self->handler, (const char*) buffer_begin, (const char*) buffer_end);
         if (self->last_error != APX_NO_ERROR)
         {
            return;
         }
      }
      self->last_error = apx_istream_handler_close(&self->handler);
   }
}

void apx_istream_write(apx_istream_t* self, uint8_t const* chunk, uint32_t chunk_len)
{
   if ( (self != NULL) && (chunk != NULL) && (chunk_len != 0u) )
   {
      const uint8_t* buffer_begin;
      const uint8_t* buffer_end;
      const uint8_t* line_begin;
      adt_bytearray_append(&self->buf, chunk, chunk_len);
      buffer_begin = line_begin = adt_bytearray_data(&self->buf);
      buffer_end = buffer_begin + adt_bytearray_length(&self->buf);

      while (line_begin < buffer_end)
      {
         const uint8_t* line_end = bstr_search_val(line_begin, buffer_end, (uint8_t)'\n');
         if (line_end == line_begin)
         {
            const uint8_t first_byte = *line_begin;
            if (first_byte == '\n')
            {
               //empty line
               self->last_error = apx_istream_handler_new_line(&self->handler, (const char*) line_begin++, (const char*) line_end);
               if (self->last_error != APX_NO_ERROR)
               {
                  return;
               }
            }
            else
            {
               break; //Wait for next write (or possible close)
            }
         }
         else if (line_end > line_begin)
         {
            size_t eol_size = 1u;
            if (line_end[-1] == '\r')
            {
               line_end--;
               eol_size = 2;
            }
            self->last_error = apx_istream_handler_new_line(&self->handler, (const char*) line_begin, (const char*) line_end);
            if (self->last_error != APX_NO_ERROR)
            {
               return;
            }
            line_begin = line_end + eol_size; //skip past the new-line character(s)
         }
         else
         {
            break; //Wait for more data
         }
      }
      if ( (line_begin > buffer_begin) )
      {
         if (line_begin == buffer_end)
         {
            adt_bytearray_clear(&self->buf);
         }
         else
         {
            assert(line_begin < buffer_end);
            adt_bytearray_trimLeft(&self->buf, line_begin);
         }
      }
   }
}

void apx_istream_vopen(void *arg)
{
   apx_istream_open((apx_istream_t*) arg);
}

void apx_istream_vwrite(void *arg, const uint8_t *pChunk, uint32_t chunkLen)
{
   apx_istream_write((apx_istream_t*) arg,pChunk,chunkLen);
}

void apx_istream_vclose(void *arg)
{
   apx_istream_close((apx_istream_t*) arg);
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_istream_handler_open(apx_istream_handler_t const* handler)
{
   if( (handler != NULL) && (handler->open != NULL) )
   {
      return handler->open(handler->arg);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_istream_handler_close(apx_istream_handler_t const* handler)
{
   if( (handler != NULL) && (handler->close != NULL) )
   {
      return handler->close(handler->arg);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_istream_handler_new_line(apx_istream_handler_t const* handler, const char* begin, const char* end)
{
   if ( (handler != NULL) && (handler->new_line != NULL) )
   {
      return handler->new_line(handler->arg, begin, end);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}