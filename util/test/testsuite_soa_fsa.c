/*****************************************************************************
* \file      testsuite_soa_fsa.c
* \author    Conny Gustafsson
* \date      2019-02-25
* \brief     Unit tests for soa_fsa_t
*
* Copyright (c) 2019 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "CuTest.h"
#include "soa.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define OVERFULL_BLOCK SOA_DEFAULT_NUM_BLOCKS+1

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_alloc_256_1_byte_blocks(CuTest* tc);
static void test_alloc_500_1_byte_blocks(CuTest* tc);
static void test_alloc_1000_1_byte_blocks(CuTest* tc);
static void test_fill_one_chunk(CuTest* tc);
static void test_create_two_chunks(CuTest* tc);
static void test_free_3_at_beginning_then_allocate_5_more(CuTest* tc);

//helper functions
static void do_1_byte_test(CuTest* tc, int32_t numElements);
static bool check_if_already_allocated(void **array, int32_t arrayLen, void *ptr);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_soa_fsa(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_alloc_256_1_byte_blocks);
   SUITE_ADD_TEST(suite, test_alloc_500_1_byte_blocks);
   SUITE_ADD_TEST(suite, test_alloc_1000_1_byte_blocks);
   SUITE_ADD_TEST(suite, test_fill_one_chunk);
   SUITE_ADD_TEST(suite, test_create_two_chunks);
   SUITE_ADD_TEST(suite, test_free_3_at_beginning_then_allocate_5_more);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_alloc_256_1_byte_blocks(CuTest* tc)
{
   const int32_t numElements = 256;
   do_1_byte_test(tc, numElements);

}

static void test_alloc_500_1_byte_blocks(CuTest* tc)
{
   const int32_t numElements = 500;
   do_1_byte_test(tc, numElements);
}

static void test_alloc_1000_1_byte_blocks(CuTest* tc)
{
   const int32_t numElements = 1000;
   do_1_byte_test(tc, numElements);

}

static void test_fill_one_chunk(CuTest* tc)
{
   soa_fsa_t fsa1;
   soa_fsa_init(&fsa1, sizeof(uint8_t), SOA_DEFAULT_NUM_BLOCKS);
   int32_t i;
   void *ptr;

   void *allocated[SOA_DEFAULT_NUM_BLOCKS];

   memset(allocated, 0, sizeof(allocated));
   for(i=0; i<SOA_DEFAULT_NUM_BLOCKS; i++)
   {
      ptr = soa_fsa_alloc(&fsa1);
      allocated[i]=ptr;
   }

   //After freeing one element in a full block, we expect the next alloc to return the previously freed pointer
   for(i=0; i<SOA_DEFAULT_NUM_BLOCKS; i++)
   {
      void *previously_freed;
      previously_freed = allocated[i];
      soa_fsa_free(&fsa1, allocated[i]);
      ptr = soa_fsa_alloc(&fsa1);
      CuAssertPtrEquals(tc, previously_freed, ptr);
   }
   soa_fsa_destroy(&fsa1);
}

static void test_create_two_chunks(CuTest* tc)
{
   soa_fsa_t fsa1;
   int32_t i;
   soa_chunk_t *chunk;
   void *ptr;

   soa_fsa_init(&fsa1, sizeof(uint8_t), SOA_DEFAULT_NUM_BLOCKS);

   void *allocated[OVERFULL_BLOCK];

   memset(allocated, 0, sizeof(allocated));
   for(i=0; i<OVERFULL_BLOCK; i++)
   {
      ptr = soa_fsa_alloc(&fsa1);
      CuAssertPtrNotNull(tc, ptr);
      allocated[i]=ptr;
   }
   CuAssertIntEquals(tc, 2, fsa1.chunks_len);
   chunk = &fsa1.chunks[0];
   for(i=0; i<SOA_DEFAULT_NUM_BLOCKS; i++)
   {
      CuAssertTrue(tc, check_if_already_allocated(&allocated[0], 255, &chunk->blockData[i]));
   }
   chunk = &fsa1.chunks[1];
   CuAssertTrue(tc, check_if_already_allocated(&allocated[255], 1, &chunk->blockData[0]));
   soa_fsa_destroy(&fsa1);
}

static void test_free_3_at_beginning_then_allocate_5_more(CuTest* tc)
{
   soa_fsa_t fsa1;
   int32_t i;
   void *ptr;
   void *allocated1[255];
   void *allocated2[4];
   soa_fsa_init(&fsa1, sizeof(uint8_t), SOA_DEFAULT_NUM_BLOCKS);
   for(i=0; i<254; i++)
   {
      ptr = soa_fsa_alloc(&fsa1);
      CuAssertPtrNotNull(tc, ptr);
      allocated1[i]=ptr;
   }
   CuAssertIntEquals(tc, 1, fsa1.chunks_len);
   CuAssertIntEquals(tc, 1, fsa1.chunks[0].freeBlocks);
   //free 3 then allocate 5 more
   for(i=0; i < 3; i++)
   {
      soa_fsa_free(&fsa1, allocated1[i]);
   }
   for(i=0; i< 5; i++)
   {
      ptr = soa_fsa_alloc(&fsa1);
      CuAssertPtrNotNull(tc, ptr);
      allocated2[i]=ptr;
   }
   CuAssertIntEquals(tc, 2, fsa1.chunks_len);
   CuAssertPtrEquals(tc, &fsa1.chunks[1].blockData[0], allocated2[4]);

   soa_fsa_destroy(&fsa1);
}

//Helper functions

static void do_1_byte_test(CuTest* tc, int32_t numElements)
{
   soa_fsa_t fsa1;
   int32_t numAllocated;
   int32_t i;
   int32_t numHalf = numElements/2;
   void** allocated = malloc(numElements*sizeof(void*));
   CuAssertPtrNotNull(tc, allocated);
   memset(&allocated[0], 0, numElements*sizeof(void*));
   soa_fsa_init(&fsa1, sizeof(uint8_t), SOA_DEFAULT_NUM_BLOCKS);
   for (numAllocated=0; numAllocated < numElements; numAllocated++)
   {
      char msg[100];
      void *ptr = soa_fsa_alloc(&fsa1);
      CuAssertPtrNotNull(tc, ptr);
      sprintf(msg, "(%p) Already exists, index=%d", ptr, (int) numAllocated);
      CuAssert(tc, msg, !check_if_already_allocated(&allocated[0], numAllocated, ptr));
      allocated[numAllocated]=ptr;
   }
   //clear first half of the elements and allocate them again
   for(i=0; i < numHalf; i++ )
   {
      soa_fsa_free(&fsa1, allocated[i]);
      allocated[i] = (void*) 0;
   }
   //check for duplicates (lower half)
   for(i=0; i < numHalf; i++)
   {
      char msg[100];
      void *ptr = soa_fsa_alloc(&fsa1);
      CuAssertPtrNotNull(tc, ptr);
      sprintf(msg, "(%p) Already exists, index=%d", ptr, (int) i);
      CuAssert(tc, msg, !check_if_already_allocated(&allocated[0], i, ptr));
      allocated[i]=ptr;
   }
   //clear other half
   for(i=0; i < numHalf; i++ )
   {
      soa_fsa_free(&fsa1, allocated[numHalf+i]);
      allocated[numHalf+i] = (void*) 0;
   }
   //check for duplicates (upper half)
   for(i=0; i < numHalf; i++)
   {
      char msg[100];
      void *ptr = soa_fsa_alloc(&fsa1);
      CuAssertPtrNotNull(tc, ptr);
      sprintf(msg, "(%p) Already exists, index=%d", ptr, (int) i);
      CuAssert(tc, msg, !check_if_already_allocated(&allocated[0], i, ptr));
      allocated[numHalf+i]=ptr;
   }
   free(allocated);
   soa_fsa_destroy(&fsa1);
}

static bool check_if_already_allocated(void **array, int32_t arrayLen, void *ptr)
{
   int32_t i;
   for(i=0; i < arrayLen; i++)
   {
      if (array[i]==ptr)
      {
         return true;
      }
   }
   return false;
}
