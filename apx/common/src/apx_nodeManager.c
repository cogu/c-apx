
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#if defined(_MSC_PLATFORM_TOOLSET) && (_MSC_PLATFORM_TOOLSET<=110)
#include "msc_bool.h"
#else
#include <stdbool.h>
#endif
#include "adt_ary.h"
#include "adt_hash.h"
#include "apx_nodeManager.h"
#include "apx_fileManager.h"
#include "apx_nodeData.h"
#include "apx_file.h"
#include "apx_nodeInfo.h"
#include "apx_router.h"
#include "apx_logging.h" //internal logging
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_nodeManager_createNode(apx_nodeManager_t *self, const uint8_t *definitionBuf, int32_t definitionLen, struct apx_fileManager_tag *fileManager);
static apx_nodeData_t *apx_nodeManager_getNodeData(const apx_nodeManager_t *self, const char *name);
static void apx_nodeManager_setLocalNodeData(apx_nodeManager_t *self, apx_nodeData_t *nodeData);
static void apx_nodeManager_executePortTriggerFunction(const apx_dataTriggerFunction_t *triggerFunction, const apx_file_t *file);
static void apx_nodeManager_attachLocalNodeToFileManager(apx_nodeData_t *nodeData, apx_fileManager_t *fileManager);
static void apx_nodeManager_removeRemoteNodeData(apx_nodeManager_t *self, apx_nodeData_t *nodeData);
static void apx_nodeManager_removeNodeInfo(apx_nodeManager_t *self, apx_nodeInfo_t *nodeInfo);
static bool apx_nodeManager_createInitData(apx_node_t *node, uint8_t *buf, int32_t bufLen);
//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_nodeManager_create(apx_nodeManager_t *self)
{
   if (self != 0)
   {
      apx_istream_handler_t apx_istream_handler;
      adt_hash_create(&self->nodeInfoMap, apx_nodeInfo_vdelete);
      apx_parser_create(&self->parser);
      memset(&apx_istream_handler,0,sizeof(apx_istream_handler));
      self->router = (apx_router_t*) 0;
      self->debugMode = APX_DEBUG_NONE;
      apx_istream_handler.arg = &self->parser;
      apx_istream_handler.open = apx_parser_vopen;
      apx_istream_handler.close = apx_parser_vclose;
      apx_istream_handler.node = apx_parser_vnode;
      apx_istream_handler.datatype = apx_parser_vdatatype;
      apx_istream_handler.provide = apx_parser_vprovide;
      apx_istream_handler.require = apx_parser_vrequire;
      apx_istream_handler.node_end = apx_parser_vnode_end;
      apx_istream_create(&self->apx_istream,&apx_istream_handler);
      adt_hash_create(&self->remoteNodeDataMap, apx_nodeData_vdelete);
      adt_hash_create(&self->localNodeDataMap, (void(*)(void*)) 0);
      adt_list_create(&self->fileManagerList, (void(*)(void*)) 0);
      MUTEX_INIT(self->lock);
   }
}

void apx_nodeManager_destroy(apx_nodeManager_t *self)
{
   if(self != 0)
   {
      apx_parser_destroy(&self->parser);
      adt_hash_destroy(&self->nodeInfoMap);
      apx_istream_destroy(&self->apx_istream);
      adt_hash_destroy(&self->remoteNodeDataMap);
      adt_hash_destroy(&self->localNodeDataMap);
      adt_list_destroy(&self->fileManagerList);
      MUTEX_DESTROY(self->lock);
   }
}

/**
 * this is called by fileManager when a new file is seen
 */
