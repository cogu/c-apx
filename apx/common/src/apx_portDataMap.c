//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include "apx_portDataMap.h"
#include "apx_dataSignature.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static int32_t apx_portDataMap_buildInternal(adt_ary_t *entryList, adt_ary_t *portList);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_portDataMapEntry_create(apx_portDataMapEntry_t *self, apx_port_t *port, int32_t offset, int32_t length)
{
   if (self != 0)
   {
      self->port=port;
      self->offset=offset;
      self->length=length;
   }
}

void apx_portDataMapEntry_destroy(apx_portDataMapEntry_t *self)
{
  //nothing to do, self->port is a shared pointer
}

apx_portDataMapEntry_t *apx_portDataMapEntry_new(apx_port_t *port,int32_t offset, int32_t length)
{
   apx_portDataMapEntry_t *self = (apx_portDataMapEntry_t*) malloc(sizeof(apx_portDataMapEntry_t));
   if(self != 0){
      apx_portDataMapEntry_create(self,port,offset,length);
   }
   else{
      errno = ENOMEM;
   }
   return self;
}

void apx_portDataMapEntry_delete(apx_portDataMapEntry_t *self)
{
   if (self != 0)
   {
      apx_portDataMapEntry_destroy(self);
      free(self);
   }
}

/**
 * virtual destructor
 */
void apx_portDataMapEntry_vdelete(void *arg)
{
   apx_portDataMapEntry_delete((apx_portDataMapEntry_t*) arg);
}

void apx_portDataMap_create(apx_portDataMap_t *self)
{
   if (self != 0)
   {
      adt_ary_create(&self->elements,apx_portDataMapEntry_vdelete);
      self->node = (apx_node_t*) 0;
      self->mapType = -1;
      self->totalLen = -1;
   }
}

void apx_portDataMap_destroy(apx_portDataMap_t *self)
{
   if (self != 0)
   {
      adt_ary_destroy(&self->elements);
   }
}

apx_portDataMap_t *apx_portDataMap_new(void)
{
   apx_portDataMap_t *self = (apx_portDataMap_t*) malloc(sizeof(apx_portDataMap_t));
   if(self != 0){
      apx_portDataMap_create(self);
   }
   else{
      errno = ENOMEM;
   }
   return self;
}

void apx_portDataMap_delete(apx_portDataMap_t *self)
{
   if (self != 0)
   {
      apx_portDataMap_destroy(self);
      free(self);
   }
}

void apx_portDataMap_vdelete(void *arg)
{
   apx_portDataMap_delete((apx_portDataMap_t*) arg);
}


int8_t apx_portDataMap_build(apx_portDataMap_t *self, apx_node_t *node, uint8_t portType)
{
   if ( (self != 0) && (node != 0) )
   {
      if (portType == APX_REQUIRE_PORT)
      {
         self->mapType = APX_REQUIRE_PORT;
         self->totalLen=apx_portDataMap_buildInternal(&self->elements,&node->requirePortList);
      }
      else if (portType == APX_PROVIDE_PORT)
      {
         self->mapType = APX_PROVIDE_PORT;
         self->totalLen=apx_portDataMap_buildInternal(&self->elements,&node->providePortList);
      }
      else
      {
         return -1;
      }
      return 0;
   }
   return -1;
}


int32_t apx_portDataMap_getDataLen(apx_portDataMap_t *self)
{
   if (self != 0)
   {
      return self->totalLen;
   }
   return -1;
}

apx_portDataMapEntry_t *apx_portDataMap_getEntry(apx_portDataMap_t *self, int32_t portIndex)
{
   if (self != 0)
   {
      int32_t numPorts = adt_ary_length(&self->elements);
      if ( (portIndex>=0) && (portIndex < numPorts) )
      {
         return (apx_portDataMapEntry_t*) *adt_ary_get(&self->elements,portIndex);
      }
   }
   errno=EINVAL;
   return 0;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static int32_t apx_portDataMap_buildInternal(adt_ary_t *entryList, adt_ary_t *portList)
{
   int32_t retval=-1;
   if ( (entryList != 0) && (portList != 0) )
   {
      int32_t i;
      int32_t end;
      int32_t offset = 0;

      adt_ary_clear(entryList);
      end = adt_ary_length(portList);
      for (i=0;i<end;i++)
      {
         void **ptr = adt_ary_get(portList,i);
         if (ptr != 0)
         {
            apx_port_t *port = (apx_port_t*) *ptr;
            apx_portDataMapEntry_t *entry;
            int32_t packLen;
            assert( port != 0);
            packLen = apx_port_getPackLen(port);
            if (packLen>0)
            {
               entry = apx_portDataMapEntry_new(port,offset,packLen);
               offset+=packLen;
               adt_ary_push(entryList,(void*) entry);
            }
            else
            {
               printf("%s has no length\n",port->name);
            }
         }
      }
      retval=offset;
   }
   return retval;
}

