//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/parser.h"
#include "apx/error.h"
#include "apx/type_attribute.h"
#include "apx/computation.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_parse_empty_node(CuTest* tc);
static void test_parse_uint8_type_declaration(CuTest* tc);
static void test_parse_uint8_type_declaration_with_attributes(CuTest* tc);
static void test_parse_record_type_declaration(CuTest* tc);
static void test_parse_record_type_declaration_containing_syntax_error(CuTest* tc);
static void test_parse_uint8_require_port_no_init(CuTest* tc);
static void test_parse_uint8_require_port_with_init(CuTest* tc);
static void test_parse_uint8_array_require_port_no_init(CuTest* tc);
static void test_parse_uint8_array_require_port_with_init(CuTest* tc);
static void test_parse_uint8_require_port_with_range_and_no_init(CuTest* tc);
static void test_parse_uint8_type_reference_require_port_no_init(CuTest* tc);
static void test_parse_uint8_type_reference_require_port_with_value_table(CuTest* tc);
static void test_parse_uint8_type_references_inside_record_reference_require_port(CuTest* tc);
static void test_parse_queued_uint8_provide_port(CuTest* tc);
static void test_parse_queued_uint8_provide_port(CuTest* tc);
static void test_parse_char_provide_port(CuTest* tc);
static void test_parse_char_array_provide_port(CuTest* tc);
static void test_parse_char_array_provide_port_with_empty_initializer(CuTest* tc);
static void test_parse_char8_provide_port(CuTest* tc);
static void test_parse_record_inside_record_type_reference_require_port(CuTest* tc);
static void test_parse_array_of_records(CuTest* tc);
static void test_parse_array_of_records_type_reference_require_port(CuTest* tc);
static void test_apx_parser_provide_port_with_invalid_attribute_string(CuTest* tc);
static void test_apx_parser_provide_port_with_invalid_data_signature(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

CuSuite* testSuite_apx_parser(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_parse_empty_node);
   SUITE_ADD_TEST(suite, test_parse_uint8_type_declaration);
   SUITE_ADD_TEST(suite, test_parse_uint8_type_declaration_with_attributes);
   SUITE_ADD_TEST(suite, test_parse_record_type_declaration);
   SUITE_ADD_TEST(suite, test_parse_record_type_declaration_containing_syntax_error);
   SUITE_ADD_TEST(suite, test_parse_uint8_require_port_no_init);
   SUITE_ADD_TEST(suite, test_parse_uint8_require_port_with_init);
   SUITE_ADD_TEST(suite, test_parse_uint8_array_require_port_no_init);
   SUITE_ADD_TEST(suite, test_parse_uint8_array_require_port_with_init);
   SUITE_ADD_TEST(suite, test_parse_uint8_require_port_with_range_and_no_init);
   SUITE_ADD_TEST(suite, test_parse_uint8_type_reference_require_port_no_init);
   SUITE_ADD_TEST(suite, test_parse_uint8_type_reference_require_port_with_value_table);
   SUITE_ADD_TEST(suite, test_parse_uint8_type_references_inside_record_reference_require_port);
   SUITE_ADD_TEST(suite, test_parse_queued_uint8_provide_port);
   SUITE_ADD_TEST(suite, test_parse_char_provide_port);
   SUITE_ADD_TEST(suite, test_parse_char_array_provide_port);
   SUITE_ADD_TEST(suite, test_parse_char_array_provide_port_with_empty_initializer);
   SUITE_ADD_TEST(suite, test_parse_char8_provide_port);
   SUITE_ADD_TEST(suite, test_parse_record_inside_record_type_reference_require_port);
   SUITE_ADD_TEST(suite, test_parse_array_of_records);
   SUITE_ADD_TEST(suite, test_parse_array_of_records_type_reference_require_port);
   SUITE_ADD_TEST(suite, test_apx_parser_provide_port_with_invalid_attribute_string);
   SUITE_ADD_TEST(suite, test_apx_parser_provide_port_with_invalid_data_signature);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_parse_empty_node(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"EmptyNode\"\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node;
   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   CuAssertStrEquals(tc, "EmptyNode", apx_node_get_name(node));
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_parse_uint8_type_declaration(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "T\"Percentage_T\"C";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_dataType_t* data_type = NULL;
   apx_dataElement_t* data_element = NULL;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   data_type = apx_node_get_last_data_type(node);
   CuAssertPtrNotNull(tc, data_type);
   CuAssertStrEquals(tc, "Percentage_T", apx_dataType_get_name(data_type));
   CuAssertFalse(tc, apx_dataType_has_attributes(data_type));
   data_element = apx_dataType_get_data_element(data_type);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));

   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_parse_uint8_type_declaration_with_attributes(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "T\"OnOff_T\"C(0,3):VT(\"OnOff_Off\", \"OnOff_On\", \"OnOff_Error\", \"OnOff_NotAvailable\")";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_dataType_t* data_type = NULL;
   apx_dataElement_t* data_element = NULL;
   apx_typeAttributes_t* attributes = NULL;
   apx_valueTable_t* value_table = NULL;
   uint32_t lower_limit = 0u;
   uint32_t upper_limit = 0u;
   adt_str_t* str;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   data_type = apx_node_get_last_data_type(node);
   CuAssertPtrNotNull(tc, data_type);
   CuAssertStrEquals(tc, "OnOff_T", apx_dataType_get_name(data_type));
   CuAssertTrue(tc, apx_dataType_has_attributes(data_type));
   data_element = apx_dataType_get_data_element(data_type);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(data_element));
   CuAssertTrue(tc, apx_dataElement_get_limits_uint32(data_element, &lower_limit, &upper_limit));
   CuAssertUIntEquals(tc, 0u, lower_limit);
   CuAssertUIntEquals(tc, 3u, upper_limit);
   attributes = apx_dataType_get_attributes(data_type);
   CuAssertPtrNotNull(tc, attributes);
   value_table = (apx_valueTable_t*)apx_typeAttributes_get_computation(attributes, 0);
   CuAssertPtrNotNull(tc, value_table);
   CuAssertIntEquals(tc, 4, apx_valueTable_length(value_table));
   str = apx_valueTable_get_value(value_table, 0);
   CuAssertStrEquals(tc, "OnOff_Off", adt_str_cstr(str));
   str = apx_valueTable_get_value(value_table, 1);
   CuAssertStrEquals(tc, "OnOff_On", adt_str_cstr(str));
   str = apx_valueTable_get_value(value_table, 2);
   CuAssertStrEquals(tc, "OnOff_Error", adt_str_cstr(str));
   str = apx_valueTable_get_value(value_table, 3);
   CuAssertStrEquals(tc, "OnOff_NotAvailable", adt_str_cstr(str));

   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_parse_record_type_declaration(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "T\"Notification_T\"{\"ID\"C(0,127)\"Stat\"C(0,3)\"Type\"C(0,7)}\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_dataType_t* data_type = NULL;
   apx_dataElement_t* record_data_element = NULL;
   apx_dataElement_t* child_data_element = NULL;
   uint32_t lower_limit = 0u;
   uint32_t upper_limit = 0u;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   data_type = apx_node_get_last_data_type(node);
   CuAssertPtrNotNull(tc, data_type);
   CuAssertStrEquals(tc, "Notification_T", apx_dataType_get_name(data_type));
   CuAssertFalse(tc, apx_dataType_has_attributes(data_type));
   record_data_element = apx_dataType_get_data_element(data_type);
   CuAssertPtrNotNull(tc, record_data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_RECORD, apx_dataElement_get_type_code(record_data_element));
   CuAssertUIntEquals(tc, 3, apx_dataElement_get_num_child_elements(record_data_element));

   child_data_element = apx_dataElement_get_child_at(record_data_element, 0);
   CuAssertPtrNotNull(tc, child_data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(child_data_element));
   CuAssertStrEquals(tc, "ID", apx_dataElement_get_name(child_data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(child_data_element));
   CuAssertTrue(tc, apx_dataElement_get_limits_uint32(child_data_element, &lower_limit, &upper_limit));
   CuAssertUIntEquals(tc, 0u, lower_limit);
   CuAssertUIntEquals(tc, 127u, upper_limit);

   child_data_element = apx_dataElement_get_child_at(record_data_element, 1);
   CuAssertPtrNotNull(tc, child_data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(child_data_element));
   CuAssertStrEquals(tc, "Stat", apx_dataElement_get_name(child_data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(child_data_element));
   CuAssertTrue(tc, apx_dataElement_get_limits_uint32(child_data_element, &lower_limit, &upper_limit));
   CuAssertUIntEquals(tc, 0u, lower_limit);
   CuAssertUIntEquals(tc, 3u, upper_limit);

   child_data_element = apx_dataElement_get_child_at(record_data_element, 2);
   CuAssertPtrNotNull(tc, child_data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(child_data_element));
   CuAssertStrEquals(tc, "Type", apx_dataElement_get_name(child_data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(child_data_element));
   CuAssertTrue(tc, apx_dataElement_get_limits_uint32(child_data_element, &lower_limit, &upper_limit));
   CuAssertUIntEquals(tc, 0u, lower_limit);
   CuAssertUIntEquals(tc, 7u, upper_limit);

   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);

}

static void test_parse_record_type_declaration_containing_syntax_error(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "T\"Notification_T\"{{\"ID\"C(0,127)\"Stat\"C(0,3)\"Type\"C(0,7)}\n";
   apx_parser_t parser;
   apx_istream_t stream;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_PARSE_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   CuAssertIntEquals(tc, 3, apx_parser_get_error_line(&parser));

   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);

}

static void test_parse_uint8_require_port_no_init(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"FuelLevel\"C";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_dataElement_t* data_element = NULL;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   CuAssertStrEquals(tc, "FuelLevel", apx_port_get_name(port));
   CuAssertFalse(tc, apx_port_has_attributes(port));
   data_element = apx_port_get_data_element(port);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));


   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);

}

