/*****************************************************************************
 * \file      node_cache.h
 * \author    Conny Gustafsson
 * \date      2021-01-20
 * \brief     Server-side disk cache for APX node definitions
 *
 * Copyright (c) 2018-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_NODE_CACHE_H
#define APX_NODE_CACHE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
#else
# include <pthread.h>
#endif
#include "apx/error.h"
#include "apx/remotefile.h"
#include "apx/types.h"
#include "fileutil.h"
#include "osmacro.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
// Forward declarations
struct apx_node_instance_tag;

#define APX_CACHE_DEFAULT_MAX_VERSIONS_PER_NODE 10u
#define APX_CACHE_DEFAULT_MAX_SIZE_MB 64u
#define APX_CACHE_MTIME_REFRESH_INTERVAL_SEC 60

typedef enum apx_cache_level_tag
{
  APX_CACHE_LEVEL_OFF = 0,
  APX_CACHE_LEVEL_TEXT = 1,
  APX_CACHE_LEVEL_BINARY = 2
} apx_cache_level_t;

typedef struct apx_node_cache_config_tag
{
  apx_cache_level_t level;
  uint32_t max_versions_per_node;
  uint32_t max_size_mb;
} apx_node_cache_config_t;

typedef struct apx_node_cache_stats_tag
{
  uint64_t text_hits;
  uint64_t text_misses;
  uint64_t text_stores;
  uint64_t text_evictions;
  uint64_t text_corruptions;
  uint64_t bin_hits;
  uint64_t bin_misses;
  uint64_t bin_stores;
  uint64_t bin_evictions;
  uint64_t bin_corruptions;
  uint64_t bin_version_mismatches;
  uint64_t io_errors;
} apx_node_cache_stats_t;

typedef struct apx_node_cache_tag
{
  apx_cache_level_t level;
  uint32_t max_versions_per_node;
  uint32_t max_size_mb;
  uint64_t max_size_bytes;
  char *root_dir;
  char *text_dir;
  char *bin_dir;
  cutil_file_lock_t lock_file;
  MUTEX_T lock;
  MUTEX_T prune_lock;
  uint64_t tmp_seq;
  int64_t logical_base_sec;
  int64_t boot_mono_sec;
#ifdef UNIT_TEST
  int64_t wall_clock_offset_sec;
#endif
  apx_node_cache_stats_t stats;
  bool initialized;
  bool read_only;
} apx_node_cache_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_node_cache_config_init(apx_node_cache_config_t *cfg);
void apx_node_cache_create(apx_node_cache_t *self);
void apx_node_cache_destroy(apx_node_cache_t *self);
apx_node_cache_t *apx_node_cache_new(void);
void apx_node_cache_delete(apx_node_cache_t *self);

/**
 * Configures the cache directory, level, and eviction limits.
 *
 * Fail-safe: if directories cannot be created/accessed, the cache stays disabled
 * (APX_CACHE_LEVEL_OFF) and an explanatory warning is written into warning_buf (if provided).
 * If the text cache directory is readable but not writable, the cache runs in read-only mode
 * and writes a warning into warning_buf.
 */
apx_error_t apx_node_cache_configure(apx_node_cache_t *self, apx_node_cache_config_t const *cfg, char const *root_path,
  char *warning_buf, size_t warning_buf_size);

bool apx_node_cache_is_text_enabled(apx_node_cache_t const *self);
bool apx_node_cache_is_binary_enabled(apx_node_cache_t const *self);
bool apx_node_cache_is_read_only(apx_node_cache_t const *self);
apx_cache_level_t apx_node_cache_get_level(apx_node_cache_t const *self);
uint32_t apx_node_cache_get_max_versions_per_node(apx_node_cache_t const *self);
uint32_t apx_node_cache_get_max_size_mb(apx_node_cache_t const *self);
char const *apx_node_cache_get_root_path(apx_node_cache_t const *self);

/**
 * Looks up a cached Level 1 text definition by (node_name, sha256) and verifies
 * regular-file status, exact byte length, and SHA-256 content integrity.
 *
 * On APX_NO_ERROR, allocates (*out_data) of size (expected_size + 1) with trailing NUL;
 * caller owns *out_data and must free() it.
 * On miss, returns APX_FILE_NOT_FOUND_ERROR.
 * On corruption (size mismatch or SHA-256 mismatch), unlinks the entry (unless read-only)
 * and returns APX_LENGTH_ERROR or APX_CHECKSUM_ERROR.
 */
apx_error_t apx_node_cache_lookup_text(apx_node_cache_t *self, char const *node_name,
  uint8_t const sha256[RMF_SHA256_SIZE], uint32_t expected_size, uint8_t **out_data);

/**
 * Atomically stores a validated Level 1 text definition in the cache and enforces
 * configured per-node and global eviction limits. Best-effort no-op when disabled or read-only.
 */
apx_error_t apx_node_cache_store_text(apx_node_cache_t *self, char const *node_name,
  uint8_t const sha256[RMF_SHA256_SIZE], uint8_t const *data, uint32_t size);

/**
 * Best-effort removal of a cached Level 1 text entry (no-op when disabled or read-only).
 */
void apx_node_cache_invalidate_text(
  apx_node_cache_t *self, char const *node_name, uint8_t const sha256[RMF_SHA256_SIZE]);

/**
 * Looks up a cached Level 2 binary definition (.apxc) by (node_name, sha256), verifies
 * regular-file status, whole-file SHA-256 integrity, version/ABI compatibility, and
 * decodes compiled port descriptors, init data, and optional VM programs into node_instance.
 *
 * On APX_NO_ERROR, node_instance has its IDL version, ports, and init data populated.
 * If out_embedded_def_text is non-NULL and the .apxc contains APXC_SEC_DEF_TEXT,
 * allocates (*out_embedded_def_text) with trailing NUL; caller owns it and must free().
 * On miss, returns APX_FILE_NOT_FOUND_ERROR.
 * On version/ABI mismatch or corruption, unlinks the entry (unless read-only) and returns
 * APX_VERSION_ERROR or a corruption error code.
 */
apx_error_t apx_node_cache_lookup_binary(apx_node_cache_t *self, char const *node_name,
  uint8_t const sha256[RMF_SHA256_SIZE], uint32_t expected_logical_size, bool load_vm_programs,
  struct apx_node_instance_tag *node_instance, uint8_t **out_embedded_def_text);

/**
 * Atomically encodes and stores a validated Level 2 binary definition (.apxc) in the cache
 * and enforces coordinated per-node and global eviction limits across text/ and bin/.
 * Best-effort no-op when binary caching is disabled or read-only.
 */
apx_error_t apx_node_cache_store_binary(apx_node_cache_t *self, char const *node_name,
  uint8_t const sha256[RMF_SHA256_SIZE], uint32_t logical_def_size, struct apx_node_instance_tag const *node_instance);

/**
 * Best-effort removal of a cached Level 2 binary entry (no-op when disabled or read-only).
 */
void apx_node_cache_invalidate_binary(
  apx_node_cache_t *self, char const *node_name, uint8_t const sha256[RMF_SHA256_SIZE]);

void apx_node_cache_get_stats(apx_node_cache_t *self, apx_node_cache_stats_t *out_stats);

#ifdef UNIT_TEST
void apx_node_cache_set_max_size_bytes(apx_node_cache_t *self, uint64_t max_size_bytes);
void apx_node_cache_set_wall_clock_offset_sec(apx_node_cache_t *self, int64_t offset_sec);
#endif

#endif // APX_NODE_CACHE_H
