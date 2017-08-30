#ifndef APX_CMD_H
#define APX_CMD_H
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#if defined(_MSC_PLATFORM_TOOLSET) && (_MSC_PLATFORM_TOOLSET<=110)
#include "msc_bool.h"
#else
#include <stdbool.h>
#endif
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#endif
#include "osmacro.h"
#include "apx_clientConnection.h"
#include "apx_msg.h"
#include "apx_types.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_CMD_NONE                  (-1)
#define APX_CMD_EXIT                  0
#define APX_CMD_COMPLETE              1 //Enqueues an entry to trigger the "completed" callback by the user
#define APX_CMD_CONNECT               2 //connect to APX broker
#define APX_CMD_DISCONNECT            3 //disconnects from APX broker
#define APX_CMD_HEARTBEAT             4 //Send heartbeat to APX broker
#define APX_CMD_PING_BROKER           5 //Sends ping to APX broker
#define APX_CMD_LIST_NODES            6 //Retrieves list of connected APX nodes currently seen in the broker
#define APX_CMD_OPEN_NODE             7 //Opens and retrieves APX definition of connected APX node
#define APX_CMD_CLOSE_NODE            8 //Closes connected APX node
#define APX_CMD_PING_NODE             9 //Sends ping to an APX node (causing the broker to relay the ping to its client)

typedef struct apx_cmd_tag
{
   int32_t cmdType;
   void *cmdData;
   void(*cmdDestructor)(void*);
}apx_cmd_t;

typedef struct apx_connectCmd_tag
{
   uint8_t connectionType;
   char *hostname_or_path; //used for tcp_hostname or lsock_path
   uint16_t tcp_port;
}apx_connectCmd_t;

//////////////////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_cmd_create(apx_cmd_t *self);
void apx_cmd_destroy(apx_cmd_t *self);
void apx_connectCmd_tcp_create(apx_connectCmd_t *self, const char *tcp_hostname, uint16_t tcp_port);
void apx_connectCmd_lsock_create(apx_connectCmd_t *self, const char *lsock_path);
void apx_connectCmd_destroy(apx_connectCmd_t *self);
apx_connectCmd_t *apx_connectCmd_tcp_new(const char *tcp_hostname, uint16_t tcp_port);
apx_connectCmd_t *apx_connectCmd_lsock_new(const char *lsock_path);
void apx_connectCmd_delete(apx_connectCmd_t *self);
void apx_connectCmd_vdelete(void *arg);



#endif //APX_CMD_H
