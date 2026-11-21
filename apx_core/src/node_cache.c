/*****************************************************************************
 * \file      node_cache.c
 * \author    Conny Gustafsson
 * \date      2018-08-03
 * \brief     Server-side disk cache for APX node definitions (Level 1 & 2)
 *
 * Copyright (c) 2018-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifndef _WIN32
# ifndef _DEFAULT_SOURCE
#  define _DEFAULT_SOURCE
# endif
#endif
#include "apx/node_cache.h"
#include "adt_ary.h"
#include "adt_hash.h"
#include "adt_str.h"
#include "apx/cfg.h"
#include "apx/crypto.h"
#include "apx/node_cache_codec.h"
#include "apx/node_instance.h"
#include "apx/util.h"
#include "fileutil.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
# include <Windows.h>
# include <process.h>
# define APX_GETPID() ((unsigned long)_getpid())
#else
# include <fcntl.h>
# include <sys/stat.h>
# include <unistd.h>
# define APX_GETPID() ((unsigned long)getpid())
#endif
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define SHA256_HEX_LEN ((size_t)RMF_SHA256_SIZE * 2u)
#define TEXT_EXT ".apx"
#define TEXT_EXT_LEN 4u
#define BIN_EXT ".apxc"
#define BIN_EXT_LEN 5u
// Fixed suffix lengths after node_name: '_' + 64 hex chars + ext
#define TEXT_CACHE_SUFFIX_LEN (1u + SHA256_HEX_LEN + TEXT_EXT_LEN)
#define BIN_CACHE_SUFFIX_LEN (1u + SHA256_HEX_LEN + BIN_EXT_LEN)
#define KEY_SUFFIX_LEN (1u + SHA256_HEX_LEN)
#define FORMAT_VERSION_FILENAME ".format_version"

typedef struct prune_version_tag
{
  char *key; // "<node_name>_<64-hex>" (no extension)
  uint64_t text_size;
  uint64_t bin_size;
  int64_t text_mtime;
  int64_t bin_mtime;
  int64_t effective_mtime;
  bool has_text;
  bool has_bin;
  bool matches_node;
  bool is_keep;
  bool evicted;
} prune_version_t;

typedef struct prune_scan_ctx_tag
{
  char const *node_name;
  size_t node_name_len;
  char const *keep_key;
  bool scanning_bin;
  adt_hash_t version_map; // key -> prune_version_t* (non-owning)
  adt_ary_t versions;     // owns prune_version_t*
  uint64_t total_bytes;
} prune_scan_ctx_t;

typedef struct startup_sweep_ctx_tag
{
  char const *dir_path;
  bool is_bin_dir;
  bool clear_all_bin;
  bool read_only;
  int64_t max_mtime_sec;
  uint64_t cleared_bin_count;
} startup_sweep_ctx_t;

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void set_warning(char *buf, size_t buf_size, char const *fmt, ...);
static bool is_safe_node_name(char const *node_name);
static void sha256_to_hex(uint8_t const sha256[RMF_SHA256_SIZE], char out_hex[SHA256_HEX_LEN + 1u]);
static bool hex_to_sha256(char const *hex, uint8_t out_sha256[RMF_SHA256_SIZE]);
static bool is_lowercase_hex_str(char const *s, size_t len);
static bool is_tmp_filename(char const *name);
static bool parse_cache_filename(
  char const *name, char const *ext, size_t ext_len, size_t *out_key_len, size_t *out_node_len);
static adt_str_t *build_cache_key(char const *node_name, uint8_t const sha256[RMF_SHA256_SIZE]);
static adt_str_t *build_cache_filename(char const *node_name, uint8_t const sha256[RMF_SHA256_SIZE], char const *ext);
static adt_str_t *build_cache_path(
  char const *dir_path, char const *node_name, uint8_t const sha256[RMF_SHA256_SIZE], char const *ext);
static char *dup_cstr(char const *src);
static bool on_dir_access_probe(void *arg, const cutil_dir_entry_t *entry);
static bool probe_dir_writable(apx_node_cache_t *self, char const *dir_path, int *out_errno);
static bool read_lock_holder_pid(char const *lock_path, char *out_pid, size_t out_pid_size);
static int read_binary_cache_file(
  char const *path, size_t max_size, uint8_t **out_buf, size_t *out_size, int64_t *out_mtime_sec);
static apx_error_t write_file_atomic(apx_node_cache_t *self, char const *final_path, uint8_t const *data, size_t size);
static int64_t current_wall_sec(apx_node_cache_t const *self);
static int64_t current_mono_sec(void);
static int64_t get_effective_now_sec(apx_node_cache_t *self);
static void maybe_apply_high_water_mtime_on_store(apx_node_cache_t *self, char const *file_path);
static void maybe_refresh_mtime_on_hit(apx_node_cache_t *self, char const *file_path, int64_t file_mtime_sec);
static bool check_format_version_matches(char const *bin_dir);
static bool on_startup_scan_entry(void *arg, const cutil_dir_entry_t *entry);
static void prune_version_vdelete(void *arg);
static bool on_prune_dir_entry(void *arg, const cutil_dir_entry_t *entry);
static int compare_prune_versions(const void *a, const void *b);
static void prune_cache(apx_node_cache_t *self, char const *node_name, char const *keep_key);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_node_cache_config_init(apx_node_cache_config_t *cfg)
{
  if (cfg != NULL)
  {
    cfg->level = APX_CACHE_LEVEL_OFF;
    cfg->max_versions_per_node = APX_CACHE_DEFAULT_MAX_VERSIONS_PER_NODE;
    cfg->max_size_mb = APX_CACHE_DEFAULT_MAX_SIZE_MB;
  }
}

void apx_node_cache_create(apx_node_cache_t *self)
{
  if (self != NULL)
  {
    self->level = APX_CACHE_LEVEL_OFF;
    self->max_versions_per_node = APX_CACHE_DEFAULT_MAX_VERSIONS_PER_NODE;
    self->max_size_mb = APX_CACHE_DEFAULT_MAX_SIZE_MB;
    self->max_size_bytes = (uint64_t)APX_CACHE_DEFAULT_MAX_SIZE_MB * 1024ULL * 1024ULL;
    self->root_dir = NULL;
    self->text_dir = NULL;
    self->bin_dir = NULL;
    cutil_file_lock_init(&self->lock_file);
    MUTEX_INIT(self->lock);
    MUTEX_INIT(self->prune_lock);
    self->tmp_seq = 0u;
    self->logical_base_sec = 0;
    self->boot_mono_sec = 0;
#ifdef UNIT_TEST
    self->wall_clock_offset_sec = 0;
#endif
    memset(&self->stats, 0, sizeof(self->stats));
    self->initialized = false;
    self->read_only = false;
  }
}

void apx_node_cache_destroy(apx_node_cache_t *self)
{
  if (self != NULL)
  {
    cutil_file_lock_release(&self->lock_file);
    free(self->root_dir);
    self->root_dir = NULL;
    free(self->text_dir);
    self->text_dir = NULL;
    free(self->bin_dir);
    self->bin_dir = NULL;
    MUTEX_DESTROY(self->lock);
    MUTEX_DESTROY(self->prune_lock);
    self->level = APX_CACHE_LEVEL_OFF;
    self->initialized = false;
    self->read_only = false;
  }
}

apx_node_cache_t *apx_node_cache_new(void)
{
  apx_node_cache_t *self = (apx_node_cache_t *)malloc(sizeof(apx_node_cache_t));
  if (self != NULL)
  {
    apx_node_cache_create(self);
  }
  return self;
}

void apx_node_cache_delete(apx_node_cache_t *self)
{
  if (self != NULL)
  {
    apx_node_cache_destroy(self);
    free(self);
  }
}

apx_error_t apx_node_cache_configure(apx_node_cache_t *self, apx_node_cache_config_t const *cfg, char const *root_path,
  char *warning_buf, size_t warning_buf_size)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (warning_buf != NULL && warning_buf_size > 0u)
  {
    warning_buf[0] = '\0';
  }

  cutil_file_lock_release(&self->lock_file);
  self->level = APX_CACHE_LEVEL_OFF;
  self->initialized = false;
  self->read_only = false;
  free(self->root_dir);
  self->root_dir = NULL;
  free(self->text_dir);
  self->text_dir = NULL;
  free(self->bin_dir);
  self->bin_dir = NULL;

  if (cfg == NULL || cfg->level == APX_CACHE_LEVEL_OFF)
  {
    return APX_NO_ERROR;
  }

  if (cfg->level != APX_CACHE_LEVEL_TEXT && cfg->level != APX_CACHE_LEVEL_BINARY)
  {
    set_warning(warning_buf, warning_buf_size, "Invalid cache level %d; disabling cache", (int)cfg->level);
    return APX_NO_ERROR;
  }

  if (root_path == NULL || root_path[0] == '\0')
  {
    set_warning(warning_buf, warning_buf_size,
      "'apx-server.cache.path' must be set when cache level is %d; disabling cache", (int)cfg->level);
    return APX_NO_ERROR;
  }

  adt_str_t *text_path = cutil_path_join(root_path, "text");
  if (text_path == NULL)
  {
    return APX_MEM_ERROR;
  }

  char const *text_dir_cstr = adt_str_cstr(text_path);
  if (!cutil_is_dir(text_dir_cstr))
  {
    if (cutil_mkdir_p(text_dir_cstr) != 0)
    {
      int saved_errno = errno;
      set_warning(warning_buf, warning_buf_size,
        "Cannot create cache directory '%s': %s; disabling cache (check directory ownership/permissions)",
        text_dir_cstr, strerror(saved_errno));
      adt_str_delete(text_path);
      return APX_NO_ERROR;
    }
  }

  if (cutil_dir_for_each(text_dir_cstr, on_dir_access_probe, NULL) != 0)
  {
    int saved_errno = errno;
    set_warning(warning_buf, warning_buf_size,
      "Cannot access cache directory '%s': %s; disabling cache (check directory ownership/permissions)", text_dir_cstr,
      strerror(saved_errno));
    adt_str_delete(text_path);
    return APX_NO_ERROR;
  }

  apx_cache_level_t effective_level = cfg->level;
  adt_str_t *bin_path = NULL;
  if (effective_level == APX_CACHE_LEVEL_BINARY)
  {
    bin_path = cutil_path_join(root_path, "bin");
    if (bin_path == NULL)
    {
      adt_str_delete(text_path);
      return APX_MEM_ERROR;
    }
    char const *bin_dir_cstr = adt_str_cstr(bin_path);
    if (!cutil_is_dir(bin_dir_cstr))
    {
      if (cutil_mkdir_p(bin_dir_cstr) != 0)
      {
        int saved_errno = errno;
        set_warning(warning_buf, warning_buf_size,
          "Cannot create binary cache directory '%s': %s; falling back to level 1 text cache", bin_dir_cstr,
          strerror(saved_errno));
        adt_str_delete(bin_path);
        bin_path = NULL;
        effective_level = APX_CACHE_LEVEL_TEXT;
      }
    }
    if (bin_path != NULL && cutil_dir_for_each(adt_str_cstr(bin_path), on_dir_access_probe, NULL) != 0)
    {
      int saved_errno = errno;
      set_warning(warning_buf, warning_buf_size,
        "Cannot access binary cache directory '%s': %s; falling back to level 1 text cache", adt_str_cstr(bin_path),
        strerror(saved_errno));
      adt_str_delete(bin_path);
      bin_path = NULL;
      effective_level = APX_CACHE_LEVEL_TEXT;
    }
  }

  int probe_errno = 0;
  bool text_writable = probe_dir_writable(self, text_dir_cstr, &probe_errno);
  bool bin_writable = true;
  int bin_probe_errno = 0;
  if (bin_path != NULL)
  {
    bin_writable = probe_dir_writable(self, adt_str_cstr(bin_path), &bin_probe_errno);
  }
  if (!text_writable || !bin_writable)
  {
    self->read_only = true;
    char const *ro_dir = !text_writable ? text_dir_cstr : adt_str_cstr(bin_path);
    int ro_errno = !text_writable ? probe_errno : bin_probe_errno;
    set_warning(warning_buf, warning_buf_size,
      "Cache directory '%s' is not writable (%s); running cache in read-only mode -- new definitions will not be "
      "cached",
      ro_dir, strerror(ro_errno));
  }
  else
  {
    adt_str_t *lock_path = cutil_path_join(root_path, ".lock");
    if (lock_path == NULL)
    {
      adt_str_delete(bin_path);
      adt_str_delete(text_path);
      return APX_MEM_ERROR;
    }
    char const *lock_path_cstr = adt_str_cstr(lock_path);
    int lock_rc = cutil_file_lock_try_acquire(&self->lock_file, lock_path_cstr);
    if (lock_rc == 1)
    {
      char pid_str[32];
      if (read_lock_holder_pid(lock_path_cstr, pid_str, sizeof(pid_str)))
      {
        set_warning(warning_buf, warning_buf_size,
          "Cache directory '%s' is already locked by another apx-server (PID %s); disabling cache -- configure a "
          "unique 'apx-server.cache.path' per server",
          root_path, pid_str);
      }
      else
      {
        set_warning(warning_buf, warning_buf_size,
          "Cache directory '%s' is already locked by another apx-server; disabling cache -- configure a unique "
          "'apx-server.cache.path' per server",
          root_path);
      }
      adt_str_delete(lock_path);
      adt_str_delete(bin_path);
      adt_str_delete(text_path);
      return APX_NO_ERROR;
    }
    if (lock_rc != 0)
    {
      int saved_errno = errno;
      set_warning(warning_buf, warning_buf_size,
        "Cannot lock cache directory '%s' (%s); disabling cache (check directory ownership/permissions)", root_path,
        strerror(saved_errno));
      adt_str_delete(lock_path);
      adt_str_delete(bin_path);
      adt_str_delete(text_path);
      return APX_NO_ERROR;
    }
    adt_str_delete(lock_path);
  }

  char *root_copy = dup_cstr(root_path);
  char *text_copy = dup_cstr(text_dir_cstr);
  char *bin_copy = (bin_path != NULL) ? dup_cstr(adt_str_cstr(bin_path)) : NULL;
  adt_str_delete(text_path);
  adt_str_delete(bin_path);
  if (root_copy == NULL || text_copy == NULL || (effective_level == APX_CACHE_LEVEL_BINARY && bin_copy == NULL))
  {
    cutil_file_lock_release(&self->lock_file);
    free(root_copy);
    free(text_copy);
    free(bin_copy);
    self->read_only = false;
    return APX_MEM_ERROR;
  }

  self->root_dir = root_copy;
  self->text_dir = text_copy;
  self->bin_dir = bin_copy;
  self->level = effective_level;
  self->max_versions_per_node = cfg->max_versions_per_node;
  self->max_size_mb = cfg->max_size_mb;
  self->max_size_bytes = (uint64_t)cfg->max_size_mb * 1024ULL * 1024ULL;
  self->initialized = true;

  // Startup hygiene pass: sweep orphan *.tmp.*, handle .format_version upgrade,
  // pre-validate bin/*.apxc (warming OS page cache), and find max_existing_mtime.
  int64_t max_existing_mtime = 0;
  startup_sweep_ctx_t text_sweep;
  memset(&text_sweep, 0, sizeof(text_sweep));
  text_sweep.dir_path = self->text_dir;
  text_sweep.is_bin_dir = false;
  text_sweep.clear_all_bin = false;
  text_sweep.read_only = self->read_only;
  (void)cutil_dir_for_each(self->text_dir, on_startup_scan_entry, &text_sweep);
  if (text_sweep.max_mtime_sec > max_existing_mtime)
  {
    max_existing_mtime = text_sweep.max_mtime_sec;
  }

  if (self->bin_dir != NULL)
  {
    bool format_changed = false;
    if (!self->read_only)
    {
      format_changed = !check_format_version_matches(self->bin_dir);
    }
    startup_sweep_ctx_t bin_sweep;
    memset(&bin_sweep, 0, sizeof(bin_sweep));
    bin_sweep.dir_path = self->bin_dir;
    bin_sweep.is_bin_dir = true;
    bin_sweep.clear_all_bin = format_changed;
    bin_sweep.read_only = self->read_only;
    (void)cutil_dir_for_each(self->bin_dir, on_startup_scan_entry, &bin_sweep);
    if (bin_sweep.max_mtime_sec > max_existing_mtime)
    {
      max_existing_mtime = bin_sweep.max_mtime_sec;
    }

    if (format_changed && !self->read_only)
    {
      adt_str_t *ver_path = cutil_path_join(self->bin_dir, FORMAT_VERSION_FILENAME);
      if (ver_path != NULL)
      {
        char const *stamp = apx_node_cache_format_version_string();
        (void)write_file_atomic(self, adt_str_cstr(ver_path), (uint8_t const *)stamp, strlen(stamp));
        adt_str_delete(ver_path);
      }
      if (bin_sweep.cleared_bin_count > 0u && warning_buf != NULL && warning_buf_size > 0u && warning_buf[0] == '\0')
      {
        set_warning(warning_buf, warning_buf_size,
          "Binary cache format changed; cleared %llu incompatible .apxc file(s) (Level 1 text cache preserved)",
          (unsigned long long)bin_sweep.cleared_bin_count);
      }
    }
    else if (!format_changed && bin_sweep.cleared_bin_count > 0u && warning_buf != NULL && warning_buf_size > 0u &&
      warning_buf[0] == '\0')
    {
      set_warning(warning_buf, warning_buf_size, "Startup binary cache pre-check cleared %llu invalid .apxc file(s)",
        (unsigned long long)bin_sweep.cleared_bin_count);
    }
  }

  // Initialize monotonic high-water mtime clock
  int64_t wall_now = current_wall_sec(self);
  self->logical_base_sec = (max_existing_mtime >= wall_now) ? (max_existing_mtime + 1) : wall_now;
  self->boot_mono_sec = current_mono_sec();
  return APX_NO_ERROR;
}

bool apx_node_cache_is_text_enabled(apx_node_cache_t const *self)
{
  if (self != NULL)
  {
    return self->initialized && (self->level >= APX_CACHE_LEVEL_TEXT);
  }
  return false;
}

bool apx_node_cache_is_binary_enabled(apx_node_cache_t const *self)
{
  if (self != NULL)
  {
    return self->initialized && (self->level >= APX_CACHE_LEVEL_BINARY) && (self->bin_dir != NULL);
  }
  return false;
}

bool apx_node_cache_is_read_only(apx_node_cache_t const *self)
{
  if (self != NULL)
  {
    return self->read_only;
  }
  return false;
}

apx_cache_level_t apx_node_cache_get_level(apx_node_cache_t const *self)
{
  if (self != NULL)
  {
    return self->level;
  }
  return APX_CACHE_LEVEL_OFF;
}

uint32_t apx_node_cache_get_max_versions_per_node(apx_node_cache_t const *self)
{
  if (self != NULL)
  {
    return self->max_versions_per_node;
  }
  return 0u;
}

uint32_t apx_node_cache_get_max_size_mb(apx_node_cache_t const *self)
{
  if (self != NULL)
  {
    return self->max_size_mb;
  }
  return 0u;
}

char const *apx_node_cache_get_root_path(apx_node_cache_t const *self)
{
  if (self != NULL && self->initialized && self->root_dir != NULL)
  {
    return self->root_dir;
  }
  return "";
}

apx_error_t apx_node_cache_lookup_text(apx_node_cache_t *self, char const *node_name,
  uint8_t const sha256[RMF_SHA256_SIZE], uint32_t expected_size, uint8_t **out_data)
{
  if (self == NULL || node_name == NULL || sha256 == NULL || out_data == NULL || expected_size == 0u)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  *out_data = NULL;
  if (!apx_node_cache_is_text_enabled(self))
  {
    return APX_FILE_NOT_FOUND_ERROR;
  }
  if (!is_safe_node_name(node_name))
  {
    MUTEX_LOCK(self->lock);
    self->stats.text_misses++;
    MUTEX_UNLOCK(self->lock);
    return APX_FILE_NOT_FOUND_ERROR;
  }

  adt_str_t *file_path = build_cache_path(self->text_dir, node_name, sha256, TEXT_EXT);
  if (file_path == NULL)
  {
    return APX_MEM_ERROR;
  }

  uint8_t *buf = (uint8_t *)malloc((size_t)expected_size + 1u);
  if (buf == NULL)
  {
    adt_str_delete(file_path);
    return APX_MEM_ERROR;
  }

  size_t actual_size = 0u;
  int64_t mtime_sec = 0;
  char const *path_cstr = adt_str_cstr(file_path);
  int rc = cutil_read_regular_file(path_cstr, buf, (size_t)expected_size, &actual_size, &mtime_sec);
  if (rc == -1)
  {
    int saved_errno = errno;
    free(buf);
    adt_str_delete(file_path);
    MUTEX_LOCK(self->lock);
    if (saved_errno == ENOENT)
    {
      self->stats.text_misses++;
    }
    else
    {
      self->stats.text_corruptions++;
    }
    MUTEX_UNLOCK(self->lock);
    return (saved_errno == ENOENT) ? APX_FILE_NOT_FOUND_ERROR : APX_CHECKSUM_ERROR;
  }
  if (rc == -2)
  {
    free(buf);
    if (!self->read_only)
    {
      (void)remove(path_cstr);
    }
    adt_str_delete(file_path);
    MUTEX_LOCK(self->lock);
    self->stats.text_corruptions++;
    MUTEX_UNLOCK(self->lock);
    return APX_LENGTH_ERROR;
  }

  uint8_t actual_hash[RMF_SHA256_SIZE];
  apx_error_t hash_rc = apx_crypto_sha256(buf, (size_t)expected_size, actual_hash);
  if (hash_rc != APX_NO_ERROR || memcmp(actual_hash, sha256, RMF_SHA256_SIZE) != 0)
  {
    free(buf);
    if (!self->read_only)
    {
      (void)remove(path_cstr);
    }
    adt_str_delete(file_path);
    MUTEX_LOCK(self->lock);
    self->stats.text_corruptions++;
    MUTEX_UNLOCK(self->lock);
    return APX_CHECKSUM_ERROR;
  }

  buf[expected_size] = '\0';
  maybe_refresh_mtime_on_hit(self, path_cstr, mtime_sec);

  adt_str_delete(file_path);
  MUTEX_LOCK(self->lock);
  self->stats.text_hits++;
  MUTEX_UNLOCK(self->lock);
  *out_data = buf;
  return APX_NO_ERROR;
}

apx_error_t apx_node_cache_store_text(apx_node_cache_t *self, char const *node_name,
  uint8_t const sha256[RMF_SHA256_SIZE], uint8_t const *data, uint32_t size)
{
  if (self == NULL || node_name == NULL || sha256 == NULL || data == NULL || size == 0u)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (!apx_node_cache_is_text_enabled(self) || self->read_only || !is_safe_node_name(node_name))
  {
    return APX_NO_ERROR;
  }

  adt_str_t *key = build_cache_key(node_name, sha256);
  adt_str_t *filename = build_cache_filename(node_name, sha256, TEXT_EXT);
  if (key == NULL || filename == NULL)
  {
    adt_str_delete(key);
    adt_str_delete(filename);
    return APX_MEM_ERROR;
  }
  adt_str_t *final_path = cutil_path_join(self->text_dir, adt_str_cstr(filename));
  adt_str_delete(filename);
  if (final_path == NULL)
  {
    adt_str_delete(key);
    return APX_MEM_ERROR;
  }

  char const *final_cstr = adt_str_cstr(final_path);
  if (cutil_file_exists(final_cstr))
  {
    adt_str_delete(final_path);
    adt_str_delete(key);
    return APX_NO_ERROR;
  }

  apx_error_t rc = write_file_atomic(self, final_cstr, data, (size_t)size);
  if (rc != APX_NO_ERROR)
  {
    adt_str_delete(final_path);
    adt_str_delete(key);
    return rc;
  }

  maybe_apply_high_water_mtime_on_store(self, final_cstr);
  adt_str_delete(final_path);

  MUTEX_LOCK(self->lock);
  self->stats.text_stores++;
  MUTEX_UNLOCK(self->lock);

  if (self->max_versions_per_node > 0u || self->max_size_bytes > 0u)
  {
    prune_cache(self, node_name, adt_str_cstr(key));
  }

  adt_str_delete(key);
  return APX_NO_ERROR;
}

void apx_node_cache_invalidate_text(
  apx_node_cache_t *self, char const *node_name, uint8_t const sha256[RMF_SHA256_SIZE])
{
  if (self == NULL || node_name == NULL || sha256 == NULL)
  {
    return;
  }
  if (!apx_node_cache_is_text_enabled(self) || self->read_only || !is_safe_node_name(node_name))
  {
    return;
  }
  adt_str_t *file_path = build_cache_path(self->text_dir, node_name, sha256, TEXT_EXT);
  if (file_path != NULL)
  {
    (void)remove(adt_str_cstr(file_path));
    adt_str_delete(file_path);
  }
}

apx_error_t apx_node_cache_lookup_binary(apx_node_cache_t *self, char const *node_name,
  uint8_t const sha256[RMF_SHA256_SIZE], uint32_t expected_logical_size, bool load_vm_programs,
  apx_node_instance_t *node_instance, uint8_t **out_embedded_def_text)
{
  if (out_embedded_def_text != NULL)
  {
    *out_embedded_def_text = NULL;
  }
  if (self == NULL || node_name == NULL || sha256 == NULL || node_instance == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (!apx_node_cache_is_binary_enabled(self))
  {
    return APX_FILE_NOT_FOUND_ERROR;
  }
  if (!is_safe_node_name(node_name))
  {
    MUTEX_LOCK(self->lock);
    self->stats.bin_misses++;
    MUTEX_UNLOCK(self->lock);
    return APX_FILE_NOT_FOUND_ERROR;
  }

  adt_str_t *file_path = build_cache_path(self->bin_dir, node_name, sha256, BIN_EXT);
  if (file_path == NULL)
  {
    return APX_MEM_ERROR;
  }
  char const *path_cstr = adt_str_cstr(file_path);

  uint8_t *buf = NULL;
  size_t buf_size = 0u;
  int64_t mtime_sec = 0;
  int read_rc = read_binary_cache_file(path_cstr, (size_t)APX_MAX_FILE_SIZE, &buf, &buf_size, &mtime_sec);
  if (read_rc == -1)
  {
    int saved_errno = errno;
    adt_str_delete(file_path);
    MUTEX_LOCK(self->lock);
    if (saved_errno == ENOENT)
    {
      self->stats.bin_misses++;
    }
    else
    {
      self->stats.bin_corruptions++;
    }
    MUTEX_UNLOCK(self->lock);
    return (saved_errno == ENOENT) ? APX_FILE_NOT_FOUND_ERROR : APX_CHECKSUM_ERROR;
  }
  if (read_rc == -2)
  {
    if (!self->read_only)
    {
      (void)remove(path_cstr);
    }
    adt_str_delete(file_path);
    MUTEX_LOCK(self->lock);
    self->stats.bin_corruptions++;
    MUTEX_UNLOCK(self->lock);
    return APX_LENGTH_ERROR;
  }
  if (read_rc == -3)
  {
    adt_str_delete(file_path);
    return APX_MEM_ERROR;
  }

  apx_error_t rc = apx_node_cache_decode_binary(
    buf, buf_size, node_name, sha256, expected_logical_size, load_vm_programs, node_instance, out_embedded_def_text);
  free(buf);

  if (rc != APX_NO_ERROR)
  {
    if (rc != APX_MEM_ERROR)
    {
      if (!self->read_only)
      {
        (void)remove(path_cstr);
      }
      MUTEX_LOCK(self->lock);
      if (rc == APX_VERSION_ERROR)
      {
        self->stats.bin_version_mismatches++;
      }
      else
      {
        self->stats.bin_corruptions++;
      }
      MUTEX_UNLOCK(self->lock);
    }
    adt_str_delete(file_path);
    return rc;
  }

  maybe_refresh_mtime_on_hit(self, path_cstr, mtime_sec);
  adt_str_delete(file_path);
  MUTEX_LOCK(self->lock);
  self->stats.bin_hits++;
  MUTEX_UNLOCK(self->lock);
  return APX_NO_ERROR;
}

apx_error_t apx_node_cache_store_binary(apx_node_cache_t *self, char const *node_name,
  uint8_t const sha256[RMF_SHA256_SIZE], uint32_t logical_def_size, apx_node_instance_t const *node_instance)
{
  if (self == NULL || node_name == NULL || sha256 == NULL || node_instance == NULL || logical_def_size == 0u)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (!apx_node_cache_is_binary_enabled(self) || self->read_only || !is_safe_node_name(node_name))
  {
    return APX_NO_ERROR;
  }

  adt_str_t *key = build_cache_key(node_name, sha256);
  adt_str_t *filename = build_cache_filename(node_name, sha256, BIN_EXT);
  if (key == NULL || filename == NULL)
  {
    adt_str_delete(key);
    adt_str_delete(filename);
    return APX_MEM_ERROR;
  }
  adt_str_t *final_path = cutil_path_join(self->bin_dir, adt_str_cstr(filename));
  adt_str_delete(filename);
  if (final_path == NULL)
  {
    adt_str_delete(key);
    return APX_MEM_ERROR;
  }

  char const *final_cstr = adt_str_cstr(final_path);
  if (cutil_file_exists(final_cstr))
  {
    adt_str_delete(final_path);
    adt_str_delete(key);
    return APX_NO_ERROR;
  }

  uint8_t *encoded = NULL;
  size_t encoded_size = 0u;
  apx_error_t rc =
    apx_node_cache_encode_binary(node_instance, sha256, logical_def_size, NULL, false, true, &encoded, &encoded_size);
  if (rc != APX_NO_ERROR)
  {
    adt_str_delete(final_path);
    adt_str_delete(key);
    return rc;
  }

  rc = write_file_atomic(self, final_cstr, encoded, encoded_size);
  free(encoded);
  if (rc != APX_NO_ERROR)
  {
    adt_str_delete(final_path);
    adt_str_delete(key);
    return rc;
  }

  maybe_apply_high_water_mtime_on_store(self, final_cstr);
  adt_str_delete(final_path);

  MUTEX_LOCK(self->lock);
  self->stats.bin_stores++;
  MUTEX_UNLOCK(self->lock);

  if (self->max_versions_per_node > 0u || self->max_size_bytes > 0u)
  {
    prune_cache(self, node_name, adt_str_cstr(key));
  }

  adt_str_delete(key);
  return APX_NO_ERROR;
}

void apx_node_cache_invalidate_binary(
  apx_node_cache_t *self, char const *node_name, uint8_t const sha256[RMF_SHA256_SIZE])
{
  if (self == NULL || node_name == NULL || sha256 == NULL)
  {
    return;
  }
  if (!apx_node_cache_is_binary_enabled(self) || self->read_only || !is_safe_node_name(node_name))
  {
    return;
  }
  adt_str_t *file_path = build_cache_path(self->bin_dir, node_name, sha256, BIN_EXT);
  if (file_path != NULL)
  {
    (void)remove(adt_str_cstr(file_path));
    adt_str_delete(file_path);
  }
}

void apx_node_cache_get_stats(apx_node_cache_t *self, apx_node_cache_stats_t *out_stats)
{
  if (self != NULL && out_stats != NULL)
  {
    MUTEX_LOCK(self->lock);
    *out_stats = self->stats;
    MUTEX_UNLOCK(self->lock);
  }
}

#ifdef UNIT_TEST
void apx_node_cache_set_max_size_bytes(apx_node_cache_t *self, uint64_t max_size_bytes)
{
  if (self != NULL)
  {
    self->max_size_bytes = max_size_bytes;
  }
}

void apx_node_cache_set_wall_clock_offset_sec(apx_node_cache_t *self, int64_t offset_sec)
{
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
    self->wall_clock_offset_sec = offset_sec;
    MUTEX_UNLOCK(self->lock);
  }
}
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void set_warning(char *buf, size_t buf_size, char const *fmt, ...)
{
  if (buf == NULL || buf_size == 0u || fmt == NULL)
  {
    return;
  }
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, buf_size, fmt, ap);
  va_end(ap);
}

static bool is_safe_node_name(char const *node_name)
{
  if (node_name == NULL || node_name[0] == '\0' || node_name[0] == '.')
  {
    return false;
  }
  size_t len = 0u;
  char prev = '\0';
  for (char const *p = node_name; *p != '\0'; p++)
  {
    char c = *p;
    bool ok =
      (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
    if (!ok)
    {
      return false;
    }
    if (c == '.' && prev == '.')
    {
      return false;
    }
    prev = c;
    len++;
    if (len > RMF_MAX_FILE_NAME_SIZE)
    {
      return false;
    }
  }
  return true;
}

static void sha256_to_hex(uint8_t const sha256[RMF_SHA256_SIZE], char out_hex[SHA256_HEX_LEN + 1u])
{
  static char const m_hex_digits[] = "0123456789abcdef";
  for (size_t i = 0u; i < RMF_SHA256_SIZE; i++)
  {
    out_hex[i * 2u] = m_hex_digits[(sha256[i] >> 4u) & 0x0Fu];
    out_hex[i * 2u + 1u] = m_hex_digits[sha256[i] & 0x0Fu];
  }
  out_hex[SHA256_HEX_LEN] = '\0';
}

static bool hex_to_sha256(char const *hex, uint8_t out_sha256[RMF_SHA256_SIZE])
{
  if (hex == NULL || out_sha256 == NULL)
  {
    return false;
  }
  for (size_t i = 0u; i < RMF_SHA256_SIZE; i++)
  {
    uint8_t nibbles[2];
    for (size_t j = 0u; j < 2u; j++)
    {
      char c = hex[i * 2u + j];
      if (c >= '0' && c <= '9')
      {
        nibbles[j] = (uint8_t)(c - '0');
      }
      else if (c >= 'a' && c <= 'f')
      {
        nibbles[j] = (uint8_t)(10 + (c - 'a'));
      }
      else
      {
        return false;
      }
    }
    out_sha256[i] = (uint8_t)((nibbles[0] << 4u) | nibbles[1]);
  }
  return true;
}

static bool is_lowercase_hex_str(char const *s, size_t len)
{
  for (size_t i = 0u; i < len; i++)
  {
    char c = s[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
    {
      return false;
    }
  }
  return true;
}

static bool is_tmp_filename(char const *name)
{
  if (name == NULL)
  {
    return false;
  }
  return strstr(name, ".tmp.") != NULL;
}

static bool parse_cache_filename(
  char const *name, char const *ext, size_t ext_len, size_t *out_key_len, size_t *out_node_len)
{
  if (name == NULL || ext == NULL)
  {
    return false;
  }
  size_t len = strlen(name);
  size_t suffix_len = KEY_SUFFIX_LEN + ext_len;
  if (len <= suffix_len)
  {
    return false;
  }
  if (memcmp(name + len - ext_len, ext, ext_len) != 0)
  {
    return false;
  }
  size_t node_len = len - suffix_len;
  if (name[node_len] != '_')
  {
    return false;
  }
  if (!is_lowercase_hex_str(name + node_len + 1u, SHA256_HEX_LEN))
  {
    return false;
  }
  if (out_key_len != NULL)
  {
    *out_key_len = len - ext_len;
  }
  if (out_node_len != NULL)
  {
    *out_node_len = node_len;
  }
  return true;
}

static adt_str_t *build_cache_key(char const *node_name, uint8_t const sha256[RMF_SHA256_SIZE])
{
  char hex[SHA256_HEX_LEN + 1u];
  sha256_to_hex(sha256, hex);
  adt_str_t *key = adt_str_new_cstr(node_name);
  if (key == NULL)
  {
    return NULL;
  }
  if (adt_str_push(key, '_') != ADT_NO_ERROR || adt_str_append_cstr(key, hex) != ADT_NO_ERROR)
  {
    adt_str_delete(key);
    return NULL;
  }
  return key;
}

static adt_str_t *build_cache_filename(char const *node_name, uint8_t const sha256[RMF_SHA256_SIZE], char const *ext)
{
  adt_str_t *filename = build_cache_key(node_name, sha256);
  if (filename == NULL)
  {
    return NULL;
  }
  if (adt_str_append_cstr(filename, ext) != ADT_NO_ERROR)
  {
    adt_str_delete(filename);
    return NULL;
  }
  return filename;
}

static adt_str_t *build_cache_path(
  char const *dir_path, char const *node_name, uint8_t const sha256[RMF_SHA256_SIZE], char const *ext)
{
  adt_str_t *filename = build_cache_filename(node_name, sha256, ext);
  if (filename == NULL)
  {
    return NULL;
  }
  adt_str_t *full_path = cutil_path_join(dir_path, adt_str_cstr(filename));
  adt_str_delete(filename);
  return full_path;
}

static char *dup_cstr(char const *src)
{
  if (src == NULL)
  {
    return NULL;
  }
  size_t len = strlen(src);
  char *copy = (char *)malloc(len + 1u);
  if (copy != NULL)
  {
    memcpy(copy, src, len + 1u);
  }
  return copy;
}

static bool on_dir_access_probe(void *arg, const cutil_dir_entry_t *entry)
{
  (void)arg;
  (void)entry;
  return false; // Stopping on first entry is enough to verify directory readability
}

static bool probe_dir_writable(apx_node_cache_t *self, char const *dir_path, int *out_errno)
{
  MUTEX_LOCK(self->lock);
  uint64_t seq = ++self->tmp_seq;
  MUTEX_UNLOCK(self->lock);

  char probe_name[64];
  snprintf(probe_name, sizeof(probe_name), ".apx_cache_probe.%lu.%llu", APX_GETPID(), (unsigned long long)seq);
  adt_str_t *probe_path = cutil_path_join(dir_path, probe_name);
  if (probe_path == NULL)
  {
    if (out_errno != NULL)
    {
      *out_errno = ENOMEM;
    }
    return false;
  }

  FILE *fh = fopen(adt_str_cstr(probe_path), "wb");
  if (fh == NULL)
  {
    if (out_errno != NULL)
    {
      *out_errno = errno;
    }
    adt_str_delete(probe_path);
    return false;
  }
  fclose(fh);
  (void)remove(adt_str_cstr(probe_path));
  adt_str_delete(probe_path);
  return true;
}

static bool read_lock_holder_pid(char const *lock_path, char *out_pid, size_t out_pid_size)
{
  if (lock_path == NULL || out_pid == NULL || out_pid_size < 2u)
  {
    return false;
  }
  uint8_t buf[32];
  size_t bytes_read = 0u;
  if (cutil_read_binary_file(lock_path, buf, sizeof(buf) - 1u, &bytes_read) != 0 || bytes_read == 0u)
  {
    return false;
  }
  size_t pos = 0u;
  while (pos < bytes_read && pos + 1u < out_pid_size && buf[pos] >= '0' && buf[pos] <= '9')
  {
    out_pid[pos] = (char)buf[pos];
    pos++;
  }
  if (pos == 0u)
  {
    return false;
  }
  out_pid[pos] = '\0';
  return true;
}

static int read_binary_cache_file(
  char const *path, size_t max_size, uint8_t **out_buf, size_t *out_size, int64_t *out_mtime_sec)
{
  if (path == NULL || max_size == 0u || out_buf == NULL || out_size == NULL)
  {
    errno = EINVAL;
    return -1;
  }
  *out_buf = NULL;
  *out_size = 0u;
  if (out_mtime_sec != NULL)
  {
    *out_mtime_sec = 0;
  }

#ifdef _WIN32
  DWORD attrs = GetFileAttributesA(path);
  if (attrs == INVALID_FILE_ATTRIBUTES)
  {
    errno = (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND) ? ENOENT : EACCES;
    return -1;
  }
  if ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0u || (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0u)
  {
    errno = EINVAL;
    return -1;
  }
  HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
  if (h == INVALID_HANDLE_VALUE)
  {
    errno = (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND) ? ENOENT : EACCES;
    return -1;
  }
  BY_HANDLE_FILE_INFORMATION info;
  if (!GetFileInformationByHandle(h, &info))
  {
    CloseHandle(h);
    errno = EIO;
    return -1;
  }
  if ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0u ||
    (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0u)
  {
    CloseHandle(h);
    errno = EINVAL;
    return -1;
  }
  uint64_t file_size64 = (((uint64_t)info.nFileSizeHigh) << 32u) | (uint64_t)info.nFileSizeLow;
  if (file_size64 == 0u || file_size64 > (uint64_t)max_size)
  {
    CloseHandle(h);
    return -2;
  }
  size_t file_size = (size_t)file_size64;
  uint8_t *buf = (uint8_t *)malloc(file_size);
  if (buf == NULL)
  {
    CloseHandle(h);
    errno = ENOMEM;
    return -3;
  }
  size_t total_read = 0u;
  while (total_read < file_size)
  {
    DWORD chunk = (file_size - total_read > 0x40000000u) ? 0x40000000u : (DWORD)(file_size - total_read);
    DWORD got = 0u;
    if (!ReadFile(h, buf + total_read, chunk, &got, NULL))
    {
      free(buf);
      CloseHandle(h);
      errno = EIO;
      return -1;
    }
    if (got == 0u)
    {
      free(buf);
      CloseHandle(h);
      return -2;
    }
    total_read += (size_t)got;
  }
  uint8_t extra = 0u;
  DWORD extra_got = 0u;
  if (ReadFile(h, &extra, 1u, &extra_got, NULL) && extra_got != 0u)
  {
    free(buf);
    CloseHandle(h);
    return -2;
  }
  if (out_mtime_sec != NULL)
  {
    ULARGE_INTEGER ull;
    ull.LowPart = info.ftLastWriteTime.dwLowDateTime;
    ull.HighPart = info.ftLastWriteTime.dwHighDateTime;
    if (ull.QuadPart >= 116444736000000000ULL)
    {
      *out_mtime_sec = (int64_t)((ull.QuadPart - 116444736000000000ULL) / 10000000ULL);
    }
  }
  CloseHandle(h);
  *out_buf = buf;
  *out_size = file_size;
  return 0;
#else
  int flags = O_RDONLY;
# ifdef O_NOFOLLOW
  flags |= O_NOFOLLOW;
# endif
  int fd = open(path, flags);
  if (fd < 0)
  {
    return -1;
  }
  struct stat st;
  if (fstat(fd, &st) != 0)
  {
    int saved_errno = errno;
    close(fd);
    errno = saved_errno;
    return -1;
  }
  if (!S_ISREG(st.st_mode))
  {
    close(fd);
    errno = EINVAL;
    return -1;
  }
  if (st.st_size <= 0 || (uint64_t)st.st_size > (uint64_t)max_size)
  {
    close(fd);
    return -2;
  }
  size_t file_size = (size_t)st.st_size;
  uint8_t *buf = (uint8_t *)malloc(file_size);
  if (buf == NULL)
  {
    close(fd);
    errno = ENOMEM;
    return -3;
  }
  size_t total_read = 0u;
  while (total_read < file_size)
  {
    ssize_t n = read(fd, buf + total_read, file_size - total_read);
    if (n < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      int saved_errno = errno;
      free(buf);
      close(fd);
      errno = saved_errno;
      return -1;
    }
    if (n == 0)
    {
      free(buf);
      close(fd);
      return -2;
    }
    total_read += (size_t)n;
  }
  uint8_t extra = 0u;
  ssize_t extra_n = read(fd, &extra, 1u);
  if (extra_n > 0)
  {
    free(buf);
    close(fd);
    return -2;
  }
  close(fd);
  if (out_mtime_sec != NULL)
  {
    *out_mtime_sec = (int64_t)st.st_mtime;
  }
  *out_buf = buf;
  *out_size = file_size;
  return 0;
#endif
}

static apx_error_t write_file_atomic(apx_node_cache_t *self, char const *final_path, uint8_t const *data, size_t size)
{
  MUTEX_LOCK(self->lock);
  uint64_t seq = ++self->tmp_seq;
  MUTEX_UNLOCK(self->lock);

  char tmp_suffix[64];
  snprintf(tmp_suffix, sizeof(tmp_suffix), ".tmp.%lu.%llu", APX_GETPID(), (unsigned long long)seq);
  adt_str_t *tmp_path = adt_str_new_cstr(final_path);
  if (tmp_path == NULL || adt_str_append_cstr(tmp_path, tmp_suffix) != ADT_NO_ERROR)
  {
    adt_str_delete(tmp_path);
    return APX_MEM_ERROR;
  }

  char const *tmp_cstr = adt_str_cstr(tmp_path);
  FILE *fh = fopen(tmp_cstr, "wb");
  if (fh == NULL)
  {
    MUTEX_LOCK(self->lock);
    self->stats.io_errors++;
    MUTEX_UNLOCK(self->lock);
    adt_str_delete(tmp_path);
    return APX_FILE_CREATE_ERROR;
  }

  bool write_ok = (size == 0u) || (fwrite(data, 1u, size, fh) == size);
  if (write_ok)
  {
    if (cutil_fsync_file(fh) != 0)
    {
      write_ok = false;
    }
  }
  if (fclose(fh) != 0)
  {
    write_ok = false;
  }

  if (!write_ok || cutil_file_replace_atomic(tmp_cstr, final_path) != 0)
  {
    (void)remove(tmp_cstr);
    MUTEX_LOCK(self->lock);
    self->stats.io_errors++;
    MUTEX_UNLOCK(self->lock);
    adt_str_delete(tmp_path);
    return APX_FILE_WRITE_ERROR;
  }

  adt_str_delete(tmp_path);
  return APX_NO_ERROR;
}

static int64_t current_wall_sec(apx_node_cache_t const *self)
{
  int64_t now = (int64_t)time(NULL);
#ifdef UNIT_TEST
  if (self != NULL)
  {
    now += self->wall_clock_offset_sec;
  }
#else
  (void)self;
#endif
  return now;
}

static int64_t current_mono_sec(void) { return (int64_t)(apx_get_monotonic_time_ns() / 1000000000ULL); }

static int64_t get_effective_now_sec(apx_node_cache_t *self)
{
  int64_t mono_now = current_mono_sec();
  int64_t elapsed = (mono_now >= self->boot_mono_sec) ? (mono_now - self->boot_mono_sec) : 0;

  MUTEX_LOCK(self->lock);
  int64_t wall_now = current_wall_sec(self);
  int64_t mono_logical = self->logical_base_sec + elapsed;
  int64_t effective_now = (wall_now > mono_logical) ? wall_now : mono_logical;
  if (wall_now > mono_logical)
  {
    self->logical_base_sec = wall_now - elapsed;
  }
  MUTEX_UNLOCK(self->lock);
  return effective_now;
}

static void maybe_apply_high_water_mtime_on_store(apx_node_cache_t *self, char const *file_path)
{
  if (self->read_only || (self->max_versions_per_node == 0u && self->max_size_bytes == 0u))
  {
    return;
  }
  int64_t eff_now = get_effective_now_sec(self);
  // Real OS mtime after write is approximately time(NULL); override if OS clock lags high-water mark
  if ((int64_t)time(NULL) < eff_now)
  {
    (void)cutil_file_set_mtime(file_path, eff_now);
  }
}

static void maybe_refresh_mtime_on_hit(apx_node_cache_t *self, char const *file_path, int64_t file_mtime_sec)
{
  if (self->read_only || (self->max_versions_per_node == 0u && self->max_size_bytes == 0u))
  {
    return;
  }
  int64_t eff_now = get_effective_now_sec(self);
  if (eff_now - file_mtime_sec >= APX_CACHE_MTIME_REFRESH_INTERVAL_SEC)
  {
    (void)cutil_file_set_mtime(file_path, eff_now);
  }
}

static bool check_format_version_matches(char const *bin_dir)
{
  adt_str_t *ver_path = cutil_path_join(bin_dir, FORMAT_VERSION_FILENAME);
  if (ver_path == NULL)
  {
    return false;
  }
  char const *expected = apx_node_cache_format_version_string();
  size_t expected_len = strlen(expected);
  uint8_t buf[64];
  size_t bytes_read = 0u;
  bool matches = false;
  if (cutil_read_binary_file(adt_str_cstr(ver_path), buf, sizeof(buf), &bytes_read) == 0)
  {
    if (bytes_read == expected_len && memcmp(buf, expected, expected_len) == 0)
    {
      matches = true;
    }
  }
  adt_str_delete(ver_path);
  return matches;
}

static bool on_startup_scan_entry(void *arg, const cutil_dir_entry_t *entry)
{
  startup_sweep_ctx_t *ctx = (startup_sweep_ctx_t *)arg;
  if (entry->is_dir)
  {
    return true;
  }

  // Sweep leftover *.tmp.* files from prior power loss (only when writable)
  if (is_tmp_filename(entry->name))
  {
    if (!ctx->read_only && entry->is_reg && !entry->is_symlink)
    {
      adt_str_t *full = cutil_path_join(ctx->dir_path, entry->name);
      if (full != NULL)
      {
        (void)remove(adt_str_cstr(full));
        adt_str_delete(full);
      }
    }
    return true;
  }

  if (!entry->is_reg || entry->is_symlink)
  {
    return true;
  }

  if (!ctx->is_bin_dir)
  {
    if (parse_cache_filename(entry->name, TEXT_EXT, TEXT_EXT_LEN, NULL, NULL))
    {
      if (entry->mtime_sec > ctx->max_mtime_sec)
      {
        ctx->max_mtime_sec = entry->mtime_sec;
      }
    }
    return true;
  }

  // Binary directory (.apxc)
  if (strcmp(entry->name, FORMAT_VERSION_FILENAME) == 0)
  {
    return true;
  }

  size_t node_len = 0u;
  bool valid_name = parse_cache_filename(entry->name, BIN_EXT, BIN_EXT_LEN, NULL, &node_len);
  if (!valid_name)
  {
    return true;
  }

  adt_str_t *full = cutil_path_join(ctx->dir_path, entry->name);
  if (full == NULL)
  {
    return true;
  }
  char const *full_cstr = adt_str_cstr(full);

  if (ctx->clear_all_bin)
  {
    if (!ctx->read_only && remove(full_cstr) == 0)
    {
      ctx->cleared_bin_count++;
    }
    adt_str_delete(full);
    return true;
  }

  // Pre-validate .apxc integrity, format/VM/ABI, and filename-to-header binding (also warms OS page cache)
  if (!ctx->read_only)
  {
    char node_name[RMF_MAX_FILE_NAME_SIZE + 1u];
    uint8_t expected_sha[RMF_SHA256_SIZE];
    bool bad = false;
    if (node_len == 0u || node_len > RMF_MAX_FILE_NAME_SIZE ||
      !hex_to_sha256(entry->name + node_len + 1u, expected_sha))
    {
      bad = true;
    }
    else
    {
      memcpy(node_name, entry->name, node_len);
      node_name[node_len] = '\0';
      uint8_t *buf = NULL;
      size_t buf_size = 0u;
      int read_rc = read_binary_cache_file(full_cstr, (size_t)APX_MAX_FILE_SIZE, &buf, &buf_size, NULL);
      if (read_rc != 0)
      {
        bad = (read_rc != -3); // Do not unlink on OOM
      }
      else
      {
        apx_error_t vrc = apx_node_cache_validate_binary(buf, buf_size, node_name, expected_sha);
        free(buf);
        if (vrc != APX_NO_ERROR)
        {
          bad = true;
        }
      }
    }
    if (bad)
    {
      if (remove(full_cstr) == 0)
      {
        ctx->cleared_bin_count++;
      }
      adt_str_delete(full);
      return true;
    }
  }

  if (entry->mtime_sec > ctx->max_mtime_sec)
  {
    ctx->max_mtime_sec = entry->mtime_sec;
  }
  adt_str_delete(full);
  return true;
}

static void prune_version_vdelete(void *arg)
{
  prune_version_t *v = (prune_version_t *)arg;
  if (v != NULL)
  {
    free(v->key);
    free(v);
  }
}

static bool on_prune_dir_entry(void *arg, const cutil_dir_entry_t *entry)
{
  prune_scan_ctx_t *ctx = (prune_scan_ctx_t *)arg;
  if (!entry->is_reg || entry->is_symlink)
  {
    return true;
  }
  char const *ext = ctx->scanning_bin ? BIN_EXT : TEXT_EXT;
  size_t ext_len = ctx->scanning_bin ? BIN_EXT_LEN : TEXT_EXT_LEN;
  size_t key_len = 0u;
  size_t node_len = 0u;
  if (!parse_cache_filename(entry->name, ext, ext_len, &key_len, &node_len))
  {
    return true;
  }

  char *key_copy = (char *)malloc(key_len + 1u);
  if (key_copy == NULL)
  {
    return false;
  }
  memcpy(key_copy, entry->name, key_len);
  key_copy[key_len] = '\0';

  prune_version_t *ver = (prune_version_t *)adt_hash_value(&ctx->version_map, key_copy);
  if (ver == NULL)
  {
    ver = (prune_version_t *)calloc(1u, sizeof(prune_version_t));
    if (ver == NULL)
    {
      free(key_copy);
      return false;
    }
    ver->key = key_copy;
    ver->matches_node = (node_len == ctx->node_name_len) && (memcmp(entry->name, ctx->node_name, node_len) == 0);
    ver->is_keep = (ctx->keep_key != NULL && strcmp(key_copy, ctx->keep_key) == 0);
    if (adt_ary_push(&ctx->versions, ver) != ADT_NO_ERROR)
    {
      prune_version_vdelete(ver);
      return false;
    }
    adt_hash_set(&ctx->version_map, ver->key, ver);
  }
  else
  {
    free(key_copy);
  }

  if (ctx->scanning_bin)
  {
    ver->has_bin = true;
    ver->bin_size = entry->size;
    ver->bin_mtime = entry->mtime_sec;
  }
  else
  {
    ver->has_text = true;
    ver->text_size = entry->size;
    ver->text_mtime = entry->mtime_sec;
  }

  if (ver->has_text && ver->has_bin)
  {
    ver->effective_mtime = (ver->text_mtime >= ver->bin_mtime) ? ver->text_mtime : ver->bin_mtime;
  }
  else if (ver->has_bin)
  {
    ver->effective_mtime = ver->bin_mtime;
  }
  else
  {
    ver->effective_mtime = ver->text_mtime;
  }

  ctx->total_bytes += entry->size;
  return true;
}

static int compare_prune_versions(const void *a, const void *b)
{
  prune_version_t const *va = *(prune_version_t const *const *)a;
  prune_version_t const *vb = *(prune_version_t const *const *)b;
  if (va->effective_mtime < vb->effective_mtime)
  {
    return -1;
  }
  if (va->effective_mtime > vb->effective_mtime)
  {
    return 1;
  }
  return strcmp(va->key, vb->key);
}

static bool evict_version(apx_node_cache_t *self, prune_version_t *ver, uint64_t *total_bytes, uint64_t *text_evictions,
  uint64_t *bin_evictions)
{
  bool removed_any = false;
  if (ver->has_text && self->text_dir != NULL)
  {
    adt_str_t *fname = adt_str_new_cstr(ver->key);
    if (fname != NULL && adt_str_append_cstr(fname, TEXT_EXT) == ADT_NO_ERROR)
    {
      adt_str_t *full = cutil_path_join(self->text_dir, adt_str_cstr(fname));
      if (full != NULL)
      {
        if (remove(adt_str_cstr(full)) == 0)
        {
          removed_any = true;
          (*text_evictions)++;
          if (*total_bytes >= ver->text_size)
          {
            *total_bytes -= ver->text_size;
          }
          else
          {
            *total_bytes = 0u;
          }
        }
        adt_str_delete(full);
      }
    }
    adt_str_delete(fname);
  }
  if (ver->has_bin && self->bin_dir != NULL)
  {
    adt_str_t *fname = adt_str_new_cstr(ver->key);
    if (fname != NULL && adt_str_append_cstr(fname, BIN_EXT) == ADT_NO_ERROR)
    {
      adt_str_t *full = cutil_path_join(self->bin_dir, adt_str_cstr(fname));
      if (full != NULL)
      {
        if (remove(adt_str_cstr(full)) == 0)
        {
          removed_any = true;
          (*bin_evictions)++;
          if (*total_bytes >= ver->bin_size)
          {
            *total_bytes -= ver->bin_size;
          }
          else
          {
            *total_bytes = 0u;
          }
        }
        adt_str_delete(full);
      }
    }
    adt_str_delete(fname);
  }
  if (removed_any)
  {
    ver->evicted = true;
  }
  return removed_any;
}

static void prune_cache(apx_node_cache_t *self, char const *node_name, char const *keep_key)
{
  MUTEX_LOCK(self->prune_lock);

  prune_scan_ctx_t ctx;
  ctx.node_name = node_name;
  ctx.node_name_len = strlen(node_name);
  ctx.keep_key = keep_key;
  ctx.scanning_bin = false;
  adt_hash_create(&ctx.version_map, NULL);
  adt_ary_create(&ctx.versions, prune_version_vdelete);
  ctx.total_bytes = 0u;

  bool scan_ok = (cutil_dir_for_each(self->text_dir, on_prune_dir_entry, &ctx) == 0);
  if (scan_ok && self->bin_dir != NULL)
  {
    ctx.scanning_bin = true;
    scan_ok = (cutil_dir_for_each(self->bin_dir, on_prune_dir_entry, &ctx) == 0);
  }

  if (scan_ok)
  {
    int32_t const count = adt_ary_length(&ctx.versions);
    if (count > 0)
    {
      prune_version_t **sorted = (prune_version_t **)malloc((size_t)count * sizeof(prune_version_t *));
      if (sorted != NULL)
      {
        uint32_t node_count = 0u;
        for (int32_t i = 0; i < count; i++)
        {
          sorted[i] = (prune_version_t *)adt_ary_value(&ctx.versions, i);
          if (sorted[i]->matches_node)
          {
            node_count++;
          }
        }
        qsort(sorted, (size_t)count, sizeof(prune_version_t *), compare_prune_versions);

        uint64_t text_evicted = 0u;
        uint64_t bin_evicted = 0u;

        // Step 1: Enforce per-node version limit (paired versions)
        if (self->max_versions_per_node > 0u && node_count > self->max_versions_per_node)
        {
          for (int32_t i = 0; i < count && node_count > self->max_versions_per_node; i++)
          {
            prune_version_t *v = sorted[i];
            if (!v->matches_node || v->is_keep || v->evicted)
            {
              continue;
            }
            if (evict_version(self, v, &ctx.total_bytes, &text_evicted, &bin_evicted))
            {
              node_count--;
            }
          }
        }

        // Step 2: Enforce global combined cache size cap (text/ + bin/)
        if (self->max_size_bytes > 0u && ctx.total_bytes > self->max_size_bytes)
        {
          for (int32_t i = 0; i < count && ctx.total_bytes > self->max_size_bytes; i++)
          {
            prune_version_t *v = sorted[i];
            if (v->is_keep || v->evicted)
            {
              continue;
            }
            (void)evict_version(self, v, &ctx.total_bytes, &text_evicted, &bin_evicted);
          }
        }

        if (text_evicted > 0u || bin_evicted > 0u)
        {
          MUTEX_LOCK(self->lock);
          self->stats.text_evictions += text_evicted;
          self->stats.bin_evictions += bin_evicted;
          MUTEX_UNLOCK(self->lock);
        }
        free(sorted);
      }
    }
  }

  adt_hash_destroy(&ctx.version_map);
  adt_ary_destroy(&ctx.versions);
  MUTEX_UNLOCK(self->prune_lock);
}
