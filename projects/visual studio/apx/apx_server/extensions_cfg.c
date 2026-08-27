/*****************************************************************************
* \file      extensions_cfg.c
* \author    Conny Gustafsson
* \date      2026-08-28
* \brief     Extension configuration and static registry implementation (Visual Studio)
*
* Copyright (c) 2026 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
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
#include "extensions_cfg.h"
#include "apx/extension/socket_server_extension.h"
#include "apx/extension/server_text_log_extension.h"
#include "apx/extension/monitor_extension.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
static const apx_server_extension_entry_t m_extension_registry[] = {
   {"socket-server", apx_socketServerExtension_register},
   {"textlog", apx_serverTextLogExtension_register},
   {"monitor", apx_monitorExtension_register},
   {NULL, NULL}
};

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

const apx_server_extension_entry_t* apx_server_get_registered_extensions(void)
{
   return m_extension_registry;
}

apx_error_t register_apx_server_extensions(struct apx_server_tag *server, dtl_hv_t *extensions_config)
{
   for (const apx_server_extension_entry_t *entry = m_extension_registry; entry->name != NULL; ++entry)
   {
      dtl_dv_t *config = (extensions_config != NULL) ? dtl_hv_get_cstr(extensions_config, entry->name) : NULL;
      apx_error_t result = entry->register_fn(server, config);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
   }
   return APX_NO_ERROR;
}
