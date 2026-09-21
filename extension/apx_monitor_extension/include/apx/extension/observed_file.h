/*****************************************************************************
* \file      observed_file.h
* \author    Conny Gustafsson
* \date      2021-04-05
* \brief     Current state of an observed file. Used by monitor extension.
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_OBSERVED_FILE_H
#define APX_OBSERVED_FILE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/extended_file_info.h"
#include "adt_str.h"
#include "adt_list.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_observedFile_tag
{
   rmf_extendedFileInfo_t file_info;
} apx_observedFile_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

void apx_observedFile_create(apx_observedFile_t* self, rmf_extendedFileInfo_t* const file_info);
void apx_observedFile_destroy(apx_observedFile_t* self);
apx_observedFile_t* apx_observedFile_new(rmf_extendedFileInfo_t* const file_info);
void apx_observedFile_delete(apx_observedFile_t* self);
void apx_observedFile_vdelete(void* arg);

#endif //APX_OBSERVED_FILE_H
