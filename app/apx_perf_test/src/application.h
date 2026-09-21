/*****************************************************************************
* \file      application.h
* \author    Conny Gustafsson
* \date      2019-10-13
* \brief     Performance test application
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APPLICATION_H
#define APPLICATION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include "apx/types.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct application_cfg_tag
{
    const char* apx_definition;
    const char* server_address;      //Content depends on value of resource_type
    uint16_t tcp_port;               //for tcp connection
    uint32_t timer_init;             //number of seconds to run the test
    apx_resource_type_t resource_type; //How to interpret server_address
} application_cfg_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
bool application_init(const application_cfg_t *cfg);
bool application_run(void);

#endif //APPLICATION_H
