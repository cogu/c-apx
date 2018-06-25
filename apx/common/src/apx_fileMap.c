//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "apx_fileMap.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define PORT_DATA_START      0x0u
#define PORT_DATA_BOUNDARY   0x400u //1KB, this must be a power of 2
#define DEFINITION_START     0x4000000 //64MB, this must be a power of 2
#define DEFINITION_BOUNDARY  0x100000u //1MB, this must be a power of 2
#define USER_DATA_START      0x20000000 //512MB, this must be a power of 2
#define USER_DATA_END        0x3FFFFC00 //Start of remote file cmd message area
#define USER_DATA_BOUNDARY   0x100000u //1MB, this must be a power of 2

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static int8_t apx_fileMap_autoInsertFile(apx_fileMap_t *self, apx_file_t *pFile, uint32_t start_address, uint32_t end_address, uint32_t address_boundary);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_fileMap_create(apx_fileMap_t *self)
{
   if (self != 0)
   {
      adt_list_create(&self->fileList, apx_file_vdelete);
   }
}
void apx_fileMap_destroy(apx_fileMap_t *self)
{
   if (self !=0)
   {
      adt_list_destroy(&self->fileList);
   }
}


int8_t apx_fileMap_autoInsertDataFile(apx_fileMap_t *self, apx_file_t *pFile)
{
   return apx_fileMap_autoInsertFile(self, pFile, USER_DATA_START, USER_DATA_END, USER_DATA_BOUNDARY);
}

/**
 * inserts file into the address range 0..64MB by automatically allocating a free address
 */
int8_t apx_fileMap_autoInsertPortDataFile(apx_fileMap_t *self, apx_file_t *pFile)
{
   return apx_fileMap_autoInsertFile(self, pFile, PORT_DATA_START, DEFINITION_START, PORT_DATA_BOUNDARY);
}


/**
 * inserts file into the address range DEFINITION_START..USER_DATA_START by automatically allocating a free address
 */
int8_t apx_fileMap_autoInsertDefinitionFile(apx_fileMap_t *self, apx_file_t *pFile)
{
   return apx_fileMap_autoInsertFile(self, pFile, DEFINITION_START, USER_DATA_START, DEFINITION_BOUNDARY);
}

/**
 * inserts pFile into self->fileList sorted by address
 * returns 0 success, -1 on error.
 */
int8_t apx_fileMap_insertFile(apx_fileMap_t *self, apx_file_t *pFile)
{
   if ( (self != 0) && (pFile != 0) )
   {
      adt_list_elem_t *pIter = adt_list_first(&self->fileList);
      if (pIter == 0)
      {
         adt_list_insert(&self->fileList, pFile);
      }
      else
      {
         apx_file_t *pLast=0;
         apx_file_t *pCurrent=0;
         adt_list_iter_init(&self->fileList);
         do
         {
            pIter = adt_list_iter_next(&self->fileList);
            if (pIter != 0)
            {
               pCurrent = (apx_file_t*) pIter->pItem;
               assert(pCurrent != 0);
               if( pCurrent->fileInfo.address > pFile->fileInfo.address)
               {
                  //try to fit pFile in before pCurrent;
                  break;
               }
               pLast = pCurrent;
            }
         } while(pIter != 0);
         if (pIter != 0)
         {
            uint32_t start_address;
            uint32_t end_address;
            assert(pCurrent != 0);
            assert( ((apx_file_t*) pIter->pItem) == pCurrent);
            if (pLast != 0)
            {
               //check if there is room to fit this file between pLast and pFile
               uint32_t start_address;
               uint32_t end_address;
               start_address = pLast->fileInfo.address;
               end_address = start_address+pLast->fileInfo.length;
               if (end_address > pFile->fileInfo.address)
               {
                  //address collision between pLast and pFile, reject insertion of pFile
                  errno = EADDRINUSE; /* Address already in use */
                  return -1;
               }
            }
            start_address = pFile->fileInfo.address;
            end_address = start_address+pFile->fileInfo.length;
            if (end_address > pCurrent->fileInfo.address)
            {
               //address collision between pCurrent and pFile, reject insertion of pFile
               errno = EFBIG; /* File too large */
               return -1;
            }
            //it should now be safe to insert the file before pIter
            adt_list_insertBefore(&self->fileList, pIter, pFile);
         }
         else
         {
            //insert pFile at end of list
            adt_list_insert(&self->fileList, pFile);
         }
         return 0;
      }
   }
   errno = EINVAL;
   return -1;
}

/**
 * removes pFile from self->fileList
 */
