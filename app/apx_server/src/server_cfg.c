/*****************************************************************************
 * \file      server_cfg.c
 * \author    Conny Gustafsson
 * \date      2026-08-28
 * \brief     APX Server configuration loader implementation
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "server_cfg.h"
#include "apx/server.h"
#include "dtl_av.h"
#include "dtl_json.h"
#include "dtl_sv.h"
#include "fileutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t load_json_file(const char *filepath, dtl_dv_t **out_dv);
static apx_error_t load_config_from_file(const char *filepath, dtl_hv_t **config);
static apx_error_t add_trusted_key_from_cstr(apx_server_t *server, const char *key_str);
static apx_error_t configure_security(apx_server_t *server, dtl_hv_t *sec_hv);

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

apx_error_t apx_server_configure(struct apx_server_tag *server, dtl_hv_t const *config)
{
  if (server == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (config == NULL)
  {
    return APX_NO_ERROR;
  }

  apx_server_t *srv = (apx_server_t *)server;

  dtl_dv_t *server_dv = dtl_hv_get_cstr((dtl_hv_t *)config, "apx-server");
  if (server_dv != NULL && dtl_dv_type(server_dv) == DTL_DV_HASH)
  {
    dtl_hv_t *server_hv = (dtl_hv_t *)server_dv;
    dtl_dv_t *sec_dv = dtl_hv_get_cstr(server_hv, "security");
    if (sec_dv != NULL && dtl_dv_type(sec_dv) == DTL_DV_HASH)
    {
      apx_error_t rc = configure_security(srv, (dtl_hv_t *)sec_dv);
      if (rc != APX_NO_ERROR)
      {
        return rc;
      }
    }
    dtl_dv_t *req_dv = dtl_hv_get_cstr(server_hv, "require-signed-nodes");
    if (req_dv == NULL)
    {
      req_dv = dtl_hv_get_cstr(server_hv, "require_signed_nodes");
    }
    if (req_dv != NULL && dtl_dv_type(req_dv) == DTL_DV_SCALAR)
    {
      bool ok = false;
      bool val = dtl_sv_to_bool((dtl_sv_t *)req_dv, &ok);
      if (ok)
      {
        apx_server_set_require_signed_nodes(srv, val);
      }
    }
  }

  dtl_dv_t *top_sec_dv = dtl_hv_get_cstr((dtl_hv_t *)config, "security");
  if (top_sec_dv != NULL && dtl_dv_type(top_sec_dv) == DTL_DV_HASH)
  {
    apx_error_t rc = configure_security(srv, (dtl_hv_t *)top_sec_dv);
    if (rc != APX_NO_ERROR)
    {
      return rc;
    }
  }

  return APX_NO_ERROR;
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
  *config = (dtl_hv_t *)json_data;
  return APX_NO_ERROR;
}

static apx_error_t add_trusted_key_from_cstr(apx_server_t *server, const char *key_str)
{
  if (key_str == NULL || *key_str == '\0')
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (strncmp(key_str, "-----BEGIN", 10) == 0 || strchr(key_str, '\n') != NULL)
  {
    return apx_server_add_trusted_public_key(server, (const uint8_t *)key_str, strlen(key_str) + 1);
  }
  apx_error_t rc = apx_server_add_trusted_public_key_file(server, key_str);
  if (rc != APX_NO_ERROR && rc != APX_FILE_NOT_FOUND_ERROR)
  {
    return rc;
  }
  if (rc == APX_FILE_NOT_FOUND_ERROR)
  {
    return apx_server_add_trusted_public_key(server, (const uint8_t *)key_str, strlen(key_str) + 1);
  }
  return APX_NO_ERROR;
}

static apx_error_t configure_security(apx_server_t *server, dtl_hv_t *sec_hv)
{
  if (sec_hv == NULL)
  {
    return APX_NO_ERROR;
  }
  dtl_dv_t *req_dv = dtl_hv_get_cstr(sec_hv, "require-signed-nodes");
  if (req_dv == NULL)
  {
    req_dv = dtl_hv_get_cstr(sec_hv, "require_signed_nodes");
  }
  if (req_dv != NULL && dtl_dv_type(req_dv) == DTL_DV_SCALAR)
  {
    bool ok = false;
    bool val = dtl_sv_to_bool((dtl_sv_t *)req_dv, &ok);
    if (ok)
    {
      apx_server_set_require_signed_nodes(server, val);
    }
  }

  dtl_dv_t *keys_dv = dtl_hv_get_cstr(sec_hv, "trusted-keys");
  if (keys_dv == NULL)
  {
    keys_dv = dtl_hv_get_cstr(sec_hv, "trusted_keys");
  }
  if (keys_dv != NULL)
  {
    if (dtl_dv_type(keys_dv) == DTL_DV_ARRAY)
    {
      dtl_av_t *av = (dtl_av_t *)keys_dv;
      int32_t len = dtl_av_length(av);
      for (int32_t i = 0; i < len; ++i)
      {
        dtl_dv_t *item = dtl_av_value(av, i);
        if (item != NULL && dtl_dv_type(item) == DTL_DV_SCALAR)
        {
          bool ok = false;
          const char *cstr = dtl_sv_to_cstr((dtl_sv_t *)item, &ok);
          if (ok && cstr != NULL)
          {
            apx_error_t rc = add_trusted_key_from_cstr(server, cstr);
            if (rc != APX_NO_ERROR)
            {
              return rc;
            }
          }
        }
      }
    }
    else if (dtl_dv_type(keys_dv) == DTL_DV_SCALAR)
    {
      bool ok = false;
      const char *cstr = dtl_sv_to_cstr((dtl_sv_t *)keys_dv, &ok);
      if (ok && cstr != NULL)
      {
        apx_error_t rc = add_trusted_key_from_cstr(server, cstr);
        if (rc != APX_NO_ERROR)
        {
          return rc;
        }
      }
    }
  }
  return APX_NO_ERROR;
}
