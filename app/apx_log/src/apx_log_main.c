/*****************************************************************************
 * \file      apx_log_main.c
 * \author    Conny Gustafsson
 * \date      2026-11-01
 * \brief     apx-log CLI application for recording and inspecting event logs
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_str.h"
#include "apx/util.h"
#include "apx_log_client.h"
#include "apx_log_event.h"
#include "apx_log_file.h"
#include "apx_version.h"
#include "argparse.h"
#include "dtl_json.h"
#include "dtl_type.h"
#include "msocket.h"
#include "osmacro.h"
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef _WIN32
# include <unistd.h>
#endif

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APP_NAME "apx-log"

#ifdef _WIN32
static const char *m_connect_address_default = "127.0.0.1:5000";
#else
static const char *m_connect_address_default = "/tmp/apx.socket";
#endif

typedef struct record_ctx_tag
{
  MUTEX_T lock;
  apx_log_writer_t writer;
  bool is_writer_open;
  uint64_t record_count;
} record_ctx_t;

//////////////////////////////////////////////////////////////////////////////
// FILE-SCOPED VARIABLES
//////////////////////////////////////////////////////////////////////////////
static volatile sig_atomic_t m_running = 1;
static bool m_json_output = false;
static bool m_use_tls = false;
static adt_str_t *m_connect_address = NULL;
static uint16_t m_connect_port = 5000;
static msocket_endpoint_type_t m_connect_resource_type = MSOCKET_ENDPOINT_UNKNOWN;
static adt_str_t *m_ca_cert = NULL;
static adt_str_t *m_client_cert = NULL;
static adt_str_t *m_client_key = NULL;

static record_ctx_t m_record_ctx;

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void sigint_handler(int sig)
{
  (void)sig;
  m_running = 0;
}

static void print_usage(const char *prog_name)
{
  printf("Usage: %s [OPTIONS] COMMAND [ARGS...]\n\n", prog_name);
  printf("Global Options:\n");
  printf("  -h, --help                Show this help message\n");
  printf("  -v, --version             Show version\n\n");
  printf("Commands:\n");
  printf("  record [OPTIONS] <FILE>   Stream server events directly to a .apxlog container\n");
  printf("  info <FILE>               Display .apxlog header and summary metadata\n");
  printf("  print [OPTIONS] <FILE>    Parse and display sequential events from a .apxlog container\n");
}

static void print_record_usage(const char *prog_name)
{
  printf("Usage: %s record [OPTIONS] <FILE>\n\n", prog_name);
  printf("Stream server events directly to a .apxlog container.\n\n");
  printf("Options:\n");
  printf("  -c, --connect <ADDR>      Diagnostic endpoint (default: %s)\n", m_connect_address_default);
  printf("  --tls                     Use TLS transport\n");
  printf("  --ca-cert <FILE>          Server trust anchor\n");
  printf("  --client-cert <FILE>      Client certificate\n");
  printf("  --client-key <FILE>       Client private key\n");
  printf("  -h, --help                Show this help message\n");
}

static void print_info_usage(const char *prog_name)
{
  printf("Usage: %s info [OPTIONS] <FILE>\n\n", prog_name);
  printf("Display .apxlog header and summary metadata.\n\n");
  printf("Options:\n");
  printf("  --json                    Emit metadata in JSON format\n");
  printf("  -h, --help                Show this help message\n");
}

static void print_print_usage(const char *prog_name)
{
  printf("Usage: %s print [OPTIONS] <FILE>\n\n", prog_name);
  printf("Parse and display sequential events from a .apxlog container.\n\n");
  printf("Options:\n");
  printf("  --json                    Output each event record as an NDJSON / JSON line\n");
  printf("  -h, --help                Show this help message\n");
}

static void print_version(void) { printf("%s %s\n", APP_NAME, APX_VERSION_STRING); }

static void print_cmd_usage(const char *command)
{
  if (command != NULL && strcmp(command, "record") == 0)
  {
    print_record_usage(APP_NAME);
  }
  else if (command != NULL && strcmp(command, "info") == 0)
  {
    print_info_usage(APP_NAME);
  }
  else if (command != NULL && strcmp(command, "print") == 0)
  {
    print_print_usage(APP_NAME);
  }
  else
  {
    print_usage(APP_NAME);
  }
}

static argparse_result_t parse_arg(const char *short_name, const char *long_name, const char *value)
{
  if ((short_name != NULL && strcmp(short_name, "c") == 0) || (long_name != NULL && strcmp(long_name, "connect") == 0))
  {
    if (m_connect_address != NULL)
    {
      adt_str_delete(m_connect_address);
    }
    m_connect_resource_type = msocket_parse_endpoint(value, &m_connect_address, &m_connect_port);
    if (m_connect_resource_type == MSOCKET_ENDPOINT_UNKNOWN || m_connect_resource_type == MSOCKET_ENDPOINT_ERROR)
    {
      return ARGPARSE_VALUE_ERROR;
    }
  }
  else if (long_name != NULL)
  {
    if (strcmp(long_name, "tls") == 0)
    {
      m_use_tls = true;
    }
    else if (strcmp(long_name, "json") == 0)
    {
      m_json_output = true;
    }
    else if (strcmp(long_name, "ca-cert") == 0)
    {
      if (m_ca_cert != NULL)
      {
        adt_str_delete(m_ca_cert);
      }
      m_ca_cert = adt_str_new_cstr(value);
    }
    else if (strcmp(long_name, "client-cert") == 0)
    {
      if (m_client_cert != NULL)
      {
        adt_str_delete(m_client_cert);
      }
      m_client_cert = adt_str_new_cstr(value);
    }
    else if (strcmp(long_name, "client-key") == 0)
    {
      if (m_client_key != NULL)
      {
        adt_str_delete(m_client_key);
      }
      m_client_key = adt_str_new_cstr(value);
    }
  }
  return ARGPARSE_SUCCESS;
}

static void format_iso8601(char *buf, size_t buf_size, uint64_t unix_ns)
{
  if (buf == NULL || buf_size == 0u)
  {
    return;
  }
  if (unix_ns == 0u)
  {
    snprintf(buf, buf_size, "-");
    return;
  }
  time_t const sec = (time_t)(unix_ns / 1000000000ULL);
  uint32_t const rem_ms = (uint32_t)((unix_ns % 1000000000ULL) / 1000000ULL);
  struct tm tm_buf;
#ifdef _WIN32
  gmtime_s(&tm_buf, &sec);
#else
  gmtime_r(&sec, &tm_buf);
#endif
  char time_str[64];
  strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S", &tm_buf);
  snprintf(buf, buf_size, "%s.%03uZ", time_str, rem_ms);
}

static apx_error_t init_client(apx_log_client_t *client)
{
  if (m_connect_resource_type == MSOCKET_ENDPOINT_UNKNOWN)
  {
    uint16_t dummy_port = 0;
    const char *default_addr = m_use_tls ? "127.0.0.1" : m_connect_address_default;
    m_connect_resource_type = msocket_parse_endpoint(default_addr, &m_connect_address, &dummy_port);
    if (m_use_tls)
    {
      m_connect_port = 5020;
    }
  }

  const char *addr_str = (m_connect_address != NULL) ? adt_str_cstr(m_connect_address) : m_connect_address_default;

#if defined(MSOCKET_ENABLE_TLS)
  if (m_use_tls)
  {
    msocket_tls_config_t tls_config;
    msocket_tls_config_create(&tls_config);
    if (m_ca_cert != NULL)
    {
      msocket_tls_config_set_ca_cert(&tls_config, adt_str_cstr(m_ca_cert));
    }
    if (m_client_cert != NULL && m_client_key != NULL)
    {
      msocket_tls_config_set_client_cert(&tls_config, adt_str_cstr(m_client_cert), adt_str_cstr(m_client_key));
    }
    apx_error_t err = apx_log_client_connect_tls(client, addr_str, m_connect_port, &tls_config);
    msocket_tls_config_destroy(&tls_config);
    if (err != APX_NO_ERROR)
    {
      return err;
    }
    return apx_log_client_wait_ready(client, 5000u);
  }
#endif

  switch (m_connect_resource_type)
  {
  case MSOCKET_ENDPOINT_IPV4:
  case MSOCKET_ENDPOINT_NAME: {
    apx_error_t err = apx_log_client_connect_tcp(client, addr_str, m_connect_port);
    if (err != APX_NO_ERROR)
    {
      return err;
    }
    return apx_log_client_wait_ready(client, 5000u);
  }
  case MSOCKET_ENDPOINT_FILE:
  default: {
#ifndef _WIN32
    apx_error_t err = apx_log_client_connect_unix(client, addr_str);
    if (err != APX_NO_ERROR)
    {
      return err;
    }
    return apx_log_client_wait_ready(client, 5000u);
#else
    (void)client;
    (void)addr_str;
    return APX_NOT_IMPLEMENTED_ERROR;
#endif
  }
  }
}

static void on_event_received(void *arg, const uint8_t *data, uint32_t size)
{
  record_ctx_t *ctx = (record_ctx_t *)arg;
  if (ctx == NULL || data == NULL || size < APX_LOG_EVENT_HEADER_SIZE)
  {
    return;
  }

  uint16_t const event_type = (uint16_t)(((uint16_t)data[0]) | (((uint16_t)data[1]) << 8));

  MUTEX_LOCK(ctx->lock);
  if (ctx->is_writer_open)
  {
    if (event_type == APX_LOG_EVENT_STREAM_BEGIN && size >= (APX_LOG_EVENT_HEADER_SIZE + 16u))
    {
      uint64_t const low = (uint64_t)(((uint32_t)data[APX_LOG_EVENT_HEADER_SIZE + 8]) |
        (((uint32_t)data[APX_LOG_EVENT_HEADER_SIZE + 9]) << 8) |
        (((uint32_t)data[APX_LOG_EVENT_HEADER_SIZE + 10]) << 16) |
        (((uint32_t)data[APX_LOG_EVENT_HEADER_SIZE + 11]) << 24));
      uint64_t const high = (uint64_t)(((uint32_t)data[APX_LOG_EVENT_HEADER_SIZE + 12]) |
        (((uint32_t)data[APX_LOG_EVENT_HEADER_SIZE + 13]) << 8) |
        (((uint32_t)data[APX_LOG_EVENT_HEADER_SIZE + 14]) << 16) |
        (((uint32_t)data[APX_LOG_EVENT_HEADER_SIZE + 15]) << 24));
      uint64_t const start_unix_ns = low | (high << 32);
      apx_log_writer_set_start_time(&ctx->writer, start_unix_ns);
    }

    apx_error_t const write_rc = apx_log_writer_write_record(&ctx->writer, data, size);
    if (write_rc == APX_NO_ERROR)
    {
      ctx->record_count++;
    }

    if (event_type == APX_LOG_EVENT_STREAM_END)
    {
      m_running = 0;
    }
  }
  MUTEX_UNLOCK(ctx->lock);
}

static int cmd_record(const char *out_path)
{
  // Open output writer
  MUTEX_INIT(m_record_ctx.lock);
  apx_log_writer_create(&m_record_ctx.writer);
  m_record_ctx.is_writer_open = false;
  m_record_ctx.record_count = 0u;

  apx_error_t open_err = apx_log_writer_open(&m_record_ctx.writer, out_path);
  if (open_err != APX_NO_ERROR)
  {
    fprintf(stderr, "Error: Failed to open output file '%s' (error=%d)\n", out_path, open_err);
    apx_log_writer_destroy(&m_record_ctx.writer);
    MUTEX_DESTROY(m_record_ctx.lock);
    return 1;
  }
  m_record_ctx.is_writer_open = true;

  // Setup client
  apx_log_client_t client;
  apx_error_t const client_err = apx_log_client_create(&client);
  if (client_err != APX_NO_ERROR)
  {
    fprintf(stderr, "Error: Failed to create client (error=%d)\n", client_err);
    apx_log_writer_destroy(&m_record_ctx.writer);
    MUTEX_DESTROY(m_record_ctx.lock);
    return 1;
  }

  apx_log_client_set_event_callback(&client, on_event_received, &m_record_ctx);

  apx_error_t const conn_err = init_client(&client);
  if (conn_err != APX_NO_ERROR)
  {
    fprintf(stderr, "Error: Failed to connect to diagnostic endpoint (error=%d)\n", conn_err);
    apx_log_client_destroy(&client);
    apx_log_writer_destroy(&m_record_ctx.writer);
    MUTEX_DESTROY(m_record_ctx.lock);
    return 1;
  }

  signal(SIGINT, sigint_handler);
  signal(SIGTERM, sigint_handler);

  printf("Recording events to %s (Ctrl+C to stop)...\n", out_path);
  fflush(stdout);

  while (m_running && apx_log_client_is_connected(&client))
  {
    SLEEP(50);
  }

  // Finalize
  MUTEX_LOCK(m_record_ctx.lock);
  m_record_ctx.is_writer_open = false;
  uint64_t const end_time_ns = apx_get_unix_time_ns();
  apx_log_writer_finalize(&m_record_ctx.writer, end_time_ns);
  uint64_t const final_count = m_record_ctx.writer.header.record_count;
  apx_log_writer_close(&m_record_ctx.writer);
  MUTEX_UNLOCK(m_record_ctx.lock);

  apx_log_client_disconnect(&client);
  apx_log_client_destroy(&client);
  apx_log_writer_destroy(&m_record_ctx.writer);
  MUTEX_DESTROY(m_record_ctx.lock);

  printf("\nRecording stopped. Finalized %" PRIu64 " records to %s\n", final_count, out_path);
  return 0;
}

static int cmd_info(const char *in_path)
{
  apx_log_reader_t reader;
  apx_log_reader_create(&reader);
  apx_error_t const err = apx_log_reader_open(&reader, in_path);
  if (err != APX_NO_ERROR)
  {
    fprintf(stderr, "Error: Failed to open '%s' (error=%d)\n", in_path, err);
    apx_log_reader_destroy(&reader);
    return 1;
  }

  bool const is_finalized = (reader.header.flags & APX_LOG_FLAG_FINALIZED) != 0u;
  const char *status_str = is_finalized ? "finalized" : "unfinalized / crashed";

  if (m_json_output)
  {
    dtl_hv_t *hv = dtl_hv_new();
    dtl_hv_set_cstr(hv, "file", (dtl_dv_t *)dtl_sv_make_cstr(in_path), false);
    dtl_hv_set_cstr(hv, "format_major", (dtl_dv_t *)dtl_sv_make_u32((uint32_t)reader.header.format_major), false);
    dtl_hv_set_cstr(hv, "format_minor", (dtl_dv_t *)dtl_sv_make_u32((uint32_t)reader.header.format_minor), false);
    dtl_hv_set_cstr(hv, "status", (dtl_dv_t *)dtl_sv_make_cstr(status_str), false);
    dtl_hv_set_cstr(hv, "finalized", (dtl_dv_t *)dtl_sv_make_bool(is_finalized), false);
    dtl_hv_set_cstr(
      hv, "capture_start_unix_ns", (dtl_dv_t *)dtl_sv_make_u64(reader.header.capture_start_unix_ns), false);
    dtl_hv_set_cstr(hv, "capture_end_unix_ns", (dtl_dv_t *)dtl_sv_make_u64(reader.header.capture_end_unix_ns), false);
    if (is_finalized && reader.header.capture_end_unix_ns >= reader.header.capture_start_unix_ns)
    {
      uint64_t const dur_ns = reader.header.capture_end_unix_ns - reader.header.capture_start_unix_ns;
      dtl_hv_set_cstr(hv, "duration_ns", (dtl_dv_t *)dtl_sv_make_u64(dur_ns), false);
    }
    dtl_hv_set_cstr(hv, "record_count", (dtl_dv_t *)dtl_sv_make_u64(reader.header.record_count), false);
    dtl_hv_set_cstr(hv, "file_size", (dtl_dv_t *)dtl_sv_make_u64(reader.file_size), false);

    dtl_json_dump((dtl_dv_t *)hv, stdout, 2, false);
    printf("\n");
    dtl_hv_delete(hv);
  }
  else
  {
    char start_str[128];
    char end_str[128];
    format_iso8601(start_str, sizeof(start_str), reader.header.capture_start_unix_ns);
    format_iso8601(end_str, sizeof(end_str), reader.header.capture_end_unix_ns);

    printf("Container:       %s\n", in_path);
    printf("Format Version:  %u.%u\n", reader.header.format_major, reader.header.format_minor);
    printf("Status:          %s\n", status_str);
    printf("Capture Start:   %s (%" PRIu64 " ns)\n", start_str, reader.header.capture_start_unix_ns);
    if (is_finalized)
    {
      printf("Capture End:     %s (%" PRIu64 " ns)\n", end_str, reader.header.capture_end_unix_ns);
      if (reader.header.capture_end_unix_ns >= reader.header.capture_start_unix_ns)
      {
        uint64_t const dur_ns = reader.header.capture_end_unix_ns - reader.header.capture_start_unix_ns;
        double const dur_sec = (double)dur_ns / 1000000000.0;
        printf("Duration:        %.3fs (%" PRIu64 " ns)\n", dur_sec, dur_ns);
      }
    }
    printf("Record Count:    %" PRIu64 "\n", reader.header.record_count);
    printf("File Size:       %" PRIu64 " bytes\n", reader.file_size);
  }

  apx_log_reader_close(&reader);
  apx_log_reader_destroy(&reader);
  return 0;
}

static int cmd_print(const char *in_path)
{
  apx_log_reader_t reader;
  apx_log_reader_create(&reader);
  apx_error_t const err = apx_log_reader_open(&reader, in_path);
  if (err != APX_NO_ERROR)
  {
    fprintf(stderr, "Error: Failed to open '%s' (error=%d)\n", in_path, err);
    apx_log_reader_destroy(&reader);
    return 1;
  }

  uint8_t *record_buf = NULL;
  uint32_t record_size = 0u;

  while (apx_log_reader_read_next_record(&reader, &record_buf, &record_size) == APX_NO_ERROR)
  {
    apx_log_event_t event;
    if (apx_log_event_parse(&event, record_buf, record_size) == APX_NO_ERROR)
    {
      if (m_json_output)
      {
        apx_log_event_print_json(&event, stdout);
      }
      else
      {
        apx_log_event_print_text(&event, stdout);
      }
    }
    free(record_buf);
    record_buf = NULL;
  }

  apx_log_reader_close(&reader);
  apx_log_reader_destroy(&reader);
  return 0;
}

//////////////////////////////////////////////////////////////////////////////
// MAIN ENTRY POINT
//////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    print_usage(APP_NAME);
    return 1;
  }

  const char *command = NULL;
  const char *file_path = NULL;
  bool display_help = false;
  bool display_version = false;

  for (int i = 1; i < argc; ++i)
  {
    if (argv[i][0] == '-')
    {
      if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
      {
        display_help = true;
      }
      else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0)
      {
        display_version = true;
      }
      else if (strcmp(argv[i], "--json") == 0)
      {
        parse_arg(NULL, "json", NULL);
      }
      else if (strcmp(argv[i], "--tls") == 0)
      {
        parse_arg(NULL, "tls", NULL);
      }
      else if ((strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--connect") == 0) && (i + 1 < argc))
      {
        if (parse_arg(NULL, "connect", argv[++i]) != ARGPARSE_SUCCESS)
        {
          fprintf(stderr, "Error: Invalid endpoint '%s'\n", argv[i]);
          return 1;
        }
      }
      else if (strcmp(argv[i], "--ca-cert") == 0 && (i + 1 < argc))
      {
        parse_arg(NULL, "ca-cert", argv[++i]);
      }
      else if (strcmp(argv[i], "--client-cert") == 0 && (i + 1 < argc))
      {
        parse_arg(NULL, "client-cert", argv[++i]);
      }
      else if (strcmp(argv[i], "--client-key") == 0 && (i + 1 < argc))
      {
        parse_arg(NULL, "client-key", argv[++i]);
      }
      else
      {
        fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
        print_usage(APP_NAME);
        return 1;
      }
    }
    else
    {
      if (command == NULL)
      {
        command = argv[i];
      }
      else if (file_path == NULL)
      {
        file_path = argv[i];
      }
      else
      {
        fprintf(stderr, "Error: Unexpected positional argument '%s'\n", argv[i]);
        return 1;
      }
    }
  }

  if (display_help)
  {
    print_cmd_usage(command);
    return 0;
  }

  if (display_version)
  {
    print_version();
    return 0;
  }

  if (command == NULL)
  {
    print_usage(APP_NAME);
    return 1;
  }

  if (file_path == NULL)
  {
    fprintf(stderr, "Error: Missing required container file argument <FILE>\n");
    print_cmd_usage(command);
    return 1;
  }

  int ret = 0;
  if (strcmp(command, "record") == 0)
  {
    ret = cmd_record(file_path);
  }
  else if (strcmp(command, "info") == 0)
  {
    ret = cmd_info(file_path);
  }
  else if (strcmp(command, "print") == 0)
  {
    ret = cmd_print(file_path);
  }
  else
  {
    fprintf(stderr, "Error: Unknown command '%s'\n\n", command);
    print_usage(APP_NAME);
    ret = 1;
  }

  if (m_connect_address != NULL)
  {
    adt_str_delete(m_connect_address);
  }
  if (m_ca_cert != NULL)
  {
    adt_str_delete(m_ca_cert);
  }
  if (m_client_cert != NULL)
  {
    adt_str_delete(m_client_cert);
  }
  if (m_client_key != NULL)
  {
    adt_str_delete(m_client_key);
  }

  return ret;
}
