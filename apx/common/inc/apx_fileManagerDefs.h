/*****************************************************************************
* \file      apx_fileManagerCommon.h
* \author    Conny Gustafsson
* \date      2018-08-02
* \brief     APX FileManager common definitions
*
* Copyright (c) 2018 Conny Gustafsson
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
#ifndef APX_FILE_MANAGER_COMMON_H
#define APX_FILE_MANAGER_COMMON_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//forward declarations
struct apx_nodeData_tag;
struct apx_nodeManager_tag;
struct apx_serverEventRecorder_tag;
struct apx_serverEventPlayer_tag;
struct apx_clientEventRecorder_tag;
struct apx_clientEventPlayer_tag;
struct apx_file2_tag;

typedef struct apx_serverEventContainer_tag
{
   struct apx_serverEventRecorder_tag *recorder;
   struct apx_serverEventPlayer_tag *player;
}apx_serverEventContainer_t;

typedef struct apx_clientEventContainer_tag
{
   struct apx_clientEventRecorder_tag *recorder;
   struct apx_clientEventPlayer_tag *player;
}apx_clientEventContainer_t;


#endif //APX_FILE_MANAGER_COMMON_H
