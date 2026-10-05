/*****************************************************************************
 * \file      socket_server.c
 * \author    Conny Gustafsson
 * \date      2019-09-07
 * \brief     Socket server for apx_server
 *
 * Copyright (c) 2019-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/extension/socket_server.h"
#include "apx/extension/socket_server_connection.h"
#include "apx/server.h"
#include "apx/types.h"
#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
# include <grp.h>
# include <pwd.h>
# include <sys/types.h>
# include <unistd.h>
#endif
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_server_info_tag
{
  uint8_t addressFamily;
} apx_server_info_t;

struct msocket_server_tag;

#ifdef UNIT_TEST
# define SOCKET_TYPE testsocket_t
# define SOCKET_DELETE testsocket_delete
# define SOCKET_START_IO(x)
# define SOCKET_SET_HANDLER testsocket_set_server_handler
#else
# define SOCKET_DELETE msocket_delete
# define SOCKET_TYPE msocket_t
# define SOCKET_START_IO(x) msocket_start_io(x)
# define SOCKET_SET_HANDLER msocket_sethandler
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_socket_server_tcp_accept(void *arg, struct msocket_server_tag *srv, void *sock);
#if !defined(UNIT_TEST) && !defined(_WIN32)
static void apx_socket_server_unix_accept(void *arg, struct msocket_server_tag *srv, void *sock);
static void apx_socket_server_vsock_accept(void *arg, struct msocket_server_tag *srv, void *sock);
static bool verify_peer_credentials(const apx_socket_server_t *self, msocket_t *sock);
static bool is_peer_in_allowed_groups(const apx_socket_server_t *self, const msocket_credentials_t *creds);
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_socket_server_create(apx_socket_server_t *self, struct apx_server_tag *apx_server)
{
  if (self != NULL)
  {
    self->parent = apx_server;
    self->tcp_port = 0u;
    self->unix_server_file = NULL;
    self->vsock_port = 0u;
    self->vsock_cid = 0u;
    self->is_tcp_server_started = false;
    self->is_unix_server_started = false;
    self->is_vsock_server_started = false;
    self->tcp_connection_tag = NULL;
    self->unix_connection_tag = NULL;
    self->vsock_connection_tag = NULL;
    adt_ary_create(&self->allowed_groups, free);
  }
}

void apx_socket_server_destroy(apx_socket_server_t *self)
{
  if (self != NULL)
  {
    if (self->unix_server_file != NULL)
    {
      free(self->unix_server_file);
    }
    if (self->tcp_connection_tag != NULL)
    {
      free(self->tcp_connection_tag);
    }
    if (self->unix_connection_tag != NULL)
    {
      free(self->unix_connection_tag);
    }
    if (self->vsock_connection_tag != NULL)
    {
      free(self->vsock_connection_tag);
    }
    adt_ary_destroy(&self->allowed_groups);
  }
}

apx_socket_server_t *apx_socket_server_new(struct apx_server_tag *apx_server)
{
  apx_socket_server_t *self = (apx_socket_server_t *)malloc(sizeof(apx_socket_server_t));
  if (self != NULL)
  {
    apx_socket_server_create(self, apx_server);
  }
  return self;
}

void apx_socket_server_delete(apx_socket_server_t *self)
{
  if (self != NULL)
  {
    apx_socket_server_destroy(self);
    free(self);
  }
}

void apx_socket_server_add_allowed_group(apx_socket_server_t *self, const char *group_name)
{
  if ((self != NULL) && (group_name != NULL))
  {
    adt_ary_push(&self->allowed_groups, STRDUP(group_name));
  }
}

void apx_socket_server_start_tcp_server(apx_socket_server_t *self, uint16_t tcp_port, const char *tag)
{
  if (self != NULL)
  {
      // char msg[80];
    msocket_handler_t server_handler;
    self->tcp_port = tcp_port;
    if (tag != NULL)
    {
      size_t length = strlen(tag);
      if (length > 0u)
      {
        self->tcp_connection_tag = STRDUP(tag);
      }
    }
    memset(&server_handler, 0, sizeof(server_handler));
#ifndef UNIT_TEST
    server_handler.tcp_accept = apx_socket_server_tcp_accept;
#endif
    msocket_server_create(&self->tcp_server, MSOCKET_ADDR_INET, NULL);
    msocket_server_disable_cleanup(&self->tcp_server); // we will use our own garbage collector
    msocket_server_sethandler(&self->tcp_server, &server_handler, self);
    msocket_server_start(&self->tcp_server, NULL, 0, self->tcp_port);
    self->is_tcp_server_started = true;
    printf("Listening on TCP port %d\n", (int)self->tcp_port);
      // sprintf(msg, "Listening on TCP port %d", (int) self->tcpPort);
      // apx_server_log_event(self->parent, APX_LOG_LEVEL_INFO, APX_SOCKET_SERVER_LABEL, &msg[0]);
  }
}

#if !defined(UNIT_TEST) && !defined(_WIN32)
void apx_socket_server_start_unix_server(apx_socket_server_t *self, const char *file_path, const char *tag)
{
  if ((self != NULL) && (file_path != NULL))
  {
      // char msg[APX_MAX_LOG_LEN];
    msocket_handler_t server_handler;
    self->unix_server_file = STRDUP(file_path);
    if (tag != NULL)
    {
      size_t length = strlen(tag);
      if (length > 0u)
      {
        self->unix_connection_tag = STRDUP(tag);
      }
    }
    memset(&server_handler, 0, sizeof(server_handler));
# ifndef UNIT_TEST
    server_handler.tcp_accept = apx_socket_server_unix_accept;
# endif
    msocket_server_create(&self->unix_server, MSOCKET_ADDR_UNIX, NULL);
    msocket_server_disable_cleanup(&self->unix_server); // we will use our own garbage collector
    msocket_server_sethandler(&self->unix_server, &server_handler, self);
    msocket_server_unix_start(&self->unix_server, self->unix_server_file);
    self->is_unix_server_started = true;
    printf("Listening on UNIX socket %s\n", self->unix_server_file);
//      sprintf(msg, "Listening on UNIX socket %s", self->unixServerFile);
//      apx_server_log_event(self->parent, APX_LOG_LEVEL_INFO, APX_SOCKET_SERVER_LABEL, &msg[0]);
  }
}

void apx_socket_server_start_unix_server_fd(apx_socket_server_t *self, int fd, const char *tag)
{
  if ((self != NULL) && (fd >= 0))
  {
    msocket_handler_t server_handler;
    if (tag != NULL)
    {
      size_t length = strlen(tag);
      if (length > 0u)
      {
        self->unix_connection_tag = STRDUP(tag);
      }
    }
    memset(&server_handler, 0, sizeof(server_handler));
# ifndef UNIT_TEST
    server_handler.tcp_accept = apx_socket_server_unix_accept;
# endif
    msocket_server_create(&self->unix_server, MSOCKET_ADDR_UNIX, NULL);
    msocket_server_disable_cleanup(&self->unix_server); // we will use our own garbage collector
    msocket_server_sethandler(&self->unix_server, &server_handler, self);
    msocket_server_unix_start_fd(&self->unix_server, fd);
    self->is_unix_server_started = true;
    printf("Listening on pre-bound UNIX socket (fd %d)\n", fd);
  }
}
void apx_socket_server_start_vsock_server(apx_socket_server_t *self, uint32_t cid, uint32_t port, const char *tag)
{
  if ((self != NULL) && (port != 0u))
  {
    msocket_handler_t server_handler;
    self->vsock_cid = cid;
    self->vsock_port = port;
    if (tag != NULL)
    {
      size_t length = strlen(tag);
      if (length > 0u)
      {
        self->vsock_connection_tag = STRDUP(tag);
      }
    }
    memset(&server_handler, 0, sizeof(server_handler));
    server_handler.tcp_accept = apx_socket_server_vsock_accept;
    msocket_server_create(&self->vsock_server, MSOCKET_ADDR_VSOCK, NULL);
    msocket_server_disable_cleanup(&self->vsock_server);
    msocket_server_sethandler(&self->vsock_server, &server_handler, self);
    msocket_server_vsock_start(&self->vsock_server, self->vsock_cid, self->vsock_port);
    self->is_vsock_server_started = true;
    printf("Listening on VSOCK port %u (CID %u)\n", self->vsock_port, self->vsock_cid);
  }
}

void apx_socket_server_stop_vsock_server(apx_socket_server_t *self)
{
  if ((self != NULL) && (self->is_vsock_server_started))
  {
    msocket_server_destroy(&self->vsock_server);
    self->is_vsock_server_started = false;
  }
}
#endif

void apx_socket_server_stop_all(apx_socket_server_t *self)
{
  if (self != NULL)
  {
    apx_socket_server_stop_tcp_server(self);
#if !defined(UNIT_TEST) && !defined(_WIN32)
    apx_socket_server_stop_unix_server(self);
    apx_socket_server_stop_vsock_server(self);
#endif
  }
}

void apx_socket_server_stop_tcp_server(apx_socket_server_t *self)
{
  if ((self != NULL) && (self->is_tcp_server_started))
  {
    msocket_server_destroy(&self->tcp_server);
    self->is_tcp_server_started = false;
  }
}

#if !defined(UNIT_TEST) && !defined(_WIN32)
void apx_socket_server_stop_unix_server(apx_socket_server_t *self)
{
  if ((self != NULL) && (self->is_unix_server_started))
  {
# ifndef _MSC_VER
    msocket_server_destroy(&self->unix_server);
# endif
    self->is_unix_server_started = false;
  }
}
#endif

#ifdef UNIT_TEST
void apx_socket_server_accept_testsocket(apx_socket_server_t *self, testsocket_t *sock)
{
  apx_socket_server_tcp_accept((void *)self, (struct msocket_server_tag *)0, sock);
}
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_socket_server_tcp_accept(void *arg, struct msocket_server_tag *srv, void *sock)
{
  apx_socket_server_t *self = (apx_socket_server_t *)arg;
  (void)srv;
#if APX_DEBUG_ENABLE
  printf("[SOCKET-SERVER] New TCP connection\n");
#endif
  if (self != NULL)
  {
    apx_socket_server_connection_t *new_connection = apx_socket_server_connection_new(sock);
    if (new_connection != NULL)
    {
      if (self->tcp_connection_tag != NULL)
      {
        apx_socket_server_connection_set_tag(new_connection, self->tcp_connection_tag);
      }
      apx_server_accept_connection(self->parent, (apx_server_connection_t *)new_connection);
    }
    else
    {
      msocket_delete((msocket_t *)sock);
    }
  }
}

#if !defined(UNIT_TEST) && !defined(_WIN32)
static bool is_peer_in_allowed_groups(const apx_socket_server_t *self, const msocket_credentials_t *creds)
{
  int32_t num_groups;
  if (self == NULL || creds == NULL)
  {
    return false;
  }
  num_groups = adt_ary_length(&self->allowed_groups);
  if (num_groups == 0)
  {
    return true;
  }

  struct passwd *pw = getpwuid((uid_t)creds->uid);

  for (int32_t i = 0; i < num_groups; ++i)
  {
    const char *allowed_group_name = (const char *)adt_ary_value(&self->allowed_groups, i);
    if (allowed_group_name == NULL)
    {
      continue;
    }
    struct group *grp = getgrnam(allowed_group_name);
    if (grp == NULL)
    {
      continue;
    }

      /* 1. Direct GID match: peer effective GID matches allowed group GID */
    if ((gid_t)creds->gid == grp->gr_gid)
    {
      return true;
    }

      /* 2. User primary GID match */
    if (pw != NULL && pw->pw_gid == grp->gr_gid)
    {
      return true;
    }

      /* 3. Group membership check via gr_mem */
    if (pw != NULL && grp->gr_mem != NULL)
    {
      for (char **mem = grp->gr_mem; *mem != NULL; ++mem)
      {
        if (strcmp(*mem, pw->pw_name) == 0)
        {
          return true;
        }
      }
    }

      /* 4. Supplementary group membership via getgrouplist */
    if (pw != NULL)
    {
      int ngroups = 64;
      gid_t groups[64];
      if (getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups) >= 0)
      {
        for (int j = 0; j < ngroups; ++j)
        {
          if (groups[j] == grp->gr_gid)
          {
            return true;
          }
        }
      }
    }
  }

  return false;
}

