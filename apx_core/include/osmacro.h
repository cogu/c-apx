/*****************************************************************************
 * \file      osmacro.h
 * \author    Conny Gustafsson
 * \date      2014-01-29
 * \brief     portable helper macros for sockets, threads etc. for Windows and Linux
 *
 * Copyright (c) 2014-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef OS_MACRO_H
#define OS_MACRO_H

#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
# include <process.h>
# ifdef _MSC_VER
#  include <intrin.h>
# endif
#else
# include <errno.h>
# include <pthread.h>
# include <semaphore.h>
# include <time.h>
# include <unistd.h>
#endif

/* SOCKETS */
#ifdef _WIN32
# define SOCKET_T SOCKET
# define INVALID_SOCKET_FD INVALID_SOCKET
# define IS_INVALID_SOCKET(sockfd) (sockfd == INVALID_SOCKET)
# define SOCKET_CLOSE(sockfd) closesocket(sockfd)
# define SOCKET_SHUTDOWN(sockfd) shutdown(sockfd, SD_BOTH);
# define SOCK_LEN_T int
#else
# define SOCKET_T int
# define INVALID_SOCKET_FD -1
# define IS_INVALID_SOCKET(sockfd) (sockfd < 0)
# define SOCKET_CLOSE(sockfd) close(sockfd)
# define SOCKET_SHUTDOWN(sockfd) shutdown(sockfd, SHUT_RDWR);
# define SOCK_LEN_T socklen_t
#endif

/* THREADS */
// include Windows.h and process.h for Windows, pthread.h for Linux/Cygwin */
#ifdef _WIN32
# define THREAD_T HANDLE
# define THREAD_CREATE(thread, func, arg, id) thread = (HANDLE)_beginthreadex(NULL, 0, func, (void *)arg, 0, &id);
# define THREAD_PROTO(name, arg) unsigned __stdcall name(void *arg)
# define THREAD_PROTO_PTR(name, arg) unsigned(__stdcall * name)(void *arg)
# define THREAD_RETURN(retval) return (unsigned int)retval
# define THREAD_JOIN(thread) WaitForSingleObject(thread, INFINITE);
# define THREAD_DESTROY(thread)                                                                                        \
   do                                                                                                                  \
   {                                                                                                                   \
     if ((thread) != NULL)                                                                                             \
     {                                                                                                                 \
       CloseHandle(thread);                                                                                            \
     }                                                                                                                 \
   }                                                                                                                   \
   while (0)
#else
# define THREAD_T pthread_t
# define THREAD_CREATE(thread, func, arg) pthread_create(&thread, NULL, func, arg);
# define THREAD_CREATE_ATTR(thread, attr, func, arg) pthread_create(&thread, &attr, func, arg);
# define THREAD_PROTO(name, arg) void *name(void *arg)
# define THREAD_PROTO_PTR(name, arg) void *(*name)(void *arg)
# define THREAD_RETURN(retval) return (void *)(intptr_t)retval
# define THREAD_JOIN(thread)                                                                                           \
   {                                                                                                                   \
     void *status;                                                                                                     \
     pthread_join(thread, &status);                                                                                    \
   }
# define THREAD_DESTROY(thread)
#endif

/* CONDITION VARIABLE */
// include Windows.h for Windows, pthread.h for Linux/Cygwin
#ifdef _WIN32
# define COND_T HANDLE
# define COND_INIT(cond) cond = CreateEvent(NULL, FALSE, FALSE, NULL)
# define COND_DESTROY(cond)                                                                                            \
   do                                                                                                                  \
   {                                                                                                                   \
     if ((cond) != NULL)                                                                                               \
     {                                                                                                                 \
       CloseHandle(cond);                                                                                              \
     }                                                                                                                 \
   }                                                                                                                   \
   while (0)
# define COND_SIGNAL(cond) SetEvent(cond)
# define COND_BROADCAST(cond) SetEvent(cond)
# define COND_WAIT(cond, mutex)                                                                                        \
   do                                                                                                                  \
   {                                                                                                                   \
     SignalObjectAndWait(mutex, cond, INFINITE, FALSE);                                                                \
     WaitForSingleObject(mutex, INFINITE);                                                                             \
   }                                                                                                                   \
   while (0)
# define COND_TIMEDWAIT_MS(cond, mutex, timeout_ms, timed_out)                                                         \
   do                                                                                                                  \
   {                                                                                                                   \
     DWORD _cond_res = SignalObjectAndWait(mutex, cond, (DWORD)(timeout_ms), FALSE);                                   \
     WaitForSingleObject(mutex, INFINITE);                                                                             \
     (timed_out) = (_cond_res == WAIT_TIMEOUT);                                                                        \
   }                                                                                                                   \
   while (0)
#else
# define COND_T pthread_cond_t
# define COND_INIT(cond) pthread_cond_init(&cond, NULL)
# define COND_DESTROY(cond) pthread_cond_destroy(&cond)
# define COND_SIGNAL(cond) pthread_cond_signal(&cond)
# define COND_BROADCAST(cond) pthread_cond_broadcast(&cond)
# define COND_WAIT(cond, mutex) pthread_cond_wait(&cond, &mutex)
# define COND_TIMEDWAIT_MS(cond, mutex, timeout_ms, timed_out)                                                         \
   do                                                                                                                  \
   {                                                                                                                   \
     struct timespec _cond_ts;                                                                                         \
     clock_gettime(CLOCK_REALTIME, &_cond_ts);                                                                         \
     uint64_t const _cond_ns = (uint64_t)_cond_ts.tv_nsec + ((uint64_t)(timeout_ms) * 1000000ULL);                     \
     _cond_ts.tv_sec += (time_t)(_cond_ns / 1000000000ULL);                                                            \
     _cond_ts.tv_nsec = (long)(_cond_ns % 1000000000ULL);                                                              \
     int const _cond_rc = pthread_cond_timedwait(&cond, &mutex, &_cond_ts);                                            \
     (timed_out) = (_cond_rc == ETIMEDOUT);                                                                            \
   }                                                                                                                   \
   while (0)
