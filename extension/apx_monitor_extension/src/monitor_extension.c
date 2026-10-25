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
