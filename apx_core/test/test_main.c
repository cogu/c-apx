/*****************************************************************************
* \file      test_main.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     Unit test entry point
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
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
CuSuite* testsuite_remotefile(void);
CuSuite* testsuite_file_info(void);


/** APX Common **/
CuSuite* testsuite_apx_allocator(void);
CuSuite* testsuite_apx_attributes_parser(void);
CuSuite* testsuite_apx_computation(void);
CuSuite* testsuite_apx_data_element(void);
CuSuite* testsuite_apx_signature_parser(void);
CuSuite* testsuite_apx_parser(void);
CuSuite* testsuite_program_encoder(void);
CuSuite* testsuite_apx_compiler_pack(void);
CuSuite* testsuite_apx_compiler_unpack(void);
CuSuite* testsuite_apx_vm_serializer(void);
CuSuite* testsuite_apx_vm_deserializer(void);
CuSuite* testsuite_program_decoder(void);
CuSuite* testsuite_apx_vm_pack(void);
CuSuite* testsuite_apx_vm_unpack(void);
CuSuite* testsuite_apx_node(void);
CuSuite* testsuite_apx_node_data(void);
CuSuite* testsuite_apx_node_manager_client_mode(void);
CuSuite* testsuite_apx_node_manager_server_mode(void);
CuSuite* testsuite_apx_file(void);
CuSuite* testsuite_apx_file_map(void);
CuSuite* testsuite_apx_file_manager_receiver(void);
CuSuite* testsuite_apx_util(void);
CuSuite* testsuite_apx_port_connector_change_entry(void);
CuSuite* testsuite_apx_port_connector_change_table(void);
CuSuite* testsuite_apx_port_signature_map(void);

//Client
CuSuite* testsuite_apx_client_test_connection(void);
CuSuite* testsuite_apx_client_socket_connection(void);
CuSuite* testsuite_apx_client(void);
CuSuite* testsuite_apx_client_socket_monitor_connection(void);

//Server
CuSuite* testsuite_apx_server_connection(void);
CuSuite* testsuite_apx_server(void);

//Server extensions
CuSuite* testsuite_apx_socket_server_extension(void);
CuSuite* testsuite_apx_socket_server_connection(void);
CuSuite* testsuite_apx_server_monitor_state(void);
CuSuite* testsuite_apx_monitor_extension(void);

//Applications / Server config
CuSuite* testsuite_server_cfg(void);

void RunAllTests(void)
{
   CuString *output = CuStringNew();
   CuSuite* suite = CuSuiteNew();

// RemoteFile
   CuSuiteAddSuite(suite, testsuite_remotefile());
   CuSuiteAddSuite(suite, testsuite_file_info());

//Util
   CuSuiteAddSuite(suite, testsuite_apx_util());


// APX Common

   CuSuiteAddSuite(suite, testsuite_apx_allocator());
   CuSuiteAddSuite(suite, testsuite_apx_attributes_parser());
   CuSuiteAddSuite(suite, testsuite_apx_data_element());
   CuSuiteAddSuite(suite, testsuite_apx_signature_parser());
   CuSuiteAddSuite(suite, testsuite_apx_parser());
   CuSuiteAddSuite(suite, testsuite_program_encoder());
   CuSuiteAddSuite(suite, testsuite_apx_compiler_pack());
   CuSuiteAddSuite(suite, testsuite_apx_compiler_unpack());
   CuSuiteAddSuite(suite, testsuite_apx_vm_serializer());
   CuSuiteAddSuite(suite, testsuite_apx_vm_deserializer());
   CuSuiteAddSuite(suite, testsuite_program_decoder());
   CuSuiteAddSuite(suite, testsuite_apx_vm_pack());
   CuSuiteAddSuite(suite, testsuite_apx_vm_unpack());
   CuSuiteAddSuite(suite, testsuite_apx_node());
   CuSuiteAddSuite(suite, testsuite_apx_node_data());
   CuSuiteAddSuite(suite, testsuite_apx_computation());
   CuSuiteAddSuite(suite, testsuite_apx_node_manager_client_mode());
   CuSuiteAddSuite(suite, testsuite_apx_node_manager_server_mode());
   CuSuiteAddSuite(suite, testsuite_apx_file());
   CuSuiteAddSuite(suite, testsuite_apx_file_map());
   CuSuiteAddSuite(suite, testsuite_apx_file_manager_receiver());
   CuSuiteAddSuite(suite, testsuite_apx_port_connector_change_entry());
   CuSuiteAddSuite(suite, testsuite_apx_port_connector_change_table());
   CuSuiteAddSuite(suite, testsuite_apx_port_signature_map());

   //Client
   CuSuiteAddSuite(suite, testsuite_apx_client_test_connection());
   CuSuiteAddSuite(suite, testsuite_apx_client_socket_connection());
   CuSuiteAddSuite(suite, testsuite_apx_client());
   CuSuiteAddSuite(suite, testsuite_apx_client_socket_monitor_connection());


   //Server
   CuSuiteAddSuite(suite, testsuite_apx_server_connection());
   CuSuiteAddSuite(suite, testsuite_apx_server());

   //Server extensions
   CuSuiteAddSuite(suite, testsuite_apx_socket_server_extension());
   CuSuiteAddSuite(suite, testsuite_apx_socket_server_connection());
   CuSuiteAddSuite(suite, testsuite_apx_server_monitor_state());
   CuSuiteAddSuite(suite, testsuite_apx_monitor_extension());

   //Applications / Server config
   CuSuiteAddSuite(suite, testsuite_server_cfg());

   //Run Tests
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
