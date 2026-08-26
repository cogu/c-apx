/*****************************************************************************
* \file      monitor_file_state.h
* \author    Conny Gustafsson
* \date      2021-04-05
* \brief     Current state of an observed file. Used by monitor extension. 
*
* Copyright (c) 2021 Conny Gustafsson
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
