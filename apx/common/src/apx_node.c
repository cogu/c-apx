#include "apx_node.h"
#include "apx_nodeInfo.h"
#include <errno.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "bscan.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

#ifdef _MSC_VER
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif

/**************** Private Function Declarations *******************/
static int apx_node_getDatatypeId(apx_port_t *port);
static const char *apx_node_resolveDataSignature(apx_node_t *self,apx_port_t *port);

/**************** Private Variable Declarations *******************/


/****************** Public Function Definitions *******************/
//constructor/destructor
apx_node_t *apx_node_new(const char *name){
   apx_node_t *self = (apx_node_t*) malloc(sizeof(apx_node_t));
   if(self != 0){
      apx_node_create(self,name);
   }
   else{
      errno = ENOMEM;
   }
   return self;
}

void apx_node_delete(apx_node_t *self){
   if(self != 0){
      apx_node_destroy(self);
      free(self);
   }
}

void apx_node_vdelete(void *arg)
{
   apx_node_delete((apx_node_t*) arg);
}

void apx_node_create(apx_node_t *self,const char *name){
   if(self != 0){
      self->name = 0;
      adt_ary_create(&self->datatypeList,apx_datatype_vdelete);
      adt_ary_create(&self->requirePortList,apx_port_vdelete);
      adt_ary_create(&self->providePortList,apx_port_vdelete);
      apx_node_setName(self,name);
      self->lastPortError=0;
      self->lastPortId=-1;
      self->lastPortType=-1;
      self->nodeInfo=(apx_nodeInfo_t*) 0;
   }
}

void apx_node_destroy(apx_node_t *self){
   if(self != 0){
      adt_ary_destroy(&self->datatypeList);
      adt_ary_destroy(&self->providePortList);
      adt_ary_destroy(&self->requirePortList);
      if(self->name != 0){
         free(self->name);
      }
   }
}

//node functions
void apx_node_setName(apx_node_t *self, const char *name){
   if( (self != 0) ){
      if(self->name != 0){
         free(self->name);
      }
      if(name != 0){
         self->name = STRDUP(name);
      }
      else{
         self->name = 0;
      }
   }
}

//datatype functions
apx_datatype_t *apx_node_createDataType(apx_node_t *self, const char* name, const char *dsg, const char *attr)
{
   apx_datatype_t *datatype=0;
   if (self != 0)
   {
     datatype = apx_datatype_new(name,dsg,attr);
     if (datatype != 0)
     {
        adt_ary_push(&self->datatypeList,datatype);
     }
   }
   return datatype;
}

//port functions
apx_port_t *apx_node_createRequirePort(apx_node_t *self, const char* name, const char *dsg, const char *attr)
{
   apx_port_t *port=0;
   if (self != 0)
   {
     port = apx_requirePort_new(name,dsg,attr);
     if (port != 0)
     {
        int32_t portIndex = adt_ary_length(&self->requirePortList);
        apx_port_setPortIndex(port,portIndex);
        adt_ary_push(&self->requirePortList,port);
     }
   }
   return port;
}

apx_port_t *apx_node_createProvidePort(apx_node_t *self, const char* name, const char *dsg, const char *attr)
{
   apx_port_t *port=0;
   if (self != 0)
   {
     port = apx_providePort_new(name,dsg,attr);
     if (port != 0)
     {
        int32_t portIndex = adt_ary_length(&self->providePortList);
        apx_port_setPortIndex(port,portIndex);
        adt_ary_push(&self->providePortList,port);
     }
   }
   return port;
}

/**
 * return 0 on success, -1 on error
 */
int8_t apx_node_resolvePortSignatures(apx_node_t *self)
{
   int32_t i;
   int32_t providePortLen;
   int32_t requirePortLen;
   providePortLen = adt_ary_length(&self->providePortList);
   requirePortLen = adt_ary_length(&self->requirePortList);
   for(i=0;i<providePortLen;i++)
   {
      void **ptr;
      ptr=adt_ary_get(&self->providePortList,i);
      if (ptr != 0)
      {
         apx_port_t *port = (apx_port_t*) *ptr;
         assert(port != 0);
         if ( port->dataSignature != 0 )
         {
            const char *dataSignature = apx_node_resolveDataSignature(self,port);
            apx_port_setDerivedDataSignature(port,dataSignature);
         }
         apx_port_derivePortSignature(port);
      }
   }
   for(i=0;i<requirePortLen;i++)
   {
      void **ptr;
      ptr=adt_ary_get(&self->requirePortList,i);
      if (ptr != 0)
      {
         apx_port_t *port = (apx_port_t*) *ptr;
         assert(port != 0);
         assert(port != 0);
         if ( port->dataSignature != 0 )
         {
            const char *dataSignature = apx_node_resolveDataSignature(self,port);
            apx_port_setDerivedDataSignature(port,dataSignature);
         }
         apx_port_derivePortSignature(port);
      }
   }
   return 0;
}

apx_port_t *apx_node_getRequirePort(apx_node_t *self, int32_t portIndex)
{
   if (self != 0)
   {
      return (apx_port_t*) *adt_ary_get(&self->requirePortList,portIndex);
   }
   return (apx_port_t*) 0;
}

apx_port_t *apx_node_getProvidePort(apx_node_t *self, int32_t portIndex)
{
   if (self != 0)
   {
      return (apx_port_t*) *adt_ary_get(&self->providePortList,portIndex);
   }
   return (apx_port_t*) 0;
}


/***************** Private Function Definitions *******************/
static int apx_node_getDatatypeId(apx_port_t *port)
{
   const uint8_t *pEnd;
   const uint8_t *pMark;
   const uint8_t *pNext;
   const uint8_t *pBegin = (const uint8_t*) port->dataSignature;
   if ( (port->dataSignature[0]=='T') && (port->dataSignature[1]=='[') )
   {
      pEnd = pBegin+strlen(port->dataSignature);
      pNext=pBegin+1;
      pMark=bscan_matchPair(pNext,pEnd,'[',']','\\');
      if (pMark>pBegin)
      {
         long value;
         const uint8_t *pResult;
         pNext+=1; //move past the '['
         pResult = bscan_toLong(pNext,pMark,&value);
         if (pResult > pNext)
         {
            return (int) value;
         }
      }
   }
   return -1;
}

/**
 * returns the datasignature of the port. If the datasignature is a typereference it will resolve the type reference and return the true data signature
 */
static const char *apx_node_resolveDataSignature(apx_node_t *self,apx_port_t *port)
{
   if ( (self != 0) && (port != 0) )
   {
      if (port->dataSignature != 0)
      {
         if (port->dataSignature[0]=='T')
         {
            if (port->dataSignature[1]=='[')
            {
               int typeId = apx_node_getDatatypeId(port);
               if ( (typeId >= 0) && (typeId < ((int)adt_ary_length(&self->datatypeList))) )
               {
                  void **ptr=adt_ary_get(&self->datatypeList,typeId);
                  apx_datatype_t *datatype = (apx_datatype_t*) *ptr;
                  return datatype->dsg;
               }
               else
               {
                  return (const char *) NULL;
               }
            }
            if (port->dataSignature[1]=='"')
            {
               //TODO: implement type reference by name
            }
         }
         else
         {
            return port->dataSignature;
         }
      }
   }
   return 0;
}



