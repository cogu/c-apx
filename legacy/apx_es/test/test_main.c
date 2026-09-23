/*****************************************************************************
* \file      test_main.c
* \author    Conny Gustafsson
* \date      2017-03-12
* \brief     Unit test entry point for APX-ES
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#include <stdio.h>
#include "CuTest.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


CuSuite* testsuite_apx_es_filemanager(void);
CuSuite* testsuite_apx_es_filemap(void);


void streambuf_lock(void){}
void streambuf_unlock(void){}

void RunAllTests(void)
{
   CuString *output = CuStringNew();
   CuSuite* suite = CuSuiteNew();

   CuSuiteAddSuite(suite, testsuite_apx_es_filemanager());
   CuSuiteAddSuite(suite, testsuite_apx_es_filemap());

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
