/*******************************************************************************
* COPYRIGHT (c) Volvo Corporation 2016
*
* The copyright of the computer program(s) herein is the property of Volvo
* Corporation, Sweden. The programs may be used and copied only with the
* written permission from Volvo Corporation, or in accordance with the terms
* and conditions stipulated in the agreement contract under which the
* program(s) have been supplied.
*
******************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "apx_routerPortMapEntry.h"
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
void apx_routerPortMapEntry_create(apx_routerPortMapEntry_t *self)
{
   if (self != 0)
   {
      adt_ary_create(&self->requirePorts,apx_portref_vdelete);
      adt_ary_create(&self->providePorts,apx_portref_vdelete);
   }
}

void apx_routerPortMapEntry_destroy(apx_routerPortMapEntry_t *self)
{
   if (self != 0)
   {
      adt_ary_destroy(&self->requirePorts);
      adt_ary_destroy(&self->providePorts);
   }
}

apx_routerPortMapEntry_t *apx_routerPortMapEntry_new(void)
{
   apx_routerPortMapEntry_t *self = (apx_routerPortMapEntry_t*) malloc(sizeof(apx_routerPortMapEntry_t));
   if(self != 0){
      apx_routerPortMapEntry_create(self);
   }
   else{
      errno = ENOMEM;
   }
   return self;
}

void apx_routerPortMapEntry_delete(apx_routerPortMapEntry_t *self)
{
   if (self != 0)
   {
      apx_routerPortMapEntry_destroy(self);
      free(self);
   }
}

void apx_routerPortMapEntry_vdelete(void *arg)
{
   apx_routerPortMapEntry_delete( (apx_routerPortMapEntry_t*) arg );
}

int8_t apx_routerPortMapEntry_insertPort(apx_routerPortMapEntry_t *self, apx_node_t *node, apx_port_t *port)
{
   if ( (self != 0) && (node != 0) && (port != 0) )
   {
      int32_t i;
      int32_t end;
      adt_ary_t *portrefList = 0;
      apx_portref_t *portref = apx_portref_new(node,port);

      if (portref == 0)
      {
         return -1; //apx_portref_new should already have set errno
      }
      if (portref->port->portType == APX_REQUIRE_PORT)
      {
         portrefList = &self->requirePorts;
      }
      else if (portref->port->portType == APX_PROVIDE_PORT)
      {
         portrefList = &self->providePorts;
      }
      else
      {
         apx_portref_delete(portref);
         errno = EINVAL;
         return -1;
      }
      //prevent the user from adding the same port reference twice.
      assert(portrefList != 0);
      end = adt_ary_length(portrefList);
      for (i=0;i<end;i++)
      {
         apx_portref_t *elem = (apx_portref_t*) *adt_ary_get(portrefList,i);
         if (apx_portref_equals(elem,portref) != 0)
         {
            //the portref already exists in this list, take no more action
            apx_portref_delete(portref);
            return 0;
         }
      }
      //New portref.
      //Add it to portrefList, destruction of portref will be automatically taken care of when destructor of apx_routerPortMapEntry_t is called.
      adt_ary_push(portrefList,portref);
      return 0;
   }
   errno = EINVAL;
   return -1;
}

int8_t apx_routerPortMapEntry_insertProvidePort(apx_routerPortMapEntry_t *self, apx_node_t *node, int32_t portIndex)
{
   if ( (self != 0) && (node != 0) )
   {
      int32_t numProvidePorts;
      numProvidePorts = adt_ary_length(&node->providePortList);
      if ( (portIndex >= 0) && (portIndex < numProvidePorts ))
      {
         apx_port_t *port = (apx_port_t*) *adt_ary_get(&node->providePortList,portIndex);
         return apx_routerPortMapEntry_insertPort(self,node,port);
      }
   }
   errno=EINVAL;
   return -1;
}

int8_t apx_routerPortMapEntry_insertRequirePort(apx_routerPortMapEntry_t *self, apx_node_t *node, int32_t portIndex)
{
   if ( (self != 0) && (node != 0) )
   {
      int32_t numRequirePorts;
      numRequirePorts = adt_ary_length(&node->requirePortList);
      if ( (portIndex >= 0) && (portIndex < numRequirePorts ))
      {
         apx_port_t *port = (apx_port_t*) *adt_ary_get(&node->requirePortList,portIndex);
         return apx_routerPortMapEntry_insertPort(self,node,port);
      }
   }
   errno=EINVAL;
   return -1;
}

int8_t apx_routerPortMapEntry_removePort(apx_routerPortMapEntry_t *self, apx_node_t *node, apx_port_t *port)
{
   if ( (self != 0) && (node != 0) && (port != 0) )
   {
      int32_t i;
      int32_t end;
      apx_portref_t portref;
      adt_ary_t *portrefList = 0;
      apx_portref_create(&portref,node,port);
      if (portref.port->portType == APX_REQUIRE_PORT)
      {
         portrefList = &self->requirePorts;
      }
      else if (portref.port->portType == APX_PROVIDE_PORT)
      {
         portrefList = &self->providePorts;
      }
      else
      {
         errno = EINVAL;
         return -1;
      }
      assert(portrefList != 0);
      end = adt_ary_length(portrefList);
      for (i=0;i<end;i++)
      {
         apx_portref_t *elem = (apx_portref_t*) *adt_ary_get(portrefList,i);
         if (apx_portref_equals(elem,&portref) != 0)
         {
            //found in list, we can now remove it and shorten the list.
            //using the adt_ary_splice method will both shorten the list and destroy the object (it calls apx_portref_vdelete)
            adt_ary_splice(portrefList,i,1);
            break;
         }
      }
      apx_portref_destroy(&portref); //nothing is actually destroyed here but call is made here to match call to the constructor
      return 0;
   }
   errno=EINVAL;
   return -1;
}

/**
 * retreives the portref_t from the self->providePorts array.
 * returns NULL on failure
 */
apx_portref_t *apx_routerPortMapEntry_getProvidePortById(apx_routerPortMapEntry_t *self,int32_t index)
{
   if (self != 0)
   {
      int32_t length = adt_ary_length(&self->providePorts);

      if( (index >= 0) && (index < length) )
      {
         return (apx_portref_t*) *adt_ary_get(&self->providePorts,index);
      }
   }
   return (apx_portref_t*) 0;
}


/**
 * retreives the portref_t from the self->requirePorts array.
 * returns NULL on failure
 */
apx_portref_t *apx_routerPortMapEntry_getRequirePortById(apx_routerPortMapEntry_t *self,int32_t index)
{
   if (self != 0)
   {
      int32_t length = adt_ary_length(&self->requirePorts);

      if( (index >= 0) && (index < length) )
      {
         return (apx_portref_t*) *adt_ary_get(&self->requirePorts,index);
      }
   }
   return (apx_portref_t*) 0;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////



