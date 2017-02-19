#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_dataSignature.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

void test_apx_dataSignature_new(CuTest* tc)
{
   apx_dataSignature_t *pSignature;
   pSignature = apx_dataSignature_new(NULL);
   CuAssertPtrNotNull(tc, pSignature);
   apx_dataSignature_delete(pSignature);
   pSignature = apx_dataSignature_new("C");
   CuAssertPtrNotNull(tc, pSignature);
   apx_dataSignature_delete(pSignature);
}

void test_apx_dataSignature_uint8(CuTest* tc)
{
   apx_dataSignature_t *pSignature;

   //base type
   pSignature = apx_dataSignature_new("C");
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT8,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->arrayLen);
   CuAssertIntEquals(tc,0,pSignature->dataElement->min.s32);
   CuAssertIntEquals(tc,0,pSignature->dataElement->max.s32);
   CuAssertUIntEquals(tc,sizeof(uint8_t),pSignature->dataElement->packLen);
   apx_dataSignature_delete(pSignature);

   //limit
   pSignature = apx_dataSignature_new("C(0,7)");
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT8,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->min.u32);
   CuAssertUIntEquals(tc,7,pSignature->dataElement->max.u32);
   CuAssertUIntEquals(tc,sizeof(uint8_t),pSignature->dataElement->packLen);
   apx_dataSignature_delete(pSignature);

   //array
   pSignature = apx_dataSignature_new("C[8]");
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT8,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,8,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,sizeof(uint8_t)*8,pSignature->dataElement->packLen);
   apx_dataSignature_delete(pSignature);

   //array with limit
   pSignature = apx_dataSignature_new("C[8](0,7)");
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT8,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,8,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->min.u32);
   CuAssertUIntEquals(tc,7,pSignature->dataElement->max.u32);
   CuAssertUIntEquals(tc,sizeof(uint8_t)*8,pSignature->dataElement->packLen);
   apx_dataSignature_delete(pSignature);

}

void test_apx_dataSignature_uint16(CuTest* tc)
{
   apx_dataSignature_t *pSignature;

   //base type
   pSignature = apx_dataSignature_new("S");
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT16,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->min.u32);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->max.u32);
   CuAssertUIntEquals(tc,sizeof(uint16_t),pSignature->dataElement->packLen);
   apx_dataSignature_delete(pSignature);

   //limit
   pSignature = apx_dataSignature_new("S(0,512)");
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT16,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->min.u32);
   CuAssertUIntEquals(tc,512,pSignature->dataElement->max.u32);
   CuAssertUIntEquals(tc,sizeof(uint16_t),pSignature->dataElement->packLen);
   apx_dataSignature_delete(pSignature);

   //array
   pSignature = apx_dataSignature_new("S[16]");
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT16,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,16,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->min.u32);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->max.u32);
   CuAssertUIntEquals(tc,sizeof(uint16_t)*16,pSignature->dataElement->packLen);
   apx_dataSignature_delete(pSignature);

   //array+limit
   pSignature = apx_dataSignature_new("S[16](0,512)");
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT16,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,16,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->min.u32);
   CuAssertUIntEquals(tc,512,pSignature->dataElement->max.u32);
   CuAssertUIntEquals(tc,sizeof(uint16_t)*16,pSignature->dataElement->packLen);
   apx_dataSignature_delete(pSignature);

}

void test_apx_dataSignature_uint32(CuTest* tc)
{
   apx_dataSignature_t *pSignature;

   //base type
   pSignature = apx_dataSignature_new("L");
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT32,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->min.u32);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->max.u32);
   CuAssertUIntEquals(tc,sizeof(uint32_t),pSignature->dataElement->packLen);
   apx_dataSignature_delete(pSignature);

   //limit
   pSignature = apx_dataSignature_new("L(0,999999)");
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT32,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->min.u32);
   CuAssertUIntEquals(tc,999999,pSignature->dataElement->max.u32);
   CuAssertUIntEquals(tc,sizeof(uint32_t),pSignature->dataElement->packLen);
   apx_dataSignature_delete(pSignature);

   //array
   pSignature = apx_dataSignature_new("L[2]");
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT32,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,2,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->min.u32);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->max.u32);
   CuAssertUIntEquals(tc,sizeof(uint32_t)*2,pSignature->dataElement->packLen);
   apx_dataSignature_delete(pSignature);

   //array+limit
   pSignature = apx_dataSignature_new("L[2](0,999999)");
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT32,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,2,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->min.u32);
   CuAssertUIntEquals(tc,999999,pSignature->dataElement->max.u32);
   CuAssertUIntEquals(tc,sizeof(uint32_t)*2,pSignature->dataElement->packLen);
   apx_dataSignature_delete(pSignature);
}

void test_apx_dataSignature_record(CuTest* tc)
{
   apx_dataSignature_t *pSignature;

   //base type
   pSignature = apx_dataSignature_new("{\"NodeId\"C\"DTCId\"S\"FailT\"C\"RqstData\"C(0,3)}");
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_RECORD,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertPtrNotNull(tc,pSignature->dataElement->childElements);
   CuAssertUIntEquals(tc,5,pSignature->dataElement->packLen);
   apx_dataSignature_delete(pSignature);



   pSignature = apx_dataSignature_new("{\"NodeId\"C\"HWPartNo\"a[9]\"HWRevNo\"a[4]\"HWSerialNo\"a[9]\"SWPartNo\"a[9]\"SWRevNo\"a[4]}");
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_RECORD,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertPtrNotNull(tc,pSignature->dataElement->childElements);
   CuAssertUIntEquals(tc,36,pSignature->dataElement->packLen);
   apx_dataSignature_delete(pSignature);

}

void test_apx_dataSignature_string(CuTest* tc)
{
   apx_dataSignature_t *pSignature;
   pSignature = apx_dataSignature_new("a[10]");
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_STRING,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,10,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->min.u32);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->max.u32);
   CuAssertUIntEquals(tc,sizeof(uint8_t)*10,pSignature->dataElement->packLen);
   apx_dataSignature_delete(pSignature);

}


CuSuite* testsuite_apx_dataSignature(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_dataSignature_new);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_uint8);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_uint16);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_uint32);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_string);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_record);


   return suite;
}
