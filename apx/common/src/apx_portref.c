//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "apx_portref.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_portref_create(apx_portref_t *self,apx_node_t *node,apx_port_t *port)
{
   if (self != 0)
   {
      self->node=node;
      self->port=port;
   }
}

void apx_portref_destroy(apx_portref_t *self)
{
   //nothing to do, object pointers are shared
}

apx_portref_t *apx_portref_new(apx_node_t *node,apx_port_t *port)
{
   apx_portref_t *self = (apx_portref_t*) malloc(sizeof(apx_portref_t));
   if(self != 0){
      apx_portref_create(self,node,port);
   }
   else{
      errno = ENOMEM;
   }
   return self;
}

void apx_portref_delete(apx_portref_t *self)
{
   if (self != 0)
   {
      apx_portref_destroy(self);
      free(self);
   }
}

void apx_portref_vdelete(void *arg)
{
   apx_portref_delete((apx_portref_t*) arg);
}

/**
 * returns nonzero if objects are equal, zero otherwise
 */
int8_t apx_portref_equals(const apx_portref_t *a, const apx_portref_t *b)
{
   if ( (a == 0) || ( b== 0) )
   {
      return 0;
   }
   return (a->node) == (b->node) && (a->port) == (b->port);
}


int8_t apx_portref_createFromRequirePort(apx_portref_t *self,apx_node_t *node,int32_t portIndex)
{
   if ( (self != 0) && (node != 0) && (portIndex>=0) )
   {
      int32_t length = adt_ary_length(&node->requirePortList);
      if (portIndex < length)
      {
         apx_port_t *port = (apx_port_t*) adt_ary_value(&node->requirePortList,portIndex);
         apx_portref_create(self,node,port);
         return 0;
      }
      else
      {
         errno=ENOENT;
         return -1;
      }
   }
   errno=EINVAL;
   return -1;
}

int8_t apx_portref_createFromProvidePort(apx_portref_t *self,apx_node_t *node,int32_t portIndex)
{
   if ( (self != 0) && (node != 0) && (portIndex>=0) )
   {
      int32_t length = adt_ary_length(&node->providePortList);
      if (portIndex < length)
      {
         apx_port_t *port = (apx_port_t*) adt_ary_value(&node->providePortList,portIndex);
         apx_portref_create(self,node,port);
         return 0;
      }
      else
      {
         errno=ENOENT;
         return -1;
      }
   }
   errno=EINVAL;
   return -1;
}


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


