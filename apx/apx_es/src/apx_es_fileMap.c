/**
 * embedded version of apx_fileMap.c. This version uses fixed-size linear list instead of a dynamic linked list (no malloc required)
 *
 */
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "apx_es_fileMap.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define PORT_DATA_START      0x0u
#define PORT_DATA_BOUNDARY   0x400u //1KB, this must be a power of 2
#define DEFINITION_START     0x4000000 //64MB, this must be a pointer of 2
#define DEFINITION_BOUNDARY  0x100000u //1MB, this must be a power of 2
#define USER_DATA_START      0x20000000 //512MB, this must be a power of 2
#define USER_DATA_END        0x3FFFFC00 //Start of remote file cmd message area
#define USER_DATA_BOUNDARY   0x100000u //1MB, this must be a power of 2


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static int8_t apx_es_fileMap_autoInsertInternal(apx_es_fileMap_t *self, apx_file_t *pFile, uint32_t start_address, uint32_t end_address, uint32_t address_boundary);
static int8_t apx_es_fileMap_insertAt(apx_es_fileMap_t *self, apx_file_t *pFile, int32_t index);
static int8_t apx_es_fileMap_removeAt(apx_es_fileMap_t *self, int32_t index);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_es_fileMap_create(apx_es_fileMap_t *self)
{
   if (self != 0)
   {
      memset(self->fileList,0,sizeof(self->fileList));
      self->curLen=0;
      self->lastIndex=-1;
   }
}

int8_t apx_es_fileMap_autoInsert(apx_es_fileMap_t *self, apx_file_t *pFile)
{
   if ( (self != 0) && (pFile != 0) )
   {
      switch(pFile->fileType)
      {
         case APX_OUTDATA_FILE: //fall-through
         case APX_INDATA_FILE:
            return apx_es_fileMap_autoInsertInternal(self, pFile, PORT_DATA_START, DEFINITION_START, PORT_DATA_BOUNDARY);
         case APX_DEFINITION_FILE:
            return apx_es_fileMap_autoInsertInternal(self, pFile, DEFINITION_START, USER_DATA_START, DEFINITION_BOUNDARY);
         case APX_USER_DATA_FILE:
            return apx_es_fileMap_autoInsertInternal(self, pFile, USER_DATA_START, USER_DATA_END, USER_DATA_BOUNDARY);
         default:
            return -1;
      }
      return 0;
   }
   return -1;
}

int8_t apx_es_fileMap_insert(apx_es_fileMap_t *self, apx_file_t *pFile)
{
   if ( (self != 0) && (pFile != 0) )
   {
      int32_t i;
      int32_t placementIndex=-1;
      if (self->curLen == 0)
      {
         placementIndex=0;
      }
      else
      {
         apx_file_t *pCurrent=0;
         placementIndex=self->curLen;
         for(i=0;i<self->curLen;i++)
         {
            pCurrent = self->fileList[i];
            if (pCurrent->fileInfo.address > pFile->fileInfo.address )
            {
               placementIndex = i;
               break;
            }
         }
      }
      return apx_es_fileMap_insertAt(self, pFile, placementIndex);
   }
   return -1;
}

int8_t apx_es_fileMap_remove(apx_es_fileMap_t *self, apx_file_t *pFile)
{
   if ( (self != 0) && (pFile != 0) )
   {
      int32_t i;
      apx_file_t *pCurrent=0;
      for(i=0;i<self->curLen;i++)
      {
         pCurrent = self->fileList[i];
         if (pCurrent == pFile)
         {
            return apx_es_fileMap_removeAt(self, i);
         }
      }
   }
   return -1;
}

void apx_es_fileMap_clear(apx_es_fileMap_t *self)
{
   if (self != 0)
   {
      apx_es_fileMap_create(self);
   }
}

