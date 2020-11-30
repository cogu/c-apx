/*****************************************************************************
* \file      server_text_event_log_extension.c
* \author    Conny Gustafsson
* \date      2019-09-08
* \brief     Text log extension for APX server
*
* Copyright (c) 2019 Conny Gustafsson
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
#include "apx/extension/server_text_log_extension.h"
#include "apx/extension/server_text_log.h"
#include "apx/server.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_serverTextLogExtension_init(struct apx_server_tag *apx_server, dtl_dv_t *config);
void apx_serverTextLogExtension_shutdown(void);
static apx_error_t apx_serverTextLogExtension_configure(apx_serverTextLog_t *instance, dtl_hv_t *cfg);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
apx_serverTextLog_t *m_instance = (apx_serverTextLog_t*) 0;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_serverTextLogExtension_register(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
   if ( (config != 0) && (dtl_dv_type(config) == DTL_DV_HASH))
   {
      dtl_sv_t *extensionEnabled;
      dtl_hv_t *cfg = (dtl_hv_t*) config;
      extensionEnabled = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "extension-enabled");
      if ( (extensionEnabled != 0) && (dtl_sv_to_bool(extensionEnabled)))
      {
         apx_serverExtensionHandler_t handler = {apx_serverTextLogExtension_init, apx_serverTextLogExtension_shutdown};
         return apx_server_addExtension(apx_server, "TEXTLOG", &handler, config);
      }
   }
   return APX_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_serverTextLogExtension_init(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
   if (m_instance == 0)
   {
      m_instance = apx_serverTextLog_new(apx_server);
      if (m_instance == 0)
      {
         return APX_MEM_ERROR;
      }
      if (config != 0)
      {
         if (dtl_dv_type(config) == DTL_DV_HASH)
         {
            return apx_serverTextLogExtension_configure(m_instance, (dtl_hv_t*) config);
         }
         else
         {
            return APX_DV_TYPE_ERROR;
         }
      }
   }
   return APX_NO_ERROR;
}

void apx_serverTextLogExtension_shutdown(void)
{
   if (m_instance != 0)
   {
      apx_serverTextLog_closeAll(m_instance);
      apx_serverTextLog_delete(m_instance);
      m_instance = (apx_serverTextLog_t*) 0;
   }
}

static apx_error_t apx_serverTextLogExtension_configure(apx_serverTextLog_t *instance, dtl_hv_t *cfg)
{
   dtl_sv_t *svFileEnabled;
   dtl_sv_t *svFilePath;
   svFileEnabled = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "file-enabled");
   svFilePath = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "file-path");
   if ( (svFileEnabled != 0) && (dtl_sv_to_bool(svFileEnabled) != false) )
   {
      if (svFilePath != 0)
      {
         const char *filePath = dtl_sv_to_cstr(svFilePath);
         if (strlen(filePath) == 0u)
         {
            apx_textLogBase_enableStdout(&m_instance->base);
         }
         else
         {
            apx_textLogBase_enableFile(&m_instance->base, filePath);
         }
      }
   }
   return APX_NO_ERROR;
}

