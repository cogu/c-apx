/*****************************************************************************
* \file      file_manager_receiver.h
* \author    Conny Gustafsson
* \date      2020-02-08
* \brief     Receive buffer mechanism for file manager
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
#ifndef APX_FILEMANAGER_RECEIVER_H
#define APX_FILEMANAGER_RECEIVER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "rmf.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_fileManagerReceiver_tag
{
   uint8_t *receiveBuf;        //Receive buffer
   apx_size_t receiveBufSize;  //Allocated size of receiveBuf
   apx_size_t receiveBufPos;   //current write position (and length) of receive buffer
   uint32_t startAddress;      //StartAddress of write, When value is RMF_INVALID_ADDRESS it means no reception is currently taking place
   bool isFragmentedWrite;     //True as long as moreBit is true
} apx_fileManagerReceiver_t;

typedef struct apx_fileManagerReception_tag
{
   uint32_t startAddress;
   const uint8_t *msgBuf;
   apx_size_t msgSize;
} apx_fileManagerReception_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_fileManagerReceiver_create(apx_fileManagerReceiver_t *self);
void apx_fileManagerReceiver_destroy(apx_fileManagerReceiver_t *self);
void apx_fileManagerReceiver_reset(apx_fileManagerReceiver_t *self);
apx_error_t apx_fileManagerReceiver_reserve(apx_fileManagerReceiver_t *self, apx_size_t size);
bool apx_fileManagerReceiver_isOngoing(apx_fileManagerReceiver_t *self);
apx_error_t apx_fileManagerReceiver_write(apx_fileManagerReceiver_t *self, uint32_t address, const uint8_t *data, apx_size_t size, bool moreBit);
uint32_t apx_fileManagerReceiver_getAddress(apx_fileManagerReceiver_t *self);
apx_size_t apx_fileManagerReceiver_getSize(apx_fileManagerReceiver_t *self, apx_size_t *size);
apx_error_t apx_fileManagerReceiver_checkComplete(apx_fileManagerReceiver_t *self, apx_fileManagerReception_t *reception);


#endif //APX_FILEMANAGER_RECEIVER_H
