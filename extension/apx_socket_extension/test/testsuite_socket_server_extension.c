/*****************************************************************************
 * \file      testsuite_socket_server_extension.c
 * \author    Conny Gustafsson
 * \date      2019-05-27
 * \brief     Unit tests for socket server extension
 *
 * Copyright (c) 2019-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "CuTest.h"
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
# include <unistd.h>
#endif
#include "apx/extension/socket_server_extension.h"
#include "apx/extension/systemd_util.h"
#include "apx/server.h"
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_extension_init_shutdown(CuTest *tc);
#ifndef _WIN32
static void test_sd_listen_fds(CuTest *tc);
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite *testsuite_apx_socket_server_extension(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_extension_init_shutdown);
#ifndef _WIN32
  SUITE_ADD_TEST(suite, test_sd_listen_fds);
#endif
  return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_extension_init_shutdown(CuTest *tc)
{
  apx_server_t apx_server;
  dtl_hv_t *extension_cfg = NULL;
  apx_server_create(&apx_server);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_socket_server_extension_register(&apx_server, (dtl_dv_t *)extension_cfg));
  apx_server_start(&apx_server);
  apx_server_run(&apx_server);
  apx_server_stop(&apx_server);
  apx_server_destroy(&apx_server);
}

#ifndef _WIN32
static void test_sd_listen_fds(CuTest *tc)
{
  unsetenv("LISTEN_PID");
  unsetenv("LISTEN_FDS");

   /* 1. Neither variable set -> 0 */
  CuAssertIntEquals(tc, 0, apx_sd_listen_fds(0));

   /* 2. PID mismatch -> 0 */
  setenv("LISTEN_PID", "999999", 1);
  setenv("LISTEN_FDS", "1", 1);
  CuAssertIntEquals(tc, 0, apx_sd_listen_fds(0));

   /* 3. PID matches, LISTEN_FDS = 3 -> 3 */
  char pid_str[32];
  snprintf(pid_str, sizeof(pid_str), "%d", (int)getpid());
  setenv("LISTEN_PID", pid_str, 1);
  setenv("LISTEN_FDS", "3", 1);
  CuAssertIntEquals(tc, 3, apx_sd_listen_fds(0));

   /* Variables should still be set when unset_environment == 0 */
  CuAssertPtrNotNull(tc, getenv("LISTEN_FDS"));

   /* 4. When unset_environment == 1, variables are cleaned up */
  CuAssertIntEquals(tc, 3, apx_sd_listen_fds(1));
  CuAssertPtrEquals(tc, NULL, getenv("LISTEN_PID"));
  CuAssertPtrEquals(tc, NULL, getenv("LISTEN_FDS"));

   /* 5. Non-numeric LISTEN_PID -> 0 */
  setenv("LISTEN_PID", "invalid", 1);
  setenv("LISTEN_FDS", "1", 1);
  CuAssertIntEquals(tc, 0, apx_sd_listen_fds(1));

   /* 6. Non-numeric LISTEN_FDS -> 0 */
  snprintf(pid_str, sizeof(pid_str), "%d", (int)getpid());
  setenv("LISTEN_PID", pid_str, 1);
  setenv("LISTEN_FDS", "invalid", 1);
  CuAssertIntEquals(tc, 0, apx_sd_listen_fds(1));

   /* 7. Negative or zero LISTEN_FDS -> 0 */
  setenv("LISTEN_PID", pid_str, 1);
  setenv("LISTEN_FDS", "-5", 1);
  CuAssertIntEquals(tc, 0, apx_sd_listen_fds(1));
  setenv("LISTEN_PID", pid_str, 1);
  setenv("LISTEN_FDS", "0", 1);
  CuAssertIntEquals(tc, 0, apx_sd_listen_fds(1));
}
#endif