static bool verify_peer_credentials(const apx_socket_server_t *self, msocket_t *sock)
{
  msocket_credentials_t creds;
  msocket_error_t cred_rc = msocket_get_peer_credentials(sock, &creds);
  if (adt_ary_length(&self->allowed_groups) > 0)
  {
    if ((cred_rc != MSOCKET_NO_ERROR) || !is_peer_in_allowed_groups(self, &creds))
    {
      if (cred_rc == MSOCKET_NO_ERROR)
      {
        fprintf(stderr, "[SOCKET-SERVER] Access denied: client (pid=%d, uid=%d, gid=%d) is not in allowed groups\n",
          creds.pid, creds.uid, creds.gid);
      }
      else
      {
        fprintf(stderr, "[SOCKET-SERVER] Access denied: unable to verify peer credentials\n");
      }
      return false;
    }
  }
  return true;
}

static void apx_socket_server_unix_accept(void *arg, struct msocket_server_tag *srv, void *sock)
{
  apx_socket_server_t *self = (apx_socket_server_t *)arg;
  (void)srv;
  if (self != NULL)
  {
    if (!verify_peer_credentials(self, (msocket_t *)sock))
    {
      msocket_close((msocket_t *)sock);
      msocket_delete((msocket_t *)sock);
      return;
    }

    apx_socket_server_connection_t *new_connection = apx_socket_server_connection_new(sock);
    if (new_connection != NULL)
    {
      if (self->unix_connection_tag != NULL)
      {
        apx_socket_server_connection_set_tag(new_connection, self->unix_connection_tag);
      }
      apx_server_accept_connection(self->parent, (apx_server_connection_t *)new_connection);
    }
    else
    {
      msocket_delete((msocket_t *)sock);
    }
  }
}

static void apx_socket_server_vsock_accept(void *arg, struct msocket_server_tag *srv, void *sock)
{
  apx_socket_server_t *self = (apx_socket_server_t *)arg;
  (void)srv;
# if APX_DEBUG_ENABLE
  printf("[SOCKET-SERVER] New VSOCK connection\n");
# endif
  if (self != NULL)
  {
    apx_socket_server_connection_t *new_connection = apx_socket_server_connection_new(sock);
    if (new_connection != NULL)
    {
      if (self->vsock_connection_tag != NULL)
      {
        apx_socket_server_connection_set_tag(new_connection, self->vsock_connection_tag);
      }
      apx_server_accept_connection(self->parent, (apx_server_connection_t *)new_connection);
    }
    else
    {
      msocket_delete((msocket_t *)sock);
    }
  }
}
#endif
