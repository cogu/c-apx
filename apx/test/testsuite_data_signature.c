//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/data_signature.h"
#include "adt_ary.h"
#include "adt_hash.h"
#include "apx/data_type.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_dataSignature_create(CuTest* tc);
static void test_apx_dataSignature_uint8(CuTest* tc);
static void test_apx_dataSignature_uint16(CuTest* tc);
static void test_apx_dataSignature_uint16(CuTest* tc);
static void test_apx_dataSignature_uint32(CuTest* tc);
static void test_apx_dataSignature_record(CuTest* tc);
static void test_apx_dataSignature_string(CuTest* tc);
static void test_apx_dataSignature_typeReferenceById(CuTest *tc);
static void test_apx_dataSignature_typeReferenceByIdWithError(CuTest *tc);
static void test_apx_dataSignature_typeReferenceByName(CuTest *tc);
static void test_apx_dataSignature_typeReferenceByNameWithError(CuTest *tc);
static void test_apx_dataSignature_recordWithError(CuTest *tc);
static void test_apx_dataSignature_resolveIndexType(CuTest *tc);
static void test_apx_dataSignature_resolveIndexTypeWithError(CuTest *tc);
static void test_apx_dataSignature_resolveNameType(CuTest *tc);
static void test_apx_dataSignature_resolveNameTypeWithError(CuTest *tc);
static void test_apx_dataSignature_getDerivedString_uint8(CuTest *tc);
static void test_apx_dataSignature_getDerivedString_uint16(CuTest *tc);
static void test_apx_dataSignature_getDerivedString_uint32(CuTest *tc);
static void test_apx_dataSignature_getDerivedString_uint8Ref(CuTest *tc);
static void test_apx_dataSignature_u8DynamicArray(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_apx_dataSignature(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_dataSignature_create);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_uint8);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_uint16);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_uint32);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_string);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_record);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_typeReferenceById);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_typeReferenceByIdWithError);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_typeReferenceByName);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_typeReferenceByNameWithError);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_recordWithError);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_resolveIndexType);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_resolveIndexTypeWithError);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_resolveNameType);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_resolveNameTypeWithError);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_getDerivedString_uint8);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_getDerivedString_uint16);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_getDerivedString_uint32);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_getDerivedString_uint8Ref);
   SUITE_ADD_TEST(suite, test_apx_dataSignature_u8DynamicArray);

   return suite;
}
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_dataSignature_create(CuTest* tc)
{
   apx_dataSignature_t dsg;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_create(&dsg, NULL));
   CuAssertPtrEquals(tc, NULL, dsg.raw);
   CuAssertPtrEquals(tc, NULL, dsg.derived);
}

static void test_apx_dataSignature_uint8(CuTest* tc)
{
   apx_error_t err;
   apx_dataSignature_t *pSignature;
   apx_size_t packLen = 0;
   //base type
   pSignature = apx_dataSignature_new("C", &err);
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT8,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->arrayLen);
   CuAssertIntEquals(tc,0,pSignature->dataElement->lowerLimit.s32);
   CuAssertIntEquals(tc,0,pSignature->dataElement->upperLimit.s32);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(pSignature, &packLen));
   CuAssertUIntEquals(tc, UINT8_SIZE, packLen);
   apx_dataSignature_delete(pSignature);

   //limit
   pSignature = apx_dataSignature_new("C(0,7)", &err);
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT8,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->lowerLimit.u32);
   CuAssertUIntEquals(tc,7,pSignature->dataElement->upperLimit.u32);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(pSignature, &packLen));
   CuAssertUIntEquals(tc, UINT8_SIZE, packLen);
   apx_dataSignature_delete(pSignature);

   //array
   pSignature = apx_dataSignature_new("C[8]", &err);
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT8,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,8,pSignature->dataElement->arrayLen);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(pSignature, &packLen));
   CuAssertUIntEquals(tc, UINT8_SIZE*8u, packLen);
   apx_dataSignature_delete(pSignature);

   //array with limit
   pSignature = apx_dataSignature_new("C[8](0,7)", &err);
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT8,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,8,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->lowerLimit.u32);
   CuAssertUIntEquals(tc,7,pSignature->dataElement->upperLimit.u32);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(pSignature, &packLen));
   CuAssertUIntEquals(tc, UINT8_SIZE*8u, packLen);
   apx_dataSignature_delete(pSignature);



}

