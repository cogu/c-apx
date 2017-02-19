#ifndef APX_NODE_DATA_CFG_H_
#define APX_NODE_DATA_CFG_H_
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#if defined(_MSC_PLATFORM_TOOLSET) && (_MSC_PLATFORM_TOOLSET<=100)
#include "msc_bool.h"
#else
#include <stdbool.h>
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

/* without APX_POLLED_DATA_MODE defined:
 * This is the expected usage when APX is built for an environment that has an underlying operating system (either Windows or Linux)
 * This solution uses modern OS semaphores, mutexes and threads to make APX an high performance, event driven solution
 *
 * with APX_POLLED_DATA_MODE defined
 * This mode should be used on systems that doesn't run an operating system or runs an RTOS.
 * Most Real Time Operating Systems (RTOS) has some kind of semaphore and mutex functionality that can be used for creating your own locking
 * mechanism to protect the byte buffers
 */

//uncomment below to enable polled mode
//#define APX_POLLED_DATA_MODE


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

#endif //APX_NODE_DATA_CFG_H_
