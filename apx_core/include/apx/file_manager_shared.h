/*****************************************************************************
* \file      file_manager_shared.h
* \author    Conny Gustafsson
* \date      2020-01-23
* \brief     APX Filemanager shared data
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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

typedef struct apx_file_manager_shared_tag
{
   apx_file_map_t local_file_map;
   apx_file_map_t remote_file_map;
   uint32_t connection_id;
   rmf_version_id_t remote_file_version_id;
   apx_connection_type_t connection_type;
   bool is_connected;
   apx_connection_interface_t parent_connection;
   apx_allocator_t* allocator;
   MUTEX_T lock;
} apx_file_manager_shared_t;


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

void apx_file_manager_shared_create(apx_file_manager_shared_t *self, apx_connection_interface_t const *parent_connection, apx_allocator_t* allocator);
void apx_file_manager_shared_destroy(apx_file_manager_shared_t *self);
void apx_file_manager_shared_start(apx_file_manager_shared_t* self);
apx_file_t *apx_file_manager_shared_create_local_file(apx_file_manager_shared_t *self, const rmf_file_info_t *file_info);
apx_file_t *apx_file_manager_shared_create_remote_file(apx_file_manager_shared_t *self, const rmf_file_info_t *file_info);
int32_t apx_file_manager_shared_get_num_local_files(apx_file_manager_shared_t* self);
int32_t apx_file_manager_shared_get_num_remote_files(apx_file_manager_shared_t* self);
apx_file_t *apx_file_manager_shared_find_local_file_by_name(apx_file_manager_shared_t* self, const char *name);
apx_file_t *apx_file_manager_shared_find_remote_file_by_name(apx_file_manager_shared_t* self, const char *name);
apx_file_t *apx_file_manager_shared_find_file_by_address(apx_file_manager_shared_t* self, uint32_t address);
uint32_t apx_file_manager_shared_get_connection_id(apx_file_manager_shared_t const* self);
apx_connection_type_t apx_file_manager_shared_get_connection_type(apx_file_manager_shared_t const* self);
rmf_version_id_t apx_file_manager_shared_get_remotefile_version_id(apx_file_manager_shared_t const* self);
int32_t apx_file_manager_shared_copy_local_file_info(apx_file_manager_shared_t *self, adt_ary_t *array);
void apx_file_manager_shared_connected(apx_file_manager_shared_t *self);
void apx_file_manager_shared_disconnected(apx_file_manager_shared_t *self);
bool apx_file_manager_shared_is_connected(apx_file_manager_shared_t *self);
apx_connection_interface_t const* apx_file_manager_shared_connection(apx_file_manager_shared_t const* self);
apx_allocator_t* apx_file_manager_shared_allocator(apx_file_manager_shared_t const* self);


#endif //APX_FILE_MANAGER_SHARED_H
