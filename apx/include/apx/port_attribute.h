/*****************************************************************************
* \file      port_attribute.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     Parse tree: APX port attributes
*
* Copyright (c) 2017-2020 Conny Gustafsson
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
#ifndef APX_PORT_ATTRIBUTES_H
#define APX_PORT_ATTRIBUTES_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "dtl_type.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_portAttributes_tag
{
   bool is_parameter;
   uint32_t queue_length;
   dtl_dv_t *init_value;
} apx_portAttributes_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
void apx_portAttributes_create(apx_portAttributes_t *self);
void apx_portAttributes_destroy(apx_portAttributes_t *self);
apx_portAttributes_t* apx_portAttributes_new(void);
void apx_portAttributes_delete(apx_portAttributes_t *self);
void apx_portAttributes_vdelete(void *arg);
void apx_portAttributes_set_parameter(apx_portAttributes_t* self);
bool apx_portAttributes_is_parameter(apx_portAttributes_t* self);
bool apx_portAttributes_is_queued(apx_portAttributes_t* self);
void apx_portAttributes_set_queue_length(apx_portAttributes_t* self, uint32_t queue_length);
uint32_t apx_portAttributes_get_queue_length(apx_portAttributes_t* self);
bool apx_portAttributes_has_init_value(apx_portAttributes_t* self);
dtl_dv_t* apx_portAttributes_get_init_value(apx_portAttributes_t* self);
void apx_portAttributes_set_init_value(apx_portAttributes_t* self, dtl_dv_t* init_value);



//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////



#endif //APX_PORT_ATTRIBUTES_H
