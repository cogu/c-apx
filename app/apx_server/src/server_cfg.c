/*****************************************************************************
* \file      server_cfg.c
* \author    Conny Gustafsson
* \date      2026-08-28
* \brief     APX Server configuration loader implementation
*
* Copyright (c) 2026 Conny Gustafsson
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "server_cfg.h"
#include "fileutil.h"
#include "dtl_json.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t load_json_file(const char *filepath, dtl_dv_t **out_dv);
static apx_error_t load_config_from_file(const char *filepath, dtl_hv_t **config);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_server_load_config(const char *path, dtl_hv_t **config)
{
   if (path == NULL || config == NULL)
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }

   *config = NULL;

   if (cutil_is_dir(path))
   {
      // 1. Try server.json, fallback to apx_server.json
      adt_str_t *filepath = cutil_path_join(path, "server.json");
      if (filepath == NULL)
      {
         return APX_MEM_ERROR;
      }
      FILE *fh = fopen(adt_str_cstr(filepath), "r");
      if (fh == NULL)
      {
         adt_str_delete(filepath);
         filepath = cutil_path_join(path, "apx_server.json");
         if (filepath == NULL)
         {
            return APX_MEM_ERROR;
         }
         fh = fopen(adt_str_cstr(filepath), "r");
         if (fh == NULL)
         {
            adt_str_delete(filepath);
            return APX_FILE_NOT_FOUND_ERROR;
         }
      }
      fclose(fh);
      apx_error_t result = load_config_from_file(adt_str_cstr(filepath), config);
      adt_str_delete(filepath);
      return result;
   }
   else
   {
      return load_config_from_file(path, config);
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t load_json_file(const char *filepath, dtl_dv_t **out_dv)
{
   *out_dv = NULL;
   FILE *fh = fopen(filepath, "r");
   if (fh == NULL)
   {
      return APX_FILE_NOT_FOUND_ERROR;
   }
   dtl_dv_t *json_data = dtl_json_load(fh);
   fclose(fh);
   if (json_data == NULL)
   {
      return APX_PARSE_ERROR;
   }
   *out_dv = json_data;
   return APX_NO_ERROR;
}

static apx_error_t load_config_from_file(const char *filepath, dtl_hv_t **config)
{
   *config = NULL;
   dtl_dv_t *json_data = NULL;
   apx_error_t result = load_json_file(filepath, &json_data);
   if (result != APX_NO_ERROR)
   {
      return result;
   }
   if (dtl_dv_type(json_data) != DTL_DV_HASH)
   {
      dtl_dec_ref(json_data);
      return APX_VALUE_TYPE_ERROR;
   }
   *config = (dtl_hv_t*) json_data;
   return APX_NO_ERROR;
}