static void test_apx_dataSignature_uint16(CuTest* tc)
{
   apx_error_t err;
   apx_dataSignature_t *pSignature;
   apx_size_t packLen = 0;

   //base type
   pSignature = apx_dataSignature_new("S", &err);
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT16,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->lowerLimit.u32);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->upperLimit.u32);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(pSignature, &packLen));
   CuAssertUIntEquals(tc, UINT16_SIZE, packLen);
   apx_dataSignature_delete(pSignature);

   //limit
   pSignature = apx_dataSignature_new("S(0,512)", &err);
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT16,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->lowerLimit.u32);
   CuAssertUIntEquals(tc,512,pSignature->dataElement->upperLimit.u32);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(pSignature, &packLen));
   CuAssertUIntEquals(tc, UINT16_SIZE, packLen);
   apx_dataSignature_delete(pSignature);

   //array
   pSignature = apx_dataSignature_new("S[16]", &err);
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT16,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,16,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->lowerLimit.u32);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->upperLimit.u32);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(pSignature, &packLen));
   CuAssertUIntEquals(tc, UINT16_SIZE*16u, packLen);
   apx_dataSignature_delete(pSignature);

   //array+limit
   pSignature = apx_dataSignature_new("S[16](0,512)", &err);
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT16,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,16,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->lowerLimit.u32);
   CuAssertUIntEquals(tc,512,pSignature->dataElement->upperLimit.u32);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(pSignature, &packLen));
   CuAssertUIntEquals(tc, UINT16_SIZE*16u, packLen);
   apx_dataSignature_delete(pSignature);

}

static void test_apx_dataSignature_uint32(CuTest* tc)
{
   apx_error_t err;
   apx_dataSignature_t *pSignature;
   apx_size_t packLen = 0;

   //base type
   pSignature = apx_dataSignature_new("L", &err);
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT32,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->lowerLimit.u32);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->upperLimit.u32);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(pSignature, &packLen));
   CuAssertUIntEquals(tc, UINT32_SIZE, packLen);
   apx_dataSignature_delete(pSignature);

   //limit
   pSignature = apx_dataSignature_new("L(0,999999)", &err);
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT32,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->lowerLimit.u32);
   CuAssertUIntEquals(tc,999999,pSignature->dataElement->upperLimit.u32);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(pSignature, &packLen));
   CuAssertUIntEquals(tc, UINT32_SIZE, packLen);
   apx_dataSignature_delete(pSignature);

   //array
   pSignature = apx_dataSignature_new("L[2]", &err);
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT32,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,2,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->lowerLimit.u32);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->upperLimit.u32);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(pSignature, &packLen));
   CuAssertUIntEquals(tc, UINT32_SIZE*2u, packLen);
   apx_dataSignature_delete(pSignature);

   //array+limit
   pSignature = apx_dataSignature_new("L[2](0,999999)", &err);
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_UINT32,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,2,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->lowerLimit.u32);
   CuAssertUIntEquals(tc,999999,pSignature->dataElement->upperLimit.u32);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(pSignature, &packLen));
   CuAssertUIntEquals(tc, UINT32_SIZE*2u, packLen);
   apx_dataSignature_delete(pSignature);
}

static void test_apx_dataSignature_record(CuTest* tc)
{
   apx_error_t err;
   apx_dataSignature_t *pSignature;
   apx_size_t packLen = 0;

   //base type
   pSignature = apx_dataSignature_new("{\"NodeId\"C\"DTCId\"S\"FailT\"C\"RqstData\"C(0,3)}", &err);
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_RECORD,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertPtrNotNull(tc,pSignature->dataElement->childElements);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(pSignature, &packLen));
   CuAssertUIntEquals(tc, UINT8_SIZE+UINT8_SIZE+UINT16_SIZE+UINT8_SIZE, packLen);
   apx_dataSignature_delete(pSignature);



   pSignature = apx_dataSignature_new("{\"NodeId\"C\"HWPartNo\"a[9]\"HWRevNo\"a[4]\"HWSerialNo\"a[9]\"SWPartNo\"a[9]\"SWRevNo\"a[4]}", &err);
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_RECORD,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertPtrNotNull(tc,pSignature->dataElement->childElements);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(pSignature, &packLen));
   CuAssertUIntEquals(tc, 36u, packLen);
   apx_dataSignature_delete(pSignature);

}

