/*****************************************************************************
* \file      apx_nodeInstance.h
* \author    Conny Gustafsson
* \date      2019-12-02
* \brief     Parent container for all things node-related.
*
* Copyright (c) 2019 Conny Gustafsson
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
#ifndef APX_NODE_INSTANCE_H
#define APX_NODE_INSTANCE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "apx_node.h"
#include "apx_nodeInfo.h"
#include "apx_nodeData.h"
#include "apx_portDataRef.h"
#include "apx_portConnectorList.h"
#include "apx_eventListener.h"
#include "apx_file.h"
#include "apx_error.h"
#include "apx_parser.h"
#include "apx_portConnectorChangeTable.h"
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
#else
# include <pthread.h>
#endif
#include "osmacro.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_connectionBase_tag;


typedef struct apx_nodeInstance_tag
{
   apx_node_t *parseTree; //Temporary parse tree of APX definition file (strong reference)
   apx_nodeInfo_t *nodeInfo; //All static information about an APX node (strong reference)
   apx_nodeData_t *nodeData; //All dynamic data in a node, things that change during runtime (strong reference)
   apx_portRef_t *requirePortReferences; //Array of apx_portRef_t, length of array: info->numRequirePorts.  This is created using a single (array-sized) malloc. Only used in server mode.
   apx_portRef_t *providePortReferences; //Array apx_portRef_t, length of array: info->numProvidePorts. This is created using a single (array-sized) malloc. Only used in server mode.
   apx_portConnectorList_t *connectorTable; //Array of apx_portConnectorList_t; Length of array: info->numProvidePorts. Created using a single malloc. Only used in server mode.
   struct apx_connectionBase_tag *connection; //Weak reference
   apx_file_t *definitionFile;       //pointer to file in file manager
   apx_file_t *providePortDataFile;  //pointer to file in file manager
   apx_file_t *requirePortDataFile;  //pointer to file in file manager
   apx_portConnectorChangeTable_t *requirePortChanges; //temporary data structure used for tracking port connector changes to requirePorts
   apx_portConnectorChangeTable_t *providePortChanges; //temporary data structure used for tracking port connector changes to providePorts
   apx_mode_t mode;
   apx_requirePortDataState_t requirePortDataState;
   apx_providePortDataState_t providePortDataState;
   MUTEX_T connectorTableLock;
} apx_nodeInstance_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_nodeInstance_create(apx_nodeInstance_t *self, apx_mode_t mode);
void apx_nodeInstance_destroy(apx_nodeInstance_t *self);
apx_nodeInstance_t *apx_nodeInstance_new(apx_mode_t mode);
void apx_nodeInstance_delete(apx_nodeInstance_t *self);
void apx_nodeInstance_vdelete(void *arg);

apx_nodeData_t* apx_nodeInstance_createNodeData(apx_nodeInstance_t *self);
apx_nodeData_t* apx_nodeInstance_getNodeData(apx_nodeInstance_t *self);


apx_error_t apx_nodeInstance_updatePortDataDirect(apx_nodeInstance_t *destNodeInstance, const apx_portDataProps_t *destDataProps, apx_nodeInstance_t *srcNodeInstance, const apx_portDataProps_t *srcDataProps);
apx_error_t apx_nodeInstance_updatePortDataDirectById(apx_nodeInstance_t *destNode, apx_portId_t destPortId, apx_nodeInstance_t *srcNode, apx_portId_t srcPortId);
apx_error_t apx_nodeInstance_parseDefinition(apx_nodeInstance_t *self, apx_parser_t *parser);
apx_node_t *apx_nodeInstance_getParseTree(apx_nodeInstance_t *self);
apx_error_t apx_nodeInstance_createDefinitionBuffer(apx_nodeInstance_t *self, apx_size_t bufferLen);
apx_error_t apx_nodeInstance_createPortDataBuffers(apx_nodeInstance_t *self);

apx_error_t apx_nodeInstance_buildNodeInfo(apx_nodeInstance_t *self, apx_programType_t *errProgramType, apx_uniquePortId_t *errPortId);
apx_nodeInfo_t *apx_nodeInstane_getNodeInfo(apx_nodeInstance_t *self);
apx_error_t apx_nodeInstance_buildPortRefs(apx_nodeInstance_t *self);
apx_portRef_t *apx_nodeInstance_getPortRef(apx_nodeInstance_t *self, apx_uniquePortId_t portId);
apx_portRef_t *apx_nodeInstance_getRequirePortRef(apx_nodeInstance_t *self, apx_portId_t portId);
apx_portRef_t *apx_nodeInstance_getProvidePortRef(apx_nodeInstance_t *self, apx_portId_t portId);
void apx_nodeInstance_setConnection(apx_nodeInstance_t *self, struct apx_connectionBase_tag *connection);
struct apx_connectionBase_tag* apx_nodeInstance_getConnection(apx_nodeInstance_t *self);
void apx_nodeInstance_registerDefinitionFileHandler(apx_nodeInstance_t *self, apx_file_t *file);
void apx_nodeInstance_registerProvidePortFileHandler(apx_nodeInstance_t *self, apx_file_t *file);
void apx_nodeInstance_registerRequirePortFileHandler(apx_nodeInstance_t *self, apx_file_t *file);
const char *apx_nodeInstance_getName(apx_nodeInstance_t *self);
void apx_nodeInstance_cleanParseTree(apx_nodeInstance_t *self);
apx_nodeInfo_t *apx_nodeInstance_getNodeInfo(apx_nodeInstance_t *self);
apx_portCount_t apx_nodeInstance_getNumProvidePorts(apx_nodeInstance_t *self);
apx_portCount_t apx_nodeInstance_getNumRequirePorts(apx_nodeInstance_t *self);
apx_error_t apx_nodeInstance_fillProvidePortDataFileInfo(apx_nodeInstance_t *self, apx_fileInfo_t *fileInfo);
apx_error_t apx_nodeInstance_fillDefinitionFileInfo(apx_nodeInstance_t *self, apx_fileInfo_t *fileInfo);
void apx_nodeInstance_setProvidePortDataState(apx_nodeInstance_t *self, apx_providePortDataState_t state);
apx_providePortDataState_t apx_nodeInstance_getProvidePortDataState(apx_nodeInstance_t *self);
void apx_nodeInstance_setRequirePortDataState(apx_nodeInstance_t *self, apx_requirePortDataState_t state);
apx_requirePortDataState_t apx_nodeInstance_getRequirePortDataState(apx_nodeInstance_t *self);

/********** Data API  ************/
apx_error_t apx_nodeInstance_writeDefinitionData(apx_nodeInstance_t *self, const uint8_t *src, uint32_t offset, uint32_t len);
apx_error_t apx_nodeInstance_readDefinitionData(apx_nodeInstance_t *self, uint8_t *dest, uint32_t offset, uint32_t len);
apx_error_t apx_nodeInstance_writeProvidePortData(apx_nodeInstance_t *self, const uint8_t *src, uint32_t offset, apx_size_t len);
apx_error_t apx_nodeInstance_readProvidePortData(apx_nodeInstance_t *self, uint8_t *dest, uint32_t offset, apx_size_t len);
apx_error_t apx_nodeInstance_readRequirePortData(apx_nodeInstance_t *self, uint8_t *dest, uint32_t offset, uint32_t len);
apx_error_t apx_nodeInstance_writeRequirePortData(apx_nodeInstance_t *self, const uint8_t *src, uint32_t offset, apx_size_t len);

