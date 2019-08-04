/*****************************************************************************
* \file      apx_client.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX client class
*
* Copyright (c) 2017-2018 Conny Gustafsson
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
#ifndef APX_CLIENT_H
#define APX_CLIENT_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>
#include "apx_error.h"
#include "apx_clientConnectionBase.h"



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_nodeData_tag;
struct adt_ary_tag;
struct adt_list_tag;
struct apx_clientEventListener_tag;
struct apx_fileManager_tag;
struct apx_parser_tag;
struct apx_nodeDataManager_tag;

#ifdef UNIT_TEST
struct testsocket_tag;
#endif

typedef struct apx_client_tag
{
   apx_clientConnectionBase_t *connection;
   struct adt_ary_tag *nodeDataList; //weak references to apx_nodeData_t. This is used to store external nodeData objects attached with apx_client_attachLocalNode
   struct adt_ary_tag *nodeInfoList; //strong references to apx_nodeInfo_t. This is used when nodeData objects are created dynamically from string or file.
   struct adt_list_tag *eventListeners; //weak references to apx_clientEventListener_t
   struct apx_parser_tag *parser;
}apx_client_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_client_create(apx_client_t *self);
void apx_client_destroy(apx_client_t *self);
apx_client_t *apx_client_new(void);
void apx_client_delete(apx_client_t *self);
void apx_client_vdelete(void *arg);

#ifdef UNIT_TEST
apx_error_t apx_client_socketConnect(apx_client_t *self, struct testsocket_tag *socketObject);
#else
apx_error_t apx_client_connectTcp(apx_client_t *self, const char *address, uint16_t port);
# ifndef _WIN32
apx_error_t apx_client_connectUnix(apx_client_t *self, const char *socketPath);
# endif
#endif
void apx_client_disconnect(apx_client_t *self);
apx_error_t apx_client_attachLocalNode(apx_client_t *self, struct apx_nodeData_tag *nodeData);
apx_error_t apx_client_attachLocalNodeFromString(apx_client_t *self, const char *apx_text);
void apx_client_registerEventListener(apx_client_t *self, struct apx_clientEventListener_tag *eventListener);
int32_t apx_client_getNumLocalNodes(apx_client_t *self);
void apx_client_attachConnection(apx_client_t *self, apx_clientConnectionBase_t *connection);
apx_clientConnectionBase_t *apx_client_getConnection(apx_client_t *self);



#ifdef UNIT_TEST
void apx_client_run(apx_client_t *self);
#endif


#endif //APX_CLIENT_H
