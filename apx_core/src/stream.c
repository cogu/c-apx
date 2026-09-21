/*****************************************************************************
* \file      stream.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX input stream
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
typedef struct apx_header_line_tag
{
   int16_t majorVersion;
   int16_t minorVersion;
} apx_header_line_t;

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
      adt_bytearray_create(&self->buf);
      apx_istream_set_handler(self, NULL);
      self->last_error = APX_NO_ERROR;
   }
}

void apx_istream_destroy(apx_istream_t *self){
   if(self != NULL)
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
   if (self != NULL)
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
         const uint8_t* line_end = bstr_find_line_feed(line_begin, buffer_end);
         if (line_end == buffer_end)
         {
            break; //Wait for more data
         }
         else if (line_end == line_begin)
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
            adt_bytearray_trim_left(&self->buf, line_begin);
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