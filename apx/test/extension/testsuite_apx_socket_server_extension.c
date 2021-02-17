//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "CuTest.h"
#include "apx/server.h"
#include "apx/extension/socket_server_extension.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_extension_init_shutdown(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_apx_socketServerExtension(void)
{
   CuSuite* suite = CuSuiteNew();
   SUITE_ADD_TEST(suite, test_extension_init_shutdown);
   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_extension_init_shutdown(CuTest* tc)
{
   apx_server_t apx_server;
   dtl_hv_t *extension_cfg = NULL;
   apx_server_create(&apx_server);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_socketServerExtension_register(&apx_server, (dtl_dv_t*) extension_cfg));
   apx_server_start(&apx_server);
   apx_server_run(&apx_server);
   apx_server_destroy(&apx_server);   
}

