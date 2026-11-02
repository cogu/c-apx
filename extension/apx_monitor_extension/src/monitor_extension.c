/*****************************************************************************
 * \file      monitor_extension.c
 * \author    Conny Gustafsson
 * \date      2021-02-18
 * \brief     Monitor extension
 *
 * Copyright (c) 2021-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/extension/monitor_extension.h"
#include "apx/extension/server_monitor.h"
#include "apx/server.h"
#include <string.h>

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
static const char *const m_config_keys[] = {"enabled", "socket-file", "unix-file", "tcp-port", "port", "bind-address",
  "tls", "tls.server-cert", "tls.certificate", "tls.server-key", "tls.key", "tls.ca-cert", "tls.require-client-cert",
  NULL};

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t init(struct apx_server_tag *apx_server, dtl_dv_t *config);
static void shutdown(void);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
static apx_server_monitor_t *m_instance = NULL;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_monitor_extension_register(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
  apx_server_extension_handler_t handler = {init, shutdown};
  return apx_server_add_extension(apx_server, "MONITOR", &handler, config);
}

const char *const *apx_monitor_extension_config_keys(void) { return m_config_keys; }

void apx_monitor_extension_check_config(dtl_hv_t *cfg, apx_config_warning_cb cb, void *arg)
{
  if (cfg == NULL || cb == NULL)
  {
    return;
  }
  bool is_enabled = true;
  dtl_sv_t *enabled_sv = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "enabled");
  if (enabled_sv != NULL)
  {
    bool ok = false;
    is_enabled = dtl_sv_to_bool(enabled_sv, &ok);
    if (!ok)
    {
      is_enabled = true;
    }
  }
  if (!is_enabled)
  {
    return;
  }

  bool has_unix = (dtl_hv_get_cstr(cfg, "socket-file") != NULL || dtl_hv_get_cstr(cfg, "unix-file") != NULL);
  bool has_tcp = (dtl_hv_get_cstr(cfg, "tcp-port") != NULL || dtl_hv_get_cstr(cfg, "port") != NULL);

  if (!has_unix && !has_tcp)
  {
    cb(arg,
      "Warning: 'monitor-extension' is enabled, but no listening endpoints are configured (expected 'socket-file' or "
      "'tcp-port')");
  }

  dtl_dv_t *tls_dv = dtl_hv_get_cstr(cfg, "tls");
  if (tls_dv != NULL && dtl_dv_type(tls_dv) == DTL_DV_HASH)
  {
    dtl_hv_t *tls_hv = (dtl_hv_t *)tls_dv;
    bool has_cert = (dtl_hv_get_cstr(tls_hv, "server-cert") != NULL || dtl_hv_get_cstr(tls_hv, "certificate") != NULL);
    bool has_key = (dtl_hv_get_cstr(tls_hv, "server-key") != NULL || dtl_hv_get_cstr(tls_hv, "key") != NULL);
    if (!has_cert || !has_key)
    {
      cb(arg,
        "Warning: 'monitor-extension' tls configuration is incomplete (requires both 'server-cert' and 'server-key')");
    }
  }
}

#ifdef UNIT_TEST
void apx_monitor_extension_accept_testsocket(struct testsocket_tag *sock)
{
  if (m_instance != NULL)
  {
    apx_server_monitor_accept_testsocket(m_instance, (testsocket_t *)sock);
  }
}
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static apx_error_t init(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
  if (m_instance == NULL)
  {
    m_instance = apx_server_monitor_new(apx_server);
    if (m_instance == NULL)
    {
      return APX_MEM_ERROR;
    }
    if (config != NULL)
    {
      if (dtl_dv_type(config) == DTL_DV_HASH)
      {
        apx_error_t err = apx_server_monitor_configure(m_instance, (dtl_hv_t *)config);
        if (err != APX_NO_ERROR)
        {
          return err;
        }
      }
      else
      {
        return APX_VALUE_TYPE_ERROR;
      }
    }
    return apx_server_monitor_start(m_instance);
  }
  return APX_NO_ERROR;
}

static void shutdown(void)
{
  if (m_instance != NULL)
  {
    apx_server_monitor_stop(m_instance);
    apx_server_monitor_delete(m_instance);
    m_instance = NULL;
  }
}
