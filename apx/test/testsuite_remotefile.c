//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/remotefile.h"
#include "pack.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_low_address_encode(CuTest* tc);
static void test_low_address_decode(CuTest* tc);
static void test_high_address_encode(CuTest* tc);
static void test_high_address_decode(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_remotefile(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_low_address_encode);
   SUITE_ADD_TEST(suite, test_low_address_decode);
   SUITE_ADD_TEST(suite, test_high_address_encode);
   SUITE_ADD_TEST(suite, test_high_address_decode);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_low_address_encode(CuTest* tc)
{
   uint8_t buffer[UINT16_SIZE];
   CuAssertUIntEquals(tc, UINT16_SIZE, rmf_address_encode(buffer, sizeof(buffer), 0u, false));
   CuAssertUIntEquals(tc, 0x00, buffer[0]);
   CuAssertUIntEquals(tc, 0x00u, buffer[1]);
   CuAssertUIntEquals(tc, UINT16_SIZE, rmf_address_encode(buffer, sizeof(buffer), 0u, true));
   CuAssertUIntEquals(tc, 0x40u, buffer[0]);
   CuAssertUIntEquals(tc, 0x00u, buffer[1]);
   CuAssertUIntEquals(tc, UINT16_SIZE, rmf_address_encode(buffer, sizeof(buffer), RMF_LOW_ADDR_MAX, false));
   CuAssertUIntEquals(tc, 0x3Fu, buffer[0]);
   CuAssertUIntEquals(tc, 0xFFu, buffer[1]);
   CuAssertUIntEquals(tc, UINT16_SIZE, rmf_address_encode(buffer, sizeof(buffer), RMF_LOW_ADDR_MAX, true));
   CuAssertUIntEquals(tc, 0x7Fu, buffer[0]);
   CuAssertUIntEquals(tc, 0xFFu, buffer[1]);
}

static void test_low_address_decode(CuTest* tc)
{
   uint8_t buffer[UINT16_SIZE] = { 0, 0 };
   uint32_t address = 0u;
   bool more_bit = false;
   CuAssertUIntEquals(tc, UINT16_SIZE, rmf_address_decode(buffer, buffer + sizeof(buffer), &address, &more_bit));
   CuAssertUIntEquals(tc, 0u, address);
   CuAssertFalse(tc, more_bit);
   buffer[0] = 0x40;
   CuAssertUIntEquals(tc, UINT16_SIZE, rmf_address_decode(buffer, buffer + sizeof(buffer), &address, &more_bit));
   CuAssertUIntEquals(tc, 0u, address);
   CuAssertTrue(tc, more_bit);
   buffer[0] = 0x3F;
   buffer[1] = 0xFF;
   CuAssertUIntEquals(tc, UINT16_SIZE, rmf_address_decode(buffer, buffer + sizeof(buffer), &address, &more_bit));
   CuAssertUIntEquals(tc, RMF_LOW_ADDR_MAX, address);
   CuAssertFalse(tc, more_bit);
   buffer[0] = 0x7F;
   CuAssertUIntEquals(tc, UINT16_SIZE, rmf_address_decode(buffer, buffer + sizeof(buffer), &address, &more_bit));
   CuAssertUIntEquals(tc, RMF_LOW_ADDR_MAX, address);
   CuAssertTrue(tc, more_bit);
}

static void test_high_address_encode(CuTest* tc)
{
   uint8_t buffer[UINT32_SIZE];
   CuAssertUIntEquals(tc, UINT32_SIZE, rmf_address_encode(buffer, sizeof(buffer), RMF_HIGH_ADDR_MIN, false));
   CuAssertUIntEquals(tc, 0x80u, buffer[0]);
   CuAssertUIntEquals(tc, 0x00u, buffer[1]);
   CuAssertUIntEquals(tc, 0x40u, buffer[2]);
   CuAssertUIntEquals(tc, 0x00u, buffer[3]);
   CuAssertUIntEquals(tc, UINT32_SIZE, rmf_address_encode(buffer, sizeof(buffer), RMF_HIGH_ADDR_MIN, true));
   CuAssertUIntEquals(tc, 0xC0u, buffer[0]);
   CuAssertUIntEquals(tc, 0x00u, buffer[1]);
   CuAssertUIntEquals(tc, 0x40u, buffer[2]);
   CuAssertUIntEquals(tc, 0x00u, buffer[3]);
   CuAssertUIntEquals(tc, UINT32_SIZE, rmf_address_encode(buffer, sizeof(buffer), RMF_HIGH_ADDR_MAX, false));
   CuAssertUIntEquals(tc, 0xBFu, buffer[0]);
   CuAssertUIntEquals(tc, 0xFFu, buffer[1]);
   CuAssertUIntEquals(tc, 0xFFu, buffer[2]);
   CuAssertUIntEquals(tc, 0xFFu, buffer[3]);
   CuAssertUIntEquals(tc, UINT32_SIZE, rmf_address_encode(buffer, sizeof(buffer), RMF_HIGH_ADDR_MAX, true));
   CuAssertUIntEquals(tc, 0xFFu, buffer[0]);
   CuAssertUIntEquals(tc, 0xFFu, buffer[1]);
   CuAssertUIntEquals(tc, 0xFFu, buffer[2]);
   CuAssertUIntEquals(tc, 0xFFu, buffer[3]);
}

static void test_high_address_decode(CuTest* tc)
{
   uint8_t buffer[UINT32_SIZE] = { 0x80u, 0x0u, 0x40u , 0x0u };
   uint32_t address = 0u;
   bool more_bit = false;
   CuAssertUIntEquals(tc, UINT32_SIZE, rmf_address_decode(buffer, buffer + sizeof(buffer), &address, &more_bit));
   CuAssertUIntEquals(tc, RMF_HIGH_ADDR_MIN, address);
   CuAssertFalse(tc, more_bit);
   buffer[0] = 0xC0;
   CuAssertUIntEquals(tc, UINT32_SIZE, rmf_address_decode(buffer, buffer + sizeof(buffer), &address, &more_bit));
   CuAssertUIntEquals(tc, RMF_HIGH_ADDR_MIN, address);
   CuAssertTrue(tc, more_bit);
   buffer[0] = 0xBF;
   buffer[1] = 0xFF;
   buffer[2] = 0xFF;
   buffer[3] = 0xFF;
   CuAssertUIntEquals(tc, UINT32_SIZE, rmf_address_decode(buffer, buffer + sizeof(buffer), &address, &more_bit));
   CuAssertUIntEquals(tc, RMF_HIGH_ADDR_MAX, address);
   CuAssertFalse(tc, more_bit);
   buffer[0] = 0xFF;
   CuAssertUIntEquals(tc, UINT32_SIZE, rmf_address_decode(buffer, buffer + sizeof(buffer), &address, &more_bit));
   CuAssertUIntEquals(tc, RMF_HIGH_ADDR_MAX, address);
   CuAssertTrue(tc, more_bit);
}
