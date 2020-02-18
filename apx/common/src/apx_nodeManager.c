/*****************************************************************************
* \file      apx_nodeManager.c
* \author    Conny Gustafsson
* \date      2019-12-29
* \brief     Manager for apx_nodeInstance objects
*
* Copyright (c) 2019-2020 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <malloc.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include "apx_nodeManager.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

/**
 * When useWeakRef is true this node manager only accepts attaching nodes managed by another object.
 * When useWeakRef is false the node manager can create and manage nodes by itself
 */
void apx_nodeManager_create(apx_nodeManager_t *self, apx_mode_t mode, bool useWeakRef)
{
   if ( (self != 0) && ( (mode == APX_CLIENT_MODE) || (mode == APX_SERVER_MODE) ) )
   {
      void (*instanceMapDestructor)(void*) = useWeakRef? 0 : apx_nodeInstance_vdelete;
      apx_istream_handler_t apx_istream_handler;
      memset(&apx_istream_handler,0,sizeof(apx_istream_handler));
      apx_istream_handler.arg = &self->parser;
      apx_istream_handler.open = apx_parser_vopen;
      apx_istream_handler.close = apx_parser_vclose;
      apx_istream_handler.header = apx_parser_vheader;
      apx_istream_handler.node = apx_parser_vnode;
      apx_istream_handler.datatype = apx_parser_vdatatype;
      apx_istream_handler.provide = apx_parser_vprovide;
      apx_istream_handler.require = apx_parser_vrequire;
      apx_istream_handler.node_end = apx_parser_vnode_end;
      apx_istream_handler.parse_error = apx_parser_vparse_error;
      apx_istream_create(&self->apx_istream, &apx_istream_handler);
      apx_parser_create(&self->parser);
      self->lastAttached = (apx_nodeInstance_t*) 0;
      self->mode = mode;
      adt_hash_create(&self->nodeInstanceMap, instanceMapDestructor);
      SPINLOCK_INIT(self->lock);
   }
}

void apx_nodeManager_destroy(apx_nodeManager_t *self)
{
   if (self != 0)
   {
      apx_parser_destroy(&self->parser);
      apx_istream_destroy(&self->apx_istream);
      adt_hash_destroy(&self->nodeInstanceMap);
      SPINLOCK_INIT(self->lock);
   }
}

apx_nodeManager_t *apx_nodeManager_new(apx_mode_t mode, bool useWeakRef)
{
   apx_nodeManager_t *self = (apx_nodeManager_t*) malloc(sizeof(apx_nodeManager_t));
   if (self != 0)
   {
      apx_nodeManager_create(self, mode, useWeakRef);
   }
   return self;
}

void apx_nodeManager_delete(apx_nodeManager_t *self)
{
   if (self != 0)
   {
      apx_nodeManager_destroy(self);
      free(self);
   }
}

apx_nodeInstance_t *apx_nodeManager_createNode(apx_nodeManager_t *self, const char *nodeName)
{
   if (self != 0)
   {
      ///TODO: only accept if the nodeManager is not in weakRef mode
      apx_nodeInstance_t *nodeInstance = apx_nodeInstance_new(self->mode);
      if (nodeInstance != 0)
      {
         adt_hash_set(&self->nodeInstanceMap, nodeName, (void*) nodeInstance);
         self->lastAttached = nodeInstance;
      }
      return nodeInstance;
   }
   return (apx_nodeInstance_t*) 0;
}