static void test_apx_dataSignature_string(CuTest* tc)
{
   apx_error_t err;
   apx_dataSignature_t *pSignature;
   apx_size_t packLen = 0;

   pSignature = apx_dataSignature_new("a[10]", &err);
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc,APX_BASE_TYPE_STRING,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc,NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc,10,pSignature->dataElement->arrayLen);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->lowerLimit.u32);
   CuAssertUIntEquals(tc,0,pSignature->dataElement->upperLimit.u32);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(pSignature, &packLen));
   CuAssertUIntEquals(tc, UINT8_SIZE*10u, packLen);
   apx_dataSignature_delete(pSignature);

}

static void test_apx_dataSignature_typeReferenceById(CuTest *tc)
{
   apx_dataSignature_t dsg;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_create(&dsg, "T[0]"));
   apx_dataSignature_destroy(&dsg);
}

static void test_apx_dataSignature_typeReferenceByIdWithError(CuTest *tc)
{
   apx_dataSignature_t dsg;
   CuAssertIntEquals(tc, APX_UNMATCHED_BRACKET_ERROR, apx_dataSignature_create(&dsg, "T[0"));
   CuAssertIntEquals(tc, APX_INVALID_TYPE_REF_ERROR, apx_dataSignature_create(&dsg, "T[]"));
   CuAssertIntEquals(tc, APX_INVALID_TYPE_REF_ERROR, apx_dataSignature_create(&dsg, "T[garbage]"));
   CuAssertIntEquals(tc, APX_INVALID_TYPE_REF_ERROR, apx_dataSignature_create(&dsg, "T[0garbage]"));
}

static void test_apx_dataSignature_typeReferenceByName(CuTest *tc)
{
   apx_dataSignature_t dsg;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_create(&dsg, "T[\"type_T\"]"));
   apx_dataSignature_destroy(&dsg);
}

static void test_apx_dataSignature_typeReferenceByNameWithError(CuTest *tc)
{
   apx_dataSignature_t dsg;
   CuAssertIntEquals(tc, APX_UNMATCHED_BRACKET_ERROR, apx_dataSignature_create(&dsg, "T[\"type_T"));
   CuAssertIntEquals(tc, APX_UNMATCHED_STRING_ERROR, apx_dataSignature_create(&dsg, "T[\"type_T]"));
}

static void test_apx_dataSignature_recordWithError(CuTest *tc)
{
   apx_dataSignature_t dsg;
   CuAssertIntEquals(tc, APX_UNMATCHED_BRACE_ERROR, apx_dataSignature_create(&dsg, "{\"UserId\"S"));

}

static void test_apx_dataSignature_resolveIndexType(CuTest *tc)
{
   apx_error_t err;
   adt_ary_t *typeList = adt_ary_new(apx_datatype_vdelete);
   apx_dataSignature_t *dsg1;
   apx_dataSignature_t *dsg2;
   adt_ary_push(typeList, apx_datatype_new("VehicleSpeed_T", "S", NULL, 0, &err));
   adt_ary_push(typeList, apx_datatype_new("EngineSpeed_T", "S", NULL, 1, &err));
   dsg1 = apx_dataSignature_new("T[0]", &err);
   dsg2 = apx_dataSignature_new("T[1]", &err);

   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_ID, dsg1->dataElement->baseType);
   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_ID, dsg2->dataElement->baseType);
   CuAssertIntEquals(tc, 0, dsg1->dataElement->typeRef.id);
   CuAssertIntEquals(tc, 1, dsg2->dataElement->typeRef.id);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_resolveTypes(dsg1, typeList, NULL));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_resolveTypes(dsg2, typeList, NULL));

   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_PTR, dsg1->dataElement->baseType);
   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_PTR, dsg2->dataElement->baseType);
   CuAssertPtrEquals(tc, adt_ary_value(typeList, 0), dsg1->dataElement->typeRef.ptr);
   CuAssertPtrEquals(tc, adt_ary_value(typeList, 1), dsg2->dataElement->typeRef.ptr);



   apx_dataSignature_delete(dsg1);
   apx_dataSignature_delete(dsg2);
   adt_ary_delete(typeList);
}