#endif

/* MUTEX MECHANISM */
// include Windows.h for Windows, pthread.h for Linux/Cygwin
#ifdef _WIN32
# define MUTEX_T HANDLE
# define MUTEX_INIT(mutex) mutex = CreateMutex(NULL, FALSE, NULL)
# define MUTEX_LOCK(mutex) WaitForSingleObject(mutex, INFINITE)
# define MUTEX_UNLOCK(mutex) ReleaseMutex(mutex)
# define MUTEX_DESTROY(mutex)                                                                                          \
   do                                                                                                                  \
   {                                                                                                                   \
     if ((mutex) != NULL)                                                                                              \
     {                                                                                                                 \
       CloseHandle(mutex);                                                                                             \
     }                                                                                                                 \
   }                                                                                                                   \
   while (0)
#else
# define MUTEX_T pthread_mutex_t
# define MUTEX_INIT(mutex) pthread_mutex_init(&mutex, 0)
# define MUTEX_LOCK(mutex) pthread_mutex_lock(&mutex)
# define MUTEX_UNLOCK(mutex) pthread_mutex_unlock(&mutex)
# define MUTEX_DESTROY(mutex) pthread_mutex_destroy(&mutex)
#endif

/* SEMAPHORE */
// include Windows.h for Windows, semaphore.h for Linux/Cygwin
#ifdef _WIN32
# define SEMAPHORE_MAX_COUNT 100
# define SEMAPHORE_T HANDLE
# define SEMAPHORE_CREATE(sem) sem = CreateSemaphore(NULL, 0, SEMAPHORE_MAX_COUNT, NULL)
# define SEMAPHORE_EV_CREATE(sem) sem = CreateSemaphore(NULL, 0, 1, NULL) // maximum semaphore count=1
# define SEMAPHORE_WAIT(sem) WaitForSingleObject(sem, INFINITE)
# define SEMAPHORE_POST(sem) ReleaseSemaphore(sem, 1, NULL)
# define SEMAPHORE_DESTROY(sem) CloseHandle(sem);
// timed semaphore wait is too different between linux and Windows, no macro defined
#else
# define SEMAPHORE_T sem_t
# define SEMAPHORE_CREATE(sem) sem_init(&sem, 0, 0)
# define SEMAPHORE_EV_CREATE(sem)                                                                                      \
   sem_init(&sem, 0, 0) // not possible to choose maximum count in Linux. Use _sem_ev_post helper function in osutil.c
# define SEMAPHORE_WAIT(sem) sem_wait(&sem)
# define SEMAPHORE_POST(sem) sem_post(&sem)
# define SEMAPHORE_DESTROY(sem) sem_destroy(&sem)
// timed semaphore wait is too different between linux and Windows, no macro defined
#endif

/* SPIN LOCK */
// include Windows.h for Windows, pthread.h for Linux/Cygwin
#ifdef _WIN32
# define SPINLOCK_T CRITICAL_SECTION
# define SPINLOCK_INIT(spin) (void)InitializeCriticalSectionAndSpinCount(&spin, 1000)
# define SPINLOCK_ENTER(spin) EnterCriticalSection(&spin)
# define SPINLOCK_LEAVE(spin) LeaveCriticalSection(&spin)
# define SPINLOCK_DESTROY(spin) DeleteCriticalSection(&spin);
#else
# define SPINLOCK_T pthread_spinlock_t
# define SPINLOCK_INIT(spin) pthread_spin_init(&spin, 0)
# define SPINLOCK_ENTER(spin) pthread_spin_lock(&spin)
# define SPINLOCK_LEAVE(spin) pthread_spin_unlock(&spin)
# define SPINLOCK_DESTROY(spin) pthread_spin_destroy(&spin);
#endif

/* ATOMIC OPERATIONS (8-BIT) */
#if defined(__GNUC__) || defined(__clang__)
# define ATOMIC_STORE_8(ptr, val) __atomic_store_n((ptr), (val), __ATOMIC_RELEASE)
# define ATOMIC_LOAD_8(ptr) __atomic_load_n((ptr), __ATOMIC_ACQUIRE)
#elif defined(_MSC_VER)
# define ATOMIC_STORE_8(ptr, val) ((void)_InterlockedExchange8((volatile char *)(ptr), (char)(val)))
# define ATOMIC_LOAD_8(ptr) (_InterlockedOr8((volatile char *)(ptr), 0))
#else
# define ATOMIC_STORE_8(ptr, val) ((void)(*(ptr) = (val)))
# define ATOMIC_LOAD_8(ptr) (*(ptr))
#endif

/* SLEEP */
// include Winsows.h for Windows, unistd.h for Linux/Cygwin
#ifdef _WIN32
# define SLEEP(ms) Sleep(ms);
#else
# define SLEEP(ms) usleep((ms) * 1000);
#endif

/* STRING UTILITY */
#ifndef STRDUP
# ifdef _WIN32
#  define STRDUP _strdup
# else
#  define STRDUP strdup
# endif
#endif


/***************** Public Function Declarations *******************/


#endif // OS_MACRO_H
