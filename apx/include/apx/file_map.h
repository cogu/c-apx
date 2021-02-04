/*****************************************************************************
* \file      file_map.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     File map data structure
*
* Copyright (c) 2017-2021 Conny Gustafsson
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
