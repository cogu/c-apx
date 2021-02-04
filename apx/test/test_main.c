#if !defined(MEM_LEAK_CHECK) && defined(_WIN32)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <stdio.h>
#include <malloc.h>
#include "CuTest.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

/** RemoteFile **/
CuSuite* testSuite_remotefile(void);
CuSuite* testSuite_file_info(void);


/** APX Common **/
CuSuite* testSuite_apx_allocator(void);
CuSuite* testsuite_apx_attributesParser(void);
CuSuite* testSuite_apx_computation(void);
CuSuite* testSuite_apx_dataElement(void);
CuSuite* testSuite_apx_signatureParser(void);
CuSuite* testSuite_apx_parser(void);
CuSuite* testSuite_program(void);
CuSuite* testSuite_apx_compiler_pack(void);
CuSuite* testSuite_apx_compiler_unpack(void);
CuSuite* testSuite_apx_vm_serializer(void);
CuSuite* testSuite_apx_vm_deserializer(void);
CuSuite* testsuite_decoder(void);
CuSuite* testSuite_apx_vm(void);
CuSuite* testSuite_apx_node(void);
CuSuite* testSuite_apx_nodeData(void);
CuSuite* testSuite_apx_nodeManager_client_mode(void);
CuSuite* testSuite_apx_nodeManager_server_mode(void);
CuSuite* testSuite_apx_file(void);
CuSuite* testSuite_apx_fileMap(void);
CuSuite* testSuite_apx_fileManagerReceiver(void);
CuSuite* testSuite_apx_util(void);
CuSuite* testSuite_apx_portConnectorChangeEntry(void);
CuSuite* testSuite_apx_portConnectorChangeTable(void);
CuSuite* testSuite_apx_portSignatureMap(void);

//Client
CuSuite* testSuite_apx_clientTestConnection(void);
CuSuite* testSuite_apx_client_socketConnection(void);
CuSuite* testSuite_apx_client(void);

//Server
CuSuite* testSuite_apx_serverConnection(void);
CuSuite* testSuite_apx_server(void);

//Server extensions
CuSuite* testsuite_apx_socketServerExtension(void);
CuSuite* testSuite_apx_socketServerConnection(void);

void RunAllTests(void)
{
   CuString *output = CuStringNew();
   CuSuite* suite = CuSuiteNew();

// APX Common

   CuSuiteAddSuite(suite, testSuite_apx_allocator());
   CuSuiteAddSuite(suite, testsuite_apx_attributesParser());
   CuSuiteAddSuite(suite, testSuite_apx_dataElement());
   CuSuiteAddSuite(suite, testSuite_apx_signatureParser());
   CuSuiteAddSuite(suite, testSuite_apx_parser());
   CuSuiteAddSuite(suite, testSuite_program());
   CuSuiteAddSuite(suite, testSuite_apx_compiler_pack());
   CuSuiteAddSuite(suite, testSuite_apx_compiler_unpack());
   CuSuiteAddSuite(suite, testSuite_apx_vm_serializer());
   CuSuiteAddSuite(suite, testSuite_apx_vm_deserializer());
   CuSuiteAddSuite(suite, testsuite_decoder());
   CuSuiteAddSuite(suite, testSuite_apx_vm());
   CuSuiteAddSuite(suite, testSuite_apx_node());
   CuSuiteAddSuite(suite, testSuite_apx_nodeData());
   CuSuiteAddSuite(suite, testSuite_apx_computation());
   CuSuiteAddSuite(suite, testSuite_apx_nodeManager_client_mode());
   CuSuiteAddSuite(suite, testSuite_apx_nodeManager_server_mode());
   CuSuiteAddSuite(suite, testSuite_apx_file());
   CuSuiteAddSuite(suite, testSuite_apx_fileMap());
   CuSuiteAddSuite(suite, testSuite_apx_fileManagerReceiver());
   CuSuiteAddSuite(suite, testSuite_apx_portConnectorChangeEntry());
   CuSuiteAddSuite(suite, testSuite_apx_portConnectorChangeTable());
   CuSuiteAddSuite(suite, testSuite_apx_portSignatureMap());

   //Client
   CuSuiteAddSuite(suite, testSuite_apx_clientTestConnection());
   CuSuiteAddSuite(suite, testSuite_apx_client_socketConnection());
   CuSuiteAddSuite(suite, testSuite_apx_client());

   //Server
   CuSuiteAddSuite(suite, testSuite_apx_serverConnection());
   CuSuiteAddSuite(suite, testSuite_apx_server());

   //Server extensions
   CuSuiteAddSuite(suite, testsuite_apx_socketServerExtension());
   CuSuiteAddSuite(suite, testSuite_apx_socketServerConnection());

   // RemoteFile
   CuSuiteAddSuite(suite, testSuite_remotefile());
   CuSuiteAddSuite(suite, testSuite_file_info());

   //Util
   CuSuiteAddSuite(suite, testSuite_apx_util());

   CuSuiteRun(suite);
   CuSuiteSummary(suite, output);
   CuSuiteDetails(suite, output);
   printf("%s\n", output->buffer);
   CuSuiteDelete(suite);
   CuStringDelete(output);

}

int main(void)
{
#if !defined(MEM_LEAK_CHECK) && defined(_WIN32)
   _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
   RunAllTests();
   return 0;
}

void vfree(void *arg)
{
   free(arg);
}
