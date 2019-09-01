/*****************************************************************************
* \file      testsuite_apx_vmSerializer.c
* \author    Conny Gustafsson
* \date      2019-08-11
* \brief     Test suite for apx_vmSerializer
*
* Copyright (c) 2019 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_nodeProgramContainer.h"
#include "apx_vm.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_nodeProgramContainer_compilePackPrograms(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_nodeProgramContainer(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_nodeProgramContainer_compilePackPrograms);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_nodeProgramContainer_compilePackPrograms(CuTest* tc)
{
   apx_node_t *node;
   apx_nodeProgramContainer_t *container;
   adt_bytes_t *portProgram = NULL;
   const uint8_t *code;
   uint8_t opcode, variant, flags;
   apx_uniquePortId_t errPortId;


   node = apx_node_new("TestNode");
   apx_node_createRequirePort(node,"VehicleSpeed","S","=65535", 1);
   apx_node_createRequirePort(node,"ParkBrakeFailure","C(0,3)", "=3", 2);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(node, NULL));


   container = apx_nodeProgramContainer_new();
   CuAssertPtrNotNull(tc, container);
   CuAssertIntEquals(tc, 0, container->numProvidePorts);
   CuAssertIntEquals(tc, 0, container->numRequirePorts);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeProgramContainer_compilePackPrograms(container, node, &errPortId));
   CuAssertIntEquals(tc, 0, container->numProvidePorts);
   CuAssertIntEquals(tc, 2, container->numRequirePorts);
   portProgram = apx_nodeProgramContainer_getRequirePortPackProgram(container, 0);
   CuAssertPtrNotNull(tc, portProgram);
   CuAssertIntEquals(tc, APX_VM_HEADER_SIZE+APX_VM_INSTRUCTION_SIZE, adt_bytes_length(portProgram));
   code = adt_bytes_data(portProgram);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decodeInstruction(code[APX_VM_HEADER_SIZE+0],&opcode, &variant, &flags));
   CuAssertUIntEquals(tc, APX_OPCODE_PACK, opcode);
   CuAssertUIntEquals(tc, APX_VARIANT_U16, variant);
   CuAssertUIntEquals(tc, 0u, flags);
   portProgram = apx_nodeProgramContainer_getRequirePortPackProgram(container, 1);
   CuAssertPtrNotNull(tc, portProgram);
   CuAssertIntEquals(tc, APX_VM_HEADER_SIZE+APX_VM_INSTRUCTION_SIZE, adt_bytes_length(portProgram));
   code = adt_bytes_data(portProgram);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decodeInstruction(code[APX_VM_HEADER_SIZE+0],&opcode, &variant, &flags));
   CuAssertUIntEquals(tc, APX_OPCODE_PACK, opcode);
   CuAssertUIntEquals(tc, APX_VARIANT_U8, variant);
   CuAssertUIntEquals(tc, 0u, flags);


   apx_node_delete(node);
   apx_nodeProgramContainer_delete(container);

}

