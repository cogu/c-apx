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
#include <string.h>
#include "apx/extension/monitor_extension.h"
#include "apx/extension/server_monitor.h"
#include "apx/server.h"

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
static apx_server_monitor_t* m_instance = NULL;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_monitor_extension_register(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
   apx_server_extension_handler_t handler = {init, shutdown};
   return apx_server_add_extension(apx_server, "MONITOR", &handler, config);
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static apx_error_t init(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
   (void)config;
   if (m_instance == NULL)
   {
      m_instance = apx_server_monitor_new(apx_server);
      if (m_instance == NULL)
      {
         return APX_MEM_ERROR;
      }
   }
   return APX_NO_ERROR;
}

static void shutdown(void)
{
   apx_server_monitor_delete(m_instance);
   m_instance = NULL;
}

