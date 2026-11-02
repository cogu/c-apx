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
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
# include <direct.h>
# define MKDIR(d) _mkdir(d)
# define RMDIR(d) _rmdir(d)
#else
# include <sys/stat.h>
# include <unistd.h>
# define MKDIR(d) mkdir(d, 0777)
# define RMDIR(d) rmdir(d)
#endif
#include "CuTest.h"
#include "apx/crypto.h"
#include "apx/error.h"
#include "apx/server.h"
#include "apx/server_extension.h"
#include "dtl_type.h"
#include "extensions_cfg.h"
#include "server_cfg.h"
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
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
static void test_register_extensions_omitted_keys_disabled(CuTest *tc);
static void test_configure_server_null_arguments(CuTest *tc);
static void test_configure_server_security_require_signed_nodes(CuTest *tc);
static void test_configure_server_security_trusted_keys(CuTest *tc);
static void test_check_config_null_or_empty(CuTest *tc);
static void test_check_config_unknown_section(CuTest *tc);
static void test_check_config_unknown_top_level_key(CuTest *tc);
static void test_check_config_unknown_key_in_extension(CuTest *tc);
static void test_check_config_unknown_key_in_server(CuTest *tc);
static void test_check_config_nested_unknown_key(CuTest *tc);
static void test_check_config_recognized_keys_and_aliases(CuTest *tc);
static void test_check_config_omitted_keys_no_warnings(CuTest *tc);
static void test_check_config_unsupported_underscore_key_warns(CuTest *tc);
static void test_check_config_socket_extension_missing_transports_warns(CuTest *tc);
static void test_check_config_monitor_extension_missing_endpoints_warns(CuTest *tc);
static void test_check_config_tls_extension_missing_keys_warns(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite *testsuite_server_cfg(void)
{
  CuSuite *suite = CuSuiteNew();
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
  SUITE_ADD_TEST(suite, test_register_extensions_omitted_keys_disabled);
  SUITE_ADD_TEST(suite, test_configure_server_null_arguments);
  SUITE_ADD_TEST(suite, test_configure_server_security_require_signed_nodes);
  SUITE_ADD_TEST(suite, test_configure_server_security_trusted_keys);
  SUITE_ADD_TEST(suite, test_check_config_null_or_empty);
  SUITE_ADD_TEST(suite, test_check_config_unknown_section);
  SUITE_ADD_TEST(suite, test_check_config_unknown_top_level_key);
  SUITE_ADD_TEST(suite, test_check_config_unknown_key_in_extension);
  SUITE_ADD_TEST(suite, test_check_config_unknown_key_in_server);
  SUITE_ADD_TEST(suite, test_check_config_nested_unknown_key);
  SUITE_ADD_TEST(suite, test_check_config_recognized_keys_and_aliases);
  SUITE_ADD_TEST(suite, test_check_config_omitted_keys_no_warnings);
  SUITE_ADD_TEST(suite, test_check_config_unsupported_underscore_key_warns);
  SUITE_ADD_TEST(suite, test_check_config_socket_extension_missing_transports_warns);
  SUITE_ADD_TEST(suite, test_check_config_monitor_extension_missing_endpoints_warns);
  SUITE_ADD_TEST(suite, test_check_config_tls_extension_missing_keys_warns);
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
  const char *content = "{\n"
                        "    \"apx-server\": {\n"
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
  dtl_sv_t *sv = (dtl_sv_t *)dtl_hv_get_cstr((dtl_hv_t *)server_node, "max-num-events");
  CuAssertPtrNotNull(tc, sv);
  bool ok = false;
  CuAssertIntEquals(tc, 200, dtl_sv_to_i32(sv, &ok));
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
  const char *content = "{\n"
                        "    \"apx-server\": {\n"
                        "        \"max-num-events\": 200\n"
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
  dtl_sv_t *sv = (dtl_sv_t *)dtl_hv_get_cstr((dtl_hv_t *)server_node, "max-num-events");
  CuAssertPtrNotNull(tc, sv);
  bool ok = false;
  CuAssertIntEquals(tc, 200, dtl_sv_to_i32(sv, &ok));
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
  const char *content = "{\n"
                        "    \"apx-server\": {\n"
                        "        \"max-num-events\": 300\n"
                        "    }\n"
                        "}\n";
  write_test_file(filepath, content);

  dtl_hv_t *cfg = NULL;
  apx_error_t result = apx_server_load_config(dirname, &cfg);
  CuAssertIntEquals(tc, APX_NO_ERROR, result);
  CuAssertPtrNotNull(tc, cfg);

  dtl_dv_t *server_node = dtl_hv_get_cstr(cfg, "apx-server");
  CuAssertPtrNotNull(tc, server_node);
  dtl_sv_t *sv = (dtl_sv_t *)dtl_hv_get_cstr((dtl_hv_t *)server_node, "max-num-events");
  CuAssertPtrNotNull(tc, sv);
  bool ok = false;
  CuAssertIntEquals(tc, 300, dtl_sv_to_i32(sv, &ok));
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
  const char *content = "{\n"
                        "    \"apx-server\": {\n"
                        "        \"max-num-events\": 200\n"
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
  CuAssertIntEquals(tc, 1, adt_list_length(&server.extension_manager));
  apx_server_extension_t *ext = (apx_server_extension_t *)adt_list_first(&server.extension_manager);
  CuAssertPtrNotNull(tc, ext);
  CuAssertStrEquals(tc, "SOCKET", ext->name);

  apx_server_destroy(&server);

  dtl_dec_ref(cfg);

  remove(filepath);
}

static void test_register_extensions_omitted_keys_disabled(CuTest *tc)
{
  const char *filepath = "test_ext_omitted.json";
  const char *content = "{\n"
                        "    \"apx-server\": {\n"
                        "        \"max-num-events\": 200\n"
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
  CuAssertIntEquals(tc, 0, adt_list_length(&server.extension_manager));

  apx_server_destroy(&server);

  dtl_dec_ref(cfg);

  remove(filepath);
}

static void test_configure_server_null_arguments(CuTest *tc)
{
  apx_server_t server;
  apx_server_create(&server);

  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_server_configure(NULL, NULL));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_configure(&server, NULL));

  apx_server_destroy(&server);
}

