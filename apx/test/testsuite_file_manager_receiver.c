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

#define SMALL_DATA_SIZE 4
#define MEDIUM_DATA_SIZE 64
#define LARGE_DATA_SIZE 128

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_fileManagerReceiver_create(CuTest* tc);
static void test_apx_fileManagerReceiver_writeWithoutBufferReturnsError(CuTest* tc);
static void test_apx_fileManagerReceiver_smallWrite(CuTest* tc);
static void test_apx_fileManagerReceiver_resizeToLargerBuffer(CuTest* tc);
static void test_apx_fileManagerReceiver_nonFragmentedWrite(CuTest* tc);
static void test_apx_fileManagerReceiver_128fragmentedWrites(CuTest* tc);
static void test_apx_fileManagerReceiver_3fragmentedWrites(CuTest* tc);
static void test_apx_fileManagerReceiver_fragmentedWriteAtWrongAddress(CuTest* tc);



//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_fileManagerReceiver(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_fileManagerReceiver_create);
   SUITE_ADD_TEST(suite, test_apx_fileManagerReceiver_writeWithoutBufferReturnsError);
   SUITE_ADD_TEST(suite, test_apx_fileManagerReceiver_smallWrite);
   SUITE_ADD_TEST(suite, test_apx_fileManagerReceiver_resizeToLargerBuffer);
   SUITE_ADD_TEST(suite, test_apx_fileManagerReceiver_nonFragmentedWrite);
   SUITE_ADD_TEST(suite, test_apx_fileManagerReceiver_128fragmentedWrites);
   SUITE_ADD_TEST(suite, test_apx_fileManagerReceiver_3fragmentedWrites);
   SUITE_ADD_TEST(suite, test_apx_fileManagerReceiver_fragmentedWriteAtWrongAddress);

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
static void test_apx_fileManagerReceiver_create(CuTest* tc)
{
   apx_fileManagerReceiver_t recvr;
   apx_fileManagerReceiver_create(&recvr);
   CuAssertPtrEquals(tc, 0, recvr.receiveBuf);
   CuAssertUIntEquals(tc, 0, recvr.receiveBufPos);
   CuAssertUIntEquals(tc, 0, recvr.receiveBufSize);
   CuAssertUIntEquals(tc, RMF_INVALID_ADDRESS, recvr.startAddress);
   CuAssertTrue(tc, recvr.isFragmentedWrite == false);
   apx_fileManagerReceiver_destroy(&recvr);
}

static void test_apx_fileManagerReceiver_writeWithoutBufferReturnsError(CuTest* tc)
{
   apx_fileManagerReceiver_t recvr;
   uint8_t data[4] = {0};
   apx_fileManagerReceiver_create(&recvr);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_fileManagerReceiver_write(&recvr, 0u, &data[0], 4, false));
   apx_fileManagerReceiver_destroy(&recvr);
}


static void test_apx_fileManagerReceiver_smallWrite(CuTest* tc)
{
   apx_fileManagerReceiver_t recvr;
   uint8_t data[SMALL_DATA_SIZE] = {0x12, 0x034, 0x56, 0x78};
   apx_fileManagerReceiver_create(&recvr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_reserve(&recvr, SMALL_DATA_SIZE));
   CuAssertUIntEquals(tc, 0u, recvr.receiveBufPos);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, 0u, &data[0], SMALL_DATA_SIZE, false));
   CuAssertUIntEquals(tc, 4u, recvr.receiveBufPos);
   CuAssertIntEquals(tc, 0, memcmp(&data[0], &recvr.receiveBuf[0], SMALL_DATA_SIZE));
   apx_fileManagerReceiver_destroy(&recvr);
}


