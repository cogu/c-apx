/*****************************************************************************
* \file      apx_file_manager_shared.h
* \author    Conny Gustafsson
* \date      2020-01-23
* \brief     APX Filemanager shared data
*
* Copyright (c) 2020-2021 Conny Gustafsson
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
#ifndef APX_FILE_MANAGER_SHARED_H
#define APX_FILE_MANAGER_SHARED_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/file_map.h"
#include "apx/allocator.h"
#include "apx/connection_interface.h"
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
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

typedef struct apx_fileManagerShared_tag
{
   apx_fileMap_t local_file_map;
   apx_fileMap_t remote_file_map;
   uint32_t connection_id;
   bool is_connected;
   apx_connectionInterface_t parent_connection;
   apx_allocator_t* allocator;
   MUTEX_T lock;
} apx_fileManagerShared_t;


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

void apx_fileManagerShared_create(apx_fileManagerShared_t *self, apx_connectionInterface_t const *parent_connection, apx_allocator_t* allocator);
void apx_fileManagerShared_destroy(apx_fileManagerShared_t *self);
apx_file_t *apx_fileManagerShared_create_local_file(apx_fileManagerShared_t *self, const rmf_fileInfo_t *fileInfo);
apx_file_t *apx_fileManagerShared_create_remote_file(apx_fileManagerShared_t *self, const rmf_fileInfo_t *fileInfo);
int32_t apx_fileManagerShared_get_num_local_files(apx_fileManagerShared_t* self);
int32_t apx_fileManagerShared_get_num_remote_files(apx_fileManagerShared_t* self);
apx_file_t *apx_fileManagerShared_find_local_file_by_name(apx_fileManagerShared_t* self, const char *name);
apx_file_t *apx_fileManagerShared_find_remote_file_by_name(apx_fileManagerShared_t* self, const char *name);
apx_file_t *apx_fileManagerShared_find_file_by_address(apx_fileManagerShared_t* self, uint32_t address);
void apx_fileManagerShared_set_connection_id(apx_fileManagerShared_t *self, uint32_t connection_id);
uint32_t apx_fileManagerShared_get_connection_id(apx_fileManagerShared_t* self);
int32_t apx_fileManagerShared_copy_local_file_info(apx_fileManagerShared_t *self, adt_ary_t *array);
void apx_fileManagerShared_connected(apx_fileManagerShared_t *self);
void apx_fileManagerShared_disconnected(apx_fileManagerShared_t *self);
bool apx_fileManagerShared_is_connected(apx_fileManagerShared_t *self);
apx_connectionInterface_t const* apx_fileManagerShared_connection(apx_fileManagerShared_t const* self);
apx_allocator_t* apx_fileManagerShared_allocator(apx_fileManagerShared_t const* self);


#endif //APX_FILE_MANAGER_SHARED_H
