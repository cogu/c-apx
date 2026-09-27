/*****************************************************************************
* \file      tls_server_extension.c
* \author    Conny Gustafsson
* \date      2026-09-27
* \brief     APX TLS server extension
*
* Copyright (c) 2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <stdio.h>
#include "apx/extension/tls_server_extension.h"
#include "apx/extension/tls_server.h"
#include "apx/server.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define TCP_USER_PORT_BEGIN 1024
#define TCP_USER_PORT_END   65535

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_tls_server_extension_init(struct apx_server_tag *apx_server, dtl_dv_t *config);
static void apx_tls_server_extension_shutdown(void);
static apx_error_t apx_tls_server_extension_configure(apx_tls_server_t *server, dtl_hv_t *cfg);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
static apx_tls_server_t *m_instance = NULL;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_tls_server_extension_register(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
   apx_server_extension_handler_t handler = {apx_tls_server_extension_init, apx_tls_server_extension_shutdown};
   return apx_server_add_extension(apx_server, "TLS", &handler, config);
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_tls_server_extension_init(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
   if (m_instance == NULL)
   {
      m_instance = apx_tls_server_new(apx_server);
      if (m_instance == NULL)
      {
         return APX_MEM_ERROR;
      }
      if (config != NULL)
      {
         if (dtl_dv_type(config) == DTL_DV_HASH)
         {
            return apx_tls_server_extension_configure(m_instance, (dtl_hv_t *)config);
         }
         else
         {
            return APX_VALUE_TYPE_ERROR;
         }
      }
   }
   return APX_NO_ERROR;
}

static void apx_tls_server_extension_shutdown(void)
{
   if (m_instance != NULL)
   {
      apx_tls_server_stop(m_instance);
      apx_tls_server_delete(m_instance);
      m_instance = NULL;
   }
}

static apx_error_t apx_tls_server_extension_configure(apx_tls_server_t *server, dtl_hv_t *cfg)
{
   dtl_sv_t *sv_tcp_port = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "tcp-port");
   if (sv_tcp_port == NULL)
   {
      sv_tcp_port = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "port");
   }

   dtl_sv_t *sv_server_cert = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "server-cert");
   if (sv_server_cert == NULL)
   {
      sv_server_cert = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "certificate");
   }

   dtl_sv_t *sv_server_key = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "server-key");
   if (sv_server_key == NULL)
   {
      sv_server_key = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "key");
   }

   dtl_sv_t *sv_ca_cert = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "ca-cert");
   dtl_sv_t *sv_require_client_cert = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "require-client-cert");
   dtl_sv_t *sv_tcp_tag = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "tag");
   if (sv_tcp_tag == NULL)
   {
      sv_tcp_tag = (dtl_sv_t *)dtl_hv_get_cstr(cfg, "tcp-tag");
   }

   bool conversion_ok = false;
   if (sv_tcp_port != NULL && sv_server_cert != NULL && sv_server_key != NULL)
   {
      uint16_t tcp_port = (uint16_t)dtl_sv_to_u32(sv_tcp_port, &conversion_ok);
      if (!conversion_ok || tcp_port < TCP_USER_PORT_BEGIN || tcp_port > TCP_USER_PORT_END)
      {
         return APX_INVALID_ARGUMENT_ERROR;
      }

      const char *server_cert = dtl_sv_to_cstr(sv_server_cert, &conversion_ok);
      if (!conversion_ok || server_cert == NULL || *server_cert == '\0')
      {
         return APX_INVALID_ARGUMENT_ERROR;
      }

      const char *server_key = dtl_sv_to_cstr(sv_server_key, &conversion_ok);
      if (!conversion_ok || server_key == NULL || *server_key == '\0')
      {
         return APX_INVALID_ARGUMENT_ERROR;
      }

      msocket_tls_config_t tls_config;
      msocket_tls_config_create(&tls_config);

      msocket_error_t m_rc = msocket_tls_config_set_server_cert(&tls_config, server_cert, server_key);
      if (m_rc != MSOCKET_NO_ERROR)
      {
         msocket_tls_config_destroy(&tls_config);
         return APX_INVALID_ARGUMENT_ERROR;
      }

      if (sv_ca_cert != NULL)
      {
         const char *ca_cert = dtl_sv_to_cstr(sv_ca_cert, &conversion_ok);
         if (conversion_ok && ca_cert != NULL && *ca_cert != '\0')
         {
            msocket_tls_config_set_ca_cert(&tls_config, ca_cert);
         }
      }

      if (sv_require_client_cert != NULL)
      {
         bool require = dtl_sv_to_bool(sv_require_client_cert, &conversion_ok);
         if (conversion_ok)
         {
            msocket_tls_config_set_require_client_cert(&tls_config, require);
         }
      }

      const char *tag = "TLS";
      if (sv_tcp_tag != NULL)
      {
         const char *tag_str = dtl_sv_to_cstr(sv_tcp_tag, &conversion_ok);
         if (conversion_ok && tag_str != NULL && *tag_str != '\0')
         {
            tag = tag_str;
         }
      }

      apx_error_t rc = apx_tls_server_start(server, tcp_port, &tls_config, tag);
      msocket_tls_config_destroy(&tls_config);
      return rc;
   }

   return APX_NO_ERROR;
}