void apx_nodeManager_remoteFileAdded(apx_nodeManager_t *self, struct apx_fileManager_tag *fileManager, apx_file_t *remoteFile)
{
   if ( (self != 0) && (fileManager != 0) && (remoteFile != 0) )
   {
      if ( remoteFile->fileType == APX_DEFINITION_FILE )
      {
         char *basename = apx_file_basename(remoteFile);
         if (basename != 0)
         {
            apx_nodeData_t *nodeData;
            //this is potentially a new node, check if it exists already
            MUTEX_LOCK(self->lock);
            nodeData = apx_nodeManager_getNodeData(self, basename);
            MUTEX_UNLOCK(self->lock);
            if (nodeData == 0)
            {
               if (fileManager->mode == APX_FILEMANAGER_SERVER_MODE)
               {
                  //create new nodeData structure and initiate download of file
                  nodeData = apx_nodeData_newRemote(basename, false); //setting weakref to false will force apx_nodeData_delete to delete all buffers we created here
                  if (nodeData != 0)
                  {
                     nodeData->definitionDataBuf = (uint8_t*) malloc(remoteFile->fileInfo.length);
                     if (nodeData->definitionDataBuf==0)
                     {
                        APX_LOG_ERROR("[APX_NODE_MANAGER] out of memory when attempting to create definitionDataBuf of length %d for node %s", (int) remoteFile->fileInfo.length, basename);
                        free(basename);
                        apx_nodeData_delete(nodeData);
                        return;
                     }
                     else
                     {
                        nodeData->definitionDataLen = remoteFile->fileInfo.length;
                        MUTEX_LOCK(self->lock);
                        adt_hash_set(&self->remoteNodeDataMap, basename, 0, nodeData);
                        MUTEX_UNLOCK(self->lock);
                        //now that memory has been allocated, send request to open the file (triggering file transfer)
                        apx_fileManager_sendFileOpen(fileManager, remoteFile->fileInfo.address);
                        //the following line binds our new nodeData object to the apx_file_t structure
                        remoteFile->nodeData=nodeData;
                     }
                  }
               }
               else
               {
                  //client mode
               }
            }
            else
            {
               APX_LOG_ERROR("[APX_NODE_MANAGER] node already exists: %s", basename);
            }
            free(basename);
         }
      }
      else if ( (remoteFile->fileType == APX_INDATA_FILE) )
      {
         char *basename = apx_file_basename(remoteFile);
         if (basename != 0)
         {
            apx_nodeData_t *nodeData;
            //this is potentially a new node, check if it exists already
            MUTEX_LOCK(self->lock);
            nodeData = apx_nodeManager_getNodeData(self, basename);
            MUTEX_UNLOCK(self->lock);
            if ( nodeData != 0 )
            {
               if (nodeData->inPortDataLen != remoteFile->fileInfo.length)
               {
                  APX_LOG_ERROR("[APX_NODE_MANAGER(%s)] file %s has length %d, expected %d\n", apx_fileManager_modeString(fileManager), remoteFile->fileInfo.name, remoteFile->fileInfo.length, nodeData->inPortDataLen);
               }
               else
               {
                  if ( nodeData->inPortDataBuf == 0)
                  {
                     APX_LOG_ERROR("[APX_NODE_MANAGER(%s)] cannot open file %s, inPortDataBuf is NULL\n", apx_fileManager_modeString(fileManager), remoteFile->fileInfo.name);
                  }
                  else
                  {                     
                     remoteFile->nodeData=nodeData;                     
                     apx_fileManager_sendFileOpen(fileManager, remoteFile->fileInfo.address);
                  }
               }
            }
            free(basename);
         }
      }
      else if (remoteFile->fileType == APX_USER_DATA_FILE)
      {
         if(self->debugMode >= APX_DEBUG_2_LOW)
         {
            APX_LOG_INFO("[APX_NODE_MANAGER(%s)] Unsupported file: %s\n", apx_fileManager_modeString(fileManager), remoteFile->fileInfo.name);
         }
      }
      else
      {
         
      }
   }
}

/**
 * this is caled by fileManager when a file has been removed
 */
void apx_nodeManager_remotefileRemoved(apx_nodeManager_t *self, struct apx_fileManager_tag *fileManager, apx_file_t *remotefile)
{
   //printf("apx_nodeManager_remotefileRemoved\n");
}

/**
 * this is called by fileManager when a file has been written to
 */
