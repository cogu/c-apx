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

  apx_error_t rc = apx_file_manager_create(&self->file_manager, mode, &iface, NULL);
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
  (void)name;
  self->last_nack_error = error_code;
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

  // 4. Publisher revoke transitions stream to APX_STREAM_STATE_REVOKED
  uint8_t revoke_cmd[RMF_CMD_TYPE_SIZE + RMF_FILE_OPEN_CMD_SIZE];
  packLE(revoke_cmd, RMF_CMD_REVOKE_FILE_MSG, UINT32_SIZE);
  packLE(revoke_cmd + UINT32_SIZE, 0x20000000u, UINT32_SIZE);
  CuAssertIntEquals(tc, APX_NO_ERROR,
    mock_endpoint_inject_raw_frame(&sub, RMF_CMD_AREA_START_ADDRESS, false, revoke_cmd, sizeof(revoke_cmd)));
  CuAssertIntEquals(tc, APX_STREAM_STATE_REVOKED, apx_file_get_stream_state(sub_file));

  mock_endpoint_destroy(&sub);
  mock_endpoint_destroy(&pub);
}