static void test_parse_uint8_require_port_with_init(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"FuelLevel\"C:=255";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_dataElement_t* data_element = NULL;
   apx_portAttributes_t* attributes = NULL;
   dtl_sv_t* init_value = NULL;
   bool ok = false;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   CuAssertStrEquals(tc, "FuelLevel", apx_port_get_name(port));
   CuAssertTrue(tc, apx_port_has_attributes(port));
   data_element = apx_port_get_data_element(port);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));
   attributes = apx_port_get_attributes(port);
   CuAssertPtrNotNull(tc, attributes);
   init_value = (dtl_sv_t*)apx_portAttributes_get_init_value(attributes);
   CuAssertPtrNotNull(tc, init_value);
   CuAssertIntEquals(tc, 255, dtl_sv_to_i32(init_value, &ok));
   CuAssertTrue(tc, ok);

   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_parse_uint8_array_require_port_no_init(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"FuelLevel\"C[8]";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_dataElement_t* data_element = NULL;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   CuAssertStrEquals(tc, "FuelLevel", apx_port_get_name(port));
   CuAssertFalse(tc, apx_port_has_attributes(port));
   data_element = apx_port_get_data_element(port);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_is_dynamic_array(data_element));
   CuAssertUIntEquals(tc, 8, apx_dataElement_get_array_length(data_element));

   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);

}