apx_file_t *apx_es_fileMap_findByAddress(apx_es_fileMap_t *self, uint32_t address)
{
   if (self != 0)
   {
      uint32_t startAddress;
      uint32_t endAddress;
      int32_t i;
      if ( (self->lastIndex>=0) && (self->lastIndex<self->curLen) )
      {
         apx_file_t *file = self->fileList[self->lastIndex];
         startAddress = file->fileInfo.address;
         endAddress = startAddress+file->fileInfo.length;
         if ( (address >= startAddress) && (address < endAddress) )
         {
            return self->fileList[self->lastIndex];
         }
      }
      for (i=0; i<self->curLen; i++)
      {
         apx_file_t *file = self->fileList[i];
         startAddress = file->fileInfo.address;
         endAddress = startAddress+file->fileInfo.length;
         if ( (address >= startAddress) && (address < endAddress) )
         {
            self->lastIndex = i;
            return file;
         }
      }
   }
   return (apx_file_t*) 0;
}
apx_file_t *apx_es_fileMap_findByName(apx_es_fileMap_t *self, const char *name);

int32_t apx_es_fileMap_length(apx_es_fileMap_t *self)
{
   if (self != 0)
   {
      return (self->curLen);
   }
   return -1;
}

apx_file_t *apx_es_fileMap_get(apx_es_fileMap_t *self, int32_t index)
{
   if ( (self != 0) && (index>=0) && (index<self->curLen) )
   {
      return self->fileList[index];
   }
   return (apx_file_t*) 0;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static int8_t apx_es_fileMap_autoInsertInternal(apx_es_fileMap_t *self, apx_file_t *pFile, uint32_t start_address, uint32_t end_address, uint32_t address_boundary)
{
   if ( (self != 0) && (pFile != 0) )
   {
      int32_t i;
      int32_t found=-1;
      int32_t placementIndex=-1;
      if (self->curLen == 0)
      {
         pFile->fileInfo.address = start_address;
         placementIndex=0;
      }
      else
      {
         apx_file_t *pCurrent=0;
         uint32_t placement_address = start_address;
         placementIndex = 0;
         for(i=0;i<self->curLen;i++)
         {
            pCurrent = self->fileList[i];
            if (pCurrent != 0)
            {
               if( pCurrent->fileInfo.address >= end_address)
               {
                  break; //we have passed into the next area, break and use latest seen value of found
               }
               else if(pCurrent->fileInfo.address >= start_address)
               {
                  found = i; //update pFound
               }
               else
               {
                  //pCurrent doesn't match range and is ignored
               }
            }
         }
         if (found >= 0)
         {
           uint32_t other_end_address;
           uint32_t other_start_address;
           apx_file_t *pOther = (apx_file_t*) self->fileList[found];
           assert(pOther != 0);
           other_start_address=pOther->fileInfo.address;
           other_end_address=other_start_address + pOther->fileInfo.length;
           placement_address  = (other_end_address + (address_boundary-1)) & (~(address_boundary-1)); //note that address_boundary must be a power of 2 for this code to work
           if (placement_address >= end_address)
           {
              //memory map full, cannot fit any more files into this region
              errno = ENOMEM;
              return -1;
           }
           placementIndex = found+1;
         }
         pFile->fileInfo.address = placement_address;
      }
      return apx_es_fileMap_insertAt(self, pFile, placementIndex);
   }
   return -1;
}

/**
 * inserts pFile at index and moves everything at index (and forward) ahead one slot
 */
static int8_t apx_es_fileMap_insertAt(apx_es_fileMap_t *self, apx_file_t *pFile, int32_t index)
{
   if ( (self != 0) && (pFile != 0) && (index >= 0) )
   {
      if (self->curLen >= APX_ES_FILEMAP_MAX_NUM_FILES)
      {
         errno = ENOMEM;
         return -1; //no more items can be inserted in the map
      }
      if (self->curLen==index)
      {
         //insert at end of list
         self->fileList[index]=pFile;
         self->curLen++;
      }
      else if (self->curLen>index)
      {
         //move all items forward one step to make room at the index slot
         int32_t i;
         for(i=self->curLen;i>index;i--)
         {
            self->fileList[i]=self->fileList[i-1];
         }
         self->curLen++;
         self->fileList[index]=pFile;
      }
      else
      {
         assert(0); //this would create a hole in the data structure
      }
      return 0;
   }
   return -1;
}

static int8_t apx_es_fileMap_removeAt(apx_es_fileMap_t *self, int32_t index)
{
   if ( (self != 0) && (self->curLen>0) && (index >= 0) && (index < self->curLen) )
   {
      int32_t i;
      for (i=index;i<self->curLen-1;i++)
      {
         self->fileList[i]=self->fileList[i+1];
      }
      --self->curLen;
      return 0;
   }
   return -1;
}
