/*****************************************************************************
 * \file      node_cache.c
 * \author    Conny Gustafsson
 * \date      2018-08-03
 * \brief     Server-side disk cache for APX node definitions
 *
 * Copyright (c) 2018-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/node_cache.h"
#include "adt_ary.h"
#include "adt_str.h"
#include "apx/crypto.h"
#include "fileutil.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
# include <process.h>
# define APX_GETPID() ((unsigned long)_getpid())
#else
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
// Fixed suffix length after node_name: '_' + 64 hex chars + ".apx"
#define CACHE_FILE_SUFFIX_LEN (1u + SHA256_HEX_LEN + TEXT_EXT_LEN)

typedef struct prune_entry_tag
{
  char *name;
  uint64_t size;
  int64_t mtime_sec;
  bool matches_node;
  bool is_keep;
  bool evicted;
} prune_entry_t;

typedef struct prune_scan_ctx_tag
{
  char const *node_name;
  size_t node_name_len;
  char const *keep_filename;
  adt_ary_t entries;
  uint64_t total_bytes;
} prune_scan_ctx_t;

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void set_warning(char *buf, size_t buf_size, char const *fmt, ...);
static bool is_safe_node_name(char const *node_name);
static void sha256_to_hex(uint8_t const sha256[RMF_SHA256_SIZE], char out_hex[SHA256_HEX_LEN + 1u]);
static bool is_lowercase_hex_str(char const *s, size_t len);
static bool is_valid_text_cache_filename(char const *name, size_t *out_len);
static adt_str_t *build_text_cache_filename(char const *node_name, uint8_t const sha256[RMF_SHA256_SIZE]);
static char *dup_cstr(char const *src);
static adt_str_t *build_text_cache_path(
  apx_node_cache_t const *self, char const *node_name, uint8_t const sha256[RMF_SHA256_SIZE]);
static bool on_dir_access_probe(void *arg, const cutil_dir_entry_t *entry);
static bool probe_dir_writable(apx_node_cache_t *self, char const *dir_path, int *out_errno);
static bool read_lock_holder_pid(char const *lock_path, char *out_pid, size_t out_pid_size);
static void prune_entry_vdelete(void *arg);
static bool on_prune_dir_entry(void *arg, const cutil_dir_entry_t *entry);
static int compare_prune_entries(const void *a, const void *b);
static void prune_text_cache(apx_node_cache_t *self, char const *node_name, char const *keep_filename);

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
    cutil_file_lock_init(&self->lock_file);
    MUTEX_INIT(self->lock);
    MUTEX_INIT(self->prune_lock);
    self->tmp_seq = 0u;
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

  int probe_errno = 0;
  bool writable = probe_dir_writable(self, text_dir_cstr, &probe_errno);
  if (!writable)
  {
    self->read_only = true;
    set_warning(warning_buf, warning_buf_size,
      "Cache directory '%s' is not writable (%s); running cache in read-only mode -- new definitions will not be "
      "cached",
      text_dir_cstr, strerror(probe_errno));
  }
  else
  {
    adt_str_t *lock_path = cutil_path_join(root_path, ".lock");
    if (lock_path == NULL)
    {
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
      adt_str_delete(text_path);
      return APX_NO_ERROR;
    }
    adt_str_delete(lock_path);
  }

  char *root_copy = dup_cstr(root_path);
  char *text_copy = dup_cstr(text_dir_cstr);
  adt_str_delete(text_path);
  if (root_copy == NULL || text_copy == NULL)
  {
    cutil_file_lock_release(&self->lock_file);
    free(root_copy);
    free(text_copy);
    self->read_only = false;
    return APX_MEM_ERROR;
  }
  self->root_dir = root_copy;
  self->text_dir = text_copy;

  self->level = cfg->level;
  self->max_versions_per_node = cfg->max_versions_per_node;
  self->max_size_mb = cfg->max_size_mb;
  self->max_size_bytes = (uint64_t)cfg->max_size_mb * 1024ULL * 1024ULL;
  self->initialized = true;
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

  adt_str_t *file_path = build_text_cache_path(self, node_name, sha256);
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

  if (!self->read_only && (self->max_versions_per_node > 0u || self->max_size_bytes > 0u))
  {
    int64_t now_sec = (int64_t)time(NULL);
    if (now_sec - mtime_sec >= APX_CACHE_MTIME_REFRESH_INTERVAL_SEC)
    {
      (void)cutil_file_touch_mtime(path_cstr);
    }
  }

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

  adt_str_t *filename = build_text_cache_filename(node_name, sha256);
  if (filename == NULL)
  {
    return APX_MEM_ERROR;
  }
  adt_str_t *final_path = cutil_path_join(self->text_dir, adt_str_cstr(filename));
  if (final_path == NULL)
  {
    adt_str_delete(filename);
    return APX_MEM_ERROR;
  }

  if (cutil_file_exists(adt_str_cstr(final_path)))
  {
    adt_str_delete(final_path);
    adt_str_delete(filename);
    return APX_NO_ERROR;
  }

  MUTEX_LOCK(self->lock);
  uint64_t seq = ++self->tmp_seq;
  MUTEX_UNLOCK(self->lock);

  char tmp_suffix[64];
  snprintf(tmp_suffix, sizeof(tmp_suffix), ".tmp.%lu.%llu", APX_GETPID(), (unsigned long long)seq);
  adt_str_t *tmp_path = adt_str_new_cstr(adt_str_cstr(final_path));
  if (tmp_path == NULL || adt_str_append_cstr(tmp_path, tmp_suffix) != ADT_NO_ERROR)
  {
    adt_str_delete(tmp_path);
    adt_str_delete(final_path);
    adt_str_delete(filename);
    return APX_MEM_ERROR;
  }

  char const *tmp_cstr = adt_str_cstr(tmp_path);
  char const *final_cstr = adt_str_cstr(final_path);
  FILE *fh = fopen(tmp_cstr, "wb");
  if (fh == NULL)
  {
    MUTEX_LOCK(self->lock);
    self->stats.io_errors++;
    MUTEX_UNLOCK(self->lock);
    adt_str_delete(tmp_path);
    adt_str_delete(final_path);
    adt_str_delete(filename);
    return APX_FILE_CREATE_ERROR;
  }

  bool write_ok = (fwrite(data, 1u, (size_t)size, fh) == (size_t)size);
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

  if (!write_ok || cutil_file_replace_atomic(tmp_cstr, final_cstr) != 0)
  {
    (void)remove(tmp_cstr);
    MUTEX_LOCK(self->lock);
    self->stats.io_errors++;
    MUTEX_UNLOCK(self->lock);
    adt_str_delete(tmp_path);
    adt_str_delete(final_path);
    adt_str_delete(filename);
    return APX_FILE_WRITE_ERROR;
  }

  adt_str_delete(tmp_path);
  adt_str_delete(final_path);

  MUTEX_LOCK(self->lock);
  self->stats.text_stores++;
  MUTEX_UNLOCK(self->lock);

  if (self->max_versions_per_node > 0u || self->max_size_bytes > 0u)
  {
    prune_text_cache(self, node_name, adt_str_cstr(filename));
  }

  adt_str_delete(filename);
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
  adt_str_t *file_path = build_text_cache_path(self, node_name, sha256);
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

static bool is_valid_text_cache_filename(char const *name, size_t *out_len)
{
  if (name == NULL)
  {
    return false;
  }
  size_t len = strlen(name);
  if (len <= CACHE_FILE_SUFFIX_LEN)
  {
    return false;
  }
  if (memcmp(name + len - TEXT_EXT_LEN, TEXT_EXT, TEXT_EXT_LEN) != 0)
  {
    return false;
  }
  if (name[len - CACHE_FILE_SUFFIX_LEN] != '_')
  {
    return false;
  }
  if (!is_lowercase_hex_str(name + len - CACHE_FILE_SUFFIX_LEN + 1u, SHA256_HEX_LEN))
  {
    return false;
  }
  if (out_len != NULL)
  {
    *out_len = len;
  }
  return true;
}

static adt_str_t *build_text_cache_filename(char const *node_name, uint8_t const sha256[RMF_SHA256_SIZE])
{
  char hex[SHA256_HEX_LEN + 1u];
  sha256_to_hex(sha256, hex);
  adt_str_t *filename = adt_str_new_cstr(node_name);
  if (filename == NULL)
  {
    return NULL;
  }
  if (adt_str_push(filename, '_') != ADT_NO_ERROR || adt_str_append_cstr(filename, hex) != ADT_NO_ERROR ||
    adt_str_append_cstr(filename, TEXT_EXT) != ADT_NO_ERROR)
  {
    adt_str_delete(filename);
    return NULL;
  }
  return filename;
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

static adt_str_t *build_text_cache_path(
  apx_node_cache_t const *self, char const *node_name, uint8_t const sha256[RMF_SHA256_SIZE])
{
  adt_str_t *filename = build_text_cache_filename(node_name, sha256);
  if (filename == NULL)
  {
    return NULL;
  }
  adt_str_t *full_path = cutil_path_join(self->text_dir, adt_str_cstr(filename));
  adt_str_delete(filename);
  return full_path;
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

static void prune_entry_vdelete(void *arg)
{
  prune_entry_t *entry = (prune_entry_t *)arg;
  if (entry != NULL)
  {
    free(entry->name);
    free(entry);
  }
}

static bool on_prune_dir_entry(void *arg, const cutil_dir_entry_t *entry)
{
  prune_scan_ctx_t *ctx = (prune_scan_ctx_t *)arg;
  if (!entry->is_reg || entry->is_symlink)
  {
    return true;
  }
  size_t name_len = 0u;
  if (!is_valid_text_cache_filename(entry->name, &name_len))
  {
    return true;
  }

  prune_entry_t *item = (prune_entry_t *)malloc(sizeof(prune_entry_t));
  if (item == NULL)
  {
    return false;
  }
  item->name = (char *)malloc(name_len + 1u);
  if (item->name == NULL)
  {
    free(item);
    return false;
  }
  memcpy(item->name, entry->name, name_len + 1u);
  item->size = entry->size;
  item->mtime_sec = entry->mtime_sec;
  item->matches_node = (name_len == ctx->node_name_len + CACHE_FILE_SUFFIX_LEN) &&
    (memcmp(entry->name, ctx->node_name, ctx->node_name_len) == 0);
  item->is_keep = (ctx->keep_filename != NULL && strcmp(entry->name, ctx->keep_filename) == 0);
  item->evicted = false;

  if (adt_ary_push(&ctx->entries, item) != ADT_NO_ERROR)
  {
    prune_entry_vdelete(item);
    return false;
  }
  ctx->total_bytes += entry->size;
  return true;
}

static int compare_prune_entries(const void *a, const void *b)
{
  prune_entry_t const *ea = *(prune_entry_t const *const *)a;
  prune_entry_t const *eb = *(prune_entry_t const *const *)b;
  if (ea->mtime_sec < eb->mtime_sec)
  {
    return -1;
  }
  if (ea->mtime_sec > eb->mtime_sec)
  {
    return 1;
  }
  return strcmp(ea->name, eb->name);
}

static void prune_text_cache(apx_node_cache_t *self, char const *node_name, char const *keep_filename)
{
  MUTEX_LOCK(self->prune_lock);

  prune_scan_ctx_t ctx;
  ctx.node_name = node_name;
  ctx.node_name_len = strlen(node_name);
  ctx.keep_filename = keep_filename;
  adt_ary_create(&ctx.entries, prune_entry_vdelete);
  ctx.total_bytes = 0u;

  char const *text_dir_cstr = self->text_dir;
  if (cutil_dir_for_each(text_dir_cstr, on_prune_dir_entry, &ctx) == 0)
  {
    int32_t const count = adt_ary_length(&ctx.entries);
    if (count > 0)
    {
      prune_entry_t **sorted = (prune_entry_t **)malloc((size_t)count * sizeof(prune_entry_t *));
      if (sorted != NULL)
      {
        uint32_t node_count = 0u;
        for (int32_t i = 0; i < count; i++)
        {
          sorted[i] = (prune_entry_t *)adt_ary_value(&ctx.entries, i);
          if (sorted[i]->matches_node)
          {
            node_count++;
          }
        }
        qsort(sorted, (size_t)count, sizeof(prune_entry_t *), compare_prune_entries);

        uint64_t evicted_count = 0u;

        // Step 1: Enforce per-node version limit
        if (self->max_versions_per_node > 0u && node_count > self->max_versions_per_node)
        {
          for (int32_t i = 0; i < count && node_count > self->max_versions_per_node; i++)
          {
            prune_entry_t *e = sorted[i];
            if (!e->matches_node || e->is_keep || e->evicted)
            {
              continue;
            }
            adt_str_t *full = cutil_path_join(text_dir_cstr, e->name);
            if (full != NULL)
            {
              if (remove(adt_str_cstr(full)) == 0)
              {
                e->evicted = true;
                node_count--;
                if (ctx.total_bytes >= e->size)
                {
                  ctx.total_bytes -= e->size;
                }
                else
                {
                  ctx.total_bytes = 0u;
                }
                evicted_count++;
              }
              adt_str_delete(full);
            }
          }
        }

        // Step 2: Enforce global cache size cap
        if (self->max_size_bytes > 0u && ctx.total_bytes > self->max_size_bytes)
        {
          for (int32_t i = 0; i < count && ctx.total_bytes > self->max_size_bytes; i++)
          {
            prune_entry_t *e = sorted[i];
            if (e->is_keep || e->evicted)
            {
              continue;
            }
            adt_str_t *full = cutil_path_join(text_dir_cstr, e->name);
            if (full != NULL)
            {
              if (remove(adt_str_cstr(full)) == 0)
              {
                e->evicted = true;
                if (ctx.total_bytes >= e->size)
                {
                  ctx.total_bytes -= e->size;
                }
                else
                {
                  ctx.total_bytes = 0u;
                }
                evicted_count++;
              }
              adt_str_delete(full);
            }
          }
        }

        if (evicted_count > 0u)
        {
          MUTEX_LOCK(self->lock);
          self->stats.text_evictions += evicted_count;
          MUTEX_UNLOCK(self->lock);
        }

        free(sorted);
      }
    }
  }

  adt_ary_destroy(&ctx.entries);
  MUTEX_UNLOCK(self->prune_lock);
}