static void test_configure_server_security_require_signed_nodes(CuTest *tc)
{
  const char *filepath = "test_server_cfg_security.json";
  const char *content = "{\n"
                        "    \"apx-server\": {\n"
                        "        \"security\": {\n"
                        "            \"require-signed-nodes\": true\n"
                        "        }\n"
                        "    }\n"
                        "}\n";
  write_test_file(filepath, content);

  dtl_hv_t *cfg = NULL;
  apx_error_t result = apx_server_load_config(filepath, &cfg);
  CuAssertIntEquals(tc, APX_NO_ERROR, result);

  apx_server_t server;
  apx_server_create(&server);
  CuAssertFalse(tc, apx_server_get_require_signed_nodes(&server));

  result = apx_server_configure(&server, cfg);
  CuAssertIntEquals(tc, APX_NO_ERROR, result);
  CuAssertTrue(tc, apx_server_get_require_signed_nodes(&server));

  apx_server_destroy(&server);
  dtl_dec_ref(cfg);
  remove(filepath);
}

static void test_configure_server_security_trusted_keys(CuTest *tc)
{
  char *priv_pem = NULL;
  char *pub_pem = NULL;
  apx_error_t rc = apx_crypto_generate_keypair_pem(&priv_pem, &pub_pem);
  CuAssertIntEquals(tc, APX_NO_ERROR, rc);

  const char *key_file = "test_pubkey.pem";
  write_test_file(key_file, pub_pem);

  const char *filepath = "test_server_cfg_keys.json";
  char content[2048];
  snprintf(content, sizeof(content),
    "{\n"
    "    \"apx-server\": {\n"
    "        \"security\": {\n"
    "            \"require-signed-nodes\": true,\n"
    "            \"trusted-keys\": [\"%s\"]\n"
    "        }\n"
    "    }\n"
    "}\n",
    key_file);
  write_test_file(filepath, content);

  dtl_hv_t *cfg = NULL;
  apx_error_t result = apx_server_load_config(filepath, &cfg);
  CuAssertIntEquals(tc, APX_NO_ERROR, result);

  apx_server_t server;
  apx_server_create(&server);
  CuAssertIntEquals(tc, 0, apx_server_get_num_trusted_public_keys(&server));

  result = apx_server_configure(&server, cfg);
  CuAssertIntEquals(tc, APX_NO_ERROR, result);
  CuAssertTrue(tc, apx_server_get_require_signed_nodes(&server));
  CuAssertIntEquals(tc, 1, apx_server_get_num_trusted_public_keys(&server));

  apx_server_destroy(&server);
  dtl_dec_ref(cfg);
  remove(filepath);
  remove(key_file);
  free(priv_pem);
  free(pub_pem);
}