static void test_parse_uint8_array_require_port_with_init(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"U8Port\"C[2]:={255, 255}";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_dataElement_t* data_element = NULL;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   CuAssertStrEquals(tc, "U8Port", apx_port_get_name(port));
   CuAssertTrue(tc, apx_port_has_attributes(port));
   data_element = apx_port_get_data_element(port);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_is_dynamic_array(data_element));
   CuAssertUIntEquals(tc, 2, apx_dataElement_get_array_length(data_element));

   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_parse_uint8_require_port_with_range_and_no_init(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"U8Value\"C(0,3):=3";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_dataElement_t* data_element = NULL;
   uint32_t lower_limit = 0u;
   uint32_t upper_limit = 0u;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   CuAssertStrEquals(tc, "U8Value", apx_port_get_name(port));
   CuAssertTrue(tc, apx_port_has_attributes(port));
   data_element = apx_port_get_data_element(port);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(data_element));
   CuAssertTrue(tc, apx_dataElement_has_limits(data_element));
   CuAssertTrue(tc, apx_dataElement_get_limits_uint32(data_element, &lower_limit, &upper_limit));
   CuAssertUIntEquals(tc, 0u, lower_limit);
   CuAssertUIntEquals(tc, 3u, upper_limit);

   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_parse_uint8_type_reference_require_port_no_init(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "T\"Percentage_T\"C\n"
      "R\"FuelLevel\"T[0]\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_dataElement_t* data_element = NULL;
   apx_dataElement_t* ref_data_element = NULL;
   apx_dataType_t* data_type = NULL;


   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   CuAssertStrEquals(tc, "FuelLevel", apx_port_get_name(port));
   CuAssertFalse(tc, apx_port_has_attributes(port));
   data_element = apx_port_get_data_element(port);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_REF_PTR, apx_dataElement_get_type_code(data_element));
   data_type = apx_dataElement_get_type_ref_ptr(data_element);
   CuAssertPtrNotNull(tc, data_type);
   ref_data_element = apx_dataType_get_data_element(data_type);
   CuAssertPtrNotNull(tc, ref_data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(ref_data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(ref_data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(ref_data_element));

   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_parse_uint8_type_reference_require_port_with_value_table(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "T\"Type_T\"C(0,3):VT(\"Off\",\"On\",\"Error\",\"NotAvailable\")\n"
      "R\"UInt8Port\"T[0]:=3\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_dataElement_t* data_element = NULL;
   apx_dataElement_t* ref_data_element = NULL;
   apx_dataType_t* data_type = NULL;
   uint32_t lower_limit = 0u;
   uint32_t upper_limit = 0u;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   CuAssertStrEquals(tc, "UInt8Port", apx_port_get_name(port));
   ref_data_element = apx_port_get_data_element(port);
   CuAssertPtrNotNull(tc, ref_data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_REF_PTR, apx_dataElement_get_type_code(ref_data_element));
   data_type = apx_dataElement_get_type_ref_ptr(ref_data_element);
   CuAssertPtrNotNull(tc, data_type);
   data_element = apx_dataType_get_data_element(data_type);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertTrue(tc, apx_dataElement_get_limits_uint32(data_element, &lower_limit, &upper_limit));
   CuAssertUIntEquals(tc, 0u, lower_limit);
   CuAssertUIntEquals(tc, 3u, upper_limit);

   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_parse_uint8_type_references_inside_record_reference_require_port(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "T\"FirstType_T\"C(0,3)\n"
      "T\"SecondType_T\"C(0,7)\n"
      "T\"RecordType_T\"{\"First\"T[0]\"Second\"T[1]}\n"
      "R\"RecordPort\"T[2]:={3,7}\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_dataElement_t* data_element = NULL;
   apx_dataElement_t* record_data_element = NULL;
   apx_dataElement_t* child_data_element = NULL;
   apx_dataElement_t* ref_child_data_element = NULL;
   apx_dataType_t* data_type = NULL;
   apx_dataType_t* child_data_type = NULL;
   uint32_t lower_limit = 0u;
   uint32_t upper_limit = 0u;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   CuAssertStrEquals(tc, "RecordPort", apx_port_get_name(port));
   data_element = apx_port_get_data_element(port);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_REF_PTR, apx_dataElement_get_type_code(data_element));
   data_type = apx_dataElement_get_type_ref_ptr(data_element);
   CuAssertPtrNotNull(tc, data_type);
   record_data_element = apx_dataType_get_data_element(data_type);
   CuAssertPtrNotNull(tc, record_data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_RECORD, apx_dataElement_get_type_code(record_data_element));
   CuAssertUIntEquals(tc, 2, apx_dataElement_get_num_child_elements(record_data_element));

   ref_child_data_element = apx_dataElement_get_child_at(record_data_element, 0);
   CuAssertPtrNotNull(tc, ref_child_data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_REF_PTR, apx_dataElement_get_type_code(ref_child_data_element));
   child_data_type = apx_dataElement_get_type_ref_ptr(ref_child_data_element);
   CuAssertStrEquals(tc, "FirstType_T", apx_dataType_get_name(child_data_type));
   child_data_element = apx_dataType_get_data_element(child_data_type);
   CuAssertPtrNotNull(tc, child_data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(child_data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(child_data_element));
   CuAssertTrue(tc, apx_dataElement_get_limits_uint32(child_data_element, &lower_limit, &upper_limit));
   CuAssertUIntEquals(tc, 0u, lower_limit);
   CuAssertUIntEquals(tc, 3u, upper_limit);

   ref_child_data_element = apx_dataElement_get_child_at(record_data_element, 1);
   CuAssertPtrNotNull(tc, ref_child_data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_REF_PTR, apx_dataElement_get_type_code(ref_child_data_element));
   child_data_type = apx_dataElement_get_type_ref_ptr(ref_child_data_element);
   CuAssertStrEquals(tc, "SecondType_T", apx_dataType_get_name(child_data_type));
   child_data_element = apx_dataType_get_data_element(child_data_type);
   CuAssertPtrNotNull(tc, child_data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(child_data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(child_data_element));
   CuAssertTrue(tc, apx_dataElement_get_limits_uint32(child_data_element, &lower_limit, &upper_limit));
   CuAssertUIntEquals(tc, 0u, lower_limit);
   CuAssertUIntEquals(tc, 7u, upper_limit);


   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_parse_queued_uint8_provide_port(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "P\"U8Signal\"C:Q[10]\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_dataElement_t* data_element = NULL;
   apx_portAttributes_t* attributes;


   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_provide_port(node);
   CuAssertPtrNotNull(tc, port);
   data_element = apx_port_get_data_element(port);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   attributes = apx_port_get_attributes(port);
   CuAssertPtrNotNull(tc, attributes);
   CuAssertTrue(tc, apx_portAttributes_is_queued(attributes));
   CuAssertUIntEquals(tc, 10u, apx_portAttributes_get_queue_length(attributes));

   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);

}

static void test_parse_char_provide_port(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "P\"CharSignal\"a\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_dataElement_t* data_element = NULL;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_provide_port(node);
   CuAssertPtrNotNull(tc, port);
   data_element = apx_port_get_data_element(port);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_CHAR, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));

   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);

}

static void test_parse_char_array_provide_port(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "P\"CharSignal\"a[10]\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_dataElement_t* data_element = NULL;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_provide_port(node);
   CuAssertPtrNotNull(tc, port);
   data_element = apx_port_get_data_element(port);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_CHAR, apx_dataElement_get_type_code(data_element));
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_is_dynamic_array(data_element));
   CuAssertUIntEquals(tc, 10, apx_dataElement_get_array_length(data_element));

   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_parse_char_array_provide_port_with_empty_initializer(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "P\"CharSignal\"a[10]:=\"\"\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_dataElement_t* data_element = NULL;
   apx_portAttributes_t* attributes = NULL;
   dtl_sv_t* init_value;
   adt_str_t* str;
   bool ok = false;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_provide_port(node);
   CuAssertPtrNotNull(tc, port);
   data_element = apx_port_get_data_element(port);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_CHAR, apx_dataElement_get_type_code(data_element));
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_is_dynamic_array(data_element));
   CuAssertUIntEquals(tc, 10, apx_dataElement_get_array_length(data_element));
   attributes = apx_port_get_attributes(port);
   CuAssertPtrNotNull(tc, attributes);
   init_value = (dtl_sv_t*)apx_portAttributes_get_init_value(attributes);
   CuAssertPtrNotNull(tc, init_value);
   CuAssertIntEquals(tc, DTL_SV_STR, dtl_sv_type(init_value));
   str = dtl_sv_to_str(init_value, &ok);
   CuAssertTrue(tc, ok);
   CuAssertPtrNotNull(tc, str);
   CuAssertIntEquals(tc, 0, adt_str_length(str));
   adt_str_delete(str);

   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_parse_char8_provide_port(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "P\"CharSignal\"A\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_dataElement_t* data_element = NULL;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_provide_port(node);
   CuAssertPtrNotNull(tc, port);
   data_element = apx_port_get_data_element(port);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_CHAR8, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));

   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);

}

