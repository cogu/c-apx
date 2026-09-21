/*****************************************************************************
* \file      file_map.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     File map data structure
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_FILE_MAP_H_
#define APX_FILE_MAP_H_

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include "adt_list.h"
#include "adt_ary.h"
#include "apx/error.h"
#include "apx/file.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_fileMap_tag
{
   bool is_remote_map;
   adt_list_t file_list; //list of apx_file_t automatically sorted by address
   apx_file_t *last_file; //Last accessed file (for caching repeated access requests)
} apx_fileMap_t;


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_fileMap_create(apx_fileMap_t *self, bool is_remote);
void apx_fileMap_destroy(apx_fileMap_t *self);
bool apx_fileMap_is_remote(apx_fileMap_t const* self);
apx_file_t* apx_fileMap_create_file(apx_fileMap_t* self, rmf_fileInfo_t const* file_info);
apx_error_t apx_fileMap_remove_file(apx_fileMap_t *self, apx_file_t* file);
apx_file_t *apx_fileMap_find_by_address(apx_fileMap_t *self, uint32_t address);
apx_file_t *apx_fileMap_find_by_name(apx_fileMap_t *self, const char *name);
int32_t apx_fileMap_length(apx_fileMap_t const*self);
adt_list_t const* apx_fileMap_get_list(apx_fileMap_t const*self);
bool apx_fileMap_exist(apx_fileMap_t const* self, apx_file_t *file);




#endif //APX_FILE_MAP_H_
