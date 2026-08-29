//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(d) _mkdir(d)
#define RMDIR(d) _rmdir(d)
#else
#include <sys/stat.h>
#include <unistd.h>
#define MKDIR(d) mkdir(d, 0777)
#define RMDIR(d) rmdir(d)
#endif
#include "CuTest.h"
#include "server_cfg.h"
#include "dtl_type.h"
#include "apx/error.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_load_config_invalid_arguments(CuTest *tc);
static void test_load_config_file_returns_not_a_directory(CuTest *tc);
static void test_load_config_nonexistent_path_returns_not_a_directory(CuTest *tc);
static void test_load_config_from_dir_valid(CuTest *tc);
static void test_load_config_from_dir_fallback(CuTest *tc);
static void test_load_config_from_dir_empty_default(CuTest *tc);
static void test_load_config_from_dir_malformed_server_json(CuTest *tc);
static void test_load_config_from_dir_malformed_extension_json(CuTest *tc);
static void test_load_config_from_dir_invalid_extension_type(CuTest *tc);
static void test_load_config_from_dir_extensions_subfolder(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_server_cfg(void)
{
   CuSuite* suite = CuSuiteNew();
   SUITE_ADD_TEST(suite, test_load_config_invalid_arguments);
   SUITE_ADD_TEST(suite, test_load_config_file_returns_not_a_directory);
   SUITE_ADD_TEST(suite, test_load_config_nonexistent_path_returns_not_a_directory);
   SUITE_ADD_TEST(suite, test_load_config_from_dir_valid);
   SUITE_ADD_TEST(suite, test_load_config_from_dir_fallback);
   SUITE_ADD_TEST(suite, test_load_config_from_dir_empty_default);
   SUITE_ADD_TEST(suite, test_load_config_from_dir_malformed_server_json);
   SUITE_ADD_TEST(suite, test_load_config_from_dir_malformed_extension_json);
   SUITE_ADD_TEST(suite, test_load_config_from_dir_invalid_extension_type);
   SUITE_ADD_TEST(suite, test_load_config_from_dir_extensions_subfolder);
   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void write_test_file(const char *path, const char *content)
{
   FILE *fh = fopen(path, "w");
   if (fh != NULL)
   {
      fputs(content, fh);
      fclose(fh);
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_load_config_invalid_arguments(CuTest *tc)
{
   dtl_hv_t *server_cfg = NULL;
   dtl_hv_t *ext_cfg = NULL;

   CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_server_load_config(NULL, &server_cfg, &ext_cfg));
   CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_server_load_config("test.json", NULL, &ext_cfg));
   CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_server_load_config("test.json", &server_cfg, NULL));
}

static void test_load_config_file_returns_not_a_directory(CuTest *tc)
{
   const char *filepath = "test_file_not_dir.json";
   write_test_file(filepath, "{}");

   dtl_hv_t *server_cfg = NULL;
   dtl_hv_t *ext_cfg = NULL;
   apx_error_t result = apx_server_load_config(filepath, &server_cfg, &ext_cfg);
   CuAssertIntEquals(tc, APX_NOT_A_DIRECTORY_ERROR, result);
   CuAssertPtrEquals(tc, NULL, server_cfg);
   CuAssertPtrEquals(tc, NULL, ext_cfg);

   remove(filepath);
}

static void test_load_config_nonexistent_path_returns_not_a_directory(CuTest *tc)
{
   dtl_hv_t *server_cfg = NULL;
   dtl_hv_t *ext_cfg = NULL;

   apx_error_t result = apx_server_load_config("non_existent_config_dir_987654", &server_cfg, &ext_cfg);
   CuAssertIntEquals(tc, APX_NOT_A_DIRECTORY_ERROR, result);
   CuAssertPtrEquals(tc, NULL, server_cfg);
   CuAssertPtrEquals(tc, NULL, ext_cfg);
}

static void test_load_config_from_dir_valid(CuTest *tc)
{
   const char *dirname = "test_dir_valid";
   MKDIR(dirname);

   char file1[128];
   char file2[128];
   snprintf(file1, sizeof(file1), "%s/server.json", dirname);
   snprintf(file2, sizeof(file2), "%s/socket-server.json", dirname);

   write_test_file(file1, "{\"shutdown-timer\": 10}");
   write_test_file(file2, "{\"port\": 8080}");

   dtl_hv_t *server_cfg = NULL;
   dtl_hv_t *ext_cfg = NULL;
   apx_error_t result = apx_server_load_config(dirname, &server_cfg, &ext_cfg);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, server_cfg);
   CuAssertPtrNotNull(tc, ext_cfg);

   dtl_sv_t *sv = (dtl_sv_t*) dtl_hv_get_cstr(server_cfg, "shutdown-timer");
   CuAssertPtrNotNull(tc, sv);
   bool ok = false;
   CuAssertIntEquals(tc, 10, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dec_ref(server_cfg);
   dtl_dec_ref(ext_cfg);

   remove(file1);
   remove(file2);
   RMDIR(dirname);
}

static void test_load_config_from_dir_fallback(CuTest *tc)
{
   const char *dirname = "test_dir_fallback";
   MKDIR(dirname);

   char file1[128];
   snprintf(file1, sizeof(file1), "%s/apx_server.json", dirname);
   write_test_file(file1, "{\"shutdown-timer\": 25}");

   dtl_hv_t *server_cfg = NULL;
   dtl_hv_t *ext_cfg = NULL;
   apx_error_t result = apx_server_load_config(dirname, &server_cfg, &ext_cfg);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, server_cfg);
   CuAssertPtrNotNull(tc, ext_cfg);

   dtl_sv_t *sv = (dtl_sv_t*) dtl_hv_get_cstr(server_cfg, "shutdown-timer");
   CuAssertPtrNotNull(tc, sv);
   bool ok = false;
   CuAssertIntEquals(tc, 25, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dec_ref(server_cfg);
   dtl_dec_ref(ext_cfg);

   remove(file1);
   RMDIR(dirname);
}

static void test_load_config_from_dir_empty_default(CuTest *tc)
{
   const char *dirname = "test_dir_empty";
   MKDIR(dirname);

   dtl_hv_t *server_cfg = NULL;
   dtl_hv_t *ext_cfg = NULL;
   apx_error_t result = apx_server_load_config(dirname, &server_cfg, &ext_cfg);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, server_cfg);
   CuAssertPtrNotNull(tc, ext_cfg);
   CuAssertIntEquals(tc, 0, dtl_hv_length(server_cfg));

   dtl_dec_ref(server_cfg);
   dtl_dec_ref(ext_cfg);

   RMDIR(dirname);
}