static void test_parse_record_inside_record_type_reference_require_port(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "T\"FirstType_T\"{\"Inner1\"C(0,3)\"Inner2\"S}\n"
      "T\"SecondType_T\"{\"Inner3\"S\"Inner4\"L}\n"
      "T\"RecordOfRecordType_T\"{\"First\"T[0]\"Second\"T[1]}\n"
      "R\"RecordPort\"T[2]:={ {3, 0xFFFF}, {0xFFFF, 0xFFFFFFFF} }\n";

   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_dataElement_t* data_element = NULL;
   apx_dataElement_t* record_data_element = NULL;
   apx_dataElement_t* child_data_element = NULL;
   apx_dataElement_t* ref_child_data_element = NULL;
   apx_dataElement_t* grand_child_data_element = NULL;
   apx_dataType_t* data_type = NULL;
   apx_dataType_t* child_data_type = NULL;
   uint32_t lower_limit = 0u;
   uint32_t upper_limit = 0u;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   CuAssertStrEquals(tc, "RecordPort", apx_port_get_name(port));
   data_element = apx_port_get_data_element(port);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_REF_PTR, apx_dataElement_get_type_code(data_element));
   data_type = apx_dataElement_get_type_ref_ptr(data_element);
   CuAssertPtrNotNull(tc, data_type);
   record_data_element = apx_dataType_get_data_element(data_type);
   CuAssertPtrNotNull(tc, record_data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_RECORD, apx_dataElement_get_type_code(record_data_element));
   CuAssertUIntEquals(tc, 2, apx_dataElement_get_num_child_elements(record_data_element));

   ref_child_data_element = apx_dataElement_get_child_at(record_data_element, 0);
   CuAssertPtrNotNull(tc, ref_child_data_element);
   CuAssertStrEquals(tc, "First", apx_dataElement_get_name(ref_child_data_element));
   CuAssertUIntEquals(tc, APX_TYPE_CODE_REF_PTR, apx_dataElement_get_type_code(ref_child_data_element));
   child_data_type = apx_dataElement_get_type_ref_ptr(ref_child_data_element);
   CuAssertStrEquals(tc, "FirstType_T", apx_dataType_get_name(child_data_type));
   child_data_element = apx_dataType_get_data_element(child_data_type);
   CuAssertPtrNotNull(tc, child_data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_RECORD, apx_dataElement_get_type_code(child_data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(child_data_element));
   CuAssertUIntEquals(tc, 2, apx_dataElement_get_num_child_elements(child_data_element));
   grand_child_data_element = apx_dataElement_get_child_at(child_data_element, 0);
   CuAssertPtrNotNull(tc, grand_child_data_element);
   CuAssertStrEquals(tc, "Inner1", apx_dataElement_get_name(grand_child_data_element));
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(grand_child_data_element));
   CuAssertTrue(tc, apx_dataElement_get_limits_uint32(grand_child_data_element, &lower_limit, &upper_limit));
   CuAssertUIntEquals(tc, 0u, lower_limit);
   CuAssertUIntEquals(tc, 3u, upper_limit);
   grand_child_data_element = apx_dataElement_get_child_at(child_data_element, 1);
   CuAssertPtrNotNull(tc, grand_child_data_element);
   CuAssertStrEquals(tc, "Inner2", apx_dataElement_get_name(grand_child_data_element));
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT16, apx_dataElement_get_type_code(grand_child_data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(grand_child_data_element));

   ref_child_data_element = apx_dataElement_get_child_at(record_data_element, 1);
   CuAssertPtrNotNull(tc, ref_child_data_element);
   CuAssertStrEquals(tc, "Second", apx_dataElement_get_name(ref_child_data_element));
   CuAssertUIntEquals(tc, APX_TYPE_CODE_REF_PTR, apx_dataElement_get_type_code(ref_child_data_element));
   child_data_type = apx_dataElement_get_type_ref_ptr(ref_child_data_element);
   CuAssertStrEquals(tc, "SecondType_T", apx_dataType_get_name(child_data_type));
   child_data_element = apx_dataType_get_data_element(child_data_type);
   CuAssertPtrNotNull(tc, child_data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_RECORD, apx_dataElement_get_type_code(child_data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(child_data_element));
   CuAssertUIntEquals(tc, 2, apx_dataElement_get_num_child_elements(child_data_element));
   grand_child_data_element = apx_dataElement_get_child_at(child_data_element, 0);
   CuAssertPtrNotNull(tc, grand_child_data_element);
   CuAssertStrEquals(tc, "Inner3", apx_dataElement_get_name(grand_child_data_element));
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT16, apx_dataElement_get_type_code(grand_child_data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(grand_child_data_element));
   grand_child_data_element = apx_dataElement_get_child_at(child_data_element, 1);
   CuAssertPtrNotNull(tc, grand_child_data_element);
   CuAssertStrEquals(tc, "Inner4", apx_dataElement_get_name(grand_child_data_element));
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT32, apx_dataElement_get_type_code(grand_child_data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(grand_child_data_element));

   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_parse_array_of_records(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"RecordPort\"{\"Id\"S\"Value\"C}[2]:={ {0xFFFF,0}, {0xFFFF,0} }\n";

   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_dataElement_t* data_element = NULL;
   apx_dataElement_t* child_data_element = NULL;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   data_element = apx_port_get_data_element(port);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_RECORD, apx_dataElement_get_type_code(data_element));
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertUIntEquals(tc, 2, apx_dataElement_get_array_length(data_element));

   child_data_element = apx_dataElement_get_child_at(data_element, 0);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT16, apx_dataElement_get_type_code(child_data_element));
   CuAssertStrEquals(tc, "Id", apx_dataElement_get_name(child_data_element));

   child_data_element = apx_dataElement_get_child_at(data_element, 1);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(child_data_element));
   CuAssertStrEquals(tc, "Value", apx_dataElement_get_name(child_data_element));


   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_parse_array_of_records_type_reference_require_port(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "T\"RecordType_T\"{\"Id\"S\"Value\"C}\n"
      "R\"RecordPort\"T[0][2]:={ {0xFFFF, 0}, {0xFFFF, 0} }\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_dataType_t* data_type = NULL;
   apx_dataElement_t* data_element = NULL;
   apx_dataElement_t* record_data_element = NULL;
   apx_dataElement_t* child_data_element = NULL;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   data_element = apx_port_get_data_element(port);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_REF_PTR, apx_dataElement_get_type_code(data_element));
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertUIntEquals(tc, 2, apx_dataElement_get_array_length(data_element));
   data_type = apx_dataElement_get_type_ref_ptr(data_element);
   CuAssertPtrNotNull(tc, data_type);
   CuAssertStrEquals(tc, "RecordType_T", apx_dataType_get_name(data_type));
   record_data_element = apx_dataType_get_data_element(data_type);
   CuAssertPtrNotNull(tc, record_data_element);
   CuAssertUIntEquals(tc, 2, apx_dataElement_get_num_child_elements(record_data_element));

   child_data_element = apx_dataElement_get_child_at(record_data_element, 0);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT16, apx_dataElement_get_type_code(child_data_element));
   CuAssertStrEquals(tc, "Id", apx_dataElement_get_name(child_data_element));

   child_data_element = apx_dataElement_get_child_at(record_data_element, 1);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(child_data_element));
   CuAssertStrEquals(tc, "Value", apx_dataElement_get_name(child_data_element));

   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);

}

static void test_apx_parser_provide_port_with_invalid_attribute_string(CuTest* tc)
{
   apx_parser_t parser;
   apx_istream_t stream;
   const char* apx_text = "APX/1.2\n"
      "N\"test\"\n"
      "P\"VehicleSpeed\"S:abcd\n";

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_PARSE_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   CuAssertIntEquals(tc, 3, apx_parser_get_error_line(&parser));
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_parser_provide_port_with_invalid_data_signature(CuTest* tc)
{
   apx_parser_t parser;
   apx_istream_t stream;
   const char* apx_text = "APX/1.2\n"
      "N\"test\"\n"
      "P\"Signal1\"{\"User\"L:=0\n" //missing terminating '}' character
      "R\"Signal2\"S:=0";

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_PARSE_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   CuAssertIntEquals(tc, 3, apx_parser_get_error_line(&parser));
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}