/*****************************************************************************
* \file      apx_clientInternal.h
* \author    Conny Gustafsson
* \date      2019-01-15
* \brief     apx_client internal API (Implemented by apx_client.c)
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
#ifndef APX_CLIENT_INTERNAL_H
#define APX_CLIENT_INTERNAL_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_client.h"
#include "apx_nodeDataManager.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
//Client internal API (do not call as end-user)
void apx_clientInternal_onConnect(apx_client_t *self, apx_clientConnectionBase_t *connection);
void apx_clientInternal_onDisconnect(apx_client_t *self, apx_clientConnectionBase_t *connection);
apx_error_t apx_clientInternal_attachLocalNodes(apx_client_t *self, apx_nodeDataManager_t *nodeDataManager);
void apx_clientInternal_onNodeComplete(apx_client_t *self, apx_nodeData_t *nodeData);


#endif //APX_CLIENT_INTERNAL_H
