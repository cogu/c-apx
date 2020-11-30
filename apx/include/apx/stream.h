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
#ifndef APX_STREAM_H
#define APX_STREAM_H
#include "apx/types.h"
#include "adt_bytearray.h"

#define APX_ISTREAM_STATE_HEADER    0
#define APX_ISTREAM_STATE_NODE      1
#define APX_ISTREAM_STATE_TYPES     2
#define APX_ISTREAM_STATE_PORTS     3


typedef struct apx_istream_handler_t{
   //user-defined argument
   void *arg;

   //events
   void (*open)(void *arg);
   void (*close)(void *arg);

   //text-based messages
   bool (*header)(void *arg, int16_t majorVersion, int16_t minorVersion);
   void (*node)(void *arg, const char *name, int32_t lineNumber); //N"<name>"
   int32_t (*datatype)(void *arg, const char *name, const char *dsg, const char *attr, int32_t lineNumber); //T"<name>"<dsg>:<attr>
   int32_t (*require)(void *arg, const char *name, const char *dsg, const char *attr, int32_t lineNumber); //R"<name>"<dsg>:<attr>
   int32_t (*provide)(void *arg, const char *name, const char *dsg, const char *attr, int32_t lineNumber); //P"<name>"<dsg>:<attr>
   void (*node_end)(void *arg);

   //errors
   void (*parse_error)(void *arg, int32_t errorCode, int32_t line);
}apx_istream_handler_t;


typedef struct apx_declarationLine_tag
{
   char* pAlloc;
   uint32_t allocLen;
   char* name;
   char* dsg;
   char* attr;
   uint8_t lineType;
}apx_declarationLine_t;

typedef struct apx_istream_t{
   apx_istream_handler_t handler;
   adt_bytearray_t buf;
   uint8_t parseState;
   apx_declarationLine_t declarationLine;
   int32_t currentLine;
}apx_istream_t;



/***************** Public Function Declarations *******************/

//constructor/destructor
void apx_istream_create(apx_istream_t *self, apx_istream_handler_t *handler);
void apx_istream_destroy(apx_istream_t *self);
apx_istream_t *apx_istream_new(apx_istream_handler_t *handler);
void apx_istream_delete(apx_istream_t *self);
void apx_istream_reset(apx_istream_t *self);

//istream functions
void apx_istream_open(apx_istream_t *self);
void apx_istream_close(apx_istream_t *self);
void apx_istream_write(apx_istream_t *self, const uint8_t *pChunk, uint32_t chunkLen);


void apx_istream_vopen(void *arg);
void apx_istream_vwrite(void *arg, const uint8_t *pChunk, uint32_t chunkLen);
void apx_istream_vclose(void *arg);

// apx_declarationLine_t functions
void apx_declarationLine_create(apx_declarationLine_t *self);
void apx_declarationLine_destroy(apx_declarationLine_t *self);
int8_t apx_declarationLine_resize(apx_declarationLine_t *self, uint32_t len);


#endif //APX_STREAM_H