/********** P-Port connector API  ************/
apx_error_t apx_nodeInstance_buildConnectorTable(apx_nodeInstance_t *self);
void apx_nodeInstance_lockPortConnectorTable(apx_nodeInstance_t *self);
void apx_nodeInstance_unlockPortConnectorTable(apx_nodeInstance_t *self);
apx_portConnectorList_t *apx_nodeInstance_getProvidePortConnectors(apx_nodeInstance_t *self, apx_portId_t portId);
apx_error_t apx_nodeInstance_insertProvidePortConnector(apx_nodeInstance_t *self, apx_portId_t portId, apx_portRef_t *requirePortRef);

/********** Port Connector Change API  ************/
apx_portConnectorChangeTable_t* apx_nodeInstance_getRequirePortConnectorChanges(apx_nodeInstance_t *self, bool autoCreate);
apx_portConnectorChangeTable_t* apx_nodeInstance_getProvidePortConnectorChanges(apx_nodeInstance_t *self, bool autoCreate);
void apx_nodeInstance_clearRequirePortConnectorChanges(apx_nodeInstance_t *self, bool releaseMemory);
void apx_nodeInstance_clearProvidePortConnectorChanges(apx_nodeInstance_t *self, bool releaseMemory);

/********** Port Routing API  ************/
apx_error_t apx_nodeInstance_handleRequirePortWasConnectedToProvidePort(apx_portRef_t *requirePortRef, apx_portRef_t *providePortRef);
apx_error_t apx_nodeInstance_handleProvidePortWasConnectedToRequirePort(apx_portRef_t *providePortRef, apx_portRef_t *requirePortRef);
apx_error_t apx_nodeInstance_sendRequirePortDataToFileManager(apx_nodeInstance_t *self);
apx_error_t apx_nodeInstance_routeProvidePortDataToReceivers(apx_nodeInstance_t *self, const uint8_t *src, uint32_t offset, apx_size_t len);


#endif //APX_NODE_INSTANCE_H
