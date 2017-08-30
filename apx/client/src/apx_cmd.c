//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_cmd.h"
#include <malloc.h>
#include <errno.h>
#include "apx_logging.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

#ifdef _MSC_VER
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_cmd_create(apx_cmd_t *self)
{
   if (self != 0)
   {
      self->cmdType = APX_CMD_NONE;
      self->cmdData = 0;
      self->cmdDestructor = (void (*)(void*)) 0;
   }
}

void apx_cmd_destroy(apx_cmd_t *self)
{
   if ( (self != 0) && (self->cmdData != (void*) 0) && (self->cmdDestructor != (void (*)(void*)) 0) )
   {
      self->cmdDestructor(self->cmdData);
      self->cmdData = (void*) 0;
   }
}

void apx_connectCmd_tcp_create(apx_connectCmd_t *self, const char *tcp_hostname, uint16_t tcp_port)
{
   if(self != 0)
   {
      self->connectionType = APX_CONNECTION_TYPE_TCP_SOCKET;
      self->hostname_or_path = (tcp_hostname != 0)? STRDUP(tcp_hostname) : (char*) 0;
      self->tcp_port = tcp_port;
   }
}

void apx_connectCmd_lsock_create(apx_connectCmd_t *self, const char *lsock_path)
{
   if(self != 0)
   {
      self->connectionType = APX_CONNECTION_TYPE_LOCAL_SOCKET;
      self->hostname_or_path = (lsock_path != 0)? STRDUP(lsock_path) : (char*) 0;
      self->tcp_port = 0;
   }
}

void apx_connectCmd_destroy(apx_connectCmd_t *self)
{
   if ( (self != 0) && (self->hostname_or_path != 0) )
   {
      free(self->hostname_or_path);
   }
}

apx_connectCmd_t *apx_connectCmd_tcp_new(const char *tcp_hostname, uint16_t tcp_port)
{
   apx_connectCmd_t *self = (apx_connectCmd_t*) malloc(sizeof(apx_connectCmd_t));
   if(self != 0)
   {
      apx_connectCmd_tcp_create(self, tcp_hostname, tcp_port);
   }
   else
   {
      errno = ENOMEM;
   }
   return self;
}
apx_connectCmd_t *apx_connectCmd_lsock_new(const char *lsock_path)
{
   apx_connectCmd_t *self = (apx_connectCmd_t*) malloc(sizeof(apx_connectCmd_t));
   if(self != 0)
   {
      apx_connectCmd_lsock_create(self, lsock_path);
   }
   else
   {
      errno = ENOMEM;
   }
   return self;
}

void apx_connectCmd_delete(apx_connectCmd_t *self)
{
   if(self != 0)
   {
      apx_connectCmd_destroy(self);
      free(self);
   }
}

void apx_connectCmd_vdelete(void *arg)
{
   apx_connectCmd_delete((apx_connectCmd_t*) arg);
}




//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

