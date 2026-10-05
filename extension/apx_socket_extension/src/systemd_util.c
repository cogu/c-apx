/*****************************************************************************
 * \file      systemd_util.c
 * \author    Conny Gustafsson
 * \date      2026-09-30
 * \brief     Systemd socket activation utility functions
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#include "apx/extension/systemd_util.h"

#ifndef _WIN32
# include <errno.h>
# include <stdlib.h>
# include <unistd.h>

int apx_sd_listen_fds(int unset_environment)
{
  const char *e;
  int n;
  pid_t pid;

  e = getenv("LISTEN_PID");
  if (e == NULL)
  {
    return 0;
  }

  errno = 0;
  char *end = NULL;
  long val = strtol(e, &end, 10);
  if (errno != 0 || end == e || *end != '\0' || val <= 0)
  {
    return 0;
  }
  pid = (pid_t)val;
  if (getpid() != pid)
  {
    return 0;
  }

  e = getenv("LISTEN_FDS");
  if (e == NULL)
  {
    return 0;
  }

  errno = 0;
  end = NULL;
  val = strtol(e, &end, 10);
  if (errno != 0 || end == e || *end != '\0' || val <= 0)
  {
    return 0;
  }
  n = (int)val;

  if (unset_environment)
  {
    unsetenv("LISTEN_PID");
    unsetenv("LISTEN_FDS");
  }

  return n;
}
#else
int apx_sd_listen_fds(int unset_environment)
{
  (void)unset_environment;
  return 0;
}
#endif