static void test_apx_fileManagerReceiver_resizeToLargerBuffer(CuTest* tc)
{
   apx_fileManagerReceiver_t recvr;
   uint8_t data[LARGE_DATA_SIZE] = {0x12, 0x034, 0x56, 0x78};
   int32_t i;
   //prepare
   apx_fileManagerReceiver_create(&recvr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_reserve(&recvr, SMALL_DATA_SIZE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, 0u, &data[0], SMALL_DATA_SIZE, false));
   CuAssertUIntEquals(tc, SMALL_DATA_SIZE, recvr.receiveBufPos);
   CuAssertUIntEquals(tc, SMALL_DATA_SIZE, recvr.receiveBufSize);

   for (i=0; i<LARGE_DATA_SIZE; i++)
   {
      data[i] = i;
   }
   //act
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_reserve(&recvr, LARGE_DATA_SIZE));
   CuAssertUIntEquals(tc, 0u, recvr.receiveBufPos);
   CuAssertUIntEquals(tc, LARGE_DATA_SIZE, recvr.receiveBufSize);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, 0u, &data[0], LARGE_DATA_SIZE, false));
   CuAssertUIntEquals(tc, LARGE_DATA_SIZE, recvr.receiveBufPos);

   //Attempt to resize back to smaller buffer shall keep the larger buffer
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_reserve(&recvr, SMALL_DATA_SIZE));
   CuAssertUIntEquals(tc, LARGE_DATA_SIZE, recvr.receiveBufSize);

   //clean
   apx_fileManagerReceiver_destroy(&recvr);

}

static void test_apx_fileManagerReceiver_nonFragmentedWrite(CuTest* tc)
{
   apx_fileManagerReceiver_t recvr;
   uint8_t data[MEDIUM_DATA_SIZE];
   int32_t i;
   uint32_t startAddress = 0x10000;
   uint32_t writeOffset = 0u;
   const uint32_t writeSize = MEDIUM_DATA_SIZE;
   apx_fileManagerReception_t reception;

   //prepare
   apx_fileManagerReceiver_create(&recvr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_reserve(&recvr, MEDIUM_DATA_SIZE));
   for (i=0; i<MEDIUM_DATA_SIZE; i++)
   {
      data[i] = i;
   }

   //act
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, startAddress + writeOffset, &data[writeOffset], writeSize, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_checkComplete(&recvr, &reception));

   //verify info
   CuAssertUIntEquals(tc, startAddress, reception.startAddress);
   CuAssertUIntEquals(tc, MEDIUM_DATA_SIZE, reception.msgSize);
   CuAssertConstPtrEquals(tc, recvr.receiveBuf, reception.msgBuf);

   //verify completed data
   CuAssertIntEquals(tc, 0, memcmp(reception.msgBuf, data, MEDIUM_DATA_SIZE));

   //verify next state
   CuAssertTrue(tc, !recvr.isFragmentedWrite);
   CuAssertUIntEquals(tc, RMF_INVALID_ADDRESS, recvr.startAddress);
   CuAssertUIntEquals(tc, 0u, recvr.receiveBufPos);

   //clean
   apx_fileManagerReceiver_destroy(&recvr);

}

static void test_apx_fileManagerReceiver_128fragmentedWrites(CuTest* tc)
{
   apx_fileManagerReceiver_t recvr;
   uint8_t data[LARGE_DATA_SIZE];
   int32_t i;
   uint32_t startAddress = 0x10000;
   uint32_t writeOffset = 0u;
   const uint32_t writeSize = 1u;
   apx_fileManagerReception_t reception;

   //prepare
   apx_fileManagerReceiver_create(&recvr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_reserve(&recvr, LARGE_DATA_SIZE));
   for (i=0; i<LARGE_DATA_SIZE; i++)
   {
      data[i] = i;
   }

   //act
   for (writeOffset=0; writeOffset<127; writeOffset++)
   {
      CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, startAddress + writeOffset, &data[writeOffset], writeSize, true));
      CuAssertUIntEquals(tc, startAddress, recvr.startAddress);
      CuAssertUIntEquals(tc, writeOffset + writeSize, recvr.receiveBufPos);
      CuAssertTrue(tc, apx_fileManagerReceiver_isOngoing(&recvr));
      CuAssertIntEquals(tc, APX_DATA_NOT_COMPLETE_ERROR, apx_fileManagerReceiver_checkComplete(&recvr, &reception));
   }
   writeOffset = 127;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, startAddress + writeOffset, &data[writeOffset], writeSize, false));
   CuAssertTrue(tc, !apx_fileManagerReceiver_isOngoing(&recvr));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_checkComplete(&recvr, &reception));

   //verify info
   CuAssertUIntEquals(tc, startAddress, reception.startAddress);
   CuAssertUIntEquals(tc, LARGE_DATA_SIZE, reception.msgSize);
   CuAssertConstPtrEquals(tc, recvr.receiveBuf, reception.msgBuf);

   //verify completed data
   CuAssertIntEquals(tc, 0, memcmp(reception.msgBuf, data, LARGE_DATA_SIZE));

   //verify next state
   CuAssertTrue(tc, !recvr.isFragmentedWrite);
   CuAssertUIntEquals(tc, RMF_INVALID_ADDRESS, recvr.startAddress);
   CuAssertUIntEquals(tc, 0u, recvr.receiveBufPos);


   //clean
   apx_fileManagerReceiver_destroy(&recvr);

}

