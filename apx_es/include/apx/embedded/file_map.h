/*****************************************************************************
* \file      file_map.h
* \author    Conny Gustafsson
* \date      2017-03-12
* \brief     APX embedded file map
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_ES_FILE_MAP_H
#define APX_ES_FILE_MAP_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>
#include "apx_file.h"
#include "apx_es_cfg.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_es_file_map_tag
{
   apx_file_t *fileList[APX_ES_FILEMAP_MAX_NUM_FILES]; //list of weak references to apx_file_t
   int32_t curLen;
   int32_t lastIndex;
}apx_es_file_map_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_es_fileMap_create(apx_es_file_map_t *self);

int8_t apx_es_fileMap_autoInsert(apx_es_file_map_t *self, apx_file_t *pFile);
int8_t apx_es_fileMap_insert(apx_es_file_map_t *self, apx_file_t *pFile);
int8_t apx_es_fileMap_remove(apx_es_file_map_t *self, apx_file_t *pFile);
void apx_es_fileMap_clear(apx_es_file_map_t *self);
apx_file_t *apx_es_fileMap_findByAddress(apx_es_file_map_t *self, uint32_t address);
apx_file_t *apx_es_fileMap_findByName(apx_es_file_map_t *self, const char *name);
int32_t apx_es_fileMap_length(apx_es_file_map_t *self);
apx_file_t *apx_es_fileMap_get(apx_es_file_map_t *self, int32_t index);

#endif //APX_ES_FILE_MAP_H

