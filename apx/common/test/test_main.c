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
CuSuite* testSuite_apx_fileManagerLocal(void);
CuSuite* testSuite_apx_fileManagerRemote(void);
CuSuite* testSuite_apx_fileManagerShared(void);
CuSuite* testSuite_apx_fileManager(void);
CuSuite* testSuite_apx_fileMap(void);
CuSuite* testSuite_apx_node(void);
CuSuite* testSuite_apx_nodeData(void);
CuSuite* testSuite_apx_nodeDataManager(void);
CuSuite* testSuite_apx_nodeInfo(void);
CuSuite* testSuite_apx_parser(void);
CuSuite* testsuite_apx_port(void);
CuSuite* testSuite_apx_portConnectionEntry(void);
CuSuite* testSuite_apx_portConnectionTable(void);
CuSuite* testSuite_apx_portDataMap(void);
CuSuite* testSuite_apx_portTriggerList(void);
CuSuite* testSuite_apx_routingTable(void);
CuSuite* testSuite_apx_vm(void);
CuSuite* testSuite_apx_vmSerializer(void);

/** APX Server **/
CuSuite* testSuite_apx_serverSocketConnection(void);
CuSuite* testSuite_apx_server(void);

/** APX Client **/
CuSuite* testSuite_apx_client_socketConnection(void);
CuSuite* testSuite_apx_client_testConnection(void);

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
   CuSuiteAddSuite(suite, testSuite_apx_file2());
   CuSuiteAddSuite(suite, testSuite_apx_fileManagerLocal());
   CuSuiteAddSuite(suite, testSuite_apx_fileManagerRemote());
   CuSuiteAddSuite(suite, testSuite_apx_fileManagerShared());
   CuSuiteAddSuite(suite, testSuite_apx_fileManager());
   CuSuiteAddSuite(suite, testSuite_apx_fileMap());
   CuSuiteAddSuite(suite, testSuite_apx_node());
   CuSuiteAddSuite(suite, testSuite_apx_nodeData());
   CuSuiteAddSuite(suite, testSuite_apx_nodeDataManager());
   CuSuiteAddSuite(suite, testSuite_apx_nodeInfo());
   CuSuiteAddSuite(suite, testSuite_apx_parser());
   CuSuiteAddSuite(suite, testsuite_apx_port());
   CuSuiteAddSuite(suite, testSuite_apx_portConnectionEntry());
   CuSuiteAddSuite(suite, testSuite_apx_portConnectionTable());
   CuSuiteAddSuite(suite, testSuite_apx_portDataMap());
   CuSuiteAddSuite(suite, testSuite_apx_portTriggerList());
   CuSuiteAddSuite(suite, testSuite_apx_routingTable());
   CuSuiteAddSuite(suite, testSuite_apx_vmSerializer());
   CuSuiteAddSuite(suite, testSuite_apx_vm());

// APX Server
   CuSuiteAddSuite(suite, testSuite_apx_serverSocketConnection());
   CuSuiteAddSuite(suite, testSuite_apx_server());

// APX Client
   CuSuiteAddSuite(suite, testSuite_apx_client_socketConnection());
   CuSuiteAddSuite(suite, testSuite_apx_client_testConnection());

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
