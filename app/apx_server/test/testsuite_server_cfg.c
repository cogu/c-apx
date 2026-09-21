/*****************************************************************************
* \file      testsuite_server_cfg.c
* \author    Conny Gustafsson
* \date      2026-08-29
* \brief     Unit tests for server configuration
*
* Copyright (c) 2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(d) _mkdir(d)
#define RMDIR(d) _rmdir(d)
#else
#include <sys/stat.h>
#include <unistd.h>
#define MKDIR(d) mkdir(d, 0777)
#define RMDIR(d) rmdir(d)
#endif
#include "CuTest.h"
#include "server_cfg.h"
#include "extensions_cfg.h"
#include "apx/server.h"
#include "dtl_type.h"
#include "apx/error.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_load_config_invalid_arguments(CuTest *tc);
static void test_load_config_nonexistent_path(CuTest *tc);
static void test_load_config_from_file_valid(CuTest *tc);
static void test_load_config_from_dir_valid(CuTest *tc);
static void test_load_config_from_dir_fallback(CuTest *tc);
static void test_load_config_from_dir_not_found(CuTest *tc);
static void test_load_config_malformed_json(CuTest *tc);
static void test_load_config_invalid_root_type(CuTest *tc);
static void test_load_config_missing_server_key(CuTest *tc);
static void test_register_extensions_with_single_config(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_server_cfg(void)
{
   CuSuite* suite = CuSuiteNew();
   SUITE_ADD_TEST(suite, test_load_config_invalid_arguments);
   SUITE_ADD_TEST(suite, test_load_config_nonexistent_path);
   SUITE_ADD_TEST(suite, test_load_config_from_file_valid);
   SUITE_ADD_TEST(suite, test_load_config_from_dir_valid);
   SUITE_ADD_TEST(suite, test_load_config_from_dir_fallback);
   SUITE_ADD_TEST(suite, test_load_config_from_dir_not_found);
   SUITE_ADD_TEST(suite, test_load_config_malformed_json);
   SUITE_ADD_TEST(suite, test_load_config_invalid_root_type);
   SUITE_ADD_TEST(suite, test_load_config_missing_server_key);
   SUITE_ADD_TEST(suite, test_register_extensions_with_single_config);
   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void write_test_file(const char *path, const char *content)
{
   FILE *fh = fopen(path, "w");
   if (fh != NULL)
   {
      fputs(content, fh);
      fclose(fh);
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_load_config_invalid_arguments(CuTest *tc)
{
   dtl_hv_t *cfg = NULL;

   CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_server_load_config(NULL, &cfg));
   CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_server_load_config("test.json", NULL));
}

static void test_load_config_nonexistent_path(CuTest *tc)
{
   dtl_hv_t *cfg = NULL;

   apx_error_t result = apx_server_load_config("non_existent_config_file_987654.json", &cfg);
   CuAssertIntEquals(tc, APX_FILE_NOT_FOUND_ERROR, result);
   CuAssertPtrEquals(tc, NULL, cfg);
}

static void test_load_config_from_file_valid(CuTest *tc)
{
   const char *filepath = "test_single_server.json";
   const char *content =
      "{\n"
      "    \"apx-server\": {\n"
      "        \"shutdown-timer\": 10,\n"
      "        \"max-num-events\": 200\n"
      "    },\n"
      "    \"socket-server-extension\": {\n"
      "        \"enabled\": true,\n"
      "        \"tcp-port\": 5000\n"
      "    },\n"
      "    \"textlog-extension\": {\n"
      "        \"enabled\": true\n"
      "    }\n"
      "}\n";
   write_test_file(filepath, content);

   dtl_hv_t *cfg = NULL;
   apx_error_t result = apx_server_load_config(filepath, &cfg);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, cfg);

   dtl_dv_t *server_node = dtl_hv_get_cstr(cfg, "apx-server");
   CuAssertPtrNotNull(tc, server_node);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type(server_node));
   dtl_sv_t *sv = (dtl_sv_t*) dtl_hv_get_cstr((dtl_hv_t*) server_node, "shutdown-timer");
   CuAssertPtrNotNull(tc, sv);
   bool ok = false;
   CuAssertIntEquals(tc, 10, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dv_t *ext_node = dtl_hv_get_cstr(cfg, "socket-server-extension");
   CuAssertPtrNotNull(tc, ext_node);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type(ext_node));

   dtl_dec_ref(cfg);
   remove(filepath);
}

static void test_load_config_from_dir_valid(CuTest *tc)
{
   const char *dirname = "test_dir_valid";
   MKDIR(dirname);

   char filepath[128];
   snprintf(filepath, sizeof(filepath), "%s/server.json", dirname);
   const char *content =
      "{\n"
      "    \"apx-server\": {\n"
      "        \"shutdown-timer\": 20\n"
      "    },\n"
      "    \"socket-server-extension\": {\n"
      "        \"enabled\": true\n"
      "    }\n"
      "}\n";
   write_test_file(filepath, content);

   dtl_hv_t *cfg = NULL;
   apx_error_t result = apx_server_load_config(dirname, &cfg);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, cfg);

   dtl_dv_t *server_node = dtl_hv_get_cstr(cfg, "apx-server");
   CuAssertPtrNotNull(tc, server_node);
   dtl_sv_t *sv = (dtl_sv_t*) dtl_hv_get_cstr((dtl_hv_t*) server_node, "shutdown-timer");
   CuAssertPtrNotNull(tc, sv);
   bool ok = false;
   CuAssertIntEquals(tc, 20, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dec_ref(cfg);

   remove(filepath);
   RMDIR(dirname);
}

static void test_load_config_from_dir_fallback(CuTest *tc)
{
   const char *dirname = "test_dir_fallback";
   MKDIR(dirname);

   char filepath[128];
   snprintf(filepath, sizeof(filepath), "%s/apx_server.json", dirname);
   const char *content =
      "{\n"
      "    \"apx-server\": {\n"
      "        \"shutdown-timer\": 30\n"
      "    }\n"
      "}\n";
   write_test_file(filepath, content);

   dtl_hv_t *cfg = NULL;
   apx_error_t result = apx_server_load_config(dirname, &cfg);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, cfg);

   dtl_dv_t *server_node = dtl_hv_get_cstr(cfg, "apx-server");
   CuAssertPtrNotNull(tc, server_node);
   dtl_sv_t *sv = (dtl_sv_t*) dtl_hv_get_cstr((dtl_hv_t*) server_node, "shutdown-timer");
   CuAssertPtrNotNull(tc, sv);
   bool ok = false;
   CuAssertIntEquals(tc, 30, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dec_ref(cfg);

   remove(filepath);
   RMDIR(dirname);
}

static void test_load_config_from_dir_not_found(CuTest *tc)
{
   const char *dirname = "test_dir_not_found";
   MKDIR(dirname);

   dtl_hv_t *cfg = NULL;
   apx_error_t result = apx_server_load_config(dirname, &cfg);
   CuAssertIntEquals(tc, APX_FILE_NOT_FOUND_ERROR, result);
   CuAssertPtrEquals(tc, NULL, cfg);

   RMDIR(dirname);
}

static void test_load_config_malformed_json(CuTest *tc)
{
   const char *filepath = "test_malformed.json";
   write_test_file(filepath, "{ malformed json ");

   dtl_hv_t *cfg = NULL;
   apx_error_t result = apx_server_load_config(filepath, &cfg);
   CuAssertIntEquals(tc, APX_PARSE_ERROR, result);
   CuAssertPtrEquals(tc, NULL, cfg);

   remove(filepath);
}

static void test_load_config_invalid_root_type(CuTest *tc)
{
   const char *filepath = "test_bad_root.json";
   write_test_file(filepath, "[1, 2, 3]");

   dtl_hv_t *cfg = NULL;
   apx_error_t result = apx_server_load_config(filepath, &cfg);
   CuAssertIntEquals(tc, APX_VALUE_TYPE_ERROR, result);
   CuAssertPtrEquals(tc, NULL, cfg);

   remove(filepath);
}

static void test_load_config_missing_server_key(CuTest *tc)
{
   const char *filepath = "test_no_server.json";
   write_test_file(filepath, "{\"socket-server-extension\": {\"enabled\": true}}");

   dtl_hv_t *cfg = NULL;
   apx_error_t result = apx_server_load_config(filepath, &cfg);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, cfg);
   dtl_dv_t *server_node = dtl_hv_get_cstr(cfg, "apx-server");
   CuAssertPtrEquals(tc, NULL, server_node);

   dtl_dec_ref(cfg);

   remove(filepath);
}

static void test_register_extensions_with_single_config(CuTest *tc)
{
   const char *filepath = "test_ext_dispatch.json";
   const char *content =
      "{\n"
      "    \"apx-server\": {\n"
      "        \"shutdown-timer\": 0\n"
      "    },\n"
      "    \"socket-server-extension\": {\n"
      "        \"enabled\": true\n"
      "    },\n"
      "    \"monitor-extension\": {\n"
      "        \"enabled\": false\n"
      "    }\n"
      "}\n";
   write_test_file(filepath, content);

   dtl_hv_t *cfg = NULL;
   apx_error_t result = apx_server_load_config(filepath, &cfg);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);

   apx_server_t server;
   apx_server_create(&server);

   result = register_apx_server_extensions(&server, cfg);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);

   apx_server_destroy(&server);

   dtl_dec_ref(cfg);

   remove(filepath);
}
