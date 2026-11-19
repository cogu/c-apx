/*****************************************************************************
 * \file      testsuite_node_cache.c
 * \author    Conny Gustafsson
 * \date      2026-11-19
 * \brief     Unit tests for apx_node_cache and server cache integration
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "CuTest.h"
#include "apx/crypto.h"
#include "apx/file.h"
#include "apx/file_info.h"
#include "apx/node_cache.h"
#include "apx/node_manager.h"
#include "apx/server.h"
#include "apx/server_test_connection.h"
#include "fileutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
# include <direct.h>
# define TEST_RMDIR(d) _rmdir(d)
#else
# include <sys/stat.h>
# include <unistd.h>
# define TEST_RMDIR(d) rmdir(d)
#endif
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct remove_tree_ctx_tag
{
  char const *dir_path;
} remove_tree_ctx_t;

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static bool on_remove_tree_entry(void *arg, const cutil_dir_entry_t *entry);
static void remove_dir_recursive(char const *path);
static void make_sha256_hex(uint8_t const hash[RMF_SHA256_SIZE], char out[65]);
static adt_str_t *make_cache_file_path(char const *root, char const *node_name, uint8_t const hash[RMF_SHA256_SIZE]);

static void test_node_cache_default_and_off(CuTest *tc);
static void test_node_cache_empty_path_disables_with_warning(CuTest *tc);
static void test_node_cache_store_and_lookup_roundtrip(CuTest *tc);
static void test_node_cache_corruption_size_hash_and_symlink(CuTest *tc);
static void test_node_cache_path_traversal_rejected(CuTest *tc);
static void test_node_cache_per_node_rotation_and_prefix_isolation(CuTest *tc);
static void test_node_cache_lru_refresh_on_hit(CuTest *tc);
static void test_node_cache_global_size_cap(CuTest *tc);
static void test_node_cache_readonly_and_unreadable_dirs(CuTest *tc);
static void test_node_cache_lock_prevents_concurrent_owners(CuTest *tc);
static void test_node_cache_server_integration_flows(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite *testsuite_apx_node_cache(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_node_cache_default_and_off);
  SUITE_ADD_TEST(suite, test_node_cache_empty_path_disables_with_warning);
  SUITE_ADD_TEST(suite, test_node_cache_store_and_lookup_roundtrip);
  SUITE_ADD_TEST(suite, test_node_cache_corruption_size_hash_and_symlink);
  SUITE_ADD_TEST(suite, test_node_cache_path_traversal_rejected);
  SUITE_ADD_TEST(suite, test_node_cache_per_node_rotation_and_prefix_isolation);
  SUITE_ADD_TEST(suite, test_node_cache_lru_refresh_on_hit);
  SUITE_ADD_TEST(suite, test_node_cache_global_size_cap);
  SUITE_ADD_TEST(suite, test_node_cache_readonly_and_unreadable_dirs);
  SUITE_ADD_TEST(suite, test_node_cache_lock_prevents_concurrent_owners);
  SUITE_ADD_TEST(suite, test_node_cache_server_integration_flows);
  return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static bool on_remove_tree_entry(void *arg, const cutil_dir_entry_t *entry)
{
  remove_tree_ctx_t *ctx = (remove_tree_ctx_t *)arg;
  adt_str_t *full = cutil_path_join(ctx->dir_path, entry->name);
  if (full != NULL)
  {
    if (entry->is_dir && !entry->is_symlink)
    {
      remove_dir_recursive(adt_str_cstr(full));
    }
    else
    {
      (void)remove(adt_str_cstr(full));
    }
    adt_str_delete(full);
  }
  return true;
}

static void remove_dir_recursive(char const *path)
{
  if (path == NULL || !cutil_is_dir(path))
  {
    return;
  }
  remove_tree_ctx_t ctx;
  ctx.dir_path = path;
  (void)cutil_dir_for_each(path, on_remove_tree_entry, &ctx);
  (void)TEST_RMDIR(path);
}

static void make_sha256_hex(uint8_t const hash[RMF_SHA256_SIZE], char out[65])
{
  static char const m_hex[] = "0123456789abcdef";
  for (size_t i = 0u; i < RMF_SHA256_SIZE; i++)
  {
    out[i * 2u] = m_hex[(hash[i] >> 4u) & 0x0Fu];
    out[i * 2u + 1u] = m_hex[hash[i] & 0x0Fu];
  }
  out[64] = '\0';
}

static adt_str_t *make_cache_file_path(char const *root, char const *node_name, uint8_t const hash[RMF_SHA256_SIZE])
{
  char hex[65];
  make_sha256_hex(hash, hex);
  char filename[512];
  snprintf(filename, sizeof(filename), "text/%s_%s.apx", node_name, hex);
  return cutil_path_join(root, filename);
}

static void test_node_cache_default_and_off(CuTest *tc)
{
  apx_node_cache_t cache;
  apx_node_cache_create(&cache);
  CuAssertTrue(tc, !apx_node_cache_is_text_enabled(&cache));
  CuAssertTrue(tc, !apx_node_cache_is_read_only(&cache));
  CuAssertIntEquals(tc, APX_CACHE_LEVEL_OFF, (int)apx_node_cache_get_level(&cache));
  CuAssertUIntEquals(tc, APX_CACHE_DEFAULT_MAX_VERSIONS_PER_NODE, apx_node_cache_get_max_versions_per_node(&cache));
  CuAssertUIntEquals(tc, APX_CACHE_DEFAULT_MAX_SIZE_MB, apx_node_cache_get_max_size_mb(&cache));

  apx_node_cache_config_t cfg;
  apx_node_cache_config_init(&cfg);
  char warn[256] = {0};
  const char *root = "test_cache_off_should_not_exist";
  remove_dir_recursive(root);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_configure(&cache, &cfg, root, warn, sizeof(warn)));
  CuAssertStrEquals(tc, "", warn);
  CuAssertTrue(tc, !apx_node_cache_is_text_enabled(&cache));
  CuAssertTrue(tc, !cutil_is_dir(root));

  apx_node_cache_destroy(&cache);
}

static void test_node_cache_empty_path_disables_with_warning(CuTest *tc)
{
  apx_node_cache_t cache;
  apx_node_cache_create(&cache);

  apx_node_cache_config_t cfg;
  apx_node_cache_config_init(&cfg);
  cfg.level = APX_CACHE_LEVEL_TEXT;
  char warn[256] = {0};
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_configure(&cache, &cfg, "", warn, sizeof(warn)));
  CuAssertTrue(tc, !apx_node_cache_is_text_enabled(&cache));
  CuAssertTrue(tc, strstr(warn, "apx-server.cache.path") != NULL);

  apx_node_cache_destroy(&cache);
}

static void test_node_cache_store_and_lookup_roundtrip(CuTest *tc)
{
  const char *root = "test_cache_roundtrip_dir";
  remove_dir_recursive(root);

  apx_node_cache_t cache;
  apx_node_cache_create(&cache);

  apx_node_cache_config_t cfg;
  apx_node_cache_config_init(&cfg);
  cfg.level = APX_CACHE_LEVEL_TEXT;
  char warn[256] = {0};
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_configure(&cache, &cfg, root, warn, sizeof(warn)));
  CuAssertStrEquals(tc, "", warn);
  CuAssertTrue(tc, apx_node_cache_is_text_enabled(&cache));
  CuAssertTrue(tc, !apx_node_cache_is_read_only(&cache));
  CuAssertStrEquals(tc, root, apx_node_cache_get_root_path(&cache));

  const char *apx = "APX/1.3\nN\"NodeA\"\nP\"Sig1\"C:=0\n";
  uint32_t const len = (uint32_t)strlen(apx);
  uint8_t hash[RMF_SHA256_SIZE];
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)apx, len, hash));

  // Initial miss
  uint8_t *out = NULL;
  CuAssertIntEquals(tc, APX_FILE_NOT_FOUND_ERROR, apx_node_cache_lookup_text(&cache, "NodeA", hash, len, &out));
  CuAssertPtrEquals(tc, NULL, out);

  // Store + duplicate store idempotency
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_store_text(&cache, "NodeA", hash, (uint8_t const *)apx, len));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_store_text(&cache, "NodeA", hash, (uint8_t const *)apx, len));

  // Lookup hit
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_lookup_text(&cache, "NodeA", hash, len, &out));
  CuAssertPtrNotNull(tc, out);
  CuAssertIntEquals(tc, 0, memcmp(out, apx, len));
  CuAssertIntEquals(tc, '\0', out[len]);
  free(out);

  apx_node_cache_stats_t stats;
  apx_node_cache_get_stats(&cache, &stats);
  CuAssertTrue(tc, stats.text_misses == 1u);
  CuAssertTrue(tc, stats.text_stores == 1u);
  CuAssertTrue(tc, stats.text_hits == 1u);
  CuAssertTrue(tc, stats.text_corruptions == 0u);

  apx_node_cache_destroy(&cache);
  remove_dir_recursive(root);
}

static void test_node_cache_corruption_size_hash_and_symlink(CuTest *tc)
{
  const char *root = "test_cache_corrupt_dir";
  remove_dir_recursive(root);

  apx_node_cache_t cache;
  apx_node_cache_create(&cache);
  apx_node_cache_config_t cfg;
  apx_node_cache_config_init(&cfg);
  cfg.level = APX_CACHE_LEVEL_TEXT;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_configure(&cache, &cfg, root, NULL, 0u));

  const char *apx = "APX/1.3\nN\"NodeB\"\nP\"Sig\"C:=1\n";
  uint32_t const len = (uint32_t)strlen(apx);
  uint8_t hash[RMF_SHA256_SIZE];
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)apx, len, hash));

  // 1. Same-length byte flip -> APX_CHECKSUM_ERROR and auto-unlinked
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_store_text(&cache, "NodeB", hash, (uint8_t const *)apx, len));
  adt_str_t *file_path = make_cache_file_path(root, "NodeB", hash);
  CuAssertPtrNotNull(tc, file_path);
  char tampered[64];
  memcpy(tampered, apx, len + 1u);
  tampered[len - 2u] = '9';
  CuAssertIntEquals(tc, 0, cutil_write_binary_file(adt_str_cstr(file_path), (uint8_t const *)tampered, len));

  uint8_t *out = NULL;
  CuAssertIntEquals(tc, APX_CHECKSUM_ERROR, apx_node_cache_lookup_text(&cache, "NodeB", hash, len, &out));
  CuAssertPtrEquals(tc, NULL, out);
  CuAssertTrue(tc, !cutil_file_exists(adt_str_cstr(file_path)));

  // 2. Truncated file -> APX_LENGTH_ERROR and auto-unlinked
  CuAssertIntEquals(tc, 0, cutil_write_binary_file(adt_str_cstr(file_path), (uint8_t const *)apx, len - 5u));
  CuAssertIntEquals(tc, APX_LENGTH_ERROR, apx_node_cache_lookup_text(&cache, "NodeB", hash, len, &out));
  CuAssertPtrEquals(tc, NULL, out);
  CuAssertTrue(tc, !cutil_file_exists(adt_str_cstr(file_path)));

#ifndef _WIN32
  // 3. Symlink rejected without following
  const char *target = "test_cache_symlink_target.apx";
  CuAssertIntEquals(tc, 0, cutil_write_binary_file(target, (uint8_t const *)apx, len));
  if (symlink(target, adt_str_cstr(file_path)) == 0)
  {
    CuAssertIntEquals(tc, APX_CHECKSUM_ERROR, apx_node_cache_lookup_text(&cache, "NodeB", hash, len, &out));
    CuAssertPtrEquals(tc, NULL, out);
    (void)remove(adt_str_cstr(file_path));
  }
  (void)remove(target);
#endif

  adt_str_delete(file_path);
  apx_node_cache_destroy(&cache);
  remove_dir_recursive(root);
}

static void test_node_cache_path_traversal_rejected(CuTest *tc)
{
  const char *root = "test_cache_traversal_dir";
  remove_dir_recursive(root);

  apx_node_cache_t cache;
  apx_node_cache_create(&cache);
  apx_node_cache_config_t cfg;
  apx_node_cache_config_init(&cfg);
  cfg.level = APX_CACHE_LEVEL_TEXT;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_configure(&cache, &cfg, root, NULL, 0u));

  const char *apx = "APX/1.3\nN\"Evil\"\n";
  uint32_t const len = (uint32_t)strlen(apx);
  uint8_t hash[RMF_SHA256_SIZE];
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)apx, len, hash));

  const char *bad_names[] = {"../evil", "a/b", "a\\b", ".hidden", "foo..bar", ""};
  for (size_t i = 0u; i < sizeof(bad_names) / sizeof(bad_names[0]); i++)
  {
    uint8_t *out = NULL;
    CuAssertIntEquals(
      tc, APX_NO_ERROR, apx_node_cache_store_text(&cache, bad_names[i], hash, (uint8_t const *)apx, len));
    CuAssertIntEquals(tc, APX_FILE_NOT_FOUND_ERROR, apx_node_cache_lookup_text(&cache, bad_names[i], hash, len, &out));
    CuAssertPtrEquals(tc, NULL, out);
  }

  apx_node_cache_stats_t stats;
  apx_node_cache_get_stats(&cache, &stats);
  CuAssertTrue(tc, stats.text_stores == 0u);

  apx_node_cache_destroy(&cache);
  remove_dir_recursive(root);
}

static void test_node_cache_per_node_rotation_and_prefix_isolation(CuTest *tc)
{
  const char *root = "test_cache_rotation_dir";
  remove_dir_recursive(root);

  apx_node_cache_t cache;
  apx_node_cache_create(&cache);
  apx_node_cache_config_t cfg;
  apx_node_cache_config_init(&cfg);
  cfg.level = APX_CACHE_LEVEL_TEXT;
  cfg.max_versions_per_node = 3u;
  cfg.max_size_mb = 0u;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_configure(&cache, &cfg, root, NULL, 0u));

  // Store 2 versions of Foo_Bar (older mtime) to ensure Foo never matches/evicts Foo_Bar
  uint8_t fb_hash[2][RMF_SHA256_SIZE];
  uint32_t fb_len[2];
  for (int i = 0; i < 2; i++)
  {
    char text[64];
    snprintf(text, sizeof(text), "APX/1.3\nN\"Foo_Bar\"\nP\"S%d\"C:=0\n", i);
    fb_len[i] = (uint32_t)strlen(text);
    CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)text, fb_len[i], fb_hash[i]));
    CuAssertIntEquals(
      tc, APX_NO_ERROR, apx_node_cache_store_text(&cache, "Foo_Bar", fb_hash[i], (uint8_t const *)text, fb_len[i]));
    adt_str_t *p = make_cache_file_path(root, "Foo_Bar", fb_hash[i]);
    CuAssertIntEquals(tc, 0, cutil_file_set_mtime(adt_str_cstr(p), 1700000000LL + i));
    adt_str_delete(p);
  }

  // Store 5 versions of Foo with increasing mtimes
  uint8_t foo_hash[5][RMF_SHA256_SIZE];
  uint32_t foo_len[5];
  for (int i = 0; i < 5; i++)
  {
    char text[64];
    snprintf(text, sizeof(text), "APX/1.3\nN\"Foo\"\nP\"S%d\"C:=0\n", i);
    foo_len[i] = (uint32_t)strlen(text);
    CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)text, foo_len[i], foo_hash[i]));
    CuAssertIntEquals(
      tc, APX_NO_ERROR, apx_node_cache_store_text(&cache, "Foo", foo_hash[i], (uint8_t const *)text, foo_len[i]));
    adt_str_t *p = make_cache_file_path(root, "Foo", foo_hash[i]);
    CuAssertIntEquals(tc, 0, cutil_file_set_mtime(adt_str_cstr(p), 1700001000LL + (int64_t)i * 10LL));
    adt_str_delete(p);
  }

  // Oldest 2 Foo versions evicted, newest 3 survive
  for (int i = 0; i < 5; i++)
  {
    uint8_t *out = NULL;
    apx_error_t rc = apx_node_cache_lookup_text(&cache, "Foo", foo_hash[i], foo_len[i], &out);
    if (i < 2)
    {
      CuAssertIntEquals(tc, APX_FILE_NOT_FOUND_ERROR, rc);
    }
    else
    {
      CuAssertIntEquals(tc, APX_NO_ERROR, rc);
      free(out);
    }
  }

  // Both Foo_Bar versions survived untouched
  for (int i = 0; i < 2; i++)
  {
    uint8_t *out = NULL;
    CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_lookup_text(&cache, "Foo_Bar", fb_hash[i], fb_len[i], &out));
    free(out);
  }

  apx_node_cache_destroy(&cache);
  remove_dir_recursive(root);
}

static void test_node_cache_lru_refresh_on_hit(CuTest *tc)
{
  const char *root = "test_cache_lru_dir";
  remove_dir_recursive(root);

  apx_node_cache_t cache;
  apx_node_cache_create(&cache);
  apx_node_cache_config_t cfg;
  apx_node_cache_config_init(&cfg);
  cfg.level = APX_CACHE_LEVEL_TEXT;
  cfg.max_versions_per_node = 2u;
  cfg.max_size_mb = 0u;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_configure(&cache, &cfg, root, NULL, 0u));

  const char *v1 = "APX/1.3\nN\"LruNode\"\nP\"V1\"C:=1\n";
  const char *v2 = "APX/1.3\nN\"LruNode\"\nP\"V2\"C:=2\n";
  const char *v3 = "APX/1.3\nN\"LruNode\"\nP\"V3\"C:=3\n";
  uint8_t h1[RMF_SHA256_SIZE], h2[RMF_SHA256_SIZE], h3[RMF_SHA256_SIZE];
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)v1, strlen(v1), h1));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)v2, strlen(v2), h2));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)v3, strlen(v3), h3));

  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_node_cache_store_text(&cache, "LruNode", h1, (uint8_t const *)v1, (uint32_t)strlen(v1)));
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_node_cache_store_text(&cache, "LruNode", h2, (uint8_t const *)v2, (uint32_t)strlen(v2)));

  // Backdate v1 older than v2 (both > 60s ago)
  adt_str_t *p1 = make_cache_file_path(root, "LruNode", h1);
  adt_str_t *p2 = make_cache_file_path(root, "LruNode", h2);
  CuAssertIntEquals(tc, 0, cutil_file_set_mtime(adt_str_cstr(p1), 1600000000LL));
  CuAssertIntEquals(tc, 0, cutil_file_set_mtime(adt_str_cstr(p2), 1600000500LL));
  adt_str_delete(p1);
  adt_str_delete(p2);

  // Accessing v1 refreshes its mtime to now -> v2 becomes the oldest
  uint8_t *out = NULL;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_lookup_text(&cache, "LruNode", h1, (uint32_t)strlen(v1), &out));
  free(out);

  // Storing v3 should evict v2, keeping v1 and v3
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_node_cache_store_text(&cache, "LruNode", h3, (uint8_t const *)v3, (uint32_t)strlen(v3)));

  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_lookup_text(&cache, "LruNode", h1, (uint32_t)strlen(v1), &out));
  free(out);
  CuAssertIntEquals(
    tc, APX_FILE_NOT_FOUND_ERROR, apx_node_cache_lookup_text(&cache, "LruNode", h2, (uint32_t)strlen(v2), &out));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_lookup_text(&cache, "LruNode", h3, (uint32_t)strlen(v3), &out));
  free(out);

  apx_node_cache_destroy(&cache);
  remove_dir_recursive(root);
}

static void test_node_cache_global_size_cap(CuTest *tc)
{
  const char *root = "test_cache_sizecap_dir";
  remove_dir_recursive(root);

  apx_node_cache_t cache;
  apx_node_cache_create(&cache);
  apx_node_cache_config_t cfg;
  apx_node_cache_config_init(&cfg);
  cfg.level = APX_CACHE_LEVEL_TEXT;
  cfg.max_versions_per_node = 0u;
  cfg.max_size_mb = 1u;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_configure(&cache, &cfg, root, NULL, 0u));
  // Cap at 80 bytes so 3 x ~33-byte nodes forces eviction of the oldest
  apx_node_cache_set_max_size_bytes(&cache, 80u);

  const char *n1 = "APX/1.3\nN\"Node1\"\nP\"Sig1\"C:=11\n";
  const char *n2 = "APX/1.3\nN\"Node2\"\nP\"Sig2\"C:=22\n";
  const char *n3 = "APX/1.3\nN\"Node3\"\nP\"Sig3\"C:=33\n";
  uint8_t h1[RMF_SHA256_SIZE], h2[RMF_SHA256_SIZE], h3[RMF_SHA256_SIZE];
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)n1, strlen(n1), h1));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)n2, strlen(n2), h2));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)n3, strlen(n3), h3));

  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_node_cache_store_text(&cache, "Node1", h1, (uint8_t const *)n1, (uint32_t)strlen(n1)));
  adt_str_t *p1 = make_cache_file_path(root, "Node1", h1);
  CuAssertIntEquals(tc, 0, cutil_file_set_mtime(adt_str_cstr(p1), 1700000001LL));
  adt_str_delete(p1);

  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_node_cache_store_text(&cache, "Node2", h2, (uint8_t const *)n2, (uint32_t)strlen(n2)));
  adt_str_t *p2 = make_cache_file_path(root, "Node2", h2);
  CuAssertIntEquals(tc, 0, cutil_file_set_mtime(adt_str_cstr(p2), 1700000002LL));
  adt_str_delete(p2);

  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_node_cache_store_text(&cache, "Node3", h3, (uint8_t const *)n3, (uint32_t)strlen(n3)));

  uint8_t *out = NULL;
  CuAssertIntEquals(
    tc, APX_FILE_NOT_FOUND_ERROR, apx_node_cache_lookup_text(&cache, "Node1", h1, (uint32_t)strlen(n1), &out));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_lookup_text(&cache, "Node2", h2, (uint32_t)strlen(n2), &out));
  free(out);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_lookup_text(&cache, "Node3", h3, (uint32_t)strlen(n3), &out));
  free(out);

  apx_node_cache_destroy(&cache);
  remove_dir_recursive(root);
}

static void test_node_cache_readonly_and_unreadable_dirs(CuTest *tc)
{
#ifndef _WIN32
  if (geteuid() == 0)
  {
    return; // Root bypasses POSIX DAC permissions
  }
  const char *root = "test_cache_perms_dir";
  remove_dir_recursive(root);

  // Pre-populate one entry while writable
  apx_node_cache_t seed;
  apx_node_cache_create(&seed);
  apx_node_cache_config_t cfg;
  apx_node_cache_config_init(&cfg);
  cfg.level = APX_CACHE_LEVEL_TEXT;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_configure(&seed, &cfg, root, NULL, 0u));

  const char *apx = "APX/1.3\nN\"Prewarmed\"\nP\"Sig\"C:=1\n";
  uint32_t const len = (uint32_t)strlen(apx);
  uint8_t hash[RMF_SHA256_SIZE];
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)apx, len, hash));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_store_text(&seed, "Prewarmed", hash, (uint8_t const *)apx, len));
  apx_node_cache_destroy(&seed);

  adt_str_t *text_dir = cutil_path_join(root, "text");
  CuAssertPtrNotNull(tc, text_dir);

  // 1. Readable but unwritable (0555) -> read-only fallback with warning
  CuAssertIntEquals(tc, 0, chmod(adt_str_cstr(text_dir), 0555));
  apx_node_cache_t ro_cache;
  apx_node_cache_create(&ro_cache);
  char warn[512] = {0};
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_configure(&ro_cache, &cfg, root, warn, sizeof(warn)));
  CuAssertTrue(tc, apx_node_cache_is_text_enabled(&ro_cache));
  CuAssertTrue(tc, apx_node_cache_is_read_only(&ro_cache));
  CuAssertTrue(tc, strstr(warn, "read-only") != NULL);

  // Existing entry still hits
  uint8_t *out = NULL;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_lookup_text(&ro_cache, "Prewarmed", hash, len, &out));
  CuAssertPtrNotNull(tc, out);
  free(out);

  // New store is a safe no-op
  const char *other = "APX/1.3\nN\"Other\"\nP\"S\"C:=0\n";
  uint8_t other_hash[RMF_SHA256_SIZE];
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)other, strlen(other), other_hash));
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_node_cache_store_text(&ro_cache, "Other", other_hash, (uint8_t const *)other, (uint32_t)strlen(other)));
  apx_node_cache_destroy(&ro_cache);

  // 2. Completely unreadable (0000) -> disabled with warning
  CuAssertIntEquals(tc, 0, chmod(adt_str_cstr(text_dir), 0000));
  apx_node_cache_t no_access;
  apx_node_cache_create(&no_access);
  warn[0] = '\0';
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_configure(&no_access, &cfg, root, warn, sizeof(warn)));
  CuAssertTrue(tc, !apx_node_cache_is_text_enabled(&no_access));
  CuAssertTrue(tc, strstr(warn, "disabling cache") != NULL);
  apx_node_cache_destroy(&no_access);

  // Restore permissions for cleanup
  CuAssertIntEquals(tc, 0, chmod(adt_str_cstr(text_dir), 0755));
  adt_str_delete(text_dir);
  remove_dir_recursive(root);
#else
  (void)tc;
#endif
}

static void test_node_cache_lock_prevents_concurrent_owners(CuTest *tc)
{
  const char *root = "test_cache_lock_dir";
  remove_dir_recursive(root);

  apx_node_cache_config_t cfg;
  apx_node_cache_config_init(&cfg);
  cfg.level = APX_CACHE_LEVEL_TEXT;

  apx_node_cache_t cache1;
  apx_node_cache_t cache2;
  apx_node_cache_create(&cache1);
  apx_node_cache_create(&cache2);

  char warn1[512] = {0};
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_configure(&cache1, &cfg, root, warn1, sizeof(warn1)));
  CuAssertTrue(tc, apx_node_cache_is_text_enabled(&cache1));
  CuAssertStrEquals(tc, "", warn1);

  adt_str_t *lock_path = cutil_path_join(root, ".lock");
  CuAssertPtrNotNull(tc, lock_path);
  CuAssertTrue(tc, cutil_file_exists(adt_str_cstr(lock_path)));
  adt_str_delete(lock_path);

  // Second instance on the same cache directory must fail-safely disable caching with warning
  char warn2[512] = {0};
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_configure(&cache2, &cfg, root, warn2, sizeof(warn2)));
  CuAssertTrue(tc, !apx_node_cache_is_text_enabled(&cache2));
  CuAssertTrue(tc, strstr(warn2, "already locked") != NULL);

  // Destroying cache1 releases the OS lock; cache2 can now acquire it cleanly
  apx_node_cache_destroy(&cache1);
  warn2[0] = '\0';
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_configure(&cache2, &cfg, root, warn2, sizeof(warn2)));
  CuAssertTrue(tc, apx_node_cache_is_text_enabled(&cache2));
  CuAssertStrEquals(tc, "", warn2);

  apx_node_cache_destroy(&cache2);
  remove_dir_recursive(root);
}

static void test_node_cache_server_integration_flows(CuTest *tc)
{
  const char *root = "test_cache_server_flow_dir";
  remove_dir_recursive(root);

  apx_server_t *server = apx_server_new();
  CuAssertPtrNotNull(tc, server);
  apx_node_cache_config_t cfg;
  apx_node_cache_config_init(&cfg);
  cfg.level = APX_CACHE_LEVEL_TEXT;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_configure_cache(server, &cfg, root, NULL, 0u));

  apx_server_test_connection_t *conn = apx_server_test_connection_new();
  CuAssertPtrNotNull(tc, conn);
  apx_server_connection_set_server(&conn->base, server);
  apx_node_manager_t *mgr = apx_connection_base_get_node_manager(&conn->base.base);
  CuAssertPtrNotNull(tc, mgr);

  const char *apx = "APX/1.3\nN\"CacheFlowNode\"\nP\"Speed\"S:=0\n";
  uint32_t const len = (uint32_t)strlen(apx);
  uint8_t hash[RMF_SHA256_SIZE];
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)apx, len, hash));

  // 1. Legacy SHA-1 is rejected with APX_UNSUPPORTED_ERROR
  uint8_t sha1_dummy[RMF_SHA256_SIZE] = {0};
  rmf_file_info_t *sha1_info = rmf_file_info_make_fixed_with_digest(
    "CacheFlowNode.apx", len, APX_DEFINITION_ADDRESS_START, RMF_DIGEST_TYPE_SHA1, sha1_dummy);
  bool open_req = false;
  CuAssertIntEquals(tc, APX_UNSUPPORTED_ERROR, apx_node_manager_init_node_from_file_info(mgr, sha1_info, &open_req));
  rmf_file_info_delete(sha1_info);

  // 2. Unsigned with RMF_DIGEST_TYPE_NONE -> always open_req = true and does NOT store in text cache
  const char *nodigest_apx = "APX/1.3\nN\"NoDigestNode\"\nP\"Val\"C:=1\n";
  uint32_t const nd_len = (uint32_t)strlen(nodigest_apx);
  rmf_file_info_t *nd_info = rmf_file_info_make_fixed("NoDigestNode.apx", nd_len, APX_DEFINITION_ADDRESS_START);
  open_req = false;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_init_node_from_file_info(mgr, nd_info, &open_req));
  CuAssertTrue(tc, open_req);
  apx_node_instance_t *nd_node = apx_node_manager_get_last_attached(mgr);
  apx_file_t *nd_file = apx_file_new(nd_info);
  apx_node_instance_remote_file_published_notification(nd_node, nd_file);
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_node_data_write_definition_data(
      apx_node_instance_get_node_data(nd_node), 0u, (uint8_t const *)nodigest_apx, nd_len));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node_from_data(mgr, nd_node));
  apx_file_delete(nd_file);
  rmf_file_info_delete(nd_info);

  apx_node_cache_stats_t stats;
  apx_node_cache_get_stats(apx_server_get_node_cache(server), &stats);
  CuAssertTrue(tc, stats.text_stores == 0u);

  // 3. Cold unsigned SHA-256 -> miss (open_req = true) -> build stores in cache
  rmf_file_info_t *sha256_info = rmf_file_info_make_fixed_with_digest(
    "CacheFlowNode.apx", len, APX_DEFINITION_ADDRESS_START, RMF_DIGEST_TYPE_SHA256, hash);
  open_req = false;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_init_node_from_file_info(mgr, sha256_info, &open_req));
  CuAssertTrue(tc, open_req);
  apx_node_instance_t *node1 = apx_node_manager_get_last_attached(mgr);
  apx_file_t *def_file1 = apx_file_new(sha256_info);
  apx_node_instance_remote_file_published_notification(node1, def_file1);
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_node_data_write_definition_data(apx_node_instance_get_node_data(node1), 0u, (uint8_t const *)apx, len));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node_from_data(mgr, node1));
  apx_file_delete(def_file1);

  apx_node_cache_get_stats(apx_server_get_node_cache(server), &stats);
  CuAssertTrue(tc, stats.text_stores == 1u);

  // 4. Second connection with plain SHA-256 -> HIT (open_req = false)
  apx_server_test_connection_t *conn2 = apx_server_test_connection_new();
  apx_server_connection_set_server(&conn2->base, server);
  apx_node_manager_t *mgr2 = apx_connection_base_get_node_manager(&conn2->base.base);
  open_req = true;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_init_node_from_file_info(mgr2, sha256_info, &open_req));
  CuAssertTrue(tc, !open_req);
  apx_node_instance_t *node2 = apx_node_manager_get_last_attached(mgr2);
  CuAssertPtrNotNull(tc, node2);
  CuAssertUIntEquals(tc, 1u, apx_node_instance_get_num_provide_ports(node2));

  // 5. Third connection with compressed SHA-256 -> HIT (open_req = false)
  apx_server_test_connection_t *conn3 = apx_server_test_connection_new();
  apx_server_connection_set_server(&conn3->base, server);
  apx_node_manager_t *mgr3 = apx_connection_base_get_node_manager(&conn3->base.base);
  rmf_file_info_t *comp_info = rmf_file_info_make_compressed(
    "CacheFlowNode.apx", 20u, len, APX_DEFINITION_ADDRESS_START, RMF_DIGEST_TYPE_SHA256, hash);
  open_req = true;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_init_node_from_file_info(mgr3, comp_info, &open_req));
  CuAssertTrue(tc, !open_req);
  rmf_file_info_delete(comp_info);

  // 6. Signed file ALWAYS requires open_req = true even when matching text is cached
  apx_server_test_connection_t *conn4 = apx_server_test_connection_new();
  apx_server_connection_set_server(&conn4->base, server);
  apx_node_manager_t *mgr4 = apx_connection_base_get_node_manager(&conn4->base.base);
  uint8_t dummy_sig[RMF_SIGNATURE_SIZE_ECDSA_P256] = {1};
  rmf_file_info_t *signed_info = rmf_file_info_make_fixed_with_signature(
    "CacheFlowNode.apx", len, APX_DEFINITION_ADDRESS_START, RMF_SIGNATURE_TYPE_ECDSA_P256, dummy_sig);
  open_req = false;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_init_node_from_file_info(mgr4, signed_info, &open_req));
  CuAssertTrue(tc, open_req);
  rmf_file_info_delete(signed_info);

  // 7. Pre-seeded syntactically invalid cached entry -> evicts & falls back to open_req = true
  const char *bad_apx = "APX/1.3\nN\"BadCached\"\nINVALID_SYNTAX\n";
  uint32_t const bad_len = (uint32_t)strlen(bad_apx);
  uint8_t bad_hash[RMF_SHA256_SIZE];
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)bad_apx, bad_len, bad_hash));
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_node_cache_store_text(
      apx_server_get_node_cache(server), "BadCached", bad_hash, (uint8_t const *)bad_apx, bad_len));
  rmf_file_info_t *bad_info = rmf_file_info_make_fixed_with_digest(
    "BadCached.apx", bad_len, APX_DEFINITION_ADDRESS_START, RMF_DIGEST_TYPE_SHA256, bad_hash);
  open_req = false;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_init_node_from_file_info(mgr4, bad_info, &open_req));
  CuAssertTrue(tc, open_req);
  adt_str_t *bad_path = make_cache_file_path(root, "BadCached", bad_hash);
  CuAssertTrue(tc, !cutil_file_exists(adt_str_cstr(bad_path)));
  adt_str_delete(bad_path);
  rmf_file_info_delete(bad_info);

  rmf_file_info_delete(sha256_info);
  apx_server_test_connection_delete(conn4);
  apx_server_test_connection_delete(conn3);
  apx_server_test_connection_delete(conn2);
  apx_server_test_connection_delete(conn);
  apx_server_delete(server);
  remove_dir_recursive(root);
}
