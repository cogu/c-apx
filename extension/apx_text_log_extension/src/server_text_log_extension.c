/*****************************************************************************
 * \file      server_text_log_extension.c
 * \author    Conny Gustafsson
 * \date      2019-09-08
 * \brief     Text log extension for APX server
 *
 * Copyright (c) 2019-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/extension/server_text_log_extension.h"
#include "apx/extension/server_text_log.h"
#include "apx/server.h"
#include "apx/util.h"
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_server_text_log_extension_init(struct apx_server_tag *apx_server, dtl_dv_t *config);
void apx_server_text_log_extension_shutdown(void);
static apx_error_t apx_server_text_log_extension_configure(apx_server_text_log_t *instance, dtl_hv_t *cfg);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
apx_server_text_log_t *m_instance = NULL;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_server_text_log_extension_register(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
  apx_server_extension_handler_t handler = {apx_server_text_log_extension_init, apx_server_text_log_extension_shutdown};
  return apx_server_add_extension(apx_server, "TEXTLOG", &handler, config);
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_server_text_log_extension_init(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
  if (m_instance == NULL)
  {
    m_instance = apx_server_text_log_new(apx_server);
    if (m_instance == NULL)
    {
      return APX_MEM_ERROR;
    }
    if (config != NULL)
    {
      if (dtl_dv_type(config) == DTL_DV_HASH)
      {
        apx_error_t result = apx_server_text_log_extension_configure(m_instance, (dtl_hv_t *)config);
        if (result != APX_NO_ERROR)
        {
          apx_server_text_log_delete(m_instance);
          m_instance = NULL;
          return result;
        }
      }
      else
      {
        apx_server_text_log_delete(m_instance);
        m_instance = NULL;
        return APX_VALUE_TYPE_ERROR;
      }
    }
  }
  return APX_NO_ERROR;
}

void apx_server_text_log_extension_shutdown(void)
{
  if (m_instance != NULL)
  {
    apx_server_text_log_close_all(m_instance);
    apx_server_text_log_delete(m_instance);
    m_instance = NULL;
  }
}

static apx_error_t apx_server_text_log_extension_configure(apx_server_text_log_t *instance, dtl_hv_t *cfg)
{
  dtl_sv_t *svFileEnabled;
  dtl_sv_t *svFilePath;
  dtl_sv_t *svLogLevel;
  dtl_sv_t *svTimestamp;
  bool ok = false;

  if ((instance == NULL) || (cfg == NULL))
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  svLogLevel = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "log-level");
  if (svLogLevel == NULL)
  {
    svLogLevel = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "log_level");
  }
  if (svLogLevel != NULL)
  {
    if (dtl_sv_type(svLogLevel) == DTL_SV_STR)
    {
      const char *level_str = dtl_sv_to_cstr(svLogLevel, &ok);
      if (ok && (level_str != NULL))
      {
        apx_log_level_t level = apx_log_level_from_string(level_str);
        if (level != APX_LOG_LEVEL_INVALID)
        {
          apx_server_text_log_set_log_level(instance, level);
        }
        else
        {
          return APX_VALUE_RANGE_ERROR;
        }
      }
    }
    else
    {
      return APX_VALUE_TYPE_ERROR;
    }
  }
  else
  {
    apx_server_text_log_set_log_level(instance, APX_LOG_LEVEL_INFO);
  }

  svTimestamp = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "use-timestamp");
  if (svTimestamp == NULL)
  {
    svTimestamp = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "use_timestamp");
  }
  if (svTimestamp != NULL)
  {
    bool timestamp_enabled = dtl_sv_to_bool(svTimestamp, &ok);
    if (ok)
    {
      apx_server_text_log_set_timestamp_enabled(instance, timestamp_enabled);
    }
  }
  else
  {
    apx_server_text_log_set_timestamp_enabled(instance, false);
  }

  svFileEnabled = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "file-enabled");
  svFilePath = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "file-path");
  if ((svFileEnabled != NULL) && (dtl_sv_to_bool(svFileEnabled, &ok) != false))
  {
    if (svFilePath != NULL)
    {
      const char *filePath = dtl_sv_to_cstr(svFilePath, &ok);
      if (strlen(filePath) == 0u)
      {
        apx_text_log_base_enable_stdout(&instance->base);
      }
      else
      {
        apx_text_log_base_enable_file(&instance->base, filePath);
      }
    }
  }
  return APX_NO_ERROR;
}
