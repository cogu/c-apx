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
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "server_cfg.h"
#include "dtl_json.h"
#include "extensions_cfg.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static bool is_directory(const char *path);
static apx_error_t load_json_file(const char *filepath, dtl_dv_t **out_dv);
static apx_error_t load_json_hash_file(const char *filepath, dtl_hv_t **out_hv);
static void build_filepath(char *dest, size_t dest_size, const char *dir, const char *filename);
static apx_error_t load_config_from_dir(const char *dir_path, dtl_hv_t **server_config, dtl_hv_t **extensions_config);
static apx_error_t load_config_from_file(const char *file_path, dtl_hv_t **server_config, dtl_hv_t **extensions_config);

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

   if (is_directory(path))
   {
      return load_config_from_dir(path, server_config, extensions_config);
   }
   else
   {
      return load_config_from_file(path, server_config, extensions_config);
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static bool is_directory(const char *path)
{
   struct stat st;
   char clean_path[1024];
   size_t len = strlen(path);
   if (len == 0 || len >= sizeof(clean_path))
   {
      return false;
   }
   memcpy(clean_path, path, len + 1);

   // Strip trailing slashes, but keep root like "/" or "C:\"
   while (len > 1 && (clean_path[len - 1] == '/' || clean_path[len - 1] == '\\'))
   {
#ifdef _WIN32
      if (len == 3 && clean_path[1] == ':')
      {
         break;
      }
#endif
      clean_path[len - 1] = '\0';
      len--;
   }

   if (stat(clean_path, &st) == 0)
   {
#ifdef _WIN32
      if (st.st_mode & _S_IFDIR)
      {
         return true;
      }
#else
      if (S_ISDIR(st.st_mode))
      {
         return true;
      }
#endif
   }
   return false;
}

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

static void build_filepath(char *dest, size_t dest_size, const char *dir, const char *filename)
{
   size_t dir_len = strlen(dir);
   if (dir_len > 0 && (dir[dir_len - 1] == '/' || dir[dir_len - 1] == '\\'))
   {
      snprintf(dest, dest_size, "%s%s", dir, filename);
   }
   else
   {
      snprintf(dest, dest_size, "%s/%s", dir, filename);
   }
}

static apx_error_t load_config_from_dir(const char *dir_path, dtl_hv_t **server_config, dtl_hv_t **extensions_config)
{
   char filepath[1024];
   dtl_hv_t *server_hv = NULL;
   apx_error_t result;

   // 1. Try server.json, fallback to apx_server.json
   build_filepath(filepath, sizeof(filepath), dir_path, "server.json");
   result = load_json_hash_file(filepath, &server_hv);
   if (result == APX_FILE_NOT_FOUND_ERROR)
   {
      build_filepath(filepath, sizeof(filepath), dir_path, "apx_server.json");
      result = load_json_hash_file(filepath, &server_hv);
   }

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
      build_filepath(filepath, sizeof(filepath), dir_path, ext_filename);

      dtl_hv_t *ext_entry_hv = NULL;
      result = load_json_hash_file(filepath, &ext_entry_hv);
      if (result == APX_FILE_NOT_FOUND_ERROR)
      {
         // Try subfolder extensions/<name>.json
         char ext_subpath[256];
         snprintf(ext_subpath, sizeof(ext_subpath), "extensions/%s.json", entry->name);
         build_filepath(filepath, sizeof(filepath), dir_path, ext_subpath);
         result = load_json_hash_file(filepath, &ext_entry_hv);
      }

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

static apx_error_t load_config_from_file(const char *file_path, dtl_hv_t **server_config, dtl_hv_t **extensions_config)
{
   dtl_hv_t *root_hv = NULL;
   apx_error_t result = load_json_hash_file(file_path, &root_hv);
   if (result != APX_NO_ERROR)
   {
      return result;
   }

   dtl_dv_t *server_node = dtl_hv_get_cstr(root_hv, "server");
   if (server_node != NULL)
   {
      if (dtl_dv_type(server_node) == DTL_DV_HASH)
      {
         dtl_inc_ref(server_node);
         *server_config = (dtl_hv_t*) server_node;
      }
      else
      {
         dtl_dec_ref(root_hv);
         return APX_VALUE_TYPE_ERROR;
      }
   }
   else
   {
      dtl_inc_ref(root_hv);
      *server_config = root_hv;
   }

   dtl_dv_t *ext_node = dtl_hv_get_cstr(root_hv, "extension");
   if (ext_node != NULL)
   {
      if (dtl_dv_type(ext_node) == DTL_DV_HASH)
      {
         dtl_inc_ref(ext_node);
         *extensions_config = (dtl_hv_t*) ext_node;
      }
      else
      {
         dtl_dec_ref(*server_config);
         *server_config = NULL;
         dtl_dec_ref(root_hv);
         return APX_VALUE_TYPE_ERROR;
      }
   }
   else
   {
      *extensions_config = dtl_hv_new();
   }

   dtl_dec_ref(root_hv);
   return APX_NO_ERROR;
}