int8_t apx_fileMap_removeFile(apx_fileMap_t *self, apx_file_t *pFile)
{
   if ( (self != 0) && (pFile != 0) )
   {
      //TODO: fix adt_list_remove so that it returns success/failure
      adt_list_remove(&self->fileList, pFile);
      return 0;
   }
   return -1;
}

apx_file_t *apx_fileMap_findByAddress(apx_fileMap_t *self, uint32_t address)
{
   apx_file_t *retval=0;
   if (self != 0)
   {
      adt_list_elem_t *pIter;
      adt_list_iter_init(&self->fileList);
      do
      {
         pIter = adt_list_iter_next(&self->fileList);
         if (pIter != 0)
         {
            uint32_t startAddress;
            uint32_t endAddress;
            apx_file_t *pFile = (apx_file_t*) pIter->pItem;
            assert(pFile != 0);
            startAddress = pFile->fileInfo.address;
            endAddress = startAddress + pFile->fileInfo.length;
            if ( (address>=startAddress) && (address<endAddress) )
            {
               retval = pFile;
               break;
            }
         }
      } while(pIter != 0);
   }
   return retval;
}

apx_file_t *apx_fileMap_findByName(apx_fileMap_t *self, const char *name)
{
   apx_file_t *retval=0;
   if (self != 0)
   {
      adt_list_elem_t *pIter;
      adt_list_iter_init(&self->fileList);
      do
      {
         pIter = adt_list_iter_next(&self->fileList);
         if (pIter != 0)
         {
            apx_file_t *pFile = (apx_file_t*) pIter->pItem;
            assert(pFile != 0);
            if ( (strcmp(pFile->fileInfo.name, name)==0) )
            {
               retval = pFile;
               break;
            }
         }
      } while(pIter != 0);
   }
   return retval;
}

int32_t apx_fileMap_length(const apx_fileMap_t *self)
{
   if (self != 0)
   {
      return adt_list_length((adt_list_t*) &self->fileList);
   }
   return -1;
}

/**
 * clears internal list of files without deleting them.
 * The caller of this function must have taken ownership of internal file objects before calling this function.
 */
void  apx_fileMap_clear_weak(apx_fileMap_t *self)
{
   if (self != 0)
   {
      adt_list_destructorEnable(&self->fileList, false);
      adt_list_clear(&self->fileList);
      adt_list_destructorEnable(&self->fileList, true);
   }
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

/**
 * attempts to find the the last file in the range start_address..end_address and then inserts pFile after it
 */
static int8_t apx_fileMap_autoInsertFile(apx_fileMap_t *self, apx_file_t *pFile, uint32_t start_address, uint32_t end_address, uint32_t address_boundary)
{
   if ( (self != 0) && (pFile != 0) )
   {
      adt_list_elem_t *pIter = adt_list_first(&self->fileList);
      if (pIter == 0)
      {
         pFile->fileInfo.address = start_address;
      }
      else
      {
         adt_list_elem_t *pIter=0;
         adt_list_elem_t *pFound=0;
         uint32_t placement_address = start_address;
         adt_list_iter_init(&self->fileList);
         do
         {
            pIter = adt_list_iter_next(&self->fileList);
            if (pIter != 0)
            {
               apx_file_t *pOther;
               pOther = (apx_file_t*) pIter->pItem;
               assert(pOther != 0);

               if( pOther->fileInfo.address >= end_address)
               {
                  break; //we have passed into the next area, break and use latest seen value of pFound
               }
               else if(pOther->fileInfo.address >= start_address)
               {
                  pFound = pIter; //update pFound
               }
               else
               {
                  //MISRA
               }
            }
         } while(pIter != 0);
         if (pFound != 0)
         {
           uint32_t other_end_address;
           uint32_t other_start_address;
           apx_file_t *pOther = (apx_file_t*) pFound->pItem;
           assert(pOther != 0);
           other_start_address=pOther->fileInfo.address;
           other_end_address=other_start_address + pOther->fileInfo.length;
           //check if address_boundary is a power of two. If not, we need to use another slower method to calculate new placement_address
           assert(address_boundary != 0);
           if ((address_boundary & (address_boundary-1)) == 0)
           {
              //address_boundary is a power of 2, use faster method
              placement_address  = (other_end_address + (address_boundary-1)) & (~(address_boundary-1)); //note that address_boundary must be a power of 2 for this code to work
           }
           else
           {
              //use slower method
              assert(0); ///TODO: implement this
           }

           if (placement_address >= end_address)
           {
              //memory map full, cannot fit any more files into this region
              errno = ENOMEM;
              return -1;
           }
         }
         pFile->fileInfo.address = placement_address;
      }
      return apx_fileMap_insertFile(self, pFile);
   }
   return -1;
}
