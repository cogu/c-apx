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
#include "extensions_cfg.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t load_json_file(const char *filepath, dtl_dv_t **out_dv);
static apx_error_t load_json_hash_file(const char *filepath, dtl_hv_t **out_hv);
static apx_error_t load_config_from_dir(const char *dir_path, dtl_hv_t **server_config, dtl_hv_t **extensions_config);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_server_load_config(const char *path, dtl_hv_t **server_config, dtl_hv_t **extensions_config)
{
   if (path == NULL || server_config == NULL || extensions_config == NULL)
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }

   *server_config = NULL;
   *extensions_config = NULL;

   if (!cutil_is_dir(path))
   {
      return APX_NOT_A_DIRECTORY_ERROR;
   }

   return load_config_from_dir(path, server_config, extensions_config);
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

static apx_error_t load_json_hash_file(const char *filepath, dtl_hv_t **out_hv)
{
   *out_hv = NULL;
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
   *out_hv = (dtl_hv_t*) json_data;
   return APX_NO_ERROR;
}

static apx_error_t load_config_from_dir(const char *dir_path, dtl_hv_t **server_config, dtl_hv_t **extensions_config)
{
   dtl_hv_t *server_hv = NULL;
   apx_error_t result;

   // 1. Try server.json, fallback to apx_server.json
   adt_str_t *filepath = cutil_path_join(dir_path, "server.json");
   if (filepath == NULL)
   {
      return APX_MEM_ERROR;
   }
   result = load_json_hash_file(adt_str_cstr(filepath), &server_hv);
   if (result == APX_FILE_NOT_FOUND_ERROR)
   {
      adt_str_delete(filepath);
      filepath = cutil_path_join(dir_path, "apx_server.json");
      if (filepath == NULL)
      {
         return APX_MEM_ERROR;
      }
      result = load_json_hash_file(adt_str_cstr(filepath), &server_hv);
   }
   adt_str_delete(filepath);

   if (result == APX_NO_ERROR)
   {
      *server_config = server_hv;
   }
   else if (result == APX_FILE_NOT_FOUND_ERROR)
   {
      // If neither server.json nor apx_server.json found, create an empty hash for default settings
      *server_config = dtl_hv_new();
   }
   else
   {
      return result;
   }

   // 2. Load individual extension JSON configs
   dtl_hv_t *ext_hash = dtl_hv_new();
   const apx_server_extension_entry_t *entry = apx_server_get_registered_extensions();
   while (entry != NULL && entry->name != NULL)
   {
      char ext_filename[128];
      snprintf(ext_filename, sizeof(ext_filename), "%s.json", entry->name);
      adt_str_t *ext_filepath = cutil_path_join(dir_path, ext_filename);
      if (ext_filepath == NULL)
      {
         dtl_dec_ref(*server_config);
         *server_config = NULL;
         dtl_dec_ref(ext_hash);
         return APX_MEM_ERROR;
      }

      dtl_hv_t *ext_entry_hv = NULL;
      result = load_json_hash_file(adt_str_cstr(ext_filepath), &ext_entry_hv);
      if (result == APX_FILE_NOT_FOUND_ERROR)
      {
         // Try subfolder extensions/<name>.json
         adt_str_delete(ext_filepath);
         char ext_subpath[256];
         snprintf(ext_subpath, sizeof(ext_subpath), "extensions/%s.json", entry->name);
         ext_filepath = cutil_path_join(dir_path, ext_subpath);
         if (ext_filepath == NULL)
         {
            dtl_dec_ref(*server_config);
            *server_config = NULL;
            dtl_dec_ref(ext_hash);
            return APX_MEM_ERROR;
         }
         result = load_json_hash_file(adt_str_cstr(ext_filepath), &ext_entry_hv);
      }
      adt_str_delete(ext_filepath);

      if (result == APX_NO_ERROR)
      {
         dtl_hv_set_cstr(ext_hash, entry->name, (dtl_dv_t*) ext_entry_hv, false);
      }
      else if (result != APX_FILE_NOT_FOUND_ERROR)
      {
         dtl_dec_ref(*server_config);
         *server_config = NULL;
         dtl_dec_ref(ext_hash);
         return result;
      }
      // If APX_FILE_NOT_FOUND_ERROR, extension config is omitted (optional)
      entry++;
   }

   *extensions_config = ext_hash;
   return APX_NO_ERROR;
}