static void test_apx_dataSignature_resolveIndexTypeWithError(CuTest *tc)
{
   apx_error_t err;
   adt_ary_t *typeList = adt_ary_new(apx_datatype_vdelete);
   apx_dataSignature_t *dsg;
   adt_ary_push(typeList, apx_datatype_new("VehicleSpeed_T", "S", NULL, 0, &err));
   adt_ary_push(typeList, apx_datatype_new("EngineSpeed_T", "S", NULL, 1, &err));
   dsg = apx_dataSignature_new("T[2]", &err);

   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_ID, dsg->dataElement->baseType);
   CuAssertIntEquals(tc, 2, dsg->dataElement->typeRef.id);
   CuAssertIntEquals(tc, APX_INVALID_TYPE_REF_ERROR, apx_dataSignature_resolveTypes(dsg, typeList, NULL));
   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_ID, dsg->dataElement->baseType);

   apx_dataSignature_delete(dsg);
   adt_ary_delete(typeList);
}

static void test_apx_dataSignature_resolveNameType(CuTest *tc)
{
   apx_error_t err;
   apx_dataSignature_t *dsg1;
   apx_dataSignature_t *dsg2;
   apx_datatype_t *t1;
   apx_datatype_t *t2;
   adt_ary_t *typeList;
   adt_hash_t *typeMap;
   typeList = adt_ary_new(apx_datatype_vdelete);
   typeMap = adt_hash_new(NULL);
   t1 = apx_datatype_new("VehicleSpeed_T", "S", NULL, 0, &err);
   t2 = apx_datatype_new("EngineSpeed_T", "S", NULL, 1, &err);
   adt_ary_push(typeList, t1);
   adt_ary_push(typeList, t2);
   adt_hash_set(typeMap, t1->name, t1);
   adt_hash_set(typeMap, t2->name, t2);
   dsg1 = apx_dataSignature_new("T[\"VehicleSpeed_T\"]", &err);
   dsg2 = apx_dataSignature_new("T[\"EngineSpeed_T\"]", &err);

   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_NAME, dsg1->dataElement->baseType);
   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_NAME, dsg2->dataElement->baseType);
   CuAssertStrEquals(tc, "VehicleSpeed_T", dsg1->dataElement->typeRef.name);
   CuAssertStrEquals(tc, "EngineSpeed_T", dsg2->dataElement->typeRef.name);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_resolveTypes(dsg1, typeList, typeMap));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_resolveTypes(dsg2, typeList, typeMap));

   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_PTR, dsg1->dataElement->baseType);
   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_PTR, dsg2->dataElement->baseType);
   CuAssertPtrEquals(tc, t1, dsg1->dataElement->typeRef.ptr);
   CuAssertPtrEquals(tc, t2, dsg2->dataElement->typeRef.ptr);

   apx_dataSignature_delete(dsg1);
   apx_dataSignature_delete(dsg2);
   adt_ary_delete(typeList);
   adt_hash_delete(typeMap);
}

static void test_apx_dataSignature_resolveNameTypeWithError(CuTest *tc)
{
   apx_error_t err;
   apx_dataSignature_t *dsg;
   apx_datatype_t *t1;
   apx_datatype_t *t2;
   adt_ary_t *typeList;
   adt_hash_t *typeMap;
   typeList = adt_ary_new(apx_datatype_vdelete);
   typeMap = adt_hash_new(NULL);
   t1 = apx_datatype_new("VehicleSpeed_T", "S", NULL, 0, &err);
   t2 = apx_datatype_new("EngineSpeed_T", "S", NULL, 1, &err);
   adt_ary_push(typeList, t1);
   adt_ary_push(typeList, t2);
   adt_hash_set(typeMap, t1->name, t1);
   adt_hash_set(typeMap, t2->name, t2);
   dsg = apx_dataSignature_new("T[\"VehicleSpeed_t\"]", &err);

   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_NAME, dsg->dataElement->baseType);
   CuAssertStrEquals(tc, "VehicleSpeed_t", dsg->dataElement->typeRef.name);

   CuAssertIntEquals(tc, APX_INVALID_TYPE_REF_ERROR, apx_dataSignature_resolveTypes(dsg, typeList, typeMap));

   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_NAME, dsg->dataElement->baseType);

   apx_dataSignature_delete(dsg);
   adt_ary_delete(typeList);
   adt_hash_delete(typeMap);
}

