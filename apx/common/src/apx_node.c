#include <errno.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "bscan.h"
#include "apx_node.h"
#include "apx_nodeInfo.h"
#include "apx_logging.h"
#include "pack.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

#ifdef _MSC_VER
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif

#define ERROR_STR_MAX 128
/**************** Private Function Declarations *******************/
static int apx_node_getDatatypeId(apx_port_t *port);
static const char *apx_node_resolveDataSignature(apx_node_t *self,apx_port_t *port);
static void apx_parser_attributeParseError(apx_port_t *port, int32_t lastError);

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
      apx_attributeParser_create(&self->attributeParser);
      apx_node_setName(self,name);
      self->lastPortError=0;
      self->lastPortId=-1;
      self->lastPortType=-1;
      self->nodeInfo=(apx_nodeInfo_t*) 0;
      self->isFinalized = false;
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
        if ( port->portAttributes != 0 )
        {
           bool result = apx_attributeParser_parseObject(&self->attributeParser, port->portAttributes);
           if (result == false)
           {
              int32_t lastError;
              lastError = apx_attributeParser_getLastError(&self->attributeParser, 0);
              apx_parser_attributeParseError(port, lastError);
              apx_port_delete(port);
              return 0;
           }
        }
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
        if ( port->portAttributes != 0 )
        {
           bool result = apx_attributeParser_parseObject(&self->attributeParser, port->portAttributes);
           if (result == false)
           {
              int32_t lastError;
              lastError = apx_attributeParser_getLastError(&self->attributeParser, 0);
              apx_parser_attributeParseError(port, lastError);
              apx_port_delete(port);
              return 0;
           }
        }
        apx_port_setPortIndex(port,portIndex);
        adt_ary_push(&self->providePortList,port);
     }
   }
   return port;
}

/**
 * return 0 on success, -1 on error
 */
int8_t apx_node_finalize(apx_node_t *self)
{
   if (self != 0)
   {
      int32_t i;
      int32_t providePortLen;
      int32_t requirePortLen;
      if (self->isFinalized == true)
      {
         return 0;
      }
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
      self->isFinalized = true;
      return 0;
   }
   errno = EINVAL;
   return -1;
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
   if ( (self != 0) && (port != 0) && (port->portAttributes != 0) )
   {
      adt_bytearray_t *initData = (adt_bytearray_t*) 0;
      apx_portAttributes_t *attr = port->portAttributes;
      apx_dataElement_t *dataElement = port->derivedDsg.dataElement;

      if ( (attr->initValue != 0) && ( dataElement->baseType != APX_BASE_TYPE_NONE) && (dataElement->packLen > 0) )
      {
         uint8_t *pEnd;
         uint8_t *pNext;
         dtl_dv_type_id dv_type;
         dtl_sv_t *sv = 0;
         dtl_av_t *av = 0;
         initData = adt_bytearray_new(0);
         adt_bytearray_resize(initData, dataElement->packLen);
         pNext = adt_bytearray_data(initData);
         pEnd = pNext + dataElement->packLen;
         dv_type = dtl_dv_type(attr->initValue);
         if (dv_type == DTL_DV_SCALAR)
         {
            sv = (dtl_sv_t*) attr->initValue;
         }
         else if (dv_type == DTL_DV_ARRAY)
         {
            av = (dtl_av_t*) attr->initValue;
         }
         else
         {
            assert(0);
         }
         switch(dataElement->baseType)
         {
         case APX_BASE_TYPE_NONE:
            break;
         case APX_BASE_TYPE_UINT8:
            if (sv != 0)
            {
               packU8(pNext, (uint8_t) dtl_sv_get_u32(sv));
            }
            else
            {
               packU8(pNext, 0);
            }
            break;
         case APX_BASE_TYPE_UINT16:
            if (sv != 0)
            {
               packU16LE(pNext, (uint16_t) dtl_sv_get_u32(sv));
            }
            else
            {
               packU16LE(pNext, 0);
            }
            break;
         case APX_BASE_TYPE_UINT32:
            if (sv != 0)
            {
               packU32LE(pNext, dtl_sv_get_u32(sv));
            }
            else
            {
               packU32LE(pNext, 0);
            }
            break;
         case APX_BASE_TYPE_UINT64:
            break;
         case APX_BASE_TYPE_SINT8:
            if (sv != 0)
            {
               packU8(pNext, (uint8_t) dtl_sv_get_i32(sv));
            }
            else
            {
               packU8(pNext, 0);
            }
            break;
         case APX_BASE_TYPE_SINT16:
            if (sv != 0)
            {
               packU16LE(pNext, (uint16_t) dtl_sv_get_i32(sv));
            }
            else
            {
               packU16LE(pNext, 0);
            }
            break;
         case APX_BASE_TYPE_SINT32:
            if (sv != 0)
            {
               packU32LE(pNext, (uint32_t) dtl_sv_get_i32(sv));
            }
            else
            {
               packU32LE(pNext, 0);
            }
            break;
         case APX_BASE_TYPE_SINT64:
            break;
         case APX_BASE_TYPE_STRING:
            break;
         case APX_BASE_TYPE_RECORD:
            break;
         }
         return initData;
      }
   }
   errno = EINVAL;
   return 0;
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


static void apx_parser_attributeParseError(apx_port_t *port, int32_t lastError)
{
   char errorStr[ERROR_STR_MAX+1];
   uint32_t remain = ERROR_STR_MAX;
   uint32_t errorStrLen=0;
   uint32_t attrLen = strlen(port->portAttributes->rawValue);
   switch(lastError)
   {
   case APX_PARSE_ERROR:
      errorStrLen = sprintf(errorStr, "Failed to parse port attribute string: ");
      break;
   default:
      return;
   }
   remain -= errorStrLen;
   if (remain >= attrLen)
   {
      strcpy(&errorStr[errorStrLen], port->portAttributes->rawValue);
   }
   else
   {
      //truncate the port attribute string adding "..." at the end
      uint32_t bytesToCopy = remain-3;
      strncpy(&errorStr[errorStrLen], port->portAttributes->rawValue, bytesToCopy);
      strcpy(&errorStr[errorStrLen+bytesToCopy], "...");
   }
   APX_LOG_ERROR("%s", errorStr);
}
