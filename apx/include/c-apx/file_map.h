/*****************************************************************************
* \file      file_map.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     File map data structure
*
* Copyright (c) 2017-2020 Conny Gustafsson
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
#include "apx/file.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_fileMap_tag
{
   adt_list_t fileList; //list of apx_file_t automatically sorted by address
   apx_file_t *lastFile; //Last accessed file (for caching repeated access requests)
} apx_fileMap_t;


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_fileMap_create(apx_fileMap_t *self);
void apx_fileMap_destroy(apx_fileMap_t *self);
int8_t apx_fileMap_insertFile(apx_fileMap_t *self, apx_file_t *pFile);
int8_t apx_fileMap_removeFile(apx_fileMap_t *self, apx_file_t *pFile);
apx_file_t *apx_fileMap_findByAddress(apx_fileMap_t *self, uint32_t address);
apx_file_t *apx_fileMap_findByName(apx_fileMap_t *self, const char *name);
int32_t apx_fileMap_length(const apx_fileMap_t *self);
void apx_fileMap_clear_weak(apx_fileMap_t *self);
adt_list_t *apx_fileMap_getList(apx_fileMap_t *self);
bool apx_fileMap_exist(apx_fileMap_t *self, apx_file_t *file);
adt_ary_t *apx_fileMap_makeFileInfoArray(apx_fileMap_t *self);



#endif //APX_FILE_MAP_H_
