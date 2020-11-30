/*****************************************************************************
* \file      byte_port_map.h
* \author    Conny Gustafsson
* \date      2018-10-09
* \brief     Byte offset to port id map
*
* Copyright (c) 2018-2020 Conny Gustafsson
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
#ifndef APX_BYTE_PORT_MAP_H
#define APX_BYTE_PORT_MAP_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/port_data_props.h"
#include "apx/error.h"


//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_bytePortMap_tag
{
   apx_portId_t *mapData;
   int32_t mapLen;
}apx_bytePortMap_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_bytePortMap_create(apx_bytePortMap_t *self, const apx_portDataProps_t *propsArray, apx_portCount_t numPorts);
void apx_bytePortMap_destroy(apx_bytePortMap_t *self);
apx_bytePortMap_t *apx_bytePortMap_new(const apx_portDataProps_t *props, apx_portCount_t numPorts, apx_error_t *errorCode);
void apx_bytePortMap_delete(apx_bytePortMap_t *self);

apx_portId_t apx_bytePortMap_lookup(const apx_bytePortMap_t *self, int32_t offset);
apx_size_t apx_bytePortMap_length(const apx_bytePortMap_t *self);

#endif //APX_BYTE_PORT_MAP_H