static void test_apx_fileManagerReceiver_3fragmentedWrites(CuTest* tc)
{
   apx_fileManagerReceiver_t recvr;
   uint8_t data[MEDIUM_DATA_SIZE];
   int32_t i;
   uint32_t startAddress = 0x10000;
   uint32_t writeOffset = 0u;
   const uint32_t writeSize1 = 17;
   const uint32_t writeSize2 = 33;
   const uint32_t writeSize3 = 14;
   apx_fileManagerReception_t reception;

   //prepare
   apx_fileManagerReceiver_create(&recvr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_reserve(&recvr, MEDIUM_DATA_SIZE));
   for (i=0; i<MEDIUM_DATA_SIZE; i++)
   {
      data[i] = i;
   }

   //act
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, startAddress + writeOffset, &data[writeOffset], writeSize1, true));
   writeOffset+=writeSize1;
   CuAssertIntEquals(tc, APX_DATA_NOT_COMPLETE_ERROR, apx_fileManagerReceiver_checkComplete(&recvr, &reception));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, startAddress + writeOffset, &data[writeOffset], writeSize2, true));
   writeOffset+=writeSize2;
   CuAssertIntEquals(tc, APX_DATA_NOT_COMPLETE_ERROR, apx_fileManagerReceiver_checkComplete(&recvr, &reception));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, startAddress + writeOffset, &data[writeOffset], writeSize3, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_checkComplete(&recvr, &reception));

   //verify info
   CuAssertUIntEquals(tc, startAddress, reception.startAddress);
   CuAssertUIntEquals(tc, MEDIUM_DATA_SIZE, reception.msgSize);
   CuAssertConstPtrEquals(tc, recvr.receiveBuf, reception.msgBuf);

   //verify completed data
   CuAssertIntEquals(tc, 0, memcmp(reception.msgBuf, data, MEDIUM_DATA_SIZE));

   //verify next state
   CuAssertTrue(tc, !recvr.isFragmentedWrite);
   CuAssertUIntEquals(tc, RMF_INVALID_ADDRESS, recvr.startAddress);
   CuAssertUIntEquals(tc, 0u, recvr.receiveBufPos);


   //clean
   apx_fileManagerReceiver_destroy(&recvr);

}

static void test_apx_fileManagerReceiver_fragmentedWriteAtWrongAddress(CuTest* tc)
{
   apx_fileManagerReceiver_t recvr;
   uint8_t data[MEDIUM_DATA_SIZE];
   int32_t i;
   uint32_t startAddress = 0x10000;
   uint32_t writeOffset = 0u;
   const uint32_t writeSize1 = 32;
   const uint32_t writeSize2 = 32;
   apx_fileManagerReception_t reception;

   //prepare
   apx_fileManagerReceiver_create(&recvr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_reserve(&recvr, MEDIUM_DATA_SIZE));
   for (i=0; i<MEDIUM_DATA_SIZE; i++)
   {
      data[i] = i;
   }

   //act
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerReceiver_write(&recvr, startAddress + writeOffset, &data[writeOffset], writeSize1, true));
   writeOffset+=writeSize1;
   CuAssertIntEquals(tc, APX_DATA_NOT_COMPLETE_ERROR, apx_fileManagerReceiver_checkComplete(&recvr, &reception));

   //Note the +1 below, indicating a wrongly calculated address by client (or data corruption)
   CuAssertIntEquals(tc, APX_INVALID_ADDRESS_ERROR, apx_fileManagerReceiver_write(&recvr, startAddress + writeOffset + 1, &data[writeOffset], writeSize2, true));

   //clean
   apx_fileManagerReceiver_destroy(&recvr);

}