static void test_apx_dataSignature_getDerivedString_uint8(CuTest *tc)
{
   apx_error_t err;
   apx_dataSignature_t *dsg;

   //simple uint8
   dsg = apx_dataSignature_new("C", &err);
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   CuAssertStrEquals(tc, "C", apx_dataSignature_getDerivedString(dsg));
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   apx_dataSignature_delete(dsg);

   //uint8 with limits
   dsg = apx_dataSignature_new("C(0,3)", &err);
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   CuAssertStrEquals(tc, "C(0,3)", apx_dataSignature_getDerivedString(dsg));
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   apx_dataSignature_delete(dsg);

   //uint8 array
   dsg = apx_dataSignature_new("C[8]", &err);
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   CuAssertStrEquals(tc, "C[8]", apx_dataSignature_getDerivedString(dsg));
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   apx_dataSignature_delete(dsg);

}

static void test_apx_dataSignature_getDerivedString_uint16(CuTest *tc)
{
   apx_error_t err;
   apx_dataSignature_t *dsg;

   //simple uint16
   dsg = apx_dataSignature_new("S", &err);
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   CuAssertStrEquals(tc, "S", apx_dataSignature_getDerivedString(dsg));
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   apx_dataSignature_delete(dsg);

   //uint16 with limits
   dsg = apx_dataSignature_new("S(0,10000)", &err);
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   CuAssertStrEquals(tc, "S(0,10000)", apx_dataSignature_getDerivedString(dsg));
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   apx_dataSignature_delete(dsg);

   //uint16 array
   dsg = apx_dataSignature_new("S[6]", &err);
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   CuAssertStrEquals(tc, "S[6]", apx_dataSignature_getDerivedString(dsg));
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   apx_dataSignature_delete(dsg);
}

static void test_apx_dataSignature_getDerivedString_uint32(CuTest *tc)
{
   apx_error_t err;
   apx_dataSignature_t *dsg;

   //simple uint32
   dsg = apx_dataSignature_new("L", &err);
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   CuAssertStrEquals(tc, "L", apx_dataSignature_getDerivedString(dsg));
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   apx_dataSignature_delete(dsg);

   //uint32 with limits
   dsg = apx_dataSignature_new("L(0,100000)", &err);
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   CuAssertStrEquals(tc, "L(0,100000)", apx_dataSignature_getDerivedString(dsg));
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   apx_dataSignature_delete(dsg);

   //uint32 array
   dsg = apx_dataSignature_new("L[4]", &err);
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   CuAssertStrEquals(tc, "L[4]", apx_dataSignature_getDerivedString(dsg));
   CuAssertPtrEquals(tc, NULL, dsg->derived);
   apx_dataSignature_delete(dsg);
}

static void test_apx_dataSignature_getDerivedString_uint8Ref(CuTest *tc)
{
   apx_error_t err;
   apx_dataSignature_t *dsg;
   adt_ary_t typeList;

   dsg = apx_dataSignature_new("T[0]", &err);
   adt_ary_create(&typeList, apx_datatype_vdelete);
   adt_ary_push(&typeList, apx_datatype_new("EnumType","C(0,3)",0, 0, &err));
   apx_dataSignature_resolveTypes(dsg, &typeList, 0);
   CuAssertStrEquals(tc, "C(0,3)", apx_dataSignature_getDerivedString(dsg));

   adt_ary_destroy(&typeList);
   apx_dataSignature_delete(dsg);
}

static void test_apx_dataSignature_u8DynamicArray(CuTest* tc)
{
   apx_error_t err;
   apx_dataSignature_t *pSignature;
   apx_size_t packLen=0;
   const uint32_t arrayLen = 4096u;

   //base type
   pSignature = apx_dataSignature_new("C[*]", &err);
   CuAssertPtrNotNull(tc, pSignature);
   CuAssertIntEquals(tc, APX_BASE_TYPE_UINT8,pSignature->dataElement->baseType);
   CuAssertPtrEquals(tc, NULL,pSignature->dataElement->name);
   CuAssertUIntEquals(tc, 0, apx_dataElement_getArrayLen(pSignature->dataElement));
   CuAssertIntEquals(tc, 0, pSignature->dataElement->lowerLimit.s32);
   CuAssertIntEquals(tc, 0, pSignature->dataElement->upperLimit.s32);
   apx_dataElement_setArrayLen(pSignature->dataElement, arrayLen);
   CuAssertTrue(tc, apx_dataElement_isDynamicArray(pSignature->dataElement));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(pSignature, &packLen));
   CuAssertUIntEquals(tc, UINT16_SIZE+UINT8_SIZE*arrayLen, packLen);
   apx_dataSignature_delete(pSignature);
}
