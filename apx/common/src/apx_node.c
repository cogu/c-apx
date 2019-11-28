#include <errno.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "bstr.h"
#include "apx_node.h"
#include "apx_logging.h"
#include "apx_error.h"
#include "apx_types.h"
#include "adt_ary.h"
#include "adt_hash.h"
#include "apx_datatype.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

#define ERROR_STR_MAX 128

/**************** Private Function Declarations *******************/
static apx_error_t apx_node_finalizePort(apx_port_t *port, adt_ary_t *typeList, adt_hash_t *typeMap);
static void setError(apx_node_t *self, apx_error_t errorCode);
//static void clearError(apx_node_t *self);

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

void apx_node_create(apx_node_t *self, const char *name){
   if(self != 0){
      self->name = 0;
      adt_ary_create(&self->datatypeList,apx_datatype_vdelete);
      adt_ary_create(&self->requirePortList,apx_port_vdelete);
      adt_ary_create(&self->providePortList,apx_port_vdelete);
      apx_attributeParser_create(&self->attributeParser);
      apx_node_setName(self,name);
      self->isFinalized = false;
      self->lastError = APX_NO_ERROR;
   }
}

void apx_node_destroy(apx_node_t *self){
   if(self != 0){
      adt_ary_destroy(&self->datatypeList);
      adt_ary_destroy(&self->providePortList);
      adt_ary_destroy(&self->requirePortList);
      apx_attributeParser_destroy(&self->attributeParser);
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

const char *apx_node_getName(const apx_node_t *self)
{
   if (self != 0)
   {
      return self->name;
   }
   return (const char*) 0;
}

void apx_node_setVersion(apx_node_t *self, int16_t majorVersion, int16_t minorVersion)
{
   if ( (self != 0) && (majorVersion >= 0) && (minorVersion >= 0))
   {
      apx_attributeParser_setVersion(&self->attributeParser, majorVersion, minorVersion);
   }
}

//datatype functions
apx_datatype_t *apx_node_createDataType(apx_node_t *self, const char* name, const char *dsg, const char *attr, int32_t lineNumber)
{
   apx_datatype_t *datatype=0;
   if (self != 0)
   {
      apx_error_t err;
      datatype = apx_datatype_new(name,dsg,attr, lineNumber, &err);
      if ( (datatype != 0) && (err == APX_NO_ERROR) )
      {
         adt_ary_push(&self->datatypeList,datatype);
      }
   }
   return datatype;
}

//port functions
apx_port_t *apx_node_createRequirePort(apx_node_t *self, const char* name, const char *dsg, const char *attr, int32_t lineNumber)
{
   apx_port_t *port=0;
   if (self != 0)
   {
      apx_error_t err;
      port = apx_requirePort_new(name, dsg, attr, lineNumber, &err);
      if (port != 0)
      {
         apx_portId_t portId = (apx_portId_t) adt_ary_length(&self->requirePortList);
         if ( port->portAttributes != 0 )
         {
            apx_error_t result = apx_attributeParser_parseObject(&self->attributeParser, port->portAttributes);
            if (result != APX_NO_ERROR)
            {
               apx_port_delete(port);
               setError(self, result);
               return NULL;
            }
         }
         apx_port_setPortId(port, portId);
         adt_ary_push(&self->requirePortList,port);
      }
      else
      {
         setError(self, err);
      }
   }
   return port;
}

apx_port_t *apx_node_createProvidePort(apx_node_t *self, const char* name, const char *dsg, const char *attr, int32_t lineNumber)
{
   apx_port_t *port=0;
   if (self != 0)
   {
      apx_error_t err;
      port = apx_providePort_new(name, dsg, attr, lineNumber, &err);
      if (port != 0)
      {
         apx_portId_t portId = (apx_portId_t )adt_ary_length(&self->providePortList);
         if ( port->portAttributes != 0 )
         {
            apx_error_t result = apx_attributeParser_parseObject(&self->attributeParser, port->portAttributes);
            if (result != APX_NO_ERROR)
            {
               apx_port_delete(port);
               setError(self, result);
               return NULL;
            }
         }
         apx_port_setPortId(port, portId);
         adt_ary_push(&self->providePortList,port);
      }
      else
      {
         setError(self, err);
      }
   }
   return port;
}

/**
 * return 0 on success, -1 on error
 */
apx_error_t apx_node_finalize(apx_node_t *self, int32_t *errorLine)
{
   apx_error_t retval = APX_INVALID_ARGUMENT_ERROR;
   if (self != 0)
   {
      if (self->isFinalized == true)
      {
         retval = APX_NO_ERROR;
      }
      else
      {
         int32_t i;
         int32_t dataTypeLen;
         int32_t providePortLen;
         int32_t requirePortLen;
         adt_hash_t typeMap;

         retval = APX_NO_ERROR;
         adt_hash_create(&typeMap, NULL);
         dataTypeLen = adt_ary_length(&self->datatypeList);
         for(i=0;i<dataTypeLen;i++)
         {
            apx_datatype_t *datatype = (apx_datatype_t*) adt_ary_value(&self->datatypeList, i);
            if ( (datatype->name != 0) && (strlen(datatype->name)>0) )
            {
               adt_hash_set(&typeMap, datatype->name, datatype);
            }
         }
         providePortLen = adt_ary_length(&self->providePortList);
         requirePortLen = adt_ary_length(&self->requirePortList);
         for(i=0;i<providePortLen;i++)
         {
            apx_error_t result;
            apx_port_t *port = (apx_port_t*) adt_ary_value(&self->providePortList, i);
            result = apx_node_finalizePort(port, &self->datatypeList, &typeMap);
            if (result != APX_NO_ERROR)
            {
               if (errorLine != 0) *errorLine = port->lineNumber;
               retval = result;
               break;
            }
            else
            {
               result = apx_port_calculateProperInitValue(port);
               if (result != APX_NO_ERROR)
               {
                  if (errorLine != 0) *errorLine = port->lineNumber;
                  retval = result;
                  break;
               }
            }
         }
         if (retval == APX_NO_ERROR)
         {
            for(i=0;i<requirePortLen;i++)
            {
               apx_error_t result;
               apx_port_t *port = (apx_port_t*) adt_ary_value(&self->requirePortList, i);
               result = apx_node_finalizePort(port, &self->datatypeList, &typeMap);
               if (result != APX_NO_ERROR)
               {
                  if (errorLine != 0) *errorLine = port->lineNumber;
                  retval = result;
                  break;
               }
               else
               {
                  result = apx_port_calculateProperInitValue(port);
                  if (result != APX_NO_ERROR)
                  {
                     if (errorLine != 0) *errorLine = port->lineNumber;
                     retval = result;
                     break;
                  }
               }
            }
         }
         adt_hash_destroy(&typeMap);
         if (retval == APX_NO_ERROR)
         {
            self->isFinalized = true;
         }
      }
   }
   return retval;
}

apx_port_t *apx_node_getRequirePort(const apx_node_t *self, apx_portId_t portIndex)
{
   if (self != 0)
   {
      return (apx_port_t*) *adt_ary_get((adt_ary_t*)&self->requirePortList,portIndex);
   }
   return (apx_port_t*) 0;
}

apx_port_t *apx_node_getProvidePort(const apx_node_t *self, apx_portId_t portIndex)
{
   if (self != 0)
   {
      return (apx_port_t*) *adt_ary_get((adt_ary_t*) &self->providePortList,portIndex);
   }
   return (apx_port_t*) 0;
}

int32_t apx_node_getNumRequirePorts(apx_node_t *self)
{
   if ( self != 0 )
   {
      return adt_ary_length(&self->requirePortList);
   }
   errno=EINVAL; //only set errno if self==0
   return -1;
}

int32_t apx_node_getNumProvidePorts(apx_node_t *self)
{
   if ( self != 0 )
   {
      return adt_ary_length(&self->providePortList);
   }
   errno=EINVAL; //only set errno if self==0
   return -1;
}

adt_bytearray_t *apx_node_createPortInitData(apx_node_t *self, apx_port_t *port)
{
   if ( (self != 0) && (port != 0) )
   {
      apx_error_t result;
      adt_bytearray_t *initData = adt_bytearray_new(0);
      result = apx_node_fillPortInitData(self, port, initData);
      if (result != APX_NO_ERROR)
      {
         adt_bytearray_delete(initData);
         setError(self, result);
         initData = (adt_bytearray_t*) 0;
      }
      return initData;
   }
   setError(self, APX_INVALID_ARGUMENT_ERROR);
   return (adt_bytearray_t*) 0;
}

apx_error_t apx_node_fillPortInitData(apx_node_t *self, apx_port_t *port, adt_bytearray_t *output)
{
   if ( (self != 0) && (port != 0) )
   {

      apx_dataElement_t *dataElement;

      if (self->isFinalized == false)
      {
         int32_t errorLine;
         apx_node_finalize(self, &errorLine);
      }
      dataElement = port->dataSignature.dataElement;
      if ( (dataElement == 0) || (dataElement->baseType == APX_BASE_TYPE_NONE) || (dataElement->packLen == 0) )
      {
         return APX_VALUE_ERROR;
      }
      if (dataElement->baseType == APX_BASE_TYPE_REF_PTR)
      {
         apx_datatype_t *datatype = dataElement->typeRef.ptr;
         if (datatype->dataSignature->dataElement != 0)
         {

            dataElement = datatype->dataSignature->dataElement;
         }
      }
      assert(dataElement->packLen > 0);
      adt_bytearray_resize(output, dataElement->packLen);
      if (port->portAttributes != 0)
      {
         apx_portAttributes_t *attr = port->portAttributes;
         if (attr->initValue == 0)
         {
            //if no init value is given, set to 0
            uint8_t *buf = adt_bytearray_data(output);
            memset(buf, 0, dataElement->packLen);
            return APX_NO_ERROR;
         }
         else
         {
            uint8_t *pBegin;
            uint8_t *pEnd;
            uint8_t *pResult;
            pBegin = adt_bytearray_data(output);
            pEnd = pBegin + dataElement->packLen;
            pResult = apx_dataElement_pack_dv(dataElement, pBegin, pEnd, attr->initValue);
            if ( (pResult == 0) || (pResult == pBegin) )
            {
               return apx_dataElement_getLastError(dataElement);
            }
            return APX_NO_ERROR;
         }
      }
      else
      {
         //if no init value is given, set to 0
         uint8_t *buf = adt_bytearray_data(output);
         memset(buf, 0, dataElement->packLen);
         return APX_NO_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

int32_t apx_node_calcOutPortDataLen(apx_node_t *self)
{
   if (self != 0)
   {
      int32_t providePortLen;
      int32_t total=0;
      int32_t i;
      providePortLen = apx_node_getNumProvidePorts(self);
      for(i=0;i<providePortLen;i++)
      {
         int32_t outPortDataLen;
         apx_port_t *port = apx_node_getProvidePort(self, i);
         outPortDataLen = apx_port_getPackLen(port);
         if (outPortDataLen < 0)
         {
            return -1;
         }
         total+=outPortDataLen;
      }
      return total;
   }
   return -1;
}

int32_t apx_node_calcInPortDataLen(apx_node_t *self)
{
   if (self != 0)
   {
      int32_t providePortLen;
      int32_t total=0;
      int32_t i;
      providePortLen = apx_node_getNumRequirePorts(self);
      for(i=0;i<providePortLen;i++)
      {
         int32_t outPortDataLen;
         apx_port_t *port = apx_node_getRequirePort(self, i);
         outPortDataLen = apx_port_getPackLen(port);
         if (outPortDataLen < 0)
         {
            return -1;
         }
         total+=outPortDataLen;
      }
      return total;
   }
   return -1;
}

apx_error_t apx_node_getLastError(apx_node_t *self)
{
   if (self != 0)
   {
      return self->lastError;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

adt_ary_t *apx_node_getRequirePortList(const apx_node_t *self)
{
   if (self != 0)
   {
      return (adt_ary_t*) &self->requirePortList;
   }
   return (adt_ary_t*) 0;
}

adt_ary_t *apx_node_getProvidePortList(const apx_node_t *self)
{
   if (self != 0)
   {
      return (adt_ary_t*) &self->providePortList;
   }
   return (adt_ary_t*) 0;
}


/***************** Private Function Definitions *******************/

static apx_error_t apx_node_finalizePort(apx_port_t *port, adt_ary_t *typeList, adt_hash_t *typeMap)
{
   apx_error_t result;
   result = apx_port_resolveTypes(port, typeList, typeMap);
   if (result == APX_NO_ERROR)
   {
      result = apx_port_updateDerivedPortSignature(port);
      if (result == APX_NO_ERROR)
      {
         result = apx_port_updatePackLen(port);
      }
   }
   return result;
}

static void setError(apx_node_t *self, apx_error_t errorCode)
{
  self->lastError = errorCode;
}

/*
static void clearError(apx_node_t *self)
{
   self->lastError = APX_NO_ERROR;
}
*/
