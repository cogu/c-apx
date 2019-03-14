//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "apx_nodeData.h"
#include "apx_file.h"
#include "rmf.h"
#ifdef APX_EMBEDDED
#include "apx_es_fileManager.h"
#else
#include <malloc.h>
#include <assert.h>
#include "apx_fileManager.h"
#include "apx_nodeInfo.h"
#endif
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif




//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_nodeData_create(apx_nodeData_t *self, const char *name, uint8_t *definitionBuf, uint32_t definitionDataLen, uint8_t *inPortDataBuf, uint8_t *inPortDirtyFlags, uint32_t inPortDataLen, uint8_t *outPortDataBuf, uint8_t *outPortDirtyFlags, uint32_t outPortDataLen)
{
   if (self != 0)
   {
      self->isRemote = false; //default false, used by nodeManager to determine whether this belongs to a remote or a local node
      self->isWeakref = true; //default true, all pointers in this object are weak referencens (will not be automatically deleted when this object is destroyed)
      self->name=name;
      self->definitionDataBuf = definitionBuf;
      self->definitionDataLen = definitionDataLen;
      self->inPortDataBuf = inPortDataBuf;
      self->inPortDataLen = inPortDataLen;
      self->inPortDirtyFlags = inPortDirtyFlags;
      self->outPortDataBuf = outPortDataBuf;
      self->outPortDataLen = outPortDataLen;
      self->outPortDirtyFlags = outPortDirtyFlags;
      apx_nodeData_setHandlerTable(self, NULL);
      self->outPortDataFile = (apx_file_t*) 0;
      self->inPortDataFile = (apx_file_t*) 0;
#ifdef APX_EMBEDDED
      self->fileManager = (apx_es_fileManager_t*) 0;
#else
      SPINLOCK_INIT(self->inPortDataLock);
      SPINLOCK_INIT(self->outPortDataLock);
      SPINLOCK_INIT(self->definitionDataLock);
      SPINLOCK_INIT(self->internalLock);
      self->fileManager = (apx_fileManager_t*) 0;
      self->nodeInfo = (apx_nodeInfo_t*) 0;
#endif
   }
}

void apx_nodeData_destroy(apx_nodeData_t *self)
{
   if (self != 0)
   {
#ifndef APX_EMBEDDED
      SPINLOCK_DESTROY(self->inPortDataLock);
      SPINLOCK_DESTROY(self->outPortDataLock);
      SPINLOCK_DESTROY(self->definitionDataLock);
      SPINLOCK_DESTROY(self->internalLock);

      if (self->isWeakref == false)
      {
         //pointers are strongly referenced when isWeakRef is false. delete all pointers that are not NULL
         if ( self->name != 0)
         {
            free((char*)self->name);
         }
         if ( self->inPortDataBuf != 0)
         {
            free(self->inPortDataBuf);
         }
         if ( self->outPortDataBuf != 0)
         {
            free(self->outPortDataBuf);
         }
         if ( self->definitionDataBuf != 0)
         {
            free(self->definitionDataBuf);
         }
         if ( self->inPortDirtyFlags != 0)
         {
            free(self->inPortDirtyFlags);
         }
         if ( self->outPortDirtyFlags != 0)
         {
            free(self->outPortDirtyFlags);
         }
      }
#endif
   }
}
#ifndef APX_EMBEDDED
/**
 * creates a new apx_nodeData_t with all pointers (except name) set to NULL
 */
apx_nodeData_t *apx_nodeData_newRemote(const char *name, bool isWeakRef)
{
   apx_nodeData_t *self = 0;
   if ( (isWeakRef == false) && (name != 0) )
   {
      char *nameCopy = STRDUP(name);
      if (nameCopy == 0)
      {
         return self;
      }
      self = (apx_nodeData_t*) malloc(sizeof(apx_nodeData_t));
      if (self != 0)
      {
         apx_nodeData_create(self, nameCopy, 0, 0, 0, 0, 0, 0, 0, 0);
         self->isWeakref=false;
         self->isRemote = true;
      }
      else
      {
         free(nameCopy);
         errno = ENOMEM;
      }
   }
   else
   {
      self = (apx_nodeData_t*) malloc(sizeof(apx_nodeData_t));
      if (self != 0)
      {
         apx_nodeData_create(self, name, 0, 0, 0, 0, 0, 0, 0, 0);
         self->isWeakref=isWeakRef;
      }
      else
      {
         errno = ENOMEM;
      }
   }
   return self;
}
#endif


