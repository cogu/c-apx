//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_file.h"
#include "apx_logging.h"
#include <errno.h>
#ifndef APX_EMBEDDED
#include <malloc.h>
#include "bstr.h"
#endif
#include <string.h>
#include <assert.h>
#include <stdio.h>
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static uint8_t apx_file_deriveFileType(apx_file_t *self);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int8_t apx_file_createLocalFile(apx_file_t *self, uint8_t fileType, apx_nodeData_t *nodeData)
{
   if ( (self != 0) && (nodeData != 0) )
   {
      size_t len;
      char name[RMF_MAX_FILE_NAME+1];
      self->fileType=fileType;
      self->nodeData=nodeData;
      self->isRemoteFile = false;
      self->isOpen = false;
      len = strlen(self->nodeData->name);

      if (len+APX_MAX_FILE_EXT_LEN <= RMF_MAX_FILE_NAME)
      {
         const char *ext=0;
         uint32_t filelen = 0;

         memcpy(name, self->nodeData->name, len);
         switch(fileType)
         {
         case APX_OUTDATA_FILE:
            ext = APX_OUTDATA_FILE_EXT;
            filelen = nodeData->outPortDataLen;
            break;
         case APX_INDATA_FILE:
            ext = APX_INDATA_FILE_EXT;
            filelen = nodeData->inPortDataLen;
            break;
         case APX_DEFINITION_FILE:
            ext = APX_DEFINITION_FILE_EXT;
            filelen = nodeData->definitionDataLen;
            break;
         default:
            errno = EINVAL;
            return -1;
         }
         strcpy(name+len, ext);
         rmf_fileInfo_create(&self->fileInfo, name, RMF_INVALID_ADDRESS, filelen, RMF_FILE_TYPE_FIXED);
         return 0;
      }
   }
   errno = EINVAL;
   return -1;
}

int8_t apx_file_createRemoteFile(apx_file_t *self, const rmf_fileInfo_t *cmdFileInfo)
{
   if ( (self != 0) && (cmdFileInfo != 0))
   {
      int8_t result;
      self->isRemoteFile = true;
      self->nodeData = 0;
      self->isOpen = false;

      result = rmf_fileInfo_create(&self->fileInfo, cmdFileInfo->name, cmdFileInfo->address, cmdFileInfo->length, cmdFileInfo->fileType);
      if (result == 0)
      {
         rmf_fileInfo_setDigestData(&self->fileInfo, cmdFileInfo->digestType, cmdFileInfo->digestData, 0);
         self->fileType = apx_file_deriveFileType(self);
      }
      return result;
   }
   errno = EINVAL;
   return -1;
}

void apx_file_destroy(apx_file_t *self)
{
   if (self != 0)
   {
      rmf_fileInfo_destroy(&self->fileInfo);
   }
}

#ifndef APX_EMBEDDED
apx_file_t *apx_file_newLocalFile(uint8_t fileType, apx_nodeData_t *nodeData)
{
   apx_file_t *self = (apx_file_t*) malloc(sizeof(apx_file_t));
   if(self != 0)
   {
      int8_t result = apx_file_createLocalFile(self, fileType, nodeData);
      if (result<0)
      {
         free(self);
         self=0;
      }
   }
   else
   {
      errno = ENOMEM;
   }
   return self;
}

apx_file_t *apx_file_newLocalDefinitionFile(apx_nodeData_t *nodeData)
{
   return apx_file_newLocalFile(APX_DEFINITION_FILE, nodeData);
}

apx_file_t *apx_file_newLocalOutPortDataFile(apx_nodeData_t *nodeData)
{
   return apx_file_newLocalFile(APX_OUTDATA_FILE, nodeData);
}

apx_file_t *apx_file_newLocalInPortDataFile(apx_nodeData_t *nodeData)
{
   return apx_file_newLocalFile(APX_INDATA_FILE, nodeData);
}

apx_file_t *apx_file_newRemoteFile(const rmf_fileInfo_t *fileInfo)
{
   apx_file_t *self = (apx_file_t*) malloc(sizeof(apx_file_t));
   if(self != 0)
   {
      int8_t result = apx_file_createRemoteFile(self, fileInfo);
      if (result<0)
      {
         free(self);
         self=0;
      }
   }
   else
   {
      errno = ENOMEM;
   }
   return self;
}

void apx_file_delete(apx_file_t *self)
{
   if (self != 0)
   {
      apx_file_destroy(self);
      free(self);
   }
}

void apx_file_vdelete(void *arg)
{
   apx_file_delete((apx_file_t*) arg);
}
/**
 * creates a new string containing the file name with the file extension removed.
 * the returned object (char*) needs to be freed by the user once returned
 */
