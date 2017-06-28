#ifndef OSTASK_H
#define OSTASK_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#include <semaphore.h>
#endif
#include <stdbool.h>
#include "osmacro.h"
#include "ringbuf.h"



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct os_task_tag
{
	SPINLOCK_T lock;  //variable lock
	SEMAPHORE_T semaphore; //thread semaphore
	THREAD_PROTO_PTR(thread_func, arg);
	THREAD_T workerThread;
#ifdef _MSC_VER
	DWORD threadId;
#endif
   bool workerThreadValid;
	//data object, all read/write accesses to these must be protected by the lock variable above
	rbfu16_t eventQueue; //pending events, type: uint16
	uint16_t *eventQueueBuf; //strong pointer to raw data used by our ringbuffer
	bool eventQueueBufIsWeakRef;

} os_task_t;

#define OS_SHUTDOWN_EVENT_ID 0 //reserved for shutting down the worker thread
#define OS_USER_EVENT_ID     1
#define OS_INVALID_EVENT_ID 0xFFFF

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
int8_t os_task_create(os_task_t *self, THREAD_PROTO_PTR(thread_func, arg), uint16_t u16MaxNumEvents);
void os_task_destroy(os_task_t *self);
void os_task_start(os_task_t *self);
void os_task_stop(os_task_t *self);
void os_task_setEvent(os_task_t *self, uint16_t eventId);
uint16_t os_task_waitEvent(os_task_t *self);

#endif //OS_H
