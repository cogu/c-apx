//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/file_manager_receiver.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define LARGE_BUFFER_SIZE 8192

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_command_area_size_on_creation(CuTest* tc);
static void test_resize_to_large_buffer(CuTest* tc);
static void test_small_size_write(CuTest* tc);
static void test_medium_size_write(CuTest* tc);
static void test_one_byte_fragmented_write(CuTest* tc);
static void test_three_piece_message_followed_by_two_piece_message(CuTest* tc);
static void test_fragmented_write_at_wrong_address(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_fileManagerReceiver(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_command_area_size_on_creation);
   SUITE_ADD_TEST(suite, test_resize_to_large_buffer);
   SUITE_ADD_TEST(suite, test_small_size_write);
   SUITE_ADD_TEST(suite, test_medium_size_write);
   SUITE_ADD_TEST(suite, test_one_byte_fragmented_write);
   SUITE_ADD_TEST(suite, test_three_piece_message_followed_by_two_piece_message);
   SUITE_ADD_TEST(suite, test_fragmented_write_at_wrong_address);

   return suite;
}
//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_command_area_size_on_creation(CuTest* tc)
{
   apx_fileManagerReceiver_t recvr;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_create(&recvr));
   CuAssertUIntEquals(tc, RMF_CMD_AREA_SIZE, apx_fileManagerReceiver_buffer_size(&recvr));
   apx_fileManagerReceiver_destroy(&recvr);
}

static void test_resize_to_large_buffer(CuTest* tc)
{
   apx_fileManagerReceiver_t recvr;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_create(&recvr));
   CuAssertUIntEquals(tc, RMF_CMD_AREA_SIZE, apx_fileManagerReceiver_buffer_size(&recvr));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_reserve(&recvr, LARGE_BUFFER_SIZE));
   CuAssertUIntEquals(tc, LARGE_BUFFER_SIZE, apx_fileManagerReceiver_buffer_size(&recvr));
   apx_fileManagerReceiver_destroy(&recvr);
}

static void test_small_size_write(CuTest* tc)
{
   uint8_t msg[4] = { 0x12, 0x034, 0x56, 0x78 };
   apx_fileManagerReceiver_t recvr;
   apx_fileManagerReceptionResult_t result;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_create(&recvr));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, &result, 0u, msg, (apx_size_t)sizeof(msg), false));
   CuAssertTrue(tc, result.is_complete);
   CuAssertUIntEquals(tc, 0u, result.address);
   CuAssertPtrNotNull(tc, result.data);
   CuAssertUIntEquals(tc, (apx_size_t)sizeof(msg), result.size);
   CuAssertIntEquals(tc, 0, memcmp(msg, result.data, result.size));
   apx_fileManagerReceiver_destroy(&recvr);
}

static void test_medium_size_write(CuTest* tc)
{
   uint8_t msg[64];
   size_t i;
   apx_fileManagerReceiver_t recvr;
   apx_fileManagerReceptionResult_t result;
   for (i = 0; i < sizeof(msg); i++)
   {
      msg[i] = (uint8_t)i;
   }
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_create(&recvr));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, &result, 0u, msg, (apx_size_t)sizeof(msg), false));
   CuAssertTrue(tc, result.is_complete);
   CuAssertUIntEquals(tc, 0u, result.address);
   CuAssertPtrNotNull(tc, result.data);
   CuAssertUIntEquals(tc, (apx_size_t)sizeof(msg), result.size);
   CuAssertIntEquals(tc, 0, memcmp(msg, result.data, result.size));
   apx_fileManagerReceiver_destroy(&recvr);
}

