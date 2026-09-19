/*****************************************************************************
* \file      observer_extension.c
* \author    Conny Gustafsson
* \date      2021-02-18
* \brief     Monitor extension
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
static apx_serverMonitor_t* m_instance = NULL;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_monitorExtension_register(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
   apx_serverExtensionHandler_t handler = {init, shutdown};
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
      m_instance = apx_serverMonitor_new(apx_server);
      if (m_instance == NULL)
      {
         return APX_MEM_ERROR;
      }
   }
   return APX_NO_ERROR;
}

static void shutdown(void)
{
   apx_serverMonitor_delete(m_instance);
   m_instance = NULL;
}