bool apx_nodeData_isOutPortDataOpen(apx_nodeData_t *self)
{
   bool retval = false;
   if (self != 0)
   {
#ifndef APX_EMBEDDED
      SPINLOCK_ENTER(self->internalLock);
#endif

      if ( (self->fileManager != 0) && (self->outPortDataFile != 0) && (self->outPortDataFile->isOpen == true) )
      {
         retval = true;
      }

#ifndef APX_EMBEDDED
      SPINLOCK_LEAVE(self->internalLock);
#endif
   }
   return retval;
}

void apx_nodeData_setHandlerTable(apx_nodeData_t *self, apx_nodeDataHandlerTable_t *handlerTable)
{
   if (self != 0)
   {
      if (handlerTable == (apx_nodeDataHandlerTable_t*) 0)
      {
         memset(&self->handlerTable, 0, sizeof(self->handlerTable));
      }
      else
      {
         memcpy(&self->handlerTable, handlerTable, sizeof(self->handlerTable));
      }
   }
}

#ifndef APX_EMBEDDED
void apx_nodeData_delete(apx_nodeData_t *self)
{
   if (self != 0)
   {
      apx_nodeData_destroy(self);
      free(self);
   }
}

void apx_nodeData_vdelete(void *arg)
{
   apx_nodeData_delete((apx_nodeData_t*) arg);
}
#endif

int8_t apx_nodeData_readDefinitionData(apx_nodeData_t *self, uint8_t *dest, uint32_t offset, uint32_t len)
{
   int8_t retval = 0;
#ifndef APX_EMBEDDED
   SPINLOCK_ENTER(self->definitionDataLock);
#endif
   if ( (offset+len) > self->definitionDataLen) //attempted read outside bounds
   {
      retval = -1;
   }
   else
   {
      memcpy(dest, &self->definitionDataBuf[offset], len);
   }
#ifndef APX_EMBEDDED
   SPINLOCK_LEAVE(self->definitionDataLock);
#endif
   return retval;
}

int8_t apx_nodeData_readOutPortData(apx_nodeData_t *self, uint8_t *dest, uint32_t offset, uint32_t len)
{
   assert((offset+len) <= self->outPortDataLen);
#ifndef APX_EMBEDDED
   SPINLOCK_ENTER(self->outPortDataLock);
#endif
   memcpy(dest, &self->outPortDataBuf[offset], len);
   if (self->outPortDirtyFlags != 0)
   {
      memset(&self->outPortDirtyFlags[offset], 0, len);
   }
#ifndef APX_EMBEDDED
   SPINLOCK_LEAVE(self->outPortDataLock);
#endif
   return 0;
}

int8_t apx_nodeData_readInPortData(apx_nodeData_t *self, uint8_t *dest, uint32_t offset, uint32_t len)
{
   if( (self != 0) && (self->inPortDataBuf != 0) )
   {
      assert((offset+len) <= self->inPortDataLen);
#ifndef APX_EMBEDDED
      SPINLOCK_ENTER(self->inPortDataLock);
#endif
      memcpy(dest, &self->inPortDataBuf[offset], len);
      if (self->inPortDirtyFlags != 0)
      {
         memset(&self->inPortDirtyFlags[offset], 0, len);
      }
#ifndef APX_EMBEDDED
      SPINLOCK_LEAVE(self->inPortDataLock);
#endif
      return 0;
   }
   errno = EINVAL;
   return -1;
}





void apx_nodeData_lockOutPortData(apx_nodeData_t *self)
{
#ifndef APX_EMBEDDED
      SPINLOCK_ENTER(self->outPortDataLock);
#endif
}

void apx_nodeData_unlockOutPortData(apx_nodeData_t *self)
{
#ifndef APX_EMBEDDED
      SPINLOCK_LEAVE(self->outPortDataLock);
#endif
}