static void test_load_config_from_dir_malformed_server_json(CuTest *tc)
{
   const char *dirname = "test_dir_bad_server";
   MKDIR(dirname);

   char file1[128];
   snprintf(file1, sizeof(file1), "%s/server.json", dirname);
   write_test_file(file1, "{ malformed json ");

   dtl_hv_t *server_cfg = NULL;
   dtl_hv_t *ext_cfg = NULL;
   apx_error_t result = apx_server_load_config(dirname, &server_cfg, &ext_cfg);
   CuAssertIntEquals(tc, APX_PARSE_ERROR, result);
   CuAssertPtrEquals(tc, NULL, server_cfg);
   CuAssertPtrEquals(tc, NULL, ext_cfg);

   remove(file1);
   RMDIR(dirname);
}

static void test_load_config_from_dir_malformed_extension_json(CuTest *tc)
{
   const char *dirname = "test_dir_bad_ext";
   MKDIR(dirname);

   char file1[128];
   char file2[128];
   snprintf(file1, sizeof(file1), "%s/server.json", dirname);
   snprintf(file2, sizeof(file2), "%s/socket-server.json", dirname);

   write_test_file(file1, "{}");
   write_test_file(file2, "{ broken json ");

   dtl_hv_t *server_cfg = NULL;
   dtl_hv_t *ext_cfg = NULL;
   apx_error_t result = apx_server_load_config(dirname, &server_cfg, &ext_cfg);
   CuAssertIntEquals(tc, APX_PARSE_ERROR, result);
   CuAssertPtrEquals(tc, NULL, server_cfg);
   CuAssertPtrEquals(tc, NULL, ext_cfg);

   remove(file1);
   remove(file2);
   RMDIR(dirname);
}

static void test_load_config_from_dir_invalid_extension_type(CuTest *tc)
{
   const char *dirname = "test_dir_type_ext";
   MKDIR(dirname);

   char file1[128];
   char file2[128];
   snprintf(file1, sizeof(file1), "%s/server.json", dirname);
   snprintf(file2, sizeof(file2), "%s/socket-server.json", dirname);

   write_test_file(file1, "{}");
   write_test_file(file2, "[1, 2, 3]");

   dtl_hv_t *server_cfg = NULL;
   dtl_hv_t *ext_cfg = NULL;
   apx_error_t result = apx_server_load_config(dirname, &server_cfg, &ext_cfg);
   CuAssertIntEquals(tc, APX_VALUE_TYPE_ERROR, result);
   CuAssertPtrEquals(tc, NULL, server_cfg);
   CuAssertPtrEquals(tc, NULL, ext_cfg);

   remove(file1);
   remove(file2);
   RMDIR(dirname);
}

static void test_load_config_from_dir_extensions_subfolder(CuTest *tc)
{
   const char *dirname = "test_dir_sub";
   char subdir[128];
   snprintf(subdir, sizeof(subdir), "%s/extensions", dirname);

   MKDIR(dirname);
   MKDIR(subdir);

   char file1[128];
   char file2[128];
   snprintf(file1, sizeof(file1), "%s/server.json", dirname);
   snprintf(file2, sizeof(file2), "%s/extensions/socket-server.json", dirname);

   write_test_file(file1, "{}");
   write_test_file(file2, "{\"port\": 9000}");

   dtl_hv_t *server_cfg = NULL;
   dtl_hv_t *ext_cfg = NULL;
   apx_error_t result = apx_server_load_config(dirname, &server_cfg, &ext_cfg);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, server_cfg);
   CuAssertPtrNotNull(tc, ext_cfg);

   dtl_dv_t *ext_node = dtl_hv_get_cstr(ext_cfg, "socket-server");
   CuAssertPtrNotNull(tc, ext_node);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type(ext_node));

   dtl_dec_ref(server_cfg);
   dtl_dec_ref(ext_cfg);

   remove(file1);
   remove(file2);
   RMDIR(subdir);
   RMDIR(dirname);
}
