/*****************************************************************************
 * \file      testsuite_file_manager.c
 * \author    Conny Gustafsson
 * \date      2026-10-20
 * \brief     Unit tests for APX file manager and RemoteFile v1.1 FileStream
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "CuTest.h"
#include "adt_ary.h"
#include "adt_bytearray.h"
#include "apx/error.h"
#include "apx/file_info.h"
#include "apx/file_manager.h"
#include "apx/remotefile.h"
#include "pack.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct mock_endpoint_tag
{
  apx_file_manager_t file_manager;
  adt_ary_t tx_packets; // adt_bytearray_t* (each is one RMF frame: AddressHeader + payload)
  adt_ary_t rx_blocks; // adt_bytearray_t* (delivered blocks via write_notify)
  uint32_t last_write_offset;
  int32_t max_buffer_size;
  uint32_t connection_id;
  rmf_version_id_t version_id;
  apx_error_t last_nack_error;
  char last_nack_name[RMF_MAX_FILE_NAME_SIZE + 1u];
  uint32_t open_notify_count;
  uint32_t close_notify_count;
} mock_endpoint_t;

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void mock_endpoint_create(mock_endpoint_t *self, uint8_t mode, int32_t max_buffer_size);
static void mock_endpoint_destroy(mock_endpoint_t *self);
static void mock_endpoint_deliver_all(mock_endpoint_t *sender, mock_endpoint_t *receiver);
static apx_error_t mock_endpoint_inject_raw_frame(
  mock_endpoint_t *target, uint32_t address, bool more_bit, uint8_t const *payload, apx_size_t payload_size);
static int32_t on_mock_transmit_max_buffer_size(void *arg);
static int32_t on_mock_transmit_current_bytes_available(void *arg);
static void on_mock_transmit_begin(void *arg);
static void on_mock_transmit_end(void *arg);
static apx_error_t on_mock_transmit_data_message(
  void *arg, uint32_t write_address, bool more_bit, uint8_t const *data, int32_t size, int32_t *bytes_available);
static uint32_t on_mock_get_connection_id(void *arg);
static rmf_version_id_t on_mock_get_version_id(void *arg);
static apx_connection_type_t on_mock_get_connection_type(void *arg);
static apx_error_t on_mock_nack_notification(void *arg, apx_error_t error_code, char const *name);
static apx_error_t on_file_open_notify(void *arg, apx_file_t *file);
static apx_error_t on_file_close_notify(void *arg, apx_file_t *file);
static apx_error_t on_file_write_notify(void *arg, apx_file_t *file, uint32_t offset, uint8_t const *src, uint32_t len);

static void test_stream_open_negotiation(CuTest *tc);
static void test_stream_open_errors(CuTest *tc);
static void test_stream_block_framing_and_boundaries(CuTest *tc);
static void test_stream_block_protocol_errors(CuTest *tc);
static void test_stream_window_one(CuTest *tc);
static void test_stream_window_batching_and_validation(CuTest *tc);
static void test_stream_isolation_and_control_progress(CuTest *tc);
static void test_stream_close_abort_revoke_and_disconnect(CuTest *tc);
static void test_small_soa_and_large_data_pipeline(CuTest *tc);
static void test_command_queue_cleanup_on_destroy(CuTest *tc);
static void test_file_manager_publish_and_receive_compressed_files(CuTest *tc);
static void test_file_manager_compressed_publish_error_reporting(CuTest *tc);
static void test_file_manager_rejects_oversized_and_misaligned_control_writes(CuTest *tc);
static void test_file_manager_rejects_trailing_bytes_after_publish_name(CuTest *tc);
static void test_stream_abort_interrupts_partial_block_and_reopens(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite *testsuite_apx_file_manager(void)
{
  CuSuite *suite = CuSuiteNew();

  SUITE_ADD_TEST(suite, test_stream_open_negotiation);
  SUITE_ADD_TEST(suite, test_stream_open_errors);
  SUITE_ADD_TEST(suite, test_stream_block_framing_and_boundaries);
  SUITE_ADD_TEST(suite, test_stream_block_protocol_errors);
  SUITE_ADD_TEST(suite, test_stream_window_one);
  SUITE_ADD_TEST(suite, test_stream_window_batching_and_validation);
  SUITE_ADD_TEST(suite, test_stream_isolation_and_control_progress);
  SUITE_ADD_TEST(suite, test_stream_close_abort_revoke_and_disconnect);
  SUITE_ADD_TEST(suite, test_small_soa_and_large_data_pipeline);
  SUITE_ADD_TEST(suite, test_command_queue_cleanup_on_destroy);
  SUITE_ADD_TEST(suite, test_file_manager_publish_and_receive_compressed_files);
  SUITE_ADD_TEST(suite, test_file_manager_compressed_publish_error_reporting);
  SUITE_ADD_TEST(suite, test_file_manager_rejects_oversized_and_misaligned_control_writes);
  SUITE_ADD_TEST(suite, test_file_manager_rejects_trailing_bytes_after_publish_name);
  SUITE_ADD_TEST(suite, test_stream_abort_interrupts_partial_block_and_reopens);

  return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void mock_endpoint_create(mock_endpoint_t *self, uint8_t mode, int32_t max_buffer_size)
{
  memset(self, 0, sizeof(mock_endpoint_t));
  self->max_buffer_size = max_buffer_size;
  self->connection_id = 1u;
  self->version_id = RMF_PROTOCOL_VERSION_ID_1_1;
  self->last_nack_error = APX_NO_ERROR;
  adt_ary_create(&self->tx_packets, adt_bytearray_vdelete);
  adt_ary_create(&self->rx_blocks, adt_bytearray_vdelete);

  apx_connection_interface_t iface;
  memset(&iface, 0, sizeof(iface));
  iface.arg = self;
  iface.transmit_max_buffer_size = on_mock_transmit_max_buffer_size;
  iface.transmit_current_bytes_avaiable = on_mock_transmit_current_bytes_available;
  iface.transmit_begin = on_mock_transmit_begin;
  iface.transmit_end = on_mock_transmit_end;
  iface.transmit_data_message = on_mock_transmit_data_message;
  iface.get_connection_id = on_mock_get_connection_id;
  iface.get_remotefile_protocol_version_id = on_mock_get_version_id;
  iface.get_connection_type = on_mock_get_connection_type;
  iface.nack_notification = on_mock_nack_notification;

  apx_error_t rc = apx_file_manager_create(&self->file_manager, mode, &iface);
  assert(rc == APX_NO_ERROR);
  (void)rc;
}

static void mock_endpoint_destroy(mock_endpoint_t *self)
{
  apx_file_manager_destroy(&self->file_manager);
  adt_ary_destroy(&self->tx_packets);
  adt_ary_destroy(&self->rx_blocks);
}

static void mock_endpoint_deliver_all(mock_endpoint_t *sender, mock_endpoint_t *receiver)
{
  (void)apx_file_manager_run(&sender->file_manager);
  int32_t const count = adt_ary_length(&sender->tx_packets);
  for (int32_t i = 0; i < count; i++)
  {
    adt_bytearray_t *pkt = (adt_bytearray_t *)adt_ary_value(&sender->tx_packets, i);
    assert(pkt != NULL);
    (void)apx_file_manager_message_received(
      &receiver->file_manager, adt_bytearray_const_data(pkt), (apx_size_t)adt_bytearray_length(pkt));
  }
  adt_ary_clear(&sender->tx_packets);
}

static apx_error_t mock_endpoint_inject_raw_frame(
  mock_endpoint_t *target, uint32_t address, bool more_bit, uint8_t const *payload, apx_size_t payload_size)
{
  uint8_t header[RMF_HIGH_ADDR_SIZE];
  apx_size_t const hdr_len = rmf_address_encode(header, sizeof(header), address, more_bit);
  assert(hdr_len > 0u);
  uint8_t *frame = (uint8_t *)malloc(hdr_len + payload_size);
  assert(frame != NULL);
  memcpy(frame, header, hdr_len);
  if ((payload != NULL) && (payload_size > 0u))
  {
    memcpy(frame + hdr_len, payload, payload_size);
  }
  apx_error_t const err = apx_file_manager_message_received(&target->file_manager, frame, hdr_len + payload_size);
  free(frame);
  return err;
}

static int32_t on_mock_transmit_max_buffer_size(void *arg)
{
  mock_endpoint_t *self = (mock_endpoint_t *)arg;
  return self->max_buffer_size;
}

static int32_t on_mock_transmit_current_bytes_available(void *arg)
{
  mock_endpoint_t *self = (mock_endpoint_t *)arg;
  return self->max_buffer_size;
}

static void on_mock_transmit_begin(void *arg) { (void)arg; }

static void on_mock_transmit_end(void *arg) { (void)arg; }

static apx_error_t on_mock_transmit_data_message(
  void *arg, uint32_t write_address, bool more_bit, uint8_t const *data, int32_t size, int32_t *bytes_available)
{
  mock_endpoint_t *self = (mock_endpoint_t *)arg;
  uint8_t header[RMF_HIGH_ADDR_SIZE];
  apx_size_t const hdr_size = rmf_address_encode(header, sizeof(header), write_address, more_bit);
  if (hdr_size == 0u)
  {
    return APX_INVALID_ADDRESS_ERROR;
  }
  adt_bytearray_t *pkt = adt_bytearray_new();
  if (pkt == NULL)
  {
    return APX_MEM_ERROR;
  }
  (void)adt_bytearray_append(pkt, header, (uint32_t)hdr_size);
  if ((data != NULL) && (size > 0))
  {
    (void)adt_bytearray_append(pkt, data, (uint32_t)size);
  }
  adt_ary_push(&self->tx_packets, pkt);
  if (bytes_available != NULL)
  {
    *bytes_available = self->max_buffer_size;
  }
  return APX_NO_ERROR;
}

static uint32_t on_mock_get_connection_id(void *arg)
{
  mock_endpoint_t *self = (mock_endpoint_t *)arg;
  return self->connection_id;
}

static rmf_version_id_t on_mock_get_version_id(void *arg)
{
  mock_endpoint_t *self = (mock_endpoint_t *)arg;
  return self->version_id;
}

static apx_connection_type_t on_mock_get_connection_type(void *arg)
{
  (void)arg;
  return APX_CONNECTION_TYPE_DEFAULT;
}

static apx_error_t on_mock_nack_notification(void *arg, apx_error_t error_code, char const *name)
{
  mock_endpoint_t *self = (mock_endpoint_t *)arg;
  self->last_nack_error = error_code;
  if (name != NULL)
  {
    strncpy(self->last_nack_name, name, sizeof(self->last_nack_name) - 1u);
    self->last_nack_name[sizeof(self->last_nack_name) - 1u] = '\0';
  }
  else
  {
    self->last_nack_name[0] = '\0';
  }
  return APX_NO_ERROR;
}

static apx_error_t on_file_open_notify(void *arg, apx_file_t *file)
{
  mock_endpoint_t *self = (mock_endpoint_t *)arg;
  (void)file;
  self->open_notify_count++;
  return APX_NO_ERROR;
}

static apx_error_t on_file_close_notify(void *arg, apx_file_t *file)
{
  mock_endpoint_t *self = (mock_endpoint_t *)arg;
  (void)file;
  self->close_notify_count++;
  return APX_NO_ERROR;
}

static apx_error_t on_file_write_notify(void *arg, apx_file_t *file, uint32_t offset, uint8_t const *src, uint32_t len)
{
  mock_endpoint_t *self = (mock_endpoint_t *)arg;
  (void)file;
  self->last_write_offset = offset;
  adt_bytearray_t *block = adt_bytearray_new();
  assert(block != NULL);
  (void)adt_bytearray_append(block, src, len);
  adt_ary_push(&self->rx_blocks, block);
  return APX_NO_ERROR;
}

static void attach_file_handlers(apx_file_t *file, mock_endpoint_t *endpoint)
{
  apx_file_notification_handler_t handler = {
    .arg = endpoint,
    .open_notify = on_file_open_notify,
    .close_notify = on_file_close_notify,
    .write_notify = on_file_write_notify,
  };
  apx_file_set_notification_handler(file, &handler);
}

static void test_stream_open_negotiation(CuTest *tc)
{
  mock_endpoint_t pub;
  mock_endpoint_t sub;
  mock_endpoint_create(&pub, APX_SERVER_MODE, 4096);
  mock_endpoint_create(&sub, APX_CLIENT_MODE, 4096);

  // Case 1: Receiver MaxBlockSize (4096) < Published FileSize (16384), Publisher local_max_window (4) < Requested (16)
  rmf_file_info_t *info1 = rmf_file_info_make_stream("server.log", 16384u, 0x20000000u);
  CuAssertPtrNotNull(tc, info1);
  apx_file_t *pub_file1 = apx_file_manager_create_local_file(&pub.file_manager, info1);
  CuAssertPtrNotNull(tc, pub_file1);
  apx_file_set_stream_local_max_window_blocks(pub_file1, 4u);
  attach_file_handlers(pub_file1, &pub);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, info1));
  rmf_file_info_delete(info1);

  // Case 2: Published FileSize (2048) < Receiver MaxBlockSize (8192)
  rmf_file_info_t *info2 = rmf_file_info_make_stream("diag.rsp", 2048u, 0x20010000u);
  CuAssertPtrNotNull(tc, info2);
  apx_file_t *pub_file2 = apx_file_manager_create_local_file(&pub.file_manager, info2);
  CuAssertPtrNotNull(tc, pub_file2);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, info2));
  rmf_file_info_delete(info2);

  mock_endpoint_deliver_all(&pub, &sub);

  apx_file_t *sub_file1 = apx_file_manager_find_remote_file_by_name(&sub.file_manager, "server.log");
  CuAssertPtrNotNull(tc, sub_file1);
  CuAssertTrue(tc, apx_file_is_stream(sub_file1));
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(sub_file1));
  attach_file_handlers(sub_file1, &sub);

  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 4096u, 16u));
  CuAssertIntEquals(tc, APX_STREAM_STATE_OPEN_PENDING, apx_file_get_stream_state(sub_file1));

  mock_endpoint_deliver_all(&sub, &pub);
  CuAssertIntEquals(tc, APX_STREAM_STATE_OPEN, apx_file_get_stream_state(pub_file1));
  CuAssertUIntEquals(tc, 4096u, apx_file_get_stream_block_size(pub_file1));
  CuAssertUIntEquals(tc, 4u, apx_file_get_stream_window_blocks(pub_file1));
  CuAssertUIntEquals(tc, 1u, pub.open_notify_count);

  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, APX_STREAM_STATE_OPEN, apx_file_get_stream_state(sub_file1));
  CuAssertUIntEquals(tc, 4096u, apx_file_get_stream_block_size(sub_file1));
  CuAssertUIntEquals(tc, 4u, apx_file_get_stream_window_blocks(sub_file1));
  CuAssertUIntEquals(tc, 1u, sub.open_notify_count);

  // Open stream 2 where published FileSize (2048) < requested MaxBlockSize (8192)
  apx_file_t *sub_file2 = apx_file_manager_find_remote_file_by_name(&sub.file_manager, "diag.rsp");
  CuAssertPtrNotNull(tc, sub_file2);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20010000u, 8192u, 8u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, APX_STREAM_STATE_OPEN, apx_file_get_stream_state(sub_file2));
  CuAssertUIntEquals(tc, 2048u, apx_file_get_stream_block_size(sub_file2));
  CuAssertUIntEquals(tc, 8u, apx_file_get_stream_window_blocks(sub_file2));

  mock_endpoint_destroy(&sub);
  mock_endpoint_destroy(&pub);
}

static void test_stream_open_errors(CuTest *tc)
{
  mock_endpoint_t pub;
  mock_endpoint_t sub;
  mock_endpoint_create(&pub, APX_SERVER_MODE, 4096);
  mock_endpoint_create(&sub, APX_CLIENT_MODE, 4096);

  rmf_file_info_t *stream_info = rmf_file_info_make_stream("server.log", 4096u, 0x20000000u);
  apx_file_t *pub_stream = apx_file_manager_create_local_file(&pub.file_manager, stream_info);
  CuAssertPtrNotNull(tc, pub_stream);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, stream_info));
  rmf_file_info_delete(stream_info);

  rmf_file_info_t *fixed_info = rmf_file_info_make_fixed("Node.out", 64u, 0x00001000u);
  apx_file_t *pub_fixed = apx_file_manager_create_local_file(&pub.file_manager, fixed_info);
  CuAssertPtrNotNull(tc, pub_fixed);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, fixed_info));
  rmf_file_info_delete(fixed_info);

  mock_endpoint_deliver_all(&pub, &sub);

  // 1. Local validation of zero MaxBlockSize / zero WindowBlocks / non-stream / unknown address
  CuAssertIntEquals(
    tc, APX_INVALID_ARGUMENT_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 0u, 4u));
  CuAssertIntEquals(
    tc, APX_INVALID_ARGUMENT_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 1024u, 0u));
  CuAssertIntEquals(
    tc, APX_FILE_NOT_FOUND_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20099000u, 1024u, 4u));
  CuAssertIntEquals(tc, APX_INVALID_FILE_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x00001000u, 64u, 4u));

  // 2. Ordinary RMF_CMD_OPEN_FILE_MSG against a FileStream is rejected locally and on wire with APX_UNSUPPORTED_ERROR
  CuAssertIntEquals(tc, APX_UNSUPPORTED_ERROR, apx_file_manager_send_open_file_request(&sub.file_manager, 0x20000000u));
  uint8_t raw_open[RMF_CMD_TYPE_SIZE + RMF_FILE_OPEN_CMD_SIZE];
  CuAssertUIntEquals(tc, sizeof(raw_open), rmf_encode_open_file_cmd(raw_open, sizeof(raw_open), 0x20000000u));
  CuAssertIntEquals(tc, APX_UNSUPPORTED_ERROR,
    mock_endpoint_inject_raw_frame(&pub, RMF_CMD_AREA_START_ADDRESS, false, raw_open, sizeof(raw_open)));
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, APX_UNSUPPORTED_ERROR, sub.last_nack_error);

  // 3. Wire RMF_CMD_STREAM_OPEN_MSG against a non-stream file returns
  // RMF_CMD_STREAM_OPEN_RSP_MSG(APX_INVALID_FILE_ERROR)
  uint8_t raw_stream_open[RMF_STREAM_OPEN_CMD_SIZE];
  rmf_stream_open_cmd_t bad_open = {
    .start_address = 0x00001000u,
    .max_block_size = 64u,
    .window_blocks = 4u,
    .flags = 0u,
  };
  CuAssertUIntEquals(
    tc, sizeof(raw_stream_open), rmf_encode_stream_open_cmd(raw_stream_open, sizeof(raw_stream_open), &bad_open));
  CuAssertIntEquals(tc, APX_NO_ERROR,
    mock_endpoint_inject_raw_frame(&pub, RMF_CMD_AREA_START_ADDRESS, false, raw_stream_open, sizeof(raw_stream_open)));
  (void)apx_file_manager_run(&pub.file_manager);
  CuAssertIntEquals(tc, 1, adt_ary_length(&pub.tx_packets));
  adt_bytearray_t *rsp_pkt = (adt_bytearray_t *)adt_ary_value(&pub.tx_packets, 0);
  rmf_stream_open_rsp_cmd_t rsp_cmd;
  uint8_t const *rsp_data = adt_bytearray_const_data(rsp_pkt) + RMF_HIGH_ADDR_SIZE;
  CuAssertUIntEquals(tc, RMF_STREAM_OPEN_RSP_CMD_SIZE,
    rmf_decode_stream_open_rsp_cmd(rsp_data, rsp_data + RMF_STREAM_OPEN_RSP_CMD_SIZE, &rsp_cmd));
  CuAssertUIntEquals(tc, (uint32_t)APX_INVALID_FILE_ERROR, rsp_cmd.status);
  adt_ary_clear(&pub.tx_packets);

  // 4. Wire RMF_CMD_STREAM_OPEN_MSG with non-zero flags returns APX_INVALID_ARGUMENT_ERROR
  bad_open.start_address = 0x20000000u;
  bad_open.flags = 1u;
  (void)rmf_encode_stream_open_cmd(raw_stream_open, sizeof(raw_stream_open), &bad_open);
  (void)mock_endpoint_inject_raw_frame(
    &pub, RMF_CMD_AREA_START_ADDRESS, false, raw_stream_open, sizeof(raw_stream_open));
  (void)apx_file_manager_run(&pub.file_manager);
  rsp_pkt = (adt_bytearray_t *)adt_ary_value(&pub.tx_packets, 0);
  rsp_data = adt_bytearray_const_data(rsp_pkt) + RMF_HIGH_ADDR_SIZE;
  (void)rmf_decode_stream_open_rsp_cmd(rsp_data, rsp_data + RMF_STREAM_OPEN_RSP_CMD_SIZE, &rsp_cmd);
  CuAssertUIntEquals(tc, (uint32_t)APX_INVALID_ARGUMENT_ERROR, rsp_cmd.status);
  adt_ary_clear(&pub.tx_packets);

  // 5. Open stream normally, then verify duplicate open returns APX_BUSY_ERROR
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 1024u, 4u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, APX_BUSY_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 1024u, 4u));

  bad_open.flags = 0u;
  (void)rmf_encode_stream_open_cmd(raw_stream_open, sizeof(raw_stream_open), &bad_open);
  (void)mock_endpoint_inject_raw_frame(
    &pub, RMF_CMD_AREA_START_ADDRESS, false, raw_stream_open, sizeof(raw_stream_open));
  (void)apx_file_manager_run(&pub.file_manager);
  rsp_pkt = (adt_bytearray_t *)adt_ary_value(&pub.tx_packets, 0);
  rsp_data = adt_bytearray_const_data(rsp_pkt) + RMF_HIGH_ADDR_SIZE;
  (void)rmf_decode_stream_open_rsp_cmd(rsp_data, rsp_data + RMF_STREAM_OPEN_RSP_CMD_SIZE, &rsp_cmd);
  CuAssertUIntEquals(tc, (uint32_t)APX_BUSY_ERROR, rsp_cmd.status);
  adt_ary_clear(&pub.tx_packets);

  mock_endpoint_destroy(&sub);
  mock_endpoint_destroy(&pub);
}

static void test_stream_block_framing_and_boundaries(CuTest *tc)
{
  mock_endpoint_t pub;
  mock_endpoint_t sub;
  // Use a small max_buffer_size (128 bytes -> max_chunk_size = 112 bytes) on publisher to test MORE_BIT fragmentation
  mock_endpoint_create(&pub, APX_SERVER_MODE, 128);
  mock_endpoint_create(&sub, APX_CLIENT_MODE, 4096);

  rmf_file_info_t *info = rmf_file_info_make_stream("server.log", 1024u, 0x20000000u);
  apx_file_t *pub_file = apx_file_manager_create_local_file(&pub.file_manager, info);
  CuAssertPtrNotNull(tc, pub_file);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, info));
  rmf_file_info_delete(info);

  mock_endpoint_deliver_all(&pub, &sub);
  apx_file_t *sub_file = apx_file_manager_find_remote_file_by_name(&sub.file_manager, "server.log");
  CuAssertPtrNotNull(tc, sub_file);
  attach_file_handlers(sub_file, &sub);

  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 1024u, 8u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);

  // Block 1: 300 bytes (fragmented into 3 RMFP frames: 112 + 112 + 76 bytes)
  uint8_t block1[300];
  for (size_t i = 0; i < sizeof(block1); i++)
  {
    block1[i] = (uint8_t)(i & 0xFFu);
  }
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_file_manager_send_local_const_data(&pub.file_manager, 0x20000000u, block1, sizeof(block1)));
  (void)apx_file_manager_run(&pub.file_manager);
  // Verify publisher split Block 1 into 3 RMF packets
  CuAssertIntEquals(tc, 3, adt_ary_length(&pub.tx_packets));

  // Deliver first 2 fragments and verify no partial block is delivered to consumer
  for (int32_t i = 0; i < 2; i++)
  {
    adt_bytearray_t *pkt = (adt_bytearray_t *)adt_ary_value(&pub.tx_packets, i);
    CuAssertIntEquals(tc, APX_NO_ERROR,
      apx_file_manager_message_received(
        &sub.file_manager, adt_bytearray_const_data(pkt), (apx_size_t)adt_bytearray_length(pkt)));
    CuAssertIntEquals(tc, 0, adt_ary_length(&sub.rx_blocks));
  }
  // Deliver 3rd fragment (MORE_BIT = 0) and verify complete 300-byte block is delivered atomically
  adt_bytearray_t *pkt2 = (adt_bytearray_t *)adt_ary_value(&pub.tx_packets, 2);
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_file_manager_message_received(
      &sub.file_manager, adt_bytearray_const_data(pkt2), (apx_size_t)adt_bytearray_length(pkt2)));
  adt_ary_clear(&pub.tx_packets);

  CuAssertIntEquals(tc, 1, adt_ary_length(&sub.rx_blocks));
  CuAssertUIntEquals(tc, 0u, sub.last_write_offset);
  adt_bytearray_t *rx1 = (adt_bytearray_t *)adt_ary_value(&sub.rx_blocks, 0);
  CuAssertUIntEquals(tc, sizeof(block1), adt_bytearray_length(rx1));
  CuAssertIntEquals(tc, 0, memcmp(block1, adt_bytearray_const_data(rx1), sizeof(block1)));

  // Block 2: 24 bytes (single packet, resets to StartAddress = 0x20000000)
  uint8_t const block2[24] = "Hello Stream Block Two!";
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_file_manager_send_local_const_data(&pub.file_manager, 0x20000000u, block2, sizeof(block2)));
  mock_endpoint_deliver_all(&pub, &sub);

  CuAssertIntEquals(tc, 2, adt_ary_length(&sub.rx_blocks));
  adt_bytearray_t *rx2 = (adt_bytearray_t *)adt_ary_value(&sub.rx_blocks, 1);
  CuAssertUIntEquals(tc, sizeof(block2), adt_bytearray_length(rx2));
  CuAssertIntEquals(tc, 0, memcmp(block2, adt_bytearray_const_data(rx2), sizeof(block2)));

  mock_endpoint_destroy(&sub);
  mock_endpoint_destroy(&pub);
}

static void test_stream_block_protocol_errors(CuTest *tc)
{
  mock_endpoint_t pub;
  mock_endpoint_t sub;
  mock_endpoint_create(&pub, APX_SERVER_MODE, 4096);
  mock_endpoint_create(&sub, APX_CLIENT_MODE, 4096);

  rmf_file_info_t *info = rmf_file_info_make_stream("server.log", 256u, 0x20000000u);
  apx_file_t *pub_file = apx_file_manager_create_local_file(&pub.file_manager, info);
  CuAssertPtrNotNull(tc, pub_file);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, info));
  rmf_file_info_delete(info);
  mock_endpoint_deliver_all(&pub, &sub);

  apx_file_t *sub_file = apx_file_manager_find_remote_file_by_name(&sub.file_manager, "server.log");
  CuAssertPtrNotNull(tc, sub_file);
  attach_file_handlers(sub_file, &sub);

  // 1. Data block before stream open -> rejected with APX_FILE_NOT_OPEN_ERROR and RMF_CMD_STREAM_ABORT_MSG
  uint8_t const sample[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  CuAssertIntEquals(
    tc, APX_FILE_NOT_OPEN_ERROR, mock_endpoint_inject_raw_frame(&sub, 0x20000000u, false, sample, sizeof(sample)));
  mock_endpoint_deliver_all(&sub, &pub);

  // 2. Open stream with BlockSize = 64
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 64u, 4u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, APX_STREAM_STATE_OPEN, apx_file_get_stream_state(sub_file));

  // Publisher local checks: non-zero offset, zero length, oversized block
  CuAssertIntEquals(tc, APX_INVALID_ADDRESS_ERROR,
    apx_file_manager_send_local_const_data(&pub.file_manager, 0x20000004u, sample, sizeof(sample)));
  CuAssertIntEquals(
    tc, APX_VALUE_LENGTH_ERROR, apx_file_manager_send_local_const_data(&pub.file_manager, 0x20000000u, sample, 0u));
  uint8_t oversized[65];
  memset(oversized, 0xAA, sizeof(oversized));
  CuAssertIntEquals(tc, APX_FILE_TOO_LARGE_ERROR,
    apx_file_manager_send_local_const_data(&pub.file_manager, 0x20000000u, oversized, sizeof(oversized)));

  // 3. Receiver rejects zero-length block with RMF_CMD_STREAM_ABORT_MSG
  CuAssertIntEquals(tc, APX_VALUE_LENGTH_ERROR, mock_endpoint_inject_raw_frame(&sub, 0x20000000u, false, sample, 0u));
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(sub_file));
  mock_endpoint_deliver_all(&sub, &pub);
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(pub_file));
  CuAssertUIntEquals(tc, (uint32_t)APX_VALUE_LENGTH_ERROR, apx_file_get_stream_last_status(pub_file));

  // 4. Reopen and test oversized multi-fragment block (> 64 bytes)
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 64u, 4u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);

  uint8_t chunk40[40];
  memset(chunk40, 0x55, sizeof(chunk40));
  CuAssertIntEquals(
    tc, APX_NO_ERROR, mock_endpoint_inject_raw_frame(&sub, 0x20000000u, true, chunk40, sizeof(chunk40)));
  CuAssertIntEquals(
    tc, APX_FILE_TOO_LARGE_ERROR, mock_endpoint_inject_raw_frame(&sub, 0x20000028u, false, chunk40, sizeof(chunk40)));
  CuAssertIntEquals(tc, 0, adt_ary_length(&sub.rx_blocks));
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(sub_file));
  mock_endpoint_deliver_all(&sub, &pub);
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(pub_file));

  // 5. Reopen and test interleaved / non-contiguous fragment error
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 64u, 4u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);

  CuAssertIntEquals(tc, APX_NO_ERROR, mock_endpoint_inject_raw_frame(&sub, 0x20000000u, true, sample, sizeof(sample)));
  // Non-contiguous continuation address (0x20000010 instead of 0x20000008)
  CuAssertIntEquals(
    tc, APX_INVALID_ADDRESS_ERROR, mock_endpoint_inject_raw_frame(&sub, 0x20000010u, false, sample, sizeof(sample)));
  CuAssertIntEquals(tc, 0, adt_ary_length(&sub.rx_blocks));
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(sub_file));
  mock_endpoint_deliver_all(&sub, &pub);
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(pub_file));

  mock_endpoint_destroy(&sub);
  mock_endpoint_destroy(&pub);
}

static void test_stream_window_one(CuTest *tc)
{
  mock_endpoint_t pub;
  mock_endpoint_t sub;
  mock_endpoint_create(&pub, APX_SERVER_MODE, 4096);
  mock_endpoint_create(&sub, APX_CLIENT_MODE, 4096);

  rmf_file_info_t *info = rmf_file_info_make_stream("server.log", 256u, 0x20000000u);
  apx_file_t *pub_file = apx_file_manager_create_local_file(&pub.file_manager, info);
  CuAssertPtrNotNull(tc, pub_file);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, info));
  rmf_file_info_delete(info);
  mock_endpoint_deliver_all(&pub, &sub);

  apx_file_t *sub_file = apx_file_manager_find_remote_file_by_name(&sub.file_manager, "server.log");
  CuAssertPtrNotNull(tc, sub_file);
  attach_file_handlers(sub_file, &sub);

  // Negotiate WindowBlocks = 1
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 256u, 1u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);

  // Send 3 blocks on publisher: only Block 1 may be transmitted immediately; Blocks 2 and 3 are queued
  uint8_t const b1[4] = {0x11, 0x11, 0x11, 0x11};
  uint8_t const b2[4] = {0x22, 0x22, 0x22, 0x22};
  uint8_t const b3[4] = {0x33, 0x33, 0x33, 0x33};
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_send_local_const_data(&pub.file_manager, 0x20000000u, b1, 4u));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_send_local_const_data(&pub.file_manager, 0x20000000u, b2, 4u));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_send_local_const_data(&pub.file_manager, 0x20000000u, b3, 4u));

  (void)apx_file_manager_run(&pub.file_manager);
  CuAssertIntEquals(tc, 1, adt_ary_length(&pub.tx_packets));
  CuAssertTrue(tc, apx_file_get_stream_transmitted_blocks(pub_file) == 1u);
  CuAssertIntEquals(tc, 2, apx_file_stream_pending_block_count(pub_file));

  // Deliver Block 1 to receiver -> auto-consumed -> sends WINDOW_UPDATE(1)
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, 1, adt_ary_length(&sub.rx_blocks));

  // Deliver WINDOW_UPDATE(1) to publisher -> publisher drains Block 2
  mock_endpoint_deliver_all(&sub, &pub);
  (void)apx_file_manager_run(&pub.file_manager);
  CuAssertIntEquals(tc, 1, adt_ary_length(&pub.tx_packets));
  CuAssertTrue(tc, apx_file_get_stream_transmitted_blocks(pub_file) == 2u);
  CuAssertIntEquals(tc, 1, apx_file_stream_pending_block_count(pub_file));

  // Deliver Block 2 to receiver -> auto-consumed -> sends WINDOW_UPDATE(2)
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, 2, adt_ary_length(&sub.rx_blocks));

  // Deliver WINDOW_UPDATE(2) to publisher -> publisher drains Block 3
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, 3, adt_ary_length(&sub.rx_blocks));
  CuAssertTrue(tc, apx_file_get_stream_transmitted_blocks(pub_file) == 3u);
  CuAssertIntEquals(tc, 0, apx_file_stream_pending_block_count(pub_file));

  mock_endpoint_destroy(&sub);
  mock_endpoint_destroy(&pub);
}

static void test_stream_window_batching_and_validation(CuTest *tc)
{
  mock_endpoint_t pub;
  mock_endpoint_t sub;
  mock_endpoint_create(&pub, APX_SERVER_MODE, 4096);
  mock_endpoint_create(&sub, APX_CLIENT_MODE, 4096);

  rmf_file_info_t *info = rmf_file_info_make_stream("server.log", 256u, 0x20000000u);
  apx_file_t *pub_file = apx_file_manager_create_local_file(&pub.file_manager, info);
  CuAssertPtrNotNull(tc, pub_file);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, info));
  rmf_file_info_delete(info);
  mock_endpoint_deliver_all(&pub, &sub);

  apx_file_t *sub_file = apx_file_manager_find_remote_file_by_name(&sub.file_manager, "server.log");
  CuAssertPtrNotNull(tc, sub_file);
  attach_file_handlers(sub_file, &sub);
  // Use manual consumption to test explicit credit return and batching
  apx_file_set_stream_auto_consume(sub_file, false);

  // Negotiate WindowBlocks = 4 (batching threshold = ceil(4 / 2) = 2)
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 256u, 4u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);

  // Send 6 blocks: publisher stops at 4 blocks on wire, queues 2
  uint8_t const blocks[6] = {1u, 2u, 3u, 4u, 5u, 6u};
  for (size_t i = 0; i < 6u; i++)
  {
    CuAssertIntEquals(
      tc, APX_NO_ERROR, apx_file_manager_send_local_const_data(&pub.file_manager, 0x20000000u, &blocks[i], 1u));
  }
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, 4, adt_ary_length(&sub.rx_blocks));
  CuAssertTrue(tc, apx_file_get_stream_transmitted_blocks(pub_file) == 4u);
  CuAssertIntEquals(tc, 2, apx_file_stream_pending_block_count(pub_file));

  // Consuming 1 block (< threshold 2) does not emit RMF_CMD_STREAM_WINDOW_UPDATE_MSG yet
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_consume_stream_blocks(&sub.file_manager, 0x20000000u, 1u));
  (void)apx_file_manager_run(&sub.file_manager);
  CuAssertIntEquals(tc, 0, adt_ary_length(&sub.tx_packets));

  // Consuming 2nd block reaches threshold 2 -> emits RMF_CMD_STREAM_WINDOW_UPDATE_MSG(ConsumedBlockCount = 2)
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_consume_stream_blocks(&sub.file_manager, 0x20000000u, 1u));
  mock_endpoint_deliver_all(&sub, &pub);
  // Publisher drains remaining 2 queued blocks (blocks 5 and 6)
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, 6, adt_ary_length(&sub.rx_blocks));
  CuAssertTrue(tc, apx_file_get_stream_transmitted_blocks(pub_file) == 6u);
  CuAssertIntEquals(tc, 0, apx_file_stream_pending_block_count(pub_file));

  // Harmless duplicate window update (ConsumedBlockCount = 2) is accepted
  uint8_t raw_win[RMF_STREAM_WINDOW_UPDATE_CMD_SIZE];
  rmf_stream_window_update_cmd_t win_cmd = {
    .start_address = 0x20000000u,
    .consumed_block_count = 2u,
  };
  (void)rmf_encode_stream_window_update_cmd(raw_win, sizeof(raw_win), &win_cmd);
  CuAssertIntEquals(tc, APX_NO_ERROR,
    mock_endpoint_inject_raw_frame(&pub, RMF_CMD_AREA_START_ADDRESS, false, raw_win, sizeof(raw_win)));
  CuAssertIntEquals(tc, APX_STREAM_STATE_OPEN, apx_file_get_stream_state(pub_file));

  // Decreasing ConsumedBlockCount (1 < 2) is rejected with APX_VALUE_RANGE_ERROR and aborts stream
  win_cmd.consumed_block_count = 1u;
  (void)rmf_encode_stream_window_update_cmd(raw_win, sizeof(raw_win), &win_cmd);
  CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR,
    mock_endpoint_inject_raw_frame(&pub, RMF_CMD_AREA_START_ADDRESS, false, raw_win, sizeof(raw_win)));
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(pub_file));
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(sub_file));

  // Reopen with WindowBlocks = 2 and verify excessive ConsumedBlockCount (> transmitted) is rejected
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 256u, 2u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);
  win_cmd.consumed_block_count = 1u; // 0 blocks transmitted so far!
  (void)rmf_encode_stream_window_update_cmd(raw_win, sizeof(raw_win), &win_cmd);
  CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR,
    mock_endpoint_inject_raw_frame(&pub, RMF_CMD_AREA_START_ADDRESS, false, raw_win, sizeof(raw_win)));
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(sub_file));

  // Reopen with WindowBlocks = 1 (auto_consume = false) and verify receiver aborts if publisher exceeds WindowBlocks
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 256u, 1u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);
  uint8_t const b = 0x42;
  CuAssertIntEquals(tc, APX_NO_ERROR, mock_endpoint_inject_raw_frame(&sub, 0x20000000u, false, &b, 1u));
  CuAssertIntEquals(tc, APX_BUFFER_FULL_ERROR, mock_endpoint_inject_raw_frame(&sub, 0x20000000u, false, &b, 1u));
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(sub_file));

  mock_endpoint_destroy(&sub);
  mock_endpoint_destroy(&pub);
}

static void test_stream_isolation_and_control_progress(CuTest *tc)
{
  mock_endpoint_t pub;
  mock_endpoint_t sub;
  mock_endpoint_create(&pub, APX_SERVER_MODE, 4096);
  mock_endpoint_create(&sub, APX_CLIENT_MODE, 4096);

  rmf_file_info_t *info_a = rmf_file_info_make_stream("streamA.log", 256u, 0x20000000u);
  rmf_file_info_t *info_b = rmf_file_info_make_stream("streamB.log", 256u, 0x20010000u);
  apx_file_t *pub_a = apx_file_manager_create_local_file(&pub.file_manager, info_a);
  apx_file_t *pub_b = apx_file_manager_create_local_file(&pub.file_manager, info_b);
  CuAssertPtrNotNull(tc, pub_a);
  CuAssertPtrNotNull(tc, pub_b);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, info_a));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, info_b));
  rmf_file_info_delete(info_a);
  rmf_file_info_delete(info_b);
  mock_endpoint_deliver_all(&pub, &sub);

  apx_file_t *sub_a = apx_file_manager_find_remote_file_by_name(&sub.file_manager, "streamA.log");
  apx_file_t *sub_b = apx_file_manager_find_remote_file_by_name(&sub.file_manager, "streamB.log");
  CuAssertPtrNotNull(tc, sub_a);
  CuAssertPtrNotNull(tc, sub_b);
  attach_file_handlers(sub_a, &sub);
  attach_file_handlers(sub_b, &sub);
  apx_file_set_stream_auto_consume(sub_a, false); // Stall stream A

  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 256u, 1u));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20010000u, 256u, 2u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);

  // Exhaust stream A window (send 2 blocks on A -> 1 sent, 1 queued)
  uint8_t const msg_a1 = 0xA1;
  uint8_t const msg_a2 = 0xA2;
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_file_manager_send_local_const_data(&pub.file_manager, 0x20000000u, &msg_a1, 1u));
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_file_manager_send_local_const_data(&pub.file_manager, 0x20000000u, &msg_a2, 1u));
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, 1, adt_ary_length(&sub.rx_blocks));
  CuAssertIntEquals(tc, 1, apx_file_stream_pending_block_count(pub_a));

  // Stream B still makes full progress even while Stream A is stalled!
  uint8_t const msg_b1 = 0xB1;
  uint8_t const msg_b2 = 0xB2;
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_file_manager_send_local_const_data(&pub.file_manager, 0x20010000u, &msg_b1, 1u));
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_file_manager_send_local_const_data(&pub.file_manager, 0x20010000u, &msg_b2, 1u));
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, 3, adt_ary_length(&sub.rx_blocks));

  // Unstall Stream A via control message -> queued msg_a2 is delivered
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_consume_stream_blocks(&sub.file_manager, 0x20000000u, 1u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, 4, adt_ary_length(&sub.rx_blocks));

  mock_endpoint_destroy(&sub);
  mock_endpoint_destroy(&pub);
}

static void test_stream_close_abort_revoke_and_disconnect(CuTest *tc)
{
  mock_endpoint_t pub;
  mock_endpoint_t sub;
  mock_endpoint_create(&pub, APX_SERVER_MODE, 4096);
  mock_endpoint_create(&sub, APX_CLIENT_MODE, 4096);

  rmf_file_info_t *info = rmf_file_info_make_stream("server.log", 256u, 0x20000000u);
  apx_file_t *pub_file = apx_file_manager_create_local_file(&pub.file_manager, info);
  CuAssertPtrNotNull(tc, pub_file);
  attach_file_handlers(pub_file, &pub);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, info));
  rmf_file_info_delete(info);
  mock_endpoint_deliver_all(&pub, &sub);

  apx_file_t *sub_file = apx_file_manager_find_remote_file_by_name(&sub.file_manager, "server.log");
  CuAssertPtrNotNull(tc, sub_file);
  attach_file_handlers(sub_file, &sub);
  apx_file_set_stream_auto_consume(sub_file, false);

  // 1. Open stream, queue blocks on publisher, then close from receiver -> queued blocks & counters are cleared
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 256u, 1u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);

  uint8_t const b = 0x55;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_send_local_const_data(&pub.file_manager, 0x20000000u, &b, 1u));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_send_local_const_data(&pub.file_manager, 0x20000000u, &b, 1u));
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, 1, apx_file_stream_pending_block_count(pub_file));

  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_send_close_file_request(&sub.file_manager, 0x20000000u));
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(sub_file));
  mock_endpoint_deliver_all(&sub, &pub);
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(pub_file));
  CuAssertIntEquals(tc, 0, apx_file_stream_pending_block_count(pub_file));
  CuAssertTrue(tc, apx_file_get_stream_transmitted_blocks(pub_file) == 0u);
  CuAssertUIntEquals(tc, 1u, pub.close_notify_count);

  // 2. Reopen stream -> counters start at 0 -> abort from publisher
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 256u, 2u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, APX_STREAM_STATE_OPEN, apx_file_get_stream_state(sub_file));

  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_file_manager_abort_stream(&pub.file_manager, 0x20000000u, APX_BUFFER_FULL_ERROR));
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(pub_file));
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(sub_file));
  CuAssertUIntEquals(tc, (uint32_t)APX_BUFFER_FULL_ERROR, apx_file_get_stream_last_status(sub_file));

  // 3. Reopen, start a fragmented block, then disconnect -> partial block is discarded without delivery
  adt_ary_clear(&sub.rx_blocks);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 256u, 2u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);

  uint8_t const frag[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  CuAssertIntEquals(tc, APX_NO_ERROR, mock_endpoint_inject_raw_frame(&sub, 0x20000000u, true, frag, sizeof(frag)));
  apx_file_manager_disconnected(&sub.file_manager);
  CuAssertIntEquals(tc, 0, adt_ary_length(&sub.rx_blocks));
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(sub_file));
  CuAssertUIntEquals(tc, RMF_INVALID_ADDRESS, apx_file_manager_receiver_active_address(&sub.file_manager.receiver));

  // 4. Publisher revoke transitions both local and remote stream to APX_STREAM_STATE_REVOKED
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_revoke_local_file(&pub.file_manager, 0x20000000u));
  CuAssertIntEquals(tc, APX_STREAM_STATE_REVOKED, apx_file_get_stream_state(pub_file));
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, APX_STREAM_STATE_REVOKED, apx_file_get_stream_state(sub_file));

  mock_endpoint_destroy(&sub);
  mock_endpoint_destroy(&pub);
}

static void test_small_soa_and_large_data_pipeline(CuTest *tc)
{
  // 1. Verify apx_build_command_with_data only copies d2 bytes (no out-of-bounds read on 1- or 2-byte stack buffers)
  uint8_t const one_byte = 0x7Au;
  uint8_t const two_bytes[2] = {0x12u, 0x34u};
  apx_command_t cmd;
  memset(&cmd, 0, sizeof(cmd));
  apx_build_command_with_data(&cmd, APX_CMD_SEND_LOCAL_SMALL_DATA, 0x100u, 1u, &one_byte, NULL);
  CuAssertUIntEquals(tc, APX_CMD_SEND_LOCAL_SMALL_DATA, cmd.cmd_type);
  CuAssertUIntEquals(tc, 1u, cmd.data2);
  CuAssertUIntEquals(tc, 0x7Au, cmd.data3.data[0]);

  apx_build_command_with_data(&cmd, APX_CMD_SEND_LOCAL_SMALL_DATA, 0x100u, 2u, two_bytes, NULL);
  CuAssertUIntEquals(tc, 2u, cmd.data2);
  CuAssertUIntEquals(tc, 0x12u, cmd.data3.data[0]);
  CuAssertUIntEquals(tc, 0x34u, cmd.data3.data[1]);

  // 2. Set up fixed local/remote file and verify all 3 tiers (Inline 1..8 B, SOA 9..32 B, Heap > 32 B)
  mock_endpoint_t pub;
  mock_endpoint_t sub;
  mock_endpoint_create(&pub, APX_CLIENT_MODE, 4096);
  mock_endpoint_create(&sub, APX_SERVER_MODE, 4096);

  rmf_file_info_t *info = rmf_file_info_make_fixed("Node.out", 256u, 0x00001000u);
  CuAssertPtrNotNull(tc, info);
  apx_file_t *pub_file = apx_file_manager_create_local_file(&pub.file_manager, info);
  CuAssertPtrNotNull(tc, pub_file);
  attach_file_handlers(pub_file, &pub);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, info));
  rmf_file_info_delete(info);
  mock_endpoint_deliver_all(&pub, &sub);

  apx_file_t *sub_file = apx_file_manager_find_remote_file_by_name(&sub.file_manager, "Node.out");
  CuAssertPtrNotNull(tc, sub_file);
  attach_file_handlers(sub_file, &sub);
  apx_file_open(sub_file);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_send_open_file_request(&sub.file_manager, 0x00001000u));
  mock_endpoint_deliver_all(&sub, &pub);

  // Tier 1: Inline Small Data (1..8 bytes) via apx_file_manager_send_local_small_data &
  // apx_file_manager_write_local_data
  uint8_t const small_2b[2] = {0xAAu, 0xBBu};
  uint8_t const small_8b[8] = {1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u};
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_file_manager_send_local_small_data(&pub.file_manager, 0x00001000u, small_2b, 2u));
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_file_manager_write_local_data(&pub.file_manager, 0x00001004u, small_8b, sizeof(small_8b)));
  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR,
    apx_file_manager_send_local_small_data(&pub.file_manager, 0x00001000u, small_8b, 9u));

  // Tier 2: Small Data via SOA (9..32 bytes)
  uint8_t soa_20b[20];
  uint8_t soa_32b[32];
  for (size_t i = 0; i < sizeof(soa_20b); i++)
  {
    soa_20b[i] = (uint8_t)(0x10u + i);
  }
  for (size_t i = 0; i < sizeof(soa_32b); i++)
  {
    soa_32b[i] = (uint8_t)(0x40u + i);
  }
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_file_manager_write_local_data(&pub.file_manager, 0x00001010u, soa_20b, sizeof(soa_20b)));
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_file_manager_write_local_data(&pub.file_manager, 0x00001030u, soa_32b, sizeof(soa_32b)));

  // Tier 3: Normal / Large Data via Heap (> 32 bytes)
  uint8_t large_64b[64];
  for (size_t i = 0; i < sizeof(large_64b); i++)
  {
    large_64b[i] = (uint8_t)(0x80u + i);
  }
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_file_manager_write_local_data(&pub.file_manager, 0x00001060u, large_64b, sizeof(large_64b)));

  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, 5, adt_ary_length(&sub.rx_blocks));

  adt_bytearray_t const *rx0 = (adt_bytearray_t const *)adt_ary_value(&sub.rx_blocks, 0);
  adt_bytearray_t const *rx1 = (adt_bytearray_t const *)adt_ary_value(&sub.rx_blocks, 1);
  adt_bytearray_t const *rx2 = (adt_bytearray_t const *)adt_ary_value(&sub.rx_blocks, 2);
  adt_bytearray_t const *rx3 = (adt_bytearray_t const *)adt_ary_value(&sub.rx_blocks, 3);
  adt_bytearray_t const *rx4 = (adt_bytearray_t const *)adt_ary_value(&sub.rx_blocks, 4);

  CuAssertUIntEquals(tc, 2u, adt_bytearray_length(rx0));
  CuAssertIntEquals(tc, 0, memcmp(adt_bytearray_const_data(rx0), small_2b, sizeof(small_2b)));
  CuAssertUIntEquals(tc, 8u, adt_bytearray_length(rx1));
  CuAssertIntEquals(tc, 0, memcmp(adt_bytearray_const_data(rx1), small_8b, sizeof(small_8b)));
  CuAssertUIntEquals(tc, 20u, adt_bytearray_length(rx2));
  CuAssertIntEquals(tc, 0, memcmp(adt_bytearray_const_data(rx2), soa_20b, sizeof(soa_20b)));
  CuAssertUIntEquals(tc, 32u, adt_bytearray_length(rx3));
  CuAssertIntEquals(tc, 0, memcmp(adt_bytearray_const_data(rx3), soa_32b, sizeof(soa_32b)));
  CuAssertUIntEquals(tc, 64u, adt_bytearray_length(rx4));
  CuAssertIntEquals(tc, 0, memcmp(adt_bytearray_const_data(rx4), large_64b, sizeof(large_64b)));

  mock_endpoint_destroy(&sub);
  mock_endpoint_destroy(&pub);
}

static void test_command_queue_cleanup_on_destroy(CuTest *tc)
{
  mock_endpoint_t pub;
  mock_endpoint_t sub;
  mock_endpoint_create(&pub, APX_CLIENT_MODE, 4096);
  mock_endpoint_create(&sub, APX_SERVER_MODE, 4096);

  rmf_file_info_t *info = rmf_file_info_make_fixed("Node.out", 256u, 0x00001000u);
  apx_file_t *pub_file = apx_file_manager_create_local_file(&pub.file_manager, info);
  CuAssertPtrNotNull(tc, pub_file);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, info));
  rmf_file_info_delete(info);
  mock_endpoint_deliver_all(&pub, &sub);

  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_send_open_file_request(&sub.file_manager, 0x00001000u));
  mock_endpoint_deliver_all(&sub, &pub);

  // Enqueue commands across all 3 tiers WITHOUT running the worker, then destroy -> cleanup_cmd_queue must free all
  uint8_t const small_buf[4] = {1u, 2u, 3u, 4u};
  uint8_t const soa_buf[16] = {0};
  uint8_t const large_buf[64] = {0};
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_file_manager_write_local_data(&pub.file_manager, 0x00001000u, small_buf, sizeof(small_buf)));
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_file_manager_write_local_data(&pub.file_manager, 0x00001010u, soa_buf, sizeof(soa_buf)));
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_file_manager_write_local_data(&pub.file_manager, 0x00001020u, large_buf, sizeof(large_buf)));
  CuAssertUIntEquals(tc, 3u, apx_file_manager_get_num_pending_worker_commands(&pub.file_manager));

  mock_endpoint_destroy(&sub);
  mock_endpoint_destroy(&pub);
}

static void test_file_manager_publish_and_receive_compressed_files(CuTest *tc)
{
  mock_endpoint_t pub;
  mock_endpoint_t sub;
  mock_endpoint_create(&pub, APX_CLIENT_MODE, 4096);
  mock_endpoint_create(&sub, APX_SERVER_MODE, 4096);

  uint8_t dummy_digest[RMF_SHA256_SIZE];
  for (size_t i = 0; i < sizeof(dummy_digest); i++)
  {
    dummy_digest[i] = (uint8_t)(0xA0u + i);
  }

  uint8_t dummy_signature[RMF_SIGNATURE_SIZE_ECDSA_P256];
  for (size_t i = 0; i < sizeof(dummy_signature); i++)
  {
    dummy_signature[i] = (uint8_t)(0x10u + i);
  }

  // 1. Publish and receive unsigned compressed file
  rmf_file_info_t *info1 =
    rmf_file_info_make_compressed("CompressedNode.apx", 120u, 450u, 0x00000000u, RMF_DIGEST_TYPE_SHA256, dummy_digest);
  CuAssertPtrNotNull(tc, info1);
  apx_file_t *pub_file1 = apx_file_manager_create_local_file(&pub.file_manager, info1);
  CuAssertPtrNotNull(tc, pub_file1);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, info1));
  rmf_file_info_delete(info1);

  // 2. Publish and receive signed compressed file
  rmf_file_info_t *info2 = rmf_file_info_make_signed_compressed(
    "SignedCompressedNode.apx", 200u, 800u, 0x00001000u, RMF_SIGNATURE_TYPE_ECDSA_P256, dummy_signature);
  CuAssertPtrNotNull(tc, info2);
  apx_file_t *pub_file2 = apx_file_manager_create_local_file(&pub.file_manager, info2);
  CuAssertPtrNotNull(tc, pub_file2);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, info2));
  rmf_file_info_delete(info2);

  mock_endpoint_deliver_all(&pub, &sub);

  // Verify unsigned compressed file on subscriber
  apx_file_t *sub_file1 = apx_file_manager_find_remote_file_by_name(&sub.file_manager, "CompressedNode.apx");
  CuAssertPtrNotNull(tc, sub_file1);
  CuAssertTrue(tc, apx_file_is_compressed(sub_file1));
  CuAssertFalse(tc, apx_file_is_signed(sub_file1));
  CuAssertUIntEquals(tc, 120u, apx_file_get_size(sub_file1));
  CuAssertUIntEquals(tc, 450u, apx_file_get_uncompressed_size(sub_file1));
  CuAssertIntEquals(tc, RMF_COMPRESSION_TYPE_ZLIB_DEFLATE, apx_file_get_compression_type(sub_file1));
  CuAssertIntEquals(tc, RMF_DIGEST_TYPE_SHA256, apx_file_get_digest_type(sub_file1));
  CuAssertIntEquals(tc, 0, memcmp(apx_file_get_digest_data(sub_file1), dummy_digest, RMF_SHA256_SIZE));

  // Verify signed compressed file on subscriber
  apx_file_t *sub_file2 = apx_file_manager_find_remote_file_by_name(&sub.file_manager, "SignedCompressedNode.apx");
  CuAssertPtrNotNull(tc, sub_file2);
  CuAssertTrue(tc, apx_file_is_compressed(sub_file2));
  CuAssertTrue(tc, apx_file_is_signed(sub_file2));
  CuAssertUIntEquals(tc, 200u, apx_file_get_size(sub_file2));
  CuAssertUIntEquals(tc, 800u, apx_file_get_uncompressed_size(sub_file2));
  CuAssertIntEquals(tc, RMF_COMPRESSION_TYPE_ZLIB_DEFLATE, apx_file_get_compression_type(sub_file2));
  CuAssertIntEquals(tc, RMF_SIGNATURE_TYPE_ECDSA_P256, apx_file_get_signature_type(sub_file2));
  CuAssertIntEquals(
    tc, 0, memcmp(apx_file_get_signature_data(sub_file2), dummy_signature, RMF_SIGNATURE_SIZE_ECDSA_P256));

  mock_endpoint_destroy(&sub);
  mock_endpoint_destroy(&pub);
}

static void test_file_manager_compressed_publish_error_reporting(CuTest *tc)
{
  mock_endpoint_t pub;
  mock_endpoint_t sub;
  mock_endpoint_create(&pub, APX_CLIENT_MODE, 4096);
  mock_endpoint_create(&sub, APX_SERVER_MODE, 4096);

  uint8_t cmd_buf[RMF_CMD_AREA_SIZE];
  rmf_publish_compressed_file_cmd_t in_cmd = {
    .start_address = 0x02000000u,
    .uncompressed_size = 200u,
    .compressed_size = 80u,
    .file_type = RMF_FILE_TYPE_FIXED,
    .compression_type = RMF_COMPRESSION_TYPE_ZLIB_DEFLATE,
    .digest_type = RMF_DIGEST_TYPE_NONE,
    .reserved = 0u,
    .name = "BadComp.apx",
  };
  apx_size_t const encoded_size = rmf_encode_publish_compressed_file_cmd(cmd_buf, sizeof(cmd_buf), &in_cmd);
  CuAssertTrue(tc, encoded_size > 0u);

  // 1. Unsupported CompressionType=2 on otherwise valid publication -> NACK(APX_UNSUPPORTED_ERROR, "BadComp")
  packLE(cmd_buf + 18, 2u, UINT16_SIZE);
  CuAssertIntEquals(
    tc, APX_NO_ERROR, mock_endpoint_inject_raw_frame(&sub, RMF_CMD_AREA_START_ADDRESS, false, cmd_buf, encoded_size));
  mock_endpoint_deliver_all(&sub, &pub);
  CuAssertIntEquals(tc, APX_UNSUPPORTED_ERROR, pub.last_nack_error);
  CuAssertStrEquals(tc, "BadComp", pub.last_nack_name);
  CuAssertPtrEquals(tc, NULL, apx_file_manager_find_remote_file_by_name(&sub.file_manager, "BadComp.apx"));
  packLE(cmd_buf + 18, RMF_U16_COMPRESSION_TYPE_ZLIB_DEFLATE, UINT16_SIZE);

  // 2. Nonzero Reserved field -> NACK(APX_INVALID_ARGUMENT_ERROR, "BadComp")
  packLE(cmd_buf + 22, 1u, UINT16_SIZE);
  CuAssertIntEquals(
    tc, APX_NO_ERROR, mock_endpoint_inject_raw_frame(&sub, RMF_CMD_AREA_START_ADDRESS, false, cmd_buf, encoded_size));
  mock_endpoint_deliver_all(&sub, &pub);
  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, pub.last_nack_error);
  CuAssertStrEquals(tc, "BadComp", pub.last_nack_name);
  packLE(cmd_buf + 22, 0u, UINT16_SIZE);

  // 3. Missing filename null terminator -> NACK(APX_INVALID_MSG_ERROR, "")
  CuAssertIntEquals(tc, APX_NO_ERROR,
    mock_endpoint_inject_raw_frame(&sub, RMF_CMD_AREA_START_ADDRESS, false, cmd_buf, encoded_size - 1u));
  mock_endpoint_deliver_all(&sub, &pub);
  CuAssertIntEquals(tc, APX_INVALID_MSG_ERROR, pub.last_nack_error);
  CuAssertStrEquals(tc, "", pub.last_nack_name);

  // 4. Signed compressed publication with unsupported SignatureType=2 -> NACK(APX_UNSUPPORTED_ERROR, "BadSigned")
  rmf_publish_signed_compressed_file_cmd_t signed_cmd = {
    .start_address = 0x03000000u,
    .uncompressed_size = 200u,
    .compressed_size = 80u,
    .file_type = RMF_FILE_TYPE_FIXED,
    .compression_type = RMF_COMPRESSION_TYPE_ZLIB_DEFLATE,
    .signature_type = RMF_SIGNATURE_TYPE_ECDSA_P256,
    .reserved = 0u,
    .name = "BadSigned.apx",
  };
  apx_size_t const signed_size = rmf_encode_publish_signed_compressed_file_cmd(cmd_buf, sizeof(cmd_buf), &signed_cmd);
  CuAssertTrue(tc, signed_size > 0u);
  packLE(cmd_buf + 20, 2u, UINT16_SIZE);
  CuAssertIntEquals(
    tc, APX_NO_ERROR, mock_endpoint_inject_raw_frame(&sub, RMF_CMD_AREA_START_ADDRESS, false, cmd_buf, signed_size));
  mock_endpoint_deliver_all(&sub, &pub);
  CuAssertIntEquals(tc, APX_UNSUPPORTED_ERROR, pub.last_nack_error);
  CuAssertStrEquals(tc, "BadSigned", pub.last_nack_name);

  // 5. Subsequent valid publication on the same connection still succeeds
  in_cmd.name = "Recovered.apx";
  apx_size_t const ok_size = rmf_encode_publish_compressed_file_cmd(cmd_buf, sizeof(cmd_buf), &in_cmd);
  CuAssertTrue(tc, ok_size > 0u);
  CuAssertIntEquals(
    tc, APX_NO_ERROR, mock_endpoint_inject_raw_frame(&sub, RMF_CMD_AREA_START_ADDRESS, false, cmd_buf, ok_size));
  CuAssertPtrNotNull(tc, apx_file_manager_find_remote_file_by_name(&sub.file_manager, "Recovered.apx"));

  mock_endpoint_destroy(&sub);
  mock_endpoint_destroy(&pub);
}

static void test_file_manager_rejects_oversized_and_misaligned_control_writes(CuTest *tc)
{
  mock_endpoint_t ep;
  mock_endpoint_create(&ep, APX_CLIENT_MODE, 4096);

  // 1. Single 1100-byte NACK command targeting control area -> rejected with APX_MSG_TOO_LARGE_ERROR before dispatch
  uint8_t oversized[1100];
  memset(oversized, 'A', sizeof(oversized));
  packLE(oversized, RMF_CMD_NACK_MSG, UINT32_SIZE);
  packLE(oversized + RMF_CMD_TYPE_SIZE, (uint32_t)APX_PARSE_ERROR, UINT32_SIZE);
  oversized[sizeof(oversized) - 1u] = '\0';
  CuAssertIntEquals(tc, APX_MSG_TOO_LARGE_ERROR,
    mock_endpoint_inject_raw_frame(&ep, RMF_CMD_AREA_START_ADDRESS, false, oversized, (apx_size_t)sizeof(oversized)));
  CuAssertIntEquals(tc, APX_NO_ERROR, ep.last_nack_error);

  // 2. Fragmented control write exceeding 1024 bytes in aggregate -> rejected with APX_MSG_TOO_LARGE_ERROR and reset
  CuAssertIntEquals(
    tc, APX_NO_ERROR, mock_endpoint_inject_raw_frame(&ep, RMF_CMD_AREA_START_ADDRESS, true, oversized, 600u));
  CuAssertIntEquals(tc, APX_MSG_TOO_LARGE_ERROR,
    mock_endpoint_inject_raw_frame(&ep, RMF_CMD_AREA_START_ADDRESS + 600u, false, oversized + 600u, 500u));
  CuAssertIntEquals(tc, APX_NO_ERROR, ep.last_nack_error);
  CuAssertUIntEquals(tc, RMF_INVALID_ADDRESS, apx_file_manager_receiver_active_address(&ep.file_manager.receiver));

  // 3. Control-area write not starting at RMF_CMD_AREA_START_ADDRESS -> rejected with APX_INVALID_ADDRESS_ERROR before
  // buffering (for both more_bit=true and more_bit=false)
  CuAssertIntEquals(tc, APX_INVALID_ADDRESS_ERROR,
    mock_endpoint_inject_raw_frame(&ep, RMF_CMD_AREA_START_ADDRESS + 4u, true, oversized, 16u));
  CuAssertUIntEquals(tc, RMF_INVALID_ADDRESS, apx_file_manager_receiver_active_address(&ep.file_manager.receiver));
  CuAssertIntEquals(tc, APX_INVALID_ADDRESS_ERROR,
    mock_endpoint_inject_raw_frame(&ep, RMF_CMD_AREA_START_ADDRESS + 4u, false, oversized, 16u));

  // 4. Exact 1024-byte control write (RMF_CMD_AREA_SIZE) is accepted and dispatched
  uint8_t max_cmd[RMF_CMD_AREA_SIZE];
  memset(max_cmd, 'B', sizeof(max_cmd));
  packLE(max_cmd, RMF_CMD_NACK_MSG, UINT32_SIZE);
  packLE(max_cmd + RMF_CMD_TYPE_SIZE, (uint32_t)APX_PARSE_ERROR, UINT32_SIZE);
  memcpy(max_cmd + RMF_CMD_NACK_SIZE, "MaxCmd", 7u);
  CuAssertIntEquals(tc, APX_NO_ERROR,
    mock_endpoint_inject_raw_frame(&ep, RMF_CMD_AREA_START_ADDRESS, false, max_cmd, (apx_size_t)sizeof(max_cmd)));
  CuAssertIntEquals(tc, APX_PARSE_ERROR, ep.last_nack_error);
  CuAssertStrEquals(tc, "MaxCmd", ep.last_nack_name);

  mock_endpoint_destroy(&ep);
}

static void test_file_manager_rejects_trailing_bytes_after_publish_name(CuTest *tc)
{
  mock_endpoint_t pub;
  mock_endpoint_t sub;
  mock_endpoint_create(&pub, APX_CLIENT_MODE, 4096);
  mock_endpoint_create(&sub, APX_SERVER_MODE, 4096);

  uint8_t cmd_buf[RMF_CMD_AREA_SIZE];

  // 1. RMF_CMD_PUBLISH_FILE_MSG with stray bytes after null terminator -> APX_INVALID_MSG_ERROR, not registered
  rmf_file_info_t *plain_info = rmf_file_info_make_fixed("PlainTrailing.apx", 64u, 0x01000000u);
  CuAssertPtrNotNull(tc, plain_info);
  apx_size_t plain_size = rmf_encode_publish_file_cmd(cmd_buf, sizeof(cmd_buf), plain_info);
  CuAssertTrue(tc, plain_size > 0u);
  cmd_buf[plain_size] = 'Z';
  CuAssertIntEquals(tc, APX_INVALID_MSG_ERROR,
    mock_endpoint_inject_raw_frame(&sub, RMF_CMD_AREA_START_ADDRESS, false, cmd_buf, plain_size + 1u));
  CuAssertPtrEquals(tc, NULL, apx_file_manager_find_remote_file_by_name(&sub.file_manager, "PlainTrailing.apx"));

  // Legacy RMF_CMD_PUBLISH_FILE_MSG without trailing null terminator still succeeds
  CuAssertIntEquals(tc, APX_NO_ERROR,
    mock_endpoint_inject_raw_frame(&sub, RMF_CMD_AREA_START_ADDRESS, false, cmd_buf, plain_size - 1u));
  CuAssertPtrNotNull(tc, apx_file_manager_find_remote_file_by_name(&sub.file_manager, "PlainTrailing.apx"));
  rmf_file_info_delete(plain_info);

  // 2. RMF_CMD_PUBLISH_SIGNED_FILE_MSG with stray bytes after null terminator -> APX_INVALID_MSG_ERROR
  uint8_t dummy_sig[RMF_SIGNATURE_SIZE_ECDSA_P256];
  memset(dummy_sig, 0xAB, sizeof(dummy_sig));
  rmf_file_info_t *signed_info = rmf_file_info_make_fixed_with_signature(
    "SignedTrailing.apx", 64u, 0x02000000u, RMF_SIGNATURE_TYPE_ECDSA_P256, dummy_sig);
  CuAssertPtrNotNull(tc, signed_info);
  apx_size_t signed_size = rmf_encode_publish_signed_file_cmd(cmd_buf, sizeof(cmd_buf), signed_info);
  CuAssertTrue(tc, signed_size > 0u);
  cmd_buf[signed_size] = 'Z';
  CuAssertIntEquals(tc, APX_INVALID_MSG_ERROR,
    mock_endpoint_inject_raw_frame(&sub, RMF_CMD_AREA_START_ADDRESS, false, cmd_buf, signed_size + 1u));
  CuAssertPtrEquals(tc, NULL, apx_file_manager_find_remote_file_by_name(&sub.file_manager, "SignedTrailing.apx"));
  rmf_file_info_delete(signed_info);

  // 3. RMF_CMD_PUBLISH_COMPRESSED_FILE_MSG with stray bytes after null terminator -> NACK(APX_INVALID_MSG_ERROR)
  rmf_publish_compressed_file_cmd_t comp_cmd = {
    .start_address = 0x03000000u,
    .uncompressed_size = 128u,
    .compressed_size = 64u,
    .file_type = RMF_FILE_TYPE_FIXED,
    .compression_type = RMF_COMPRESSION_TYPE_ZLIB_DEFLATE,
    .digest_type = RMF_DIGEST_TYPE_NONE,
    .reserved = 0u,
    .name = "CompTrailing.apx",
  };
  apx_size_t comp_size = rmf_encode_publish_compressed_file_cmd(cmd_buf, sizeof(cmd_buf), &comp_cmd);
  CuAssertTrue(tc, comp_size > 0u);
  cmd_buf[comp_size] = 'Z';
  CuAssertIntEquals(
    tc, APX_NO_ERROR, mock_endpoint_inject_raw_frame(&sub, RMF_CMD_AREA_START_ADDRESS, false, cmd_buf, comp_size + 1u));
  mock_endpoint_deliver_all(&sub, &pub);
  CuAssertIntEquals(tc, APX_INVALID_MSG_ERROR, pub.last_nack_error);
  CuAssertPtrEquals(tc, NULL, apx_file_manager_find_remote_file_by_name(&sub.file_manager, "CompTrailing.apx"));

  mock_endpoint_destroy(&sub);
  mock_endpoint_destroy(&pub);
}

static void test_stream_abort_interrupts_partial_block_and_reopens(CuTest *tc)
{
  mock_endpoint_t pub;
  mock_endpoint_t sub;
  mock_endpoint_create(&pub, APX_SERVER_MODE, 4096);
  mock_endpoint_create(&sub, APX_CLIENT_MODE, 4096);

  rmf_file_info_t *info_a = rmf_file_info_make_stream("streamA.log", 256u, 0x20000000u);
  rmf_file_info_t *info_b = rmf_file_info_make_stream("streamB.log", 256u, 0x20010000u);
  CuAssertPtrNotNull(tc, apx_file_manager_create_local_file(&pub.file_manager, info_a));
  CuAssertPtrNotNull(tc, apx_file_manager_create_local_file(&pub.file_manager, info_b));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, info_a));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, info_b));
  rmf_file_info_delete(info_a);
  rmf_file_info_delete(info_b);
  mock_endpoint_deliver_all(&pub, &sub);

  apx_file_t *sub_a = apx_file_manager_find_remote_file_by_name(&sub.file_manager, "streamA.log");
  apx_file_t *sub_b = apx_file_manager_find_remote_file_by_name(&sub.file_manager, "streamB.log");
  CuAssertPtrNotNull(tc, sub_a);
  CuAssertPtrNotNull(tc, sub_b);
  attach_file_handlers(sub_a, &sub);
  attach_file_handlers(sub_b, &sub);

  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 256u, 2u));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20010000u, 256u, 2u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, APX_STREAM_STATE_OPEN, apx_file_get_stream_state(sub_a));
  CuAssertIntEquals(tc, APX_STREAM_STATE_OPEN, apx_file_get_stream_state(sub_b));

  // 1. Start a fragmented block on streamA (first fragment only, more_bit = true)
  uint8_t partial_frag[16];
  memset(partial_frag, 0xAA, sizeof(partial_frag));
  CuAssertIntEquals(tc, APX_NO_ERROR,
    mock_endpoint_inject_raw_frame(&sub, 0x20000000u, true, partial_frag, (apx_size_t)sizeof(partial_frag)));
  CuAssertUIntEquals(tc, 0x20000000u, apx_file_manager_receiver_active_address(&sub.file_manager.receiver));
  CuAssertIntEquals(tc, 0, adt_ary_length(&sub.rx_blocks));

  // 2. Unrelated control-area command (publishing a fixed file) arrives mid-reassembly -> succeeds without resetting
  // streamA's partial block
  rmf_file_info_t *info_fixed = rmf_file_info_make_fixed("status.out", 8u, 0x01000000u);
  CuAssertPtrNotNull(tc, apx_file_manager_create_local_file(&pub.file_manager, info_fixed));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_publish_local_file(&pub.file_manager, info_fixed));
  rmf_file_info_delete(info_fixed);
  (void)apx_file_manager_run(&pub.file_manager);
  CuAssertIntEquals(tc, 1, adt_ary_length(&pub.tx_packets));
  adt_bytearray_t *pub_pkt = (adt_bytearray_t *)adt_ary_value(&pub.tx_packets, 0);
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_file_manager_message_received(
      &sub.file_manager, adt_bytearray_const_data(pub_pkt), (apx_size_t)adt_bytearray_length(pub_pkt)));
  adt_ary_clear(&pub.tx_packets);
  CuAssertPtrNotNull(tc, apx_file_manager_find_remote_file_by_name(&sub.file_manager, "status.out"));
  CuAssertUIntEquals(tc, 0x20000000u, apx_file_manager_receiver_active_address(&sub.file_manager.receiver));

  // 3. Deliver RMF_CMD_STREAM_ABORT_MSG with status 3 (APX_PARSE_ERROR) -> interrupts partial block cleanly, preserves
  // connection
  adt_ary_clear(&sub.tx_packets);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_abort_stream(&pub.file_manager, 0x20000000u, APX_PARSE_ERROR));
  (void)apx_file_manager_run(&pub.file_manager);
  CuAssertIntEquals(tc, 1, adt_ary_length(&pub.tx_packets));
  adt_bytearray_t *abort_pkt = (adt_bytearray_t *)adt_ary_value(&pub.tx_packets, 0);
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_file_manager_message_received(
      &sub.file_manager, adt_bytearray_const_data(abort_pkt), (apx_size_t)adt_bytearray_length(abort_pkt)));
  adt_ary_clear(&pub.tx_packets);
  (void)apx_file_manager_run(&sub.file_manager);

  CuAssertUIntEquals(tc, RMF_INVALID_ADDRESS, apx_file_manager_receiver_active_address(&sub.file_manager.receiver));
  CuAssertIntEquals(tc, APX_STREAM_STATE_CLOSED, apx_file_get_stream_state(sub_a));
  CuAssertUIntEquals(tc, (uint32_t)APX_PARSE_ERROR, apx_file_get_stream_last_status(sub_a));
  CuAssertUIntEquals(tc, 1u, sub.close_notify_count);
  CuAssertIntEquals(tc, 0, adt_ary_length(&sub.rx_blocks));
  CuAssertIntEquals(tc, 0, adt_ary_length(&sub.tx_packets));

  // 4. Unrelated connection traffic (streamB) continues uninterrupted
  uint8_t const b_payload[6] = {0x10u, 0x20u, 0x30u, 0x40u, 0x50u, 0x60u};
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_file_manager_send_local_const_data(&pub.file_manager, 0x20010000u, b_payload, (apx_size_t)sizeof(b_payload)));
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, 1, adt_ary_length(&sub.rx_blocks));
  adt_bytearray_t *rx_b = (adt_bytearray_t *)adt_ary_value(&sub.rx_blocks, 0);
  CuAssertUIntEquals(tc, (uint32_t)sizeof(b_payload), adt_bytearray_length(rx_b));
  CuAssertIntEquals(tc, 0, memcmp(b_payload, adt_bytearray_const_data(rx_b), sizeof(b_payload)));

  // 5. Reopen streamA and deliver a complete fragmented block -> no stale bytes from aborted block
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_file_manager_open_stream(&sub.file_manager, 0x20000000u, 256u, 2u));
  mock_endpoint_deliver_all(&sub, &pub);
  mock_endpoint_deliver_all(&pub, &sub);
  CuAssertIntEquals(tc, APX_STREAM_STATE_OPEN, apx_file_get_stream_state(sub_a));

  uint8_t fresh_block[32];
  for (uint32_t i = 0u; i < (uint32_t)sizeof(fresh_block); ++i)
  {
    fresh_block[i] = (uint8_t)(i + 1u);
  }
  CuAssertIntEquals(tc, APX_NO_ERROR, mock_endpoint_inject_raw_frame(&sub, 0x20000000u, true, fresh_block, 16u));
  CuAssertIntEquals(tc, APX_NO_ERROR, mock_endpoint_inject_raw_frame(&sub, 0x20000010u, false, fresh_block + 16u, 16u));
  CuAssertIntEquals(tc, 2, adt_ary_length(&sub.rx_blocks));
  adt_bytearray_t *rx_a = (adt_bytearray_t *)adt_ary_value(&sub.rx_blocks, 1);
  CuAssertUIntEquals(tc, (uint32_t)sizeof(fresh_block), adt_bytearray_length(rx_a));
  CuAssertIntEquals(tc, 0, memcmp(fresh_block, adt_bytearray_const_data(rx_a), sizeof(fresh_block)));

  mock_endpoint_destroy(&sub);
  mock_endpoint_destroy(&pub);
}