void apx_nodeManager_remoteFileWritten(apx_nodeManager_t *self, struct apx_fileManager_tag *fileManager, apx_file_t *remoteFile, uint32_t offset, int32_t length)
{
   if ( (self != 0) && (remoteFile != 0) )
   {
      if (remoteFile->fileType == APX_DEFINITION_FILE)
      {
         MUTEX_LOCK(self->lock);
         apx_nodeManager_createNode(self, remoteFile->nodeData->definitionDataBuf, remoteFile->nodeData->definitionDataLen, fileManager);
         MUTEX_UNLOCK(self->lock);
      }
      else
      {
         uint32_t endOffset = offset + length;
         if ( (offset == 0) && (length == (int32_t) remoteFile->fileInfo.length) )
         {
            //printf("[APX_NODE_MANAGER(%s)] file received name=%s, len=%u\n", apx_fileManager_modeString(fileManager), remoteFile->fileInfo.name, length);
         }
         else
         {
            //printf("[APX_NODE_MANAGER(%s)] file updated name=%s, offset=%d, len=%u\n", apx_fileManager_modeString(fileManager), remoteFile->fileInfo.name, offset, length);
         }
         if (remoteFile->fileType == APX_OUTDATA_FILE)
         {
            apx_nodeInfo_t *nodeInfo = remoteFile->nodeData->nodeInfo;
            assert(nodeInfo != 0);
            while (offset < endOffset)
            {
               apx_dataTriggerFunction_t *triggerFunction;
               triggerFunction = apx_nodeInfo_getTriggerFunction(nodeInfo, offset);
               if (triggerFunction != 0)
               {
                  apx_nodeManager_executePortTriggerFunction(triggerFunction, remoteFile);
                  offset = triggerFunction->srcOffset + triggerFunction->dataLength;
               }
               else
               {
                  //fprintf(stderr, "APX_NODE_MANAGER(%s)] no trigger function found for file %s at offset %d\n", apx_fileManager_modeString(fileManager), remoteFile->fileInfo.name, offset);               
                  offset++;
               }
            }
         }
      }
   }
}

/**
 * setter for self->router
 */
void apx_nodeManager_setRouter(apx_nodeManager_t *self, struct apx_router_tag *router)
{
   if ( (self != 0) )
   {
      self->router = router;
   }
}

/**
 * attaches local node to nodeManager.
 * It is expected that at least the definitionDataBuf is set to non-NULL pointer
 */
void apx_nodeManager_attachLocalNode(apx_nodeManager_t *self, apx_nodeData_t *nodeData)
{
   if ( (self != 0) && (nodeData != 0) )
   {
      adt_list_elem_t *pIter;
      apx_nodeManager_setLocalNodeData(self, nodeData);
      //for each attached fileManager, create a new file
      adt_list_iter_init(&self->fileManagerList);
      do
      {
         pIter = adt_list_iter_next(&self->fileManagerList);
         if (pIter != 0)
         {
            apx_fileManager_t *fileManager = (apx_fileManager_t*) pIter->pItem;
            apx_nodeManager_attachLocalNodeToFileManager(nodeData, fileManager);
         }
      }while(pIter != 0);

   }
}

/**
 * attaches a fileManager to this nodeManager.
 * the fileManager pointer is stored as a weak reference inside a fileManagerList
 */
void apx_nodeManager_attachFileManager(apx_nodeManager_t *self, struct apx_fileManager_tag *fileManager)
{
   if ( (self != 0) && (fileManager != 0) )
   {
      //search for duplicates
      adt_list_elem_t *pIter;
      void **ppVal;
      adt_list_iter_init(&self->fileManagerList);

      do
      {
         pIter = adt_list_iter_next(&self->fileManagerList);
         if (pIter != 0)
         {
            if (pIter->pItem == (void*) fileManager)
            {
               printf("FileManager already attached to list\n");
               //fileManager already attached to list, take no action
               return;
            }
         }
      }while(pIter != 0);
      //add fileManager to list
      adt_list_insert(&self->fileManagerList, (void*) fileManager);
      apx_fileManager_setNodeManager(fileManager, self);

      adt_hash_iter_init(&self->localNodeDataMap);
      do
      {
         const char *key;
         uint32_t keyLen;
         ppVal = adt_hash_iter_next(&self->localNodeDataMap, &key, &keyLen);
         if (ppVal != 0 )
         {
            apx_nodeData_t *nodeData = *ppVal;
            apx_nodeManager_attachLocalNodeToFileManager(nodeData, fileManager);
         }
      } while(ppVal != 0);
   }
}

