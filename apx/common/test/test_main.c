#include <stdio.h>
#include "CuTest.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

/** RemoteFile **/
CuSuite* testSuite_remotefile(void);


/** APX Common **/
CuSuite* testSuite_apx_allocator(void);
CuSuite* testsuite_apx_attributesParser(void);
CuSuite* testSuite_apx_bytePortMap(void);
CuSuite* testSuite_apx_compiler(void);
CuSuite* testSuite_apx_dataElement(void);
CuSuite* testsuite_apx_dataSignature(void);
CuSuite* testsuite_apx_datatype(void);
CuSuite* testSuite_apx_eventLoop(void);
CuSuite* testSuite_apx_file2(void);
CuSuite* testSuite_apx_fileManagerShared(void);
CuSuite* testSuite_apx_fileManagerWorker(void);
CuSuite* testSuite_apx_fileManagerReceiver(void);
CuSuite* testSuite_apx_fileManager(void);
CuSuite* testSuite_apx_fileMap(void);
CuSuite* testSuite_apx_node(void);
CuSuite* testSuite_apx_nodeData2(void);
CuSuite* testSuite_apx_nodeManager(void);
CuSuite* testSuite_apx_nodeInfo(void);
CuSuite* testSuite_apx_nodeInstance(void);
CuSuite* testSuite_apx_parser(void);
CuSuite* testsuite_apx_port(void);
CuSuite* testSuite_apx_portConnectorChangeEntry(void);
CuSuite* testSuite_apx_portConnectorChangeTable(void);
CuSuite* testSuite_apx_portSignatureMap(void);
CuSuite* testSuite_apx_vm(void);
CuSuite* testSuite_apx_vmSerializer(void);
CuSuite* testSuite_apx_vmDeserializer(void);
CuSuite* testSuite_apx_connectionBase(void);

/** APX Server **/
CuSuite* testSuite_apx_serverConnection(void);
CuSuite* testSuite_apx_dataRouting(void);


/** APX Server Extensions **/
CuSuite* testSuite_apx_serverSocketConnection(void);
CuSuite* testsuite_apx_socketServerExtension(void);
CuSuite* testsuite_apx_serverTextLogExtension(void);

/** APX Client **/
CuSuite* testSuite_apx_client_socketConnection(void);
CuSuite* testSuite_apx_client_testConnection(void);
CuSuite* testsuite_apx_dynamic_client(void);
CuSuite* testSuite_apx_client(void);

void RunAllTests(void)
{
   CuString *output = CuStringNew();
   CuSuite* suite = CuSuiteNew();

// APX Common

   CuSuiteAddSuite(suite, testSuite_apx_allocator());
   CuSuiteAddSuite(suite, testsuite_apx_attributesParser());
   CuSuiteAddSuite(suite, testSuite_apx_bytePortMap());
   CuSuiteAddSuite(suite, testSuite_apx_compiler());
   CuSuiteAddSuite(suite, testSuite_apx_dataElement());
   CuSuiteAddSuite(suite, testsuite_apx_dataSignature());
   CuSuiteAddSuite(suite, testsuite_apx_datatype());
   CuSuiteAddSuite(suite, testSuite_apx_eventLoop());

   CuSuiteAddSuite(suite, testSuite_apx_node());
   CuSuiteAddSuite(suite, testSuite_apx_nodeData2());
   CuSuiteAddSuite(suite, testSuite_apx_nodeInfo());
   CuSuiteAddSuite(suite, testSuite_apx_nodeInstance());
   CuSuiteAddSuite(suite, testSuite_apx_parser());
   CuSuiteAddSuite(suite, testsuite_apx_port());
   CuSuiteAddSuite(suite, testSuite_apx_vm());
   CuSuiteAddSuite(suite, testSuite_apx_nodeManager());
   CuSuiteAddSuite(suite, testSuite_apx_connectionBase());


   CuSuiteAddSuite(suite, testSuite_apx_file2());
   CuSuiteAddSuite(suite, testSuite_apx_fileMap());
   CuSuiteAddSuite(suite, testSuite_apx_vmSerializer());
   CuSuiteAddSuite(suite, testSuite_apx_vmDeserializer());

   //File Manager
   CuSuiteAddSuite(suite, testSuite_apx_fileManagerShared());
   CuSuiteAddSuite(suite, testSuite_apx_fileManagerWorker());
   CuSuiteAddSuite(suite, testSuite_apx_fileManagerReceiver());
   CuSuiteAddSuite(suite, testSuite_apx_fileManager());

   //Routing Tables
   CuSuiteAddSuite(suite, testSuite_apx_portConnectorChangeEntry());
   CuSuiteAddSuite(suite, testSuite_apx_portConnectorChangeTable());
   CuSuiteAddSuite(suite, testSuite_apx_portSignatureMap());


   // APX Server
   CuSuiteAddSuite(suite, testSuite_apx_serverConnection());
   CuSuiteAddSuite(suite, testSuite_apx_dataRouting());

   // APX Client
   CuSuiteAddSuite(suite, testSuite_apx_client());
   CuSuiteAddSuite(suite, testSuite_apx_client_testConnection());
   CuSuiteAddSuite(suite, testSuite_apx_client_socketConnection());


// APX Server
   CuSuiteAddSuite(suite, testSuite_apx_serverSocketConnection());



// APX Server Extensions
   CuSuiteAddSuite(suite, testsuite_apx_socketServerExtension());
/*
   CuSuiteAddSuite(suite, testsuite_apx_serverTextLogExtension());
*/

// RemoteFile
   CuSuiteAddSuite(suite, testSuite_remotefile());

   CuSuiteRun(suite);
   CuSuiteSummary(suite, output);
   CuSuiteDetails(suite, output);
   printf("%s\n", output->buffer);
   CuSuiteDelete(suite);
   CuStringDelete(output);
   
}

int main(void)
{
   RunAllTests();
   return 0;
}

void vfree(void *arg)
{
   free(arg);
}
