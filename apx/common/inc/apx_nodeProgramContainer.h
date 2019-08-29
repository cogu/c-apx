/*****************************************************************************
* \file      apx_nodeProgramContainer.h
* \author    Conny Gustafsson
* \date      2019-08-27
* \brief     Container for compiled byte code programs
*
* Copyright (c) 2019 Conny Gustafsson
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
#ifndef APX_NODE_PROGRAM_CONTAINER_H
#define APX_NODE_PROGRAM_CONTAINER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_node.h"
#include "apx_error.h"
#include "apx_types.h"
#include "adt_bytes.h"


//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_nodeProgramContainer_tag
{
   adt_bytes_t **requirePortUnpackPrograms;
   adt_bytes_t **providePortUnpackPrograms;
   adt_bytes_t **requirePortPackPrograms;
   adt_bytes_t **providePortPackPrograms;
   int32_t numRequirePorts;
   int32_t numProvidePorts;
}apx_nodeProgramContainer_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_nodeProgramContainer_create(apx_nodeProgramContainer_t *self);
void apx_nodeProgramContainer_destroy(apx_nodeProgramContainer_t *self);
apx_nodeProgramContainer_t* apx_nodeProgramContainer_new(void);
void apx_nodeProgramContainer_delete(apx_nodeProgramContainer_t *self);
apx_error_t apx_nodeProgramContainer_compilePackPrograms(apx_nodeProgramContainer_t *self, apx_node_t *node);
apx_error_t apx_nodeProgramContainer_compileUnpackPrograms(apx_nodeProgramContainer_t *self, apx_node_t *node);
adt_bytes_t* apx_nodeProgramContainer_getRequirePortPackProgram(apx_nodeProgramContainer_t *self, apx_portId_t portId);



#endif //APX_NODE_PROGRAM_CONTAINER_H
