/*****************************************************************************
* \file      apx_socket_server_extension.c
* \author    Conny Gustafsson
* \date      2019-09-04
* \brief     APX socket server extension (TCP+UNIX)
*
* Copyright (c) 2019-2021 Conny Gustafsson
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
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
#include <Windows.h>
#endif
#include "apx/extension/socket_server_extension.h"
#include "apx/extension/socket_server.h"
#include "apx/server.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_socketServerExtension_init(struct apx_server_tag *apx_server, dtl_dv_t *config);
static void apx_socketServerExtension_shutdown(void);
static apx_error_t apx_socketServerExtension_configure(apx_socketServer_t *server, dtl_hv_t *cfg);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
static apx_socketServer_t *m_instance = (apx_socketServer_t*) 0; //singleton

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_socketServerExtension_register(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
   apx_serverExtensionHandler_t handler = {apx_socketServerExtension_init, apx_socketServerExtension_shutdown};
   return apx_server_add_extension(apx_server, "SOCKET", &handler, config);
}

#ifdef UNIT_TEST
void apx_socketServerExtension_accept_testsocket(testsocket_t *sock)
{
   if (m_instance != 0)
   {
      apx_socketServer_accept_testsocket(m_instance, sock);
   }
}
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_socketServerExtension_init(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
   if (m_instance == 0)
   {
      m_instance = apx_socketServer_new(apx_server);
      if (m_instance == 0)
      {
         return APX_MEM_ERROR;
      }
      if (config != 0)
      {
         if (dtl_dv_type(config) == DTL_DV_HASH)
         {
            return apx_socketServerExtension_configure(m_instance, (dtl_hv_t*) config);
         }
         else
         {
            return APX_VALUE_TYPE_ERROR;
         }
      }
   }
   return APX_NO_ERROR;
}

static void apx_socketServerExtension_shutdown(void)
{
   if (m_instance != 0)
   {
      apx_socketServer_stop_all(m_instance);
      apx_socketServer_delete(m_instance);
      m_instance = (apx_socketServer_t*) 0;
   }
}

static apx_error_t apx_socketServerExtension_configure(apx_socketServer_t *server, dtl_hv_t *cfg)
{
   dtl_sv_t *sv_tcp_port;
#ifndef _WIN32
   dtl_sv_t *sv_unix_file;
#endif
   dtl_sv_t *sv_tcp_tag;
   dtl_sv_t *sv_unix_tag;
   bool conversion_ok;

   (void)server;
   sv_tcp_port = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "tcp-port");
#ifndef _WIN32
   sv_unix_file = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "unix-file");
#endif
   sv_tcp_tag = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "tcp-tag");
   sv_unix_tag = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "unix-tag");
   if (sv_tcp_port != 0)
   {
      uint16_t tcp_port = (uint16_t) dtl_sv_to_u32(sv_tcp_port, &conversion_ok);
      if (conversion_ok && (tcp_port >=TCP_USER_PORT_BEGIN) && (tcp_port <= TCP_USER_PORT_END) )
      {
         const char *tag = "";
         if (sv_tcp_tag != NULL)
         {
            tag = dtl_sv_to_cstr(sv_tcp_tag, &conversion_ok);
         }
         apx_socketServer_start_tcp_server(m_instance, tcp_port, tag);
      }
   }
#ifndef _WIN32
   if (sv_unix_file != 0)
   {
      const char *unix_file_path = dtl_sv_to_cstr(sv_unix_file, &conversion_ok);
      if (conversion_ok)
      {
          if (strlen(unix_file_path) > 0)
          {
             const char *tag = "";
             if (sv_unix_tag != 0)
             {
                tag = dtl_sv_to_cstr(sv_unix_tag, &conversion_ok);
             }
             apx_socketServer_start_unix_server(m_instance, unix_file_path, tag);
          }
      }
   }
#endif
   return APX_NO_ERROR;
}

