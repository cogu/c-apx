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
apx_serverTextLog_t *m_instance = NULL;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_serverTextLogExtension_register(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
   apx_serverExtensionHandler_t handler = {apx_serverTextLogExtension_init, apx_serverTextLogExtension_shutdown};
   return apx_server_add_extension(apx_server, "TEXTLOG", &handler, config);
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_serverTextLogExtension_init(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
   if (m_instance == NULL)
   {
      m_instance = apx_serverTextLog_new(apx_server);
      if (m_instance == NULL)
      {
         return APX_MEM_ERROR;
      }
      if (config != NULL)
      {
         if (dtl_dv_type(config) == DTL_DV_HASH)
         {
            return apx_serverTextLogExtension_configure(m_instance, (dtl_hv_t*) config);
         }
         else
         {
            return APX_VALUE_TYPE_ERROR;
         }
      }
   }
   return APX_NO_ERROR;
}

void apx_serverTextLogExtension_shutdown(void)
{
   if (m_instance != NULL)
   {
      apx_serverTextLog_closeAll(m_instance);
      apx_serverTextLog_delete(m_instance);
      m_instance = NULL;
   }
}

static apx_error_t apx_serverTextLogExtension_configure(apx_serverTextLog_t *instance, dtl_hv_t *cfg)
{
   dtl_sv_t *svFileEnabled;
   dtl_sv_t *svFilePath;
   bool ok = false;
   svFileEnabled = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "file-enabled");
   svFilePath = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "file-path");
   (void)instance;
   if ( (svFileEnabled != NULL) && (dtl_sv_to_bool(svFileEnabled, &ok) != false) )
   {
      if (svFilePath != NULL)
      {
         const char *filePath = dtl_sv_to_cstr(svFilePath, &ok);
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

