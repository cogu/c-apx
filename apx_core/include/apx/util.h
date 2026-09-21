/*****************************************************************************
* \file      util.h
* \author    Conny Gustafsson
* \date      2020-02-17
* \brief     Various APX-related utility functions
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_UTIL_H
#define APX_UTIL_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdint.h>
#include "apx/types.h"
#include "adt_str.h"
#include "apx/error.h"
#include "adt_error.h"
//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_fprint_hex_bytes(FILE *file, int32_t maxColumns, const uint8_t *dataBuf, apx_size_t dataSize);
#define apx_print_hex_bytes(c, b, s) apx_fprint_hex_bytes(stdout, c, b, s)

apx_resource_type_t apx_parse_resource_name(const char *text, adt_str_t **address, uint16_t *port);
apx_error_t convert_from_adt_to_apx_error(adt_error_t error_code);


#endif //APX_UTIL_H
