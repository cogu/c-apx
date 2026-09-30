/*****************************************************************************
* \file      socket_server_extension.c
* \author    Conny Gustafsson
* \date      2019-09-04
* \brief     APX socket server extension (TCP+UNIX)
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
#include <string.h>
#include <stdio.h>
#include "apx/extension/socket_server_extension.h"
#include "apx/extension/socket_server.h"
#include "apx/extension/systemd_util.h"
#include "apx/server.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define SYSTEMD_MODE_OFF  0
#define SYSTEMD_MODE_ON   1
#define SYSTEMD_MODE_AUTO 2


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_socket_server_extension_init(struct apx_server_tag *apx_server, dtl_dv_t *config);
static void apx_socket_server_extension_shutdown(void);
static apx_error_t apx_socket_server_extension_configure(apx_socket_server_t *server, dtl_hv_t *cfg);
static apx_error_t configure_tcp(apx_socket_server_t *server, dtl_hv_t *cfg);
#ifndef _WIN32
static apx_error_t configure_unix(apx_socket_server_t *server, dtl_hv_t *cfg);
static apx_error_t configure_vsock(apx_socket_server_t *server, dtl_hv_t *cfg);
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
static apx_socket_server_t *m_instance = NULL; //singleton

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_socket_server_extension_register(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
   apx_server_extension_handler_t handler = {apx_socket_server_extension_init, apx_socket_server_extension_shutdown};
   return apx_server_add_extension(apx_server, "SOCKET", &handler, config);
}

#ifdef UNIT_TEST
void apx_socket_server_extension_accept_testsocket(testsocket_t *sock)
{
   if (m_instance != NULL)
   {
      apx_socket_server_accept_testsocket(m_instance, sock);
   }
}
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_socket_server_extension_init(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
   if (m_instance == NULL)
   {
      m_instance = apx_socket_server_new(apx_server);
      if (m_instance == NULL)
      {
         return APX_MEM_ERROR;
      }
      if (config != NULL)
      {
         if (dtl_dv_type(config) == DTL_DV_HASH)
         {
            return apx_socket_server_extension_configure(m_instance, (dtl_hv_t*) config);
         }
         else
         {
            return APX_VALUE_TYPE_ERROR;
         }
      }
   }
   return APX_NO_ERROR;
}

static void apx_socket_server_extension_shutdown(void)
{
   if (m_instance != NULL)
   {
      apx_socket_server_stop_all(m_instance);
      apx_socket_server_delete(m_instance);
      m_instance = NULL;
   }
}

static apx_error_t apx_socket_server_extension_configure(apx_socket_server_t *server, dtl_hv_t *cfg)
{
   apx_error_t result = configure_tcp(server, cfg);
   if (result != APX_NO_ERROR)
   {
      return result;
   }
#ifndef _WIN32
   result = configure_unix(server, cfg);
   if (result != APX_NO_ERROR)
   {
      return result;
   }
   result = configure_vsock(server, cfg);
   if (result != APX_NO_ERROR)
   {
      return result;
   }
#endif
   return APX_NO_ERROR;
}

static apx_error_t configure_tcp(apx_socket_server_t *server, dtl_hv_t *cfg)
{
   dtl_sv_t *sv_tcp_port = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "tcp-port");
   if (sv_tcp_port != NULL)
   {
      bool conversion_ok;
      uint16_t tcp_port = (uint16_t) dtl_sv_to_u32(sv_tcp_port, &conversion_ok);
      if (conversion_ok && (tcp_port >= TCP_USER_PORT_BEGIN) && (tcp_port <= TCP_USER_PORT_END))
      {
         const char *tag = "";
         dtl_sv_t *sv_tcp_tag = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "tcp-tag");
         if (sv_tcp_tag != NULL)
         {
            tag = dtl_sv_to_cstr(sv_tcp_tag, &conversion_ok);
         }
         apx_socket_server_start_tcp_server(server, tcp_port, tag);
      }
   }
   return APX_NO_ERROR;
}

#ifndef _WIN32
static apx_error_t configure_unix(apx_socket_server_t *server, dtl_hv_t *cfg)
{
   bool conversion_ok;
   dtl_sv_t *sv_unix_file = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "unix-file");
   dtl_sv_t *sv_unix_tag = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "unix-tag");
   dtl_sv_t *sv_unix_systemd = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "unix-systemd");

#ifdef UNIT_TEST
   (void)server;
#endif

   if (sv_unix_systemd == NULL)
   {
      sv_unix_systemd = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "systemd");
   }
   int systemd_mode = SYSTEMD_MODE_OFF;
   if (sv_unix_systemd != NULL)
   {
      if (dtl_sv_type(sv_unix_systemd) == DTL_SV_STR)
      {
         const char *mode_str = dtl_sv_to_cstr(sv_unix_systemd, &conversion_ok);
         if (conversion_ok && (mode_str != NULL))
         {
            if (strcmp(mode_str, "auto") == 0)
            {
               systemd_mode = SYSTEMD_MODE_AUTO;
            }
            else if ( (strcmp(mode_str, "true") == 0) || (strcmp(mode_str, "1") == 0) )
            {
               systemd_mode = SYSTEMD_MODE_ON;
            }
            else if ( (strcmp(mode_str, "false") == 0) || (strcmp(mode_str, "0") == 0) )
            {
               systemd_mode = SYSTEMD_MODE_OFF;
            }
         }
      }
      else
      {
         bool b = dtl_sv_to_bool(sv_unix_systemd, &conversion_ok);
         if (conversion_ok && b)
         {
            systemd_mode = SYSTEMD_MODE_ON;
         }
      }
   }

   const char *unix_tag = "";
   if (sv_unix_tag != NULL)
   {
      unix_tag = dtl_sv_to_cstr(sv_unix_tag, &conversion_ok);
   }

   if (systemd_mode == SYSTEMD_MODE_ON)
   {
      int num_fds = apx_sd_listen_fds(0);
      if (num_fds < 1)
      {
         fprintf(stderr, "[SOCKET-SERVER] Systemd socket activation requested, but no file descriptors inherited\n");
         return APX_NOT_FOUND_ERROR;
      }
#ifndef UNIT_TEST
      apx_socket_server_start_unix_server_fd(server, SD_LISTEN_FDS_START, unix_tag);
#else
      (void)unix_tag;
#endif
   }
   else if (systemd_mode == SYSTEMD_MODE_AUTO)
   {
      int num_fds = apx_sd_listen_fds(0);
      if (num_fds >= 1)
      {
#ifndef UNIT_TEST
         apx_socket_server_start_unix_server_fd(server, SD_LISTEN_FDS_START, unix_tag);
#else
         (void)unix_tag;
#endif
      }
      else if (sv_unix_file != NULL)
      {
         const char *unix_file_path = dtl_sv_to_cstr(sv_unix_file, &conversion_ok);
         if (conversion_ok && (strlen(unix_file_path) > 0))
         {
#ifndef UNIT_TEST
            apx_socket_server_start_unix_server(server, unix_file_path, unix_tag);
#else
            (void)unix_tag;
#endif
         }
      }
   }
   else // SYSTEMD_MODE_OFF
   {
      if (sv_unix_file != NULL)
      {
         const char *unix_file_path = dtl_sv_to_cstr(sv_unix_file, &conversion_ok);
         if (conversion_ok && (strlen(unix_file_path) > 0))
         {
#ifndef UNIT_TEST
            apx_socket_server_start_unix_server(server, unix_file_path, unix_tag);
#else
            (void)unix_tag;
#endif
         }
      }
   }

   return APX_NO_ERROR;
}

static apx_error_t configure_vsock(apx_socket_server_t *server, dtl_hv_t *cfg)
{
   bool conversion_ok;
   dtl_sv_t *sv_vsock_port = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "vsock-port");
   dtl_sv_t *sv_vsock_cid = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "vsock-cid");
   dtl_sv_t *sv_vsock_tag = (dtl_sv_t*) dtl_hv_get_cstr(cfg, "vsock-tag");

#ifdef UNIT_TEST
   (void)server;
#endif

   if (sv_vsock_port != NULL)
   {
      uint32_t vsock_port = dtl_sv_to_u32(sv_vsock_port, &conversion_ok);
      if (conversion_ok && (vsock_port > 0))
      {
         uint32_t vsock_cid = MSOCKET_VMADDR_CID_ANY;
         if (sv_vsock_cid != NULL)
         {
            uint32_t parsed_cid = dtl_sv_to_u32(sv_vsock_cid, &conversion_ok);
            if (conversion_ok)
            {
               vsock_cid = parsed_cid;
            }
         }
         const char *vsock_tag = "";
         if (sv_vsock_tag != NULL)
         {
            vsock_tag = dtl_sv_to_cstr(sv_vsock_tag, &conversion_ok);
         }
#ifndef UNIT_TEST
         apx_socket_server_start_vsock_server(server, vsock_cid, vsock_port, vsock_tag);
#else
         (void)vsock_tag;
         (void)vsock_cid;
#endif
      }
   }
   return APX_NO_ERROR;
}
#endif