void apx_nodeData_lockInPortData(apx_nodeData_t *self)
{
#ifndef APX_EMBEDDED
      SPINLOCK_ENTER(self->inPortDataLock);
#endif
}

void apx_nodeData_unlockInPortData(apx_nodeData_t *self)
{
#ifndef APX_EMBEDDED
      SPINLOCK_LEAVE(self->inPortDataLock);
#endif
}

void apx_nodeData_outPortDataNotify(apx_nodeData_t *self, apx_offset_t offset, apx_size_t length)
{
   if (self != 0)
   {
      if ( (self->fileManager != 0) && (self->outPortDataFile != 0) && (self->outPortDataFile->isOpen == true) )
      {
#ifdef APX_EMBEDDED
         apx_es_fileManager_onFileUpdate(self->fileManager, self->outPortDataFile, offset, length);
#else
         apx_fileManager_triggerFileUpdatedEvent(self->fileManager, self->outPortDataFile, offset, length);
#endif
      }
   }
}

int8_t apx_nodeData_writeInPortData(apx_nodeData_t *self, const uint8_t *src, uint32_t offset, uint32_t len)
{
   int8_t retval = 0;
#ifndef APX_EMBEDDED
   SPINLOCK_ENTER(self->inPortDataLock);
#endif
   if ( (offset+len) > self->inPortDataLen) //attempted write outside bounds
   {
      retval = -1;
   }
   else
   {
      memcpy(&self->inPortDataBuf[offset], src, len);
   }
#ifndef APX_EMBEDDED
   SPINLOCK_LEAVE(self->inPortDataLock);
#endif
   return retval;

}

int8_t apx_nodeData_writeOutPortData(apx_nodeData_t *self, const uint8_t *src, uint32_t offset, uint32_t len)
{
   int8_t retval = 0;
#ifndef APX_EMBEDDED
   SPINLOCK_ENTER(self->outPortDataLock);
#endif
   if ( (offset+len) > self->outPortDataLen)
   {
      retval = -1;
   }
   else
   {
      memcpy(&self->outPortDataBuf[offset], src, len);
   }
#ifndef APX_EMBEDDED
   SPINLOCK_LEAVE(self->outPortDataLock);
#endif
   return retval;
}

int8_t apx_nodeData_writeDefinitionData(apx_nodeData_t *self, const uint8_t *src, uint32_t offset, uint32_t len)
{
   int8_t retval = 0;
#ifndef APX_EMBEDDED
   SPINLOCK_ENTER(self->definitionDataLock);
#endif
   if ( (offset+len) > self->definitionDataLen)
   {
      retval = -1;
   }
   else
   {
      memcpy(&self->definitionDataBuf[offset], src, len);
   }
#ifndef APX_EMBEDDED
   SPINLOCK_LEAVE(self->definitionDataLock);
#endif
   return retval;
}

void apx_nodeData_setInPortDataFile(apx_nodeData_t *self, struct apx_file_tag *file)
{
   if (self != 0)
   {
      self->inPortDataFile = file;
   }
}

void apx_nodeData_setOutPortDataFile(apx_nodeData_t *self, struct apx_file_tag *file)
{
   if (self != 0)
   {
      self->outPortDataFile = file;
   }
}


#ifndef APX_EMBEDDED
void apx_nodeData_setNodeInfo(apx_nodeData_t *self, struct apx_nodeInfo_tag *nodeInfo)
{
   if (self != 0)
   {
      self->nodeInfo = nodeInfo;
   }
}
#endif

#ifdef APX_EMBEDDED
void apx_nodeData_setFileManager(apx_nodeData_t *self, struct apx_es_fileManager_tag *fileManager)
#else
void apx_nodeData_setFileManager(apx_nodeData_t *self, struct apx_fileManager_tag *fileManager)
#endif
{
   if (self != 0)
   {
      self->fileManager = fileManager;
   }
}


void apx_nodeData_triggerInPortDataWritten(apx_nodeData_t *self, uint32_t offset, uint32_t len)
{   
   if ( (self != 0) && (self->handlerTable.inPortDataWritten != 0) )
   {
      self->handlerTable.inPortDataWritten(self->handlerTable.arg, self, offset, len);
   }
}
//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