#define MAX_TEST_WARNINGS 16

typedef struct test_warning_collector_tag
{
  int count;
  char messages[MAX_TEST_WARNINGS][256];
} test_warning_collector_t;

static void test_warning_collector_init(test_warning_collector_t *collector)
{
  collector->count = 0;
  for (int i = 0; i < MAX_TEST_WARNINGS; ++i)
  {
    collector->messages[i][0] = '\0';
  }
}

static void test_warning_cb(void *arg, const char *warning)
{
  test_warning_collector_t *collector = (test_warning_collector_t *)arg;
  if (collector->count < MAX_TEST_WARNINGS)
  {
    strncpy(collector->messages[collector->count], warning, 255);
    collector->messages[collector->count][255] = '\0';
    collector->count++;
  }
}

static bool test_warning_collector_has(const test_warning_collector_t *collector, const char *expected)
{
  for (int i = 0; i < collector->count; ++i)
  {
    if (strcmp(collector->messages[i], expected) == 0)
    {
      return true;
    }
  }
  return false;
}

static void test_check_config_null_or_empty(CuTest *tc)
{
  test_warning_collector_t collector;
  test_warning_collector_init(&collector);

  // NULL config
  apx_server_check_config_unknown_keys_cb(NULL, test_warning_cb, &collector);
  CuAssertIntEquals(tc, 0, collector.count);

  // Empty config
  dtl_hv_t *cfg = dtl_hv_new();
  CuAssertPtrNotNull(tc, cfg);
  apx_server_check_config_unknown_keys_cb(cfg, test_warning_cb, &collector);
  CuAssertIntEquals(tc, 0, collector.count);
  dtl_dec_ref(cfg);
}

static void test_check_config_unknown_section(CuTest *tc)
{
  test_warning_collector_t collector;
  test_warning_collector_init(&collector);

  const char *filepath = "test_cfg_unknown_sec.json";
  write_test_file(filepath, "{\"unknown-section\": {\"enabled\": true}}");

  dtl_hv_t *cfg = NULL;
  apx_error_t result = apx_server_load_config(filepath, &cfg);
  CuAssertIntEquals(tc, APX_NO_ERROR, result);
  CuAssertPtrNotNull(tc, cfg);

  apx_server_check_config_unknown_keys_cb(cfg, test_warning_cb, &collector);
  CuAssertIntEquals(tc, 1, collector.count);
  CuAssertTrue(tc, test_warning_collector_has(&collector, "Warning: Unknown configuration section 'unknown-section'"));

  dtl_dec_ref(cfg);
  remove(filepath);
}

static void test_check_config_unknown_top_level_key(CuTest *tc)
{
  test_warning_collector_t collector;
  test_warning_collector_init(&collector);

  const char *filepath = "test_cfg_unknown_top_key.json";
  write_test_file(filepath, "{\"unknown-key\": 42}");

  dtl_hv_t *cfg = NULL;
  apx_error_t result = apx_server_load_config(filepath, &cfg);
  CuAssertIntEquals(tc, APX_NO_ERROR, result);
  CuAssertPtrNotNull(tc, cfg);

  apx_server_check_config_unknown_keys_cb(cfg, test_warning_cb, &collector);
  CuAssertIntEquals(tc, 1, collector.count);
  CuAssertTrue(tc, test_warning_collector_has(&collector, "Warning: Unknown configuration key 'unknown-key'"));

  dtl_dec_ref(cfg);
  remove(filepath);
}