void apx_nodeManager_shutdownFileManager(apx_nodeManager_t *self, struct apx_fileManager_tag *fileManager)
{
   if ( (self != 0) && (fileManager != 0) )
   {
      int32_t i;
      int32_t numFiles;
      int32_t numNodes;
      adt_ary_t detachedFiles;
      adt_ary_t nodesToBeDeleted; //list of string pointers to apx_nodeData_t

      adt_list_remove(&self->fileManagerList, fileManager);
      adt_ary_create(&detachedFiles, apx_file_vdelete);
      adt_ary_create(&nodesToBeDeleted, apx_nodeData_vdelete);
      apx_fileManager_stop(fileManager);
      apx_fileManager_detachFiles(fileManager, &detachedFiles); //this will transfer ownership of file objects to the detachedFiles list
      numFiles = adt_ary_length(&detachedFiles);
      for (i=0; i<numFiles; i++)
      {
         apx_file_t *file = (apx_file_t*) adt_ary_value(&detachedFiles, i);
         if (file->nodeData != 0)
         {
            adt_ary_push_unique(&nodesToBeDeleted, file->nodeData);
         }
      }
      numNodes = adt_ary_length(&nodesToBeDeleted);
      for (i = 0; i < numNodes; i++)
      {
         apx_nodeData_t *nodeData = (apx_nodeData_t*)adt_ary_value(&nodesToBeDeleted, i);
         apx_nodeManager_removeRemoteNodeData(self, nodeData);
         if (nodeData->nodeInfo != 0)
         {
            apx_nodeInfo_t *nodeInfo = nodeData->nodeInfo;
            if (self->router != 0)
            {
               apx_router_detachNodeInfo(self->router, nodeInfo);
            }
            apx_nodeManager_removeNodeInfo(self, nodeInfo);
            apx_nodeInfo_delete(nodeInfo);
         }
      }
      adt_ary_destroy(&nodesToBeDeleted); //this will delete all nodeData objects using its virtual destructor.
      adt_ary_destroy(&detachedFiles); //this will delete all files using the virtual destructor.
   }
}