apx_error_t apx_nodeManager_parseDefinition(apx_nodeManager_t *self, apx_nodeInstance_t *nodeInstance)
{
   if ( (self != 0) && (nodeInstance != 0) )
   {
      return apx_nodeInstance_parseDefinition(nodeInstance, &self->parser);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/********** Utility functions  ************/
apx_nodeInstance_t *apx_nodeManager_find(apx_nodeManager_t *self, const char *name)
{
   if ( (self != 0 ) && (name != 0) )
   {
      void **result = adt_hash_get(&self->nodeInstanceMap, name);
      if (result != 0)
      {
         return (apx_nodeInstance_t*) *result;
      }
   }
   return (apx_nodeInstance_t*) 0;
}

int32_t apx_nodeManager_length(apx_nodeManager_t *self)
{
   if (self != 0)
   {
      int32_t retval;
      SPINLOCK_ENTER(self->lock);
      retval = adt_hash_length(&self->nodeInstanceMap);
      SPINLOCK_LEAVE(self->lock);
      return retval;
   }
   return -1;
}

int32_t apx_nodeManager_keys(apx_nodeManager_t *self, adt_ary_t* array)
{
   if (self != 0)
   {
      int32_t retval;
      SPINLOCK_ENTER(self->lock);
      retval = adt_hash_keys(&self->nodeInstanceMap, array);
      SPINLOCK_LEAVE(self->lock);
      return retval;
   }
   return -1;
}

int32_t apx_nodeManager_values(apx_nodeManager_t *self, adt_ary_t* array)
{
   if (self != 0)
   {
      int32_t retval;
      SPINLOCK_ENTER(self->lock);
      retval = adt_hash_values(&self->nodeInstanceMap, array);
      SPINLOCK_LEAVE(self->lock);
      return retval;
   }
   return -1;
}

apx_nodeInstance_t *apx_nodeManager_getLastAttached(apx_nodeManager_t *self)
{
   if (self != 0)
   {
      return self->lastAttached;
   }
   return (apx_nodeInstance_t*) 0;
}

apx_error_t apx_nodeManager_buildNode_cstr(apx_nodeManager_t *self, const char *definition_text)
{
   if (self != 0 && definition_text != 0)
   {
      ///TODO: only accept if the nodeManager is not in weakRef mode
      apx_size_t definition_len = (apx_size_t) strlen(definition_text);
      if (definition_len > 0u)
      {
         apx_nodeInstance_t *nodeInstance = apx_nodeInstance_new(self->mode);
         if (nodeInstance != 0)
         {
            apx_programType_t errProgramType;
            apx_uniquePortId_t errPortId;
            apx_error_t rc;
            apx_nodeData_t *nodeData = apx_nodeInstance_getNodeData(nodeInstance);

            rc = apx_nodeData_createDefinitionBuffer(nodeData, definition_len );
            if (rc != APX_NO_ERROR)
            {
               apx_nodeInstance_delete(nodeInstance);
               return rc;
            }
            rc = apx_nodeData_writeDefinitionData(nodeData, (const uint8_t*) definition_text, 0u, definition_len);
            if (rc != APX_NO_ERROR)
            {
               apx_nodeInstance_delete(nodeInstance);
               return rc;
            }
            rc = apx_nodeInstance_parseDefinition(nodeInstance, &self->parser);
            if (rc != APX_NO_ERROR)
            {
               apx_nodeInstance_delete(nodeInstance);
               return rc;
            }
            rc = apx_nodeInstance_buildNodeInfo(nodeInstance, &errProgramType, &errPortId);
            if (rc != APX_NO_ERROR)
            {
               apx_nodeInstance_delete(nodeInstance);
               return rc;
            }
            rc = apx_nodeManager_attachNode(self, nodeInstance);
            if (rc != APX_NO_ERROR)
            {
               apx_nodeInstance_delete(nodeInstance);
               return APX_NAME_MISSING_ERROR;
            }
            apx_nodeInstance_cleanParseTree(nodeInstance);
            rc = apx_nodeInstance_createPortDataBuffers(nodeInstance);
            if (rc != APX_NO_ERROR)
            {
               apx_nodeInstance_delete(nodeInstance);
               return rc;
            }
            if (self->mode == APX_SERVER_MODE)
            {
               rc = apx_nodeInstance_buildPortRefs(nodeInstance);
               if (rc != APX_NO_ERROR)
               {
                  apx_nodeInstance_delete(nodeInstance);
                  return rc;
               }
            }
            return APX_NO_ERROR;
         }
      }
      return APX_LENGTH_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeManager_attachNode(apx_nodeManager_t *self, apx_nodeInstance_t *nodeInstance) //Used when useWeakRef: tru
{
   if ( (self != 0) && (nodeInstance != 0) )
   {
      const char *nodeName = apx_nodeInstance_getName(nodeInstance);
      if (nodeName != 0)
      {
         adt_hash_set(&self->nodeInstanceMap, nodeName, (void*) nodeInstance);
         self->lastAttached = nodeInstance;
         return APX_NO_ERROR;
      }
      return APX_NAME_MISSING_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