static void test_check_config_unknown_key_in_extension(CuTest *tc)
{
  test_warning_collector_t collector;
  test_warning_collector_init(&collector);

  const char *filepath = "test_cfg_unknown_ext_key.json";
  write_test_file(filepath, "{\"textlog-extension\": {\"use-timestmp\": true}}");

  dtl_hv_t *cfg = NULL;
  apx_error_t result = apx_server_load_config(filepath, &cfg);
  CuAssertIntEquals(tc, APX_NO_ERROR, result);
  CuAssertPtrNotNull(tc, cfg);

  apx_server_check_config_unknown_keys_cb(cfg, test_warning_cb, &collector);
  CuAssertIntEquals(tc, 1, collector.count);
  CuAssertTrue(
    tc, test_warning_collector_has(&collector, "Warning: Unknown configuration key 'textlog-extension.use-timestmp'"));

  dtl_dec_ref(cfg);
  remove(filepath);
}

static void test_check_config_unknown_key_in_server(CuTest *tc)
{
  test_warning_collector_t collector;
  test_warning_collector_init(&collector);

  const char *filepath = "test_cfg_unknown_srv_key.json";
  write_test_file(filepath, "{\"apx-server\": {\"invalid-setting\": 123}}");

  dtl_hv_t *cfg = NULL;
  apx_error_t result = apx_server_load_config(filepath, &cfg);
  CuAssertIntEquals(tc, APX_NO_ERROR, result);
  CuAssertPtrNotNull(tc, cfg);

  apx_server_check_config_unknown_keys_cb(cfg, test_warning_cb, &collector);
  CuAssertIntEquals(tc, 1, collector.count);
  CuAssertTrue(
    tc, test_warning_collector_has(&collector, "Warning: Unknown configuration key 'apx-server.invalid-setting'"));

  dtl_dec_ref(cfg);
  remove(filepath);
}

static void test_check_config_nested_unknown_key(CuTest *tc)
{
  test_warning_collector_t collector;
  test_warning_collector_init(&collector);

  const char *filepath = "test_cfg_nested_unknown.json";
  const char *content = "{\n"
                        "    \"apx-server\": {\n"
                        "        \"security\": {\n"
                        "            \"bad-sec-key\": true\n"
                        "        }\n"
                        "    },\n"
                        "    \"monitor-extension\": {\n"
                        "        \"tcp-port\": 5080,\n"
                        "        \"tls\": {\n"
                        "            \"server-cert\": \"cert.pem\",\n"
                        "            \"server-key\": \"key.pem\",\n"
                        "            \"bad-tls-key\": \"foo\"\n"
                        "        }\n"
                        "    }\n"
                        "}\n";
  write_test_file(filepath, content);

  dtl_hv_t *cfg = NULL;
  apx_error_t result = apx_server_load_config(filepath, &cfg);
  CuAssertIntEquals(tc, APX_NO_ERROR, result);
  CuAssertPtrNotNull(tc, cfg);

  apx_server_check_config_unknown_keys_cb(cfg, test_warning_cb, &collector);
  CuAssertIntEquals(tc, 2, collector.count);
  CuAssertTrue(
    tc, test_warning_collector_has(&collector, "Warning: Unknown configuration key 'apx-server.security.bad-sec-key'"));
  CuAssertTrue(tc,
    test_warning_collector_has(&collector, "Warning: Unknown configuration key 'monitor-extension.tls.bad-tls-key'"));

  dtl_dec_ref(cfg);
  remove(filepath);
}

