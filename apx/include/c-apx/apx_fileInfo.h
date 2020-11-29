/*****************************************************************************
* \file      apx_fileInfo.h
* \author    Conny Gustafsson
* \date      2020-01-03
* \brief     Disposable file info data structure
*
* Copyright (c) 2020 Conny Gustafsson
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
#ifndef APX_FILE_INFO_H
#define APX_FILE_INFO_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "apx_error.h"
#include "rmf.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_fileInfo_tag
{
   uint32_t address;
   uint32_t addressWithoutFlags;
   uint32_t length;
   uint16_t fileType;
   uint16_t digestType;
   uint8_t *digestData;
   char *name;
} apx_fileInfo_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_fileInfo_create(apx_fileInfo_t *self, uint32_t address, uint32_t length, const char *name, uint16_t fileType, uint16_t digestType, const uint8_t *digestData);
apx_error_t apx_fileInfo_create_rmf(apx_fileInfo_t *self, const rmf_fileInfo_t *fileInfo, bool isRemoteAddress);
void apx_fileInfo_destroy(apx_fileInfo_t *self);
void apx_fileInfo_delete(apx_fileInfo_t *self);
void apx_fileInfo_vdelete(void *arg);
apx_fileInfo_t* apx_fileInfo_new(uint32_t address, uint32_t length, const char *name, uint16_t fileType, uint16_t digestType, const uint8_t *digestData);
apx_fileInfo_t* apx_fileInfo_new_rmf(rmf_fileInfo_t *fileInfo, bool isRemoteAddress);
apx_error_t apx_fileInfo_assign(apx_fileInfo_t *self, const apx_fileInfo_t *other);
apx_fileInfo_t* apx_fileInfo_clone(const apx_fileInfo_t *other);
void apx_fileInfo_setAddress(apx_fileInfo_t *self, uint32_t address);
void apx_fileInfo_fillRmfInfo(const apx_fileInfo_t *self, rmf_fileInfo_t *rmfInfo);
bool apx_fileInfo_isRemoteAddress(apx_fileInfo_t *self);
bool apx_fileInfo_nameEndsWith(const apx_fileInfo_t *self, const char* suffix);
char *apx_fileInfo_getBaseName(const apx_fileInfo_t *self);
void apx_fileInfo_copyBaseName(const apx_fileInfo_t *self, char *dest, uint32_t maxDestLen);

#endif //APX_FILE_INFO_H
