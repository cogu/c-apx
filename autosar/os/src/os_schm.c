//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
#include <Windows.h>
#include <Process.h>
#else
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>
#endif
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include "osmacro.h"
#include "osutil.h"
#include "os_schm.h"
#include "priority_queue.h"
#include "systime.h"
#include "adt_ary.h"
#include <malloc.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
static THREAD_PROTO(TimerEventWorker,arg);

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void initOsTasks(void);
static void shutdownOsTasks(void);
static void startOsTasks(void);
static void stopOsTasks(void);
static void initScheduler(void);

#ifdef UNIT_TEST
#define DYN_STATIC
#else
#define DYN_STATIC static
DYN_STATIC void os_schm_run(void);
DYN_STATIC priority_queue_t *os_time_getPriorityQueue(void);
#endif




//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
extern os_task_cfg_t g_os_task_cfg[];
//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
static priority_queue_t m_pq;
static bool m_workerThreadValid;
static os_schm_cfg_t *m_cfg = 0;

THREAD_T m_thread_worker;
SPINLOCK_T m_spin;
#ifdef _MSC_VER
DWORD m_threadId;
#endif
uint8_t m_running = 0;
uint32_t (*m_getTimeFn)(void) = 0;
void (*m_eventTriggerHook)(const os_timer_ev_cfg_t *cfg);


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

/**
 * \param cfg array of event object.
 * \param u32CfgLen number of elements in configuration array
 * \timerfunc pointer to function that returns current system time (default: SysTime_getTime)
 */
void os_schm_init(os_schm_cfg_t *cfg)
{   
   m_cfg = cfg;
   priority_queue_create(&m_pq);
   
   m_workerThreadValid = false;
   if (m_cfg->timerEventHookFunc != 0)
   {
      m_getTimeFn = m_cfg->timerFunc;
   }
   else
   {
      m_getTimeFn = SysTime_getTime;
   }   
   
   initOsTasks();
   initScheduler();
}

void os_schm_shutdown(void)
{
   priority_queue_destroy(&m_pq);
   shutdownOsTasks();
}

void os_schm_start(void)
{
#ifndef _MSC_VER
   pthread_attr_t attr;
#endif

   SPINLOCK_INIT(m_spin);   
   m_workerThreadValid = false;
   m_running = 0;

   startOsTasks();

#ifdef _MSC_VER
   THREAD_CREATE(m_thread_worker, TimerEventWorker, 0, m_threadId);
   if (m_thread_worker == INVALID_HANDLE_VALUE)
   {
      m_workerThreadValid = false;
      return;
   }
#else
   pthread_attr_init(&attr);
   pthread_attr_setstacksize(&attr, PROG_MAX_STACK_SIZE);
   THREAD_CREATE_ATTR(m_thread_worker,attr,TimerEventWorker,0);
#endif
   m_workerThreadValid = true;
   m_running = 1;
}

void os_schm_stop(void)
{
   SPINLOCK_ENTER(m_spin);
   m_running = 0;
   SPINLOCK_LEAVE(m_spin);
   THREAD_JOIN(m_thread_worker);
   THREAD_DESTROY(m_thread_worker);
   SPINLOCK_DESTROY(m_spin);
   m_workerThreadValid = false;
   stopOsTasks();
}


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
DYN_STATIC void os_schm_run(void)
{
   uint32_t currentTimeMs = m_getTimeFn();
   while(1)
   {
      adt_heap_elem_t* elem = priority_queue_top(&m_pq);
      if (currentTimeMs >= elem->u32Value)
      {
         const os_timer_ev_cfg_t *cfg = (const os_timer_ev_cfg_t*) elem->pItem;
         //printf("{%u, %d},\n", currentTimeMs, (int) cfg->eventID);

         //call hook if set
         if (m_eventTriggerHook != 0)
         {
            m_eventTriggerHook(cfg);
         }
         //call task handler if task is set
         if (cfg->task != 0)
         {
            os_task_setEvent(cfg->task, cfg->eventID);
         }
         priority_queue_incrementTopPriority(&m_pq, cfg->u32PeriodMs);
      }
      else
      {
         break;
      }
   }
}

THREAD_PROTO(TimerEventWorker,arg){
   SysTime_reset();
   os_schm_run();
   for(;;)
   {
      uint8_t running;
      SPINLOCK_ENTER(m_spin);
      running = m_running;
      SPINLOCK_LEAVE(m_spin);
      if(running == 0){
         break;
      }
      SysTime_wait(1);
      os_schm_run();
   }
   THREAD_RETURN(0);
}


DYN_STATIC priority_queue_t *os_time_getPriorityQueue(void)
{
   return &m_pq;
}

static void initOsTasks(void)  
{
   uint32_t i;
   for (i = 0; i<m_cfg->numOsTasks; i++)
   {
      os_task_create(m_cfg->osTaskList[i].taskPtr, m_cfg->osTaskList[i].threadFuncPtr, m_cfg->osTaskList[i].u16MaxNumEvents);
   }
}

static void shutdownOsTasks(void)
{
   uint32_t i;
   for (i = 0; i<m_cfg->numOsTasks; i++)
   {
      os_task_destroy(m_cfg->osTaskList[i].taskPtr);
   }
}

static void startOsTasks(void)
{
   uint32_t i;
   for (i = 0; i<m_cfg->numOsTasks; i++)
   {    
      os_task_start(m_cfg->osTaskList[i].taskPtr);
   }
}

static void stopOsTasks(void)
{
   uint32_t i;
   for (i = 0; i<m_cfg->numOsTasks; i++)
   {
      os_task_stop(m_cfg->osTaskList[i].taskPtr);
   }
}

static void initScheduler(void)
{
   uint32_t i;
   for (i = 0; i<m_cfg->numTimerEvents; i++)
   {
      priority_queue_push(&m_pq, (void*)&m_cfg->timerEventList[i], m_cfg->timerEventList[i].u32InitDelayMs);
   }
}