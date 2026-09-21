/*****************************************************************************
* \file      port_attribute.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     Parse tree: APX port attributes
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
typedef struct apx_port_attributes_tag
{
   bool is_parameter;
   uint32_t queue_length;
   dtl_dv_t *init_value;
} apx_port_attributes_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
void apx_portAttributes_create(apx_port_attributes_t *self);
void apx_portAttributes_destroy(apx_port_attributes_t *self);
apx_port_attributes_t* apx_portAttributes_new(void);
void apx_portAttributes_delete(apx_port_attributes_t *self);
void apx_portAttributes_vdelete(void *arg);
void apx_portAttributes_set_parameter(apx_port_attributes_t* self);
bool apx_portAttributes_is_parameter(apx_port_attributes_t* self);
bool apx_portAttributes_is_queued(apx_port_attributes_t* self);
void apx_portAttributes_set_queue_length(apx_port_attributes_t* self, uint32_t queue_length);
uint32_t apx_portAttributes_get_queue_length(apx_port_attributes_t* self);
bool apx_portAttributes_has_init_value(apx_port_attributes_t* self);
dtl_dv_t* apx_portAttributes_get_init_value(apx_port_attributes_t* self);
void apx_portAttributes_set_init_value(apx_port_attributes_t* self, dtl_dv_t* init_value);



//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////



#endif //APX_PORT_ATTRIBUTES_H
