//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "os_tem.h"
#include "systime.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define Os_Event_Shutdown_Task         0
#define Os_Event_Test_Task_TMT_5ms     1
#define Os_Event_Test_Task_TMT_20ms    2
#define Os_Event_Test_Task_TMT_100ms   3

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_tem(CuTest* tc);
THREAD_PROTO(Test_Task, arg);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
static os_task_t m_os_Test_Task;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testsuite_os_tem(void)
{
	CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_tem);

	return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_tem(CuTest* tc)
{
   const os_timerEventCfg_t m_timerEventCfg[] =
   {
      //InitDelayMs, PeriodMs, os_task_t* eventId
      { 10u, 5u, &m_os_Test_Task, Os_Event_Test_Task_TMT_5ms },
      { 10u, 20u, &m_os_Test_Task, Os_Event_Test_Task_TMT_20ms },
      { 10u, 100u, &m_os_Test_Task, Os_Event_Test_Task_TMT_100ms },
      //adjust number of elements in this array to match m_timerEventState
   };

   os_timerEventState_t m_timerEventState[] =
   {
      //u8Enabled, u32Next(leave as zero)
      { 1, 0 },
      { 1, 0 },
      { 1, 0 },
      //adjust number of elements in this array to match m_timerEventCfg
   };

   SysTime_initSimulated();
   CuAssertUIntEquals(tc, 0, SysTime_getTime());
   os_task_create(&m_os_Test_Task, Test_Task, 100);
   os_tem_init(m_timerEventCfg, m_timerEventState, sizeof(m_timerEventState) / sizeof(m_timerEventState[0]));
   os_tem_run();
   os_tem_run();
   os_tem_run();
}

THREAD_PROTO(Test_Task, arg)
{
   /* init */
   boolean isRunning = TRUE;
   os_task_t *self = (os_task_t*)arg;
   if (self == 0)
   {
      fprintf(stderr, "Error: Serive_Task called with null argument\n");
   }

   while (isRunning == TRUE)
   {
#ifdef _MSC_VER
      DWORD result = WaitForSingleObject(self->semaphore, INFINITE);
      if (result == WAIT_OBJECT_0)
#else
      int result = sem_wait(&self->semaphore);
      if (result == 0)
#endif
      {
         uint16_t eventType;
         SPINLOCK_ENTER(self->lock);
         rbfu16_remove(&self->eventQueue, &eventType);
         SPINLOCK_LEAVE(self->lock);
         switch (eventType)
         {
         case Os_Event_Shutdown_Task:
            isRunning = FALSE;
            break;
         case Os_Event_Test_Task_TMT_5ms:
            printf("Os_Event_Service_Task_TMT_Frt_5ms\n");
            break;
         case Os_Event_Test_Task_TMT_20ms:
            printf("Os_Event_Service_Task_TMT_Frt_100ms\n");
            break;
         case Os_Event_Test_Task_TMT_100ms:
            printf("Os_Event_Service_Task_TMT_Frt_100ms\n");
            break;
         default:
            fprintf(stderr, "[RTE]: unknown eventType: %u\n", eventType);
            isRunning = FALSE;
            break;
         }
      }
      else
      {
#ifdef _MSC_VER
         fprintf(stderr, "[RTE]: failure while waiting for semaphore, error=%d", (int)GetLastError());
#else			
         fprintf(stderr, "[RTE]: failure while waiting for semaphore, errno=%d", errno);
#endif
         break;
      }
   }
   THREAD_RETURN(0);
}