char *apx_file_basename(const apx_file_t *self)
{
   if (self != 0)
   {
      const char *pBegin;
      const char *pEnd;
      size_t len = strlen(self->fileInfo.name);
      pBegin = self->fileInfo.name;
      pEnd = self->fileInfo.name+len;
      if (len >= APX_MAX_FILE_EXT_LEN ) //there is room for file extension of at least 3 characters (plus the '.')
      {
         const char *p = pEnd-1;
         //search in string backwards to find a '.' character
         while(p>=pBegin)
         {
            if (*p == '.')
            {
               return (char*) bstr_make((const uint8_t*) pBegin, (const uint8_t*) p);
            }
            --p;
         }
         //no '.' character was found, continue out of the if-statement to return entire string
      }
      //return entire string
      return (char*) bstr_make((const uint8_t*) pBegin, (const uint8_t*) pEnd);
   }
   errno = EINVAL;
   return 0;
}
#endif

void apx_file_open(apx_file_t *self)
{
   if (self != 0)
   {
      self->isOpen = true;
   }
}

void apx_file_close(apx_file_t *self)
{
   if (self != 0)
   {
      self->isOpen = false;
   }
}


int8_t apx_file_read(apx_file_t *self, uint8_t *pDest, uint32_t offset, uint32_t length)
{
   if ( (self != 0) && (pDest != 0) && (length > 0) )
   {
      int8_t result;
      switch(self->fileType)
      {
      case APX_UNKNOWN_FILE:
         result = 0;
         break;
      case APX_OUTDATA_FILE:
         result = apx_nodeData_readOutPortData(self->nodeData, pDest, offset, length);
         if (result != 0)
         {
            APX_LOG_ERROR("[APX_FILE] apx_nodeData_readOutPortData failed");
         }
         break;
      case APX_INDATA_FILE:
         result = apx_nodeData_readInPortData(self->nodeData, pDest, offset, length);
         if (result != 0)
         {
            APX_LOG_ERROR("[APX_FILE] apx_nodeData_writeInData failed");
         }
         break;
      case APX_DEFINITION_FILE:
         result = apx_nodeData_readDefinitionData(self->nodeData, pDest, offset, length);
         if (result != 0)
         {
            APX_LOG_ERROR("[APX_FILE] apx_nodeData_readDefinitionData failed");
         }
         break;
      default:
         result=-1;
         break;
      }
      return result;
   }
   return -1;
}

int8_t apx_file_write(apx_file_t *self, const uint8_t *pSrc, uint32_t offset, uint32_t length)
{

   if ( (self != 0) && (pSrc != 0) && (self->nodeData != 0) )
   {
      int8_t result;
      switch(self->fileType)
      {
      case APX_DEFINITION_FILE:
         result = apx_nodeData_writeDefinitionData(self->nodeData, pSrc, offset, length);
         if (result != 0)
         {
            APX_LOG_ERROR("[APX_FILE] apx_nodeData_writeDefinitionData failed with %d", result);
         }
         break;
      case APX_INDATA_FILE:
         result = apx_nodeData_writeInPortData(self->nodeData, pSrc, offset, length);
         if (result != 0)
         {
            APX_LOG_ERROR("[APX_FILE] apx_nodeData_writeInPortData failed with %d", result);            
         }
         else
         {
            apx_nodeData_triggerInPortDataWritten(self->nodeData, offset, length);
         }
         break;
      case APX_OUTDATA_FILE:
         result = apx_nodeData_writeOutPortData(self->nodeData, pSrc, offset, length);
         if (result != 0)
         {
            APX_LOG_ERROR("[APX_FILE] apx_nodeData_writeOutPortData failed with %d", result);            
         }
         break;
      default:
         result=-1;
         break;
      }
      return result;
   }
   return -1;
}




//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
/**
 * gets file extension from file name and uses that to set fileType property
 */
static uint8_t apx_file_deriveFileType(apx_file_t *self)
{
   if (self != 0)
   {
      size_t len = strlen(self->fileInfo.name);
      if (len > 0 ) //4 characters means that there is room for file extension
      {
         char *pBegin = self->fileInfo.name;
         char *p = self->fileInfo.name+len-1;
         //search in string backwards to find a '.' character
         while(p>=pBegin)
         {
            if (*p == '.')
            {
               if ( (strcmp(p, APX_DEFINITION_FILE_EXT)==0) )
               {
                  return APX_DEFINITION_FILE;
               }
               else if ( (strcmp(p, APX_INDATA_FILE_EXT)==0) )
               {
                  return APX_INDATA_FILE;
               }
               else if ( (strcmp(p, APX_OUTDATA_FILE_EXT)==0) )
               {
                  return APX_OUTDATA_FILE;
               }
               else
               {
                  return APX_USER_DATA_FILE;
               }
            }
            --p;
         }
         return APX_USER_DATA_FILE;
      }
   }
   return APX_UNKNOWN_FILE;
}


