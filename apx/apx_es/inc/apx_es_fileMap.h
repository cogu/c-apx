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
typedef struct apx_es_fileMap_tag
{
   apx_file_t *fileList[APX_ES_FILEMAP_MAX_NUM_FILES]; //list of weak references to apx_file_t
   int32_t curLen;
   int32_t lastIndex;
}apx_es_fileMap_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_es_fileMap_create(apx_es_fileMap_t *self);

int8_t apx_es_fileMap_autoInsert(apx_es_fileMap_t *self, apx_file_t *pFile);
int8_t apx_es_fileMap_insert(apx_es_fileMap_t *self, apx_file_t *pFile);
int8_t apx_es_fileMap_remove(apx_es_fileMap_t *self, apx_file_t *pFile);
apx_file_t *apx_es_fileMap_findByAddress(apx_es_fileMap_t *self, uint32_t address);
apx_file_t *apx_es_fileMap_findByName(apx_es_fileMap_t *self, const char *name);
int32_t apx_es_fileMap_length(apx_es_fileMap_t *self);
apx_file_t *apx_es_fileMap_get(apx_es_fileMap_t *self, int32_t index);

#endif //APX_ES_FILE_MAP_H

