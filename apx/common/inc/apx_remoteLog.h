#ifndef APX_REMOTE_LOG_H
#define APX_REMOTE_LOG_H
#if defined(_MSC_PLATFORM_TOOLSET) && (_MSC_PLATFORM_TOOLSET<=110)
#include "msc_bool.h"
#else
#include <stdbool.h>
#endif
#include <stdint.h>
#include "rmf.h"

typedef struct apx_remoteLog_tag
{
   rmf_fileInfo_t fileInfo;
   bool isOpen;
}apx_remoteLog_t;

#define APX_LOG_FILE_NAME "apx.log"
#define APX_LOG_FILE_LEN 2048
#define APX_LOG_FILE_ADDRESS 0x3FFFEC00

#define APX_LOG_MODE_NONE     0u
#define APX_LOG_MODE_CAPTURE  1u
#define APX_LOG_MODE_PLAYBACK 2u

/***************** Public Function Declarations *******************/
void apx_remoteLog_create(apx_remoteLog_t *self);
void apx_remoteLog_destroy(apx_remoteLog_t *self);
apx_remoteLog_t *apx_remoteLog_new(void);
void apx_remoteLog_delete(apx_remoteLog_t *self);

#endif //APX_REMOTE_LOG_H
