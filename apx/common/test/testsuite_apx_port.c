#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_port.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

void test_apx_port_create(CuTest* tc)
{
   apx_port_t port;
   const char *psg;
   apx_port_create(&port,APX_REQUIRE_PORT,NULL,NULL,NULL);
   CuAssertPtrEquals(tc,NULL,port.portAttributes);
   CuAssertPtrEquals(tc,NULL,port.dataSignature);
   CuAssertPtrEquals(tc,NULL,(void*)port.derivedDsg.str);
   CuAssertPtrEquals(tc,NULL,port.name);
   CuAssertPtrEquals(tc,NULL,port.portSignature);
   apx_port_destroy(&port);

   apx_port_create(&port,APX_REQUIRE_PORT,"DPFSootLevel","T[95]",NULL);
   apx_port_setDerivedDataSignature(&port,"C");
   psg = apx_port_getPortSignature(&port);

   CuAssertStrEquals(tc,"\"DPFSootLevel\"C",psg);
   apx_port_destroy(&port);

}




CuSuite* testsuite_apx_port(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_port_create);

   return suite;
}