static void test_check_config_recognized_keys_and_aliases(CuTest *tc)
{
  test_warning_collector_t collector;
  test_warning_collector_init(&collector);

  const char *filepath = "test_cfg_recognized_aliases.json";
  const char *content = "{\n"
                        "    \"apx-server\": {\n"
                        "        \"security\": {\n"
                        "            \"require_signed_nodes\": true,\n"
                        "            \"trusted_keys\": [\"key1.pem\"]\n"
                        "        },\n"
                        "        \"max-num-events\": 200,\n"
                        "        \"apx-cache-enabled\": false,\n"
                        "        \"apx-cache-path\": \"/tmp\"\n"
                        "    },\n"
                        "    \"socket-server-extension\": {\n"
                        "        \"enabled\": true,\n"
                        "        \"port\": 5000,\n"
                        "        \"systemd\": \"auto\",\n"
                        "        \"unix-file\": \"/tmp/apx.socket\",\n"
                        "        \"tag\": \"test2\",\n"
                        "        \"allowed-groups\": [\"wheel\"],\n"
                        "        \"vsock-port\": 1234,\n"
                        "        \"vsock-cid\": 3,\n"
                        "        \"vsock-tag\": \"vtag\"\n"
                        "    },\n"
                        "    \"textlog-extension\": {\n"
                        "        \"enabled\": true,\n"
                        "        \"log_level\": \"DEBUG\",\n"
                        "        \"use_timestamp\": true,\n"
                        "        \"file_enabled\": true,\n"
                        "        \"file_path\": \"/tmp/test.log\",\n"
                        "        \"syslog_enabled\": false,\n"
                        "        \"syslog_label\": \"apx\"\n"
                        "    },\n"
                        "    \"monitor-extension\": {\n"
                        "        \"enabled\": true,\n"
                        "        \"unix-file\": \"/tmp/mon.socket\",\n"
                        "        \"port\": 5080,\n"
                        "        \"bind-address\": \"127.0.0.1\",\n"
                        "        \"tls\": {\n"
                        "            \"certificate\": \"cert.pem\",\n"
                        "            \"key\": \"key.pem\",\n"
                        "            \"ca-cert\": \"ca.pem\",\n"
                        "            \"require-client-cert\": false\n"
                        "        }\n"
                        "    },\n"
                        "    \"tls-server-extension\": {\n"
                        "        \"enabled\": true,\n"
                        "        \"port\": 5020,\n"
                        "        \"certificate\": \"cert.pem\",\n"
                        "        \"key\": \"key.pem\",\n"
                        "        \"ca-cert\": \"ca.pem\",\n"
                        "        \"require-client-cert\": true,\n"
                        "        \"tag\": \"TLS\"\n"
                        "    }\n"
                        "}\n";
  write_test_file(filepath, content);

  dtl_hv_t *cfg = NULL;
  apx_error_t result = apx_server_load_config(filepath, &cfg);
  CuAssertIntEquals(tc, APX_NO_ERROR, result);
  CuAssertPtrNotNull(tc, cfg);

  apx_server_check_config_unknown_keys_cb(cfg, test_warning_cb, &collector);
  CuAssertIntEquals(tc, 0, collector.count);

  dtl_dec_ref(cfg);
  remove(filepath);
}

static void test_check_config_omitted_keys_no_warnings(CuTest *tc)
{
  test_warning_collector_t collector;
  test_warning_collector_init(&collector);

  const char *filepath = "test_cfg_omitted_keys.json";
  write_test_file(filepath, "{\"socket-server-extension\": {\"enabled\": true, \"unix-file\": \"/tmp/apx.socket\"}}");

  dtl_hv_t *cfg = NULL;
  apx_error_t result = apx_server_load_config(filepath, &cfg);
  CuAssertIntEquals(tc, APX_NO_ERROR, result);
  CuAssertPtrNotNull(tc, cfg);

  apx_server_check_config_unknown_keys_cb(cfg, test_warning_cb, &collector);
  CuAssertIntEquals(tc, 0, collector.count);

  dtl_dec_ref(cfg);
  remove(filepath);
}