static void test_one_byte_fragmented_write(CuTest* tc)
{
   uint8_t msg[128];
   uint32_t i;
   uint32_t const write_address = 0x10000;
   uint32_t write_offset = 0u;
   apx_fileManagerReceiver_t recvr;
   apx_fileManagerReceptionResult_t result;
   for (i = 0; i < sizeof(msg); i++)
   {
      msg[i] = (uint8_t)i;
   }
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_create(&recvr));
   for (i = 0; i < 127; i++)
   {
      CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, &result, write_address + write_offset, msg + write_offset, 1, true));
      CuAssertFalse(tc, result.is_complete);
      write_offset++;
   }
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, &result, write_address + write_offset, msg + write_offset, 1, false));
   CuAssertTrue(tc, result.is_complete);
   CuAssertUIntEquals(tc, write_address, result.address);
   CuAssertPtrNotNull(tc, result.data);
   CuAssertUIntEquals(tc, (apx_size_t)sizeof(msg), result.size);
   CuAssertIntEquals(tc, 0, memcmp(msg, result.data, result.size));
   apx_fileManagerReceiver_destroy(&recvr);
}

static void test_three_piece_message_followed_by_two_piece_message(CuTest* tc)
{
   uint32_t i;
   uint32_t write_address = 0x10000;
   uint32_t write_offset = 0u;
   apx_size_t const write_size1 = 17;
   apx_size_t const write_size2 = 33;
   apx_size_t const write_size3 = 14;
   apx_fileManagerReceiver_t recvr;
   apx_fileManagerReceptionResult_t result;
   uint8_t msg[17+33+14];

   for (i = 0; i < sizeof(msg); i++)
   {
      msg[i] = (uint8_t)i;
   }

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_create(&recvr));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, &result, write_address + write_offset, msg + write_offset, write_size1, true));
   CuAssertFalse(tc, result.is_complete);
   write_offset += write_size1;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, &result, write_address + write_offset, msg + write_offset, write_size2, true));
   CuAssertFalse(tc, result.is_complete);
   write_offset += write_size2;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, &result, write_address + write_offset, msg + write_offset, write_size3, false));
   CuAssertTrue(tc, result.is_complete);
   CuAssertUIntEquals(tc, write_address, result.address);
   CuAssertPtrNotNull(tc, result.data);
   CuAssertUIntEquals(tc, (apx_size_t)sizeof(msg), result.size);
   CuAssertIntEquals(tc, 0, memcmp(msg, result.data, result.size));

   apx_size_t const write_size4 = 17;
   apx_size_t const write_size5 = 33;
   uint8_t msg2[17 + 33];
   for (i = 0; i < sizeof(msg2); i++)
   {
      msg2[i] = (uint8_t)i;
   }
   write_address = 0x20000;
   write_offset = 0u;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, &result, write_address + write_offset, msg + write_offset, write_size4, true));
   CuAssertFalse(tc, result.is_complete);
   write_offset += write_size4;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, &result, write_address + write_offset, msg + write_offset, write_size5, false));
   CuAssertTrue(tc, result.is_complete);
   CuAssertUIntEquals(tc, write_address, result.address);
   CuAssertPtrNotNull(tc, result.data);
   CuAssertUIntEquals(tc, (apx_size_t)sizeof(msg2), result.size);
   CuAssertIntEquals(tc, 0, memcmp(msg2, result.data, result.size));

   apx_fileManagerReceiver_destroy(&recvr);
}

static void test_fragmented_write_at_wrong_address(CuTest* tc)
{
   uint32_t i;
   uint32_t write_address = 0x10000;
   uint32_t write_offset = 0u;
   apx_size_t const write_size1 = 15;
   apx_size_t const write_size2 = 15;
   apx_fileManagerReceiver_t recvr;
   apx_fileManagerReceptionResult_t result;
   uint8_t msg[15 + 15];

   for (i = 0; i < sizeof(msg); i++)
   {
      msg[i] = (uint8_t)i;
   }

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_create(&recvr));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, &result, write_address + write_offset, msg + write_offset, write_size1, true));
   CuAssertFalse(tc, result.is_complete);
   write_offset += write_size1;

   //Write to same address again
   CuAssertIntEquals(tc, APX_INVALID_ADDRESS_ERROR, apx_fileManagerReceiver_write(&recvr, &result, write_address, msg + write_offset, write_size2, false));
   CuAssertFalse(tc, result.is_complete);

   apx_fileManagerReceiver_destroy(&recvr);
}