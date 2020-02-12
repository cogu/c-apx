/*****************************************************************************
* \file      apx_fileManager2.c
* \author    Conny Gustafsson
* \date      2020-01-27
* \brief     New APX file manager
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <stdio.h> //DEBUG only
#include "apx_connectionBase.h"
#include "apx_portDataRef2.h"
#include "apx_nodeData2.h"

#include "apx_portConnectionTable.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_fileManager2_create(apx_fileManager2_t *self, uint8_t mode, struct apx_connectionBase_tag *parentConnection)
{
   if (self != 0)
   {
      apx_fileManagerShared2_create(&self->shared);
      apx_fileManagerWorker_create(&self->worker, &self->shared, mode);
      self->parentConnection = parentConnection;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_fileManager2_destroy(apx_fileManager2_t *self)
{
   if (self != 0)
   {
      apx_fileManagerWorker_destroy(&self->worker);
      apx_fileManagerShared2_destroy(&self->shared);
   }
}

void apx_fileManager2_start(apx_fileManager2_t *self)
{

}

void apx_fileManager2_stop(apx_fileManager2_t *self)
{

}

apx_file2_t* apx_fileManager2_findFileByAddress(apx_fileManager2_t *self, uint32_t address)
{
   if (self != 0)
   {
      return apx_fileManagerShared2_findFileByAddress(&self->shared, address);
   }
   return (apx_file2_t*) 0;
}

void apx_fileManager2_setTransmitHandler(apx_fileManager2_t *self, apx_transmitHandler_t *handler)
{
   if (self != 0)
   {
      apx_fileManagerWorker_setTransmitHandler(&self->worker, handler);
   }
}

void apx_fileManager2_copyTransmitHandler(apx_fileManager2_t *self, apx_transmitHandler_t *handler)
{
   if (self != 0)
   {
      apx_fileManagerWorker_copyTransmitHandler(&self->worker, handler);
   }
}

/**
 * Server Mode
 */
void apx_fileManager2_headerReceived(apx_fileManager2_t *self)
{

}

/**
 * Client Mode
 */
void apx_fileManager2_headerAccepted(apx_fileManager2_t *self)
{
   adt_ary_t *localFiles = apx_fileManagerShared2_getLocalFileList(&self->shared);
   if (localFiles != 0)
   {
      int32_t len = adt_ary_length(localFiles);
      int32_t i;
      for (i=0; i < len; i++)
      {
         apx_fileInfo_t *fileInfo = (apx_fileInfo_t*) adt_ary_value(localFiles, i);
         apx_fileManagerWorker_sendFileInfoMsg(&self->worker, fileInfo);
      }
      adt_ary_destructor_enable(localFiles, false);
      adt_ary_delete(localFiles);
   }
}

apx_error_t apx_fileManager2_requestOpenFile(apx_fileManager2_t *self, uint32_t address)
{
   if (self != 0 && address != RMF_INVALID_ADDRESS)
   {
      apx_file2_t *file = apx_fileManagerShared2_findFileByAddress(&self->shared, address);
      if (file != 0)
      {
         if (apx_file2_isRemoteFile(file) )
         {
            apx_fileManagerWorker_sendFileOpenMsg(&self->worker, address);
         }
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_fileManager2_setConnectionId(apx_fileManager2_t *self, uint32_t connectionId)
{
   if (self != 0)
   {
      apx_fileManagerShared2_setConnectionId(&self->shared, connectionId);
   }
}

int32_t apx_fileManager2_getNumLocalFiles(apx_fileManager2_t *self)
{
   if (self != 0)
   {
      return apx_fileManagerShared2_getNumLocalFiles(&self->shared);
   }
   return -1;
}

int32_t apx_fileManager2_getNumRemoteFiles(apx_fileManager2_t *self)
{
   if (self != 0)
   {
      return apx_fileManagerShared2_getNumRemoteFiles(&self->shared);
   }
   return -1;
}

apx_file2_t *apx_fileManager2_createLocalFile(apx_fileManager2_t *self, const apx_fileInfo_t *fileInfo)
{
   if (self != 0)
   {
      apx_file2_t *file = apx_fileManagerShared2_createLocalFile(&self->shared, fileInfo);
      if (file != 0)
      {
         apx_file2_setFileManager(file, self);
      }
      return file;
   }
   return (apx_file2_t*) 0;
}

apx_file2_t *apx_fileManager2_createRemoteFile(apx_fileManager2_t *self, const apx_fileInfo_t *fileInfo)
{
   if (self != 0)
   {
      apx_file2_t *file = apx_fileManagerShared2_createRemoteFile(&self->shared, fileInfo);
      if (file != 0)
      {
         apx_file2_setFileManager(file, self);
      }
      return file;
   }
   return (apx_file2_t*) 0;
}

apx_error_t apx_fileManager2_onFileOpenNotify(apx_fileManager2_t *self, uint32_t address)
{
   if (self != 0)
   {
      apx_file2_t *file = apx_fileManager2_findFileByAddress(self, address & RMF_ADDRESS_MASK_INTERNAL);
      if (file != 0)
      {
         apx_file2_open(file);
         return apx_file2_fileOpenNotify(file);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_fileManager2_writeConstData(apx_fileManager2_t *self, uint32_t address, uint32_t len, apx_file_read_const_data_func *readFunc, void *arg)
{
   if (self != 0)
   {
      return apx_fileManagerWorker_sendConstData(&self->worker, address, len, readFunc, arg);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

#ifdef UNIT_TEST
bool apx_fileManager2_run(apx_fileManager2_t *self)
{
   if (self != 0)
   {
      return apx_fileManagerWorker_run(&self->worker);
   }
   return false;
}

int32_t apx_fileManager2_numPendingMessages(apx_fileManager2_t *self)
{
   if (self != 0)
   {
      return apx_fileManagerWorker_numPendingMessages(&self->worker);
   }
   return -1;
}
#endif //UNIT_TEST
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