void apx_nodeManager_setDebugMode(apx_nodeManager_t *self, int8_t debugMode)
{
   if (self != 0)
   {
      self->debugMode=debugMode;
   }
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

/**
 * used to create new remote nodes on server side
 */
static void apx_nodeManager_createNode(apx_nodeManager_t *self, const uint8_t *definitionBuf, int32_t definitionLen, struct apx_fileManager_tag *fileManager)
{
   if( (self != 0) && (definitionBuf != 0) && (definitionLen > 0) )
   {
      int32_t numNodes;
      int32_t i;
      char debugInfoStr[APX_DEBUG_INFO_MAX_LEN];
      debugInfoStr[0]=0;
      if (fileManager->debugInfo != 0)
      {
         snprintf(debugInfoStr, APX_DEBUG_INFO_MAX_LEN, " (%p)", fileManager->debugInfo);
      }
      APX_LOG_INFO("[APX_NODE_MANAGER]%s Server processing APX definition, len=%d", debugInfoStr, (int) definitionLen);


      apx_istream_reset(&self->apx_istream);
      apx_istream_open(&self->apx_istream);
      apx_istream_write(&self->apx_istream, definitionBuf, (uint32_t) definitionLen);
      apx_istream_close(&self->apx_istream);
      numNodes = apx_parser_getNumNodes(&self->parser);
      for (i=0;i<numNodes;i++)
      {
         apx_nodeInfo_t *nodeInfo;
         apx_node_t *apxNode = apx_parser_getNode(&self->parser, i);
         assert(apxNode != 0);
         apx_node_finalize(apxNode);
         nodeInfo = apx_nodeInfo_new(apxNode);
         if (nodeInfo != 0)
         {
            apx_nodeData_t *nodeData=0;
            char fileName[RMF_MAX_FILE_NAME];
            char *p;
            int32_t inPortDataLen;
            int32_t outPortDataLen;


            nodeData = apx_nodeManager_getNodeData(self, apxNode->name);
            if (nodeData == 0)
            {
               APX_LOG_ERROR("[APX_NODE_MANAGER] %s", "Failed to create nodeData object");
               return;
            }
            apx_nodeData_setFileManager(nodeData,fileManager);
            apx_nodeData_setNodeInfo(nodeData, nodeInfo);
            apx_nodeInfo_setNodeData(nodeInfo, nodeData);
            nodeInfo->isWeakRef_node = false; //nodeInfo is now the owner of the node pointer (will trigger deletion when apx_nodeInfo_delete is called)
            adt_hash_set(&self->nodeInfoMap, apxNode->name, 0, nodeInfo);
            inPortDataLen = apx_nodeInfo_getInPortDataLen(nodeInfo);
            outPortDataLen = apx_nodeInfo_getOutPortDataLen(nodeInfo);
            
            //if node has output data, search a file called "<node_name>.out"
            if (outPortDataLen > 0)
            {
               apx_file_t *outDataFile;
               strcpy(fileName,apxNode->name);
               p=fileName+strlen(fileName);
               strcpy(p,".out");
               
               outDataFile = apx_fileManager_findRemoteFile(fileManager, fileName);
               if (outDataFile != 0)
               {
                  //check if length of file is the expected length of our outPortDataLen calculation
                  if (outPortDataLen != (int32_t) outDataFile->fileInfo.length)
                  {
                     APX_LOG_ERROR("[APX_NODE_MANAGER] length of file %s is %d, expected length was %d\n", fileName, outDataFile->fileInfo.length, outPortDataLen);
                  }
                  else
                  {
                     if (outDataFile->nodeData==0)
                     {
                        outDataFile->nodeData=nodeData;
                        //now create memory for the outPortData
                        nodeData->outPortDataBuf = (uint8_t*) malloc(outPortDataLen);
                        assert(nodeData->outPortDataBuf);
                        nodeData->outPortDirtyFlags = (uint8_t*) malloc(outPortDataLen);
                        assert(nodeData->outPortDirtyFlags);
                        nodeData->outPortDataLen = outPortDataLen;
                        APX_LOG_INFO("[APX_NODE_MANAGER]%s Server opening client file %s[%d,%d]", debugInfoStr, fileName, outDataFile->fileInfo.address, outDataFile->fileInfo.length);
                        apx_nodeData_setNodeInfo(nodeData, nodeInfo);
                        apx_fileManager_sendFileOpen(fileManager, outDataFile->fileInfo.address);
                     }
                  }
               }
               else
               {
                  APX_LOG_WARNING("[APX_NODE_MANAGER] '%s': no file found", fileName);
               }
            }
            if (inPortDataLen > 0)
            {
               //create local inPortData file
               apx_file_t *inDataFile;
               bool result;
               strcpy(fileName,apxNode->name);
               p=fileName+strlen(fileName);
               strcpy(p,".in");
               
               nodeData->inPortDataBuf = (uint8_t*) malloc(inPortDataLen);
               assert(nodeData->inPortDataBuf);
               nodeData->inPortDirtyFlags = (uint8_t*) malloc(inPortDataLen);
               assert(nodeData->inPortDirtyFlags);
               result = apx_nodeManager_createInitData(apxNode, nodeData->inPortDataBuf, inPortDataLen);
               if (result == false)
               {
                  APX_LOG_ERROR("[APX_NODE_MANAGER] Failed to create init data for node %s", apx_node_getName(apxNode));
               }
               nodeData->inPortDataLen = inPortDataLen;
               inDataFile = apx_file_newLocalInPortDataFile(nodeData);
               if (inDataFile != 0)
               {
                  apx_fileManager_attachLocalPortDataFile(fileManager, inDataFile);
                  APX_LOG_INFO("[APX_NODE_MANAGER]%s Server created file %s[%d,%d]", debugInfoStr, fileName, inDataFile->fileInfo.address, inDataFile->fileInfo.length);
               }
               else
               {
                  APX_LOG_ERROR("[APX_NODE_MANAGER]%s Server failed to create local file '%s'", debugInfoStr, fileName);
               }
            }
            //router is set, attach the newly create nodeInfo to the router
            if (self->router != 0)
            {
               apx_router_attachNodeInfo(self->router, nodeInfo);
            }
            //for all connected require ports copy data from the provide port into our newly create inDataFile buffer
            apx_nodeInfo_copyInitDataFromProvideConnectors(nodeInfo);
         }
      }
      apx_parser_clearNodes(&self->parser);
   }
}

/**
 * searches both local and remote nodeData maps using name as key
 */
static apx_nodeData_t *apx_nodeManager_getNodeData(const apx_nodeManager_t *self, const char *name)
{
   if ( (self != 0) && (name != 0) )
   {
      void** ppVal;
      ppVal = adt_hash_get(&self->remoteNodeDataMap, name, 0);
      if (ppVal != 0)
      {
         return (apx_nodeData_t*) *ppVal;
      }
      ppVal = adt_hash_get(&self->localNodeDataMap, name, 0);
      if (ppVal != 0)
      {
         return (apx_nodeData_t*) *ppVal;
      }
   }
   return (apx_nodeData_t*) 0;
}

/**
 * adds nodeData to localNodeDataMap
 */
static void apx_nodeManager_setLocalNodeData(apx_nodeManager_t *self, apx_nodeData_t *nodeData)
{
   if ( (self != 0) && (nodeData != 0) )
   {
      adt_hash_set(&self->localNodeDataMap, nodeData->name, 0, nodeData);
   }
}

/**
 * applies the portTriggerFunction
 */
static void apx_nodeManager_executePortTriggerFunction(const apx_dataTriggerFunction_t *triggerFunction, const apx_file_t *file)
{
   if( (triggerFunction != 0) && (file != 0) )
   {
      if (file->fileType == APX_OUTDATA_FILE)
      {
         uint8_t *dataBuf = (uint8_t*) malloc(triggerFunction->dataLength);
         if (dataBuf != 0)
         {
            int8_t result = apx_nodeData_readOutPortData(file->nodeData, dataBuf, triggerFunction->srcOffset, triggerFunction->dataLength);
            if (result == 0)
            {
               int32_t i;
               int32_t end = adt_ary_length(&triggerFunction->writeInfoList);
               for(i=0;i<end;i++)
               {
                  apx_dataWriteInfo_t *writeInfo = (apx_dataWriteInfo_t*) adt_ary_value(&triggerFunction->writeInfoList, i);
                  apx_nodeInfo_t *targetNodeInfo = writeInfo->requesterNodeInfo;
                  if( targetNodeInfo->nodeData != 0)
                  {
                     apx_nodeData_t *targetNodeData = targetNodeInfo->nodeData;
                     if( (targetNodeData->inPortDataFile != 0) && (targetNodeData->fileManager != 0) )
                     {
                        apx_fileManager_triggerFileWriteCmdEvent(targetNodeData->fileManager, targetNodeData->inPortDataFile, dataBuf, writeInfo->destOffset, triggerFunction->dataLength);
                     }
                  }
               }
            }
            free(dataBuf);
         }
      }
   }
}

static void apx_nodeManager_attachLocalNodeToFileManager(apx_nodeData_t *nodeData, apx_fileManager_t *fileManager)
{
   if (nodeData->definitionDataLen > 0)
   {
      apx_file_t *definitionFile;
      definitionFile = apx_file_newLocalDefinitionFile(nodeData);
      assert(definitionFile != 0);
      apx_fileManager_attachLocalDefinitionFile(fileManager, definitionFile);      

   }
   if (nodeData->outPortDataLen)
   {
      apx_file_t *outPortDataFile;
      outPortDataFile = apx_file_newLocalOutPortDataFile(nodeData);
      assert(outPortDataFile != 0);
      apx_fileManager_attachLocalPortDataFile(fileManager, outPortDataFile);      
   }
}

static void apx_nodeManager_removeRemoteNodeData(apx_nodeManager_t *self, apx_nodeData_t *nodeData)
{
   if ( (self != 0) && (nodeData != 0) )
   {
      void **tmp = adt_hash_remove(&self->remoteNodeDataMap, nodeData->name, 0);
      assert(tmp != 0);
   }
}

static void apx_nodeManager_removeNodeInfo(apx_nodeManager_t *self, apx_nodeInfo_t *nodeInfo)
{
   if ( (self != 0) && (nodeInfo != 0) )
   {
      void **tmp = adt_hash_remove(&self->nodeInfoMap, nodeInfo->node->name, 0);
      assert(tmp != 0);
   }
}

static bool apx_nodeManager_createInitData(apx_node_t *node, uint8_t *buf, int32_t bufLen)
{
   if ( (node != 0) && (buf != 0) && (bufLen > 0))
   {
      uint8_t *pNext = buf;
      uint8_t *pEnd = buf+bufLen;
      int32_t i;
      int32_t numRequirePorts;
      adt_bytearray_t *portData;
      portData = adt_bytearray_new(0);
      numRequirePorts = apx_node_getNumRequirePorts(node);
      for(i=0; i<numRequirePorts; i++)
      {
         int32_t packLen;
         int32_t dataLen;
         apx_port_t *port = apx_node_getRequirePort(node, i);
         assert(port != 0);
         packLen = apx_port_getPackLen(port);
         apx_node_fillPortInitData(node, port, portData);
         dataLen = adt_bytearray_length(portData);
         assert(packLen == dataLen);
         memcpy(pNext, adt_bytearray_data(portData), packLen);
         pNext+=packLen;
         assert(pNext<=pEnd);
      }
      assert(pNext==pEnd);
      adt_bytearray_delete(portData);
      return true;
   }
   return false;
}
