/*****************************************************************************
* \file      integration_template.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX embedded integration template
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
/**
 * This is a template header for apx_es_integration.h needed for compiling apx_es.
 */

#ifndef APX_ES_INTEGRATION_TEMPLATE_H
#define APX_ES_INTEGRATION_TEMPLATE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

void apx_es_nodeData_lock(void); //If needed, this can be converted into a function-like macro
void apx_es_nodeData_unlock(void); //If needed, this can be converted into a function-like macro

#endif //APX_ES_INTEGRATION_TEMPLATE_H