static void test_check_config_unsupported_underscore_key_warns(CuTest *tc)
{
  test_warning_collector_t collector;
  test_warning_collector_init(&collector);

  const char *filepath = "test_cfg_underscore_key.json";
  write_test_file(filepath, "{\"socket-server-extension\": {\"unix_file\": \"/tmp/apx.socket\"}}");

  dtl_hv_t *cfg = NULL;
  apx_error_t result = apx_server_load_config(filepath, &cfg);
  CuAssertIntEquals(tc, APX_NO_ERROR, result);
  CuAssertPtrNotNull(tc, cfg);

  apx_server_check_config_unknown_keys_cb(cfg, test_warning_cb, &collector);
  CuAssertIntEquals(tc, 2, collector.count); // 1 unknown key ("unix_file"), 1 missing transport
  CuAssertTrue(tc,
    test_warning_collector_has(&collector, "Warning: Unknown configuration key 'socket-server-extension.unix_file'"));
  CuAssertTrue(tc,
    test_warning_collector_has(&collector,
      "Warning: 'socket-server-extension' is enabled, but no listening transports are configured (expected at least "
      "one of: 'unix-file', 'tcp-port', 'vsock-port', or 'systemd')"));

  dtl_dec_ref(cfg);
  remove(filepath);
}

static void test_check_config_socket_extension_missing_transports_warns(CuTest *tc)
{
  test_warning_collector_t collector;
  test_warning_collector_init(&collector);

  const char *filepath = "test_cfg_sock_missing_transport.json";
  write_test_file(filepath, "{\"socket-server-extension\": {\"enabled\": true}}");

  dtl_hv_t *cfg = NULL;
  apx_error_t result = apx_server_load_config(filepath, &cfg);
  CuAssertIntEquals(tc, APX_NO_ERROR, result);
  CuAssertPtrNotNull(tc, cfg);

  apx_server_check_config_unknown_keys_cb(cfg, test_warning_cb, &collector);
  CuAssertIntEquals(tc, 1, collector.count);
  CuAssertTrue(tc,
    test_warning_collector_has(&collector,
      "Warning: 'socket-server-extension' is enabled, but no listening transports are configured (expected at least "
      "one of: 'unix-file', 'tcp-port', 'vsock-port', or 'systemd')"));

  dtl_dec_ref(cfg);
  remove(filepath);
}

static void test_check_config_monitor_extension_missing_endpoints_warns(CuTest *tc)
{
  test_warning_collector_t collector;
  test_warning_collector_init(&collector);

  const char *filepath = "test_cfg_mon_missing_endpoint.json";
  write_test_file(filepath, "{\"monitor-extension\": {\"enabled\": true}}");

  dtl_hv_t *cfg = NULL;
  apx_error_t result = apx_server_load_config(filepath, &cfg);
  CuAssertIntEquals(tc, APX_NO_ERROR, result);
  CuAssertPtrNotNull(tc, cfg);

  apx_server_check_config_unknown_keys_cb(cfg, test_warning_cb, &collector);
  CuAssertIntEquals(tc, 1, collector.count);
  CuAssertTrue(tc,
    test_warning_collector_has(&collector,
      "Warning: 'monitor-extension' is enabled, but no listening endpoints are configured (expected 'socket-file' or "
      "'tcp-port')"));

  dtl_dec_ref(cfg);
  remove(filepath);
}

static void test_check_config_tls_extension_missing_keys_warns(CuTest *tc)
{
  test_warning_collector_t collector;
  test_warning_collector_init(&collector);

  const char *filepath = "test_cfg_tls_missing_keys.json";
  write_test_file(filepath, "{\"tls-server-extension\": {\"enabled\": true}}");

  dtl_hv_t *cfg = NULL;
  apx_error_t result = apx_server_load_config(filepath, &cfg);
  CuAssertIntEquals(tc, APX_NO_ERROR, result);
  CuAssertPtrNotNull(tc, cfg);

  apx_server_check_config_unknown_keys_cb(cfg, test_warning_cb, &collector);
  CuAssertIntEquals(tc, 1, collector.count);
  CuAssertTrue(tc,
    test_warning_collector_has(&collector,
      "Warning: 'tls-server-extension' is enabled, but missing required configuration (requires 'tcp-port', "
      "'server-cert', and 'server-key')"));

  dtl_dec_ref(cfg);
  remove(filepath);
}
