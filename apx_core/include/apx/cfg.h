/*****************************************************************************
 * \file      cfg.h
 * \author    Conny Gustafsson
 * \date      2017-02-20
 * \brief     APX configuration definitions
 *
 * Copyright (c) 2017-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_CFG_H
#define APX_CFG_H

#include <stdint.h>

#ifndef APX_MAX_NAME_LEN
# define APX_MAX_NAME_LEN 256
#endif

#ifndef APX_MAX_PSG_LEN
# define APX_MAX_PSG_LEN 1024
#endif

#ifndef APX_MAX_FILE_SIZE
# define APX_MAX_FILE_SIZE 0x1000000 // 16MB
#endif

#ifndef APX_PORT_ID_TYPE
# define APX_PORT_ID_TYPE uint32_t // valid selections are: uint8_t, uint16_t and uint32_t. uint32_t is default
#endif

#define APX_MAX_TAG_STR_SIZE 255

#ifndef APX_PORT_COUNT_TYPE
# define APX_PORT_COUNT_TYPE uint16_t  // Using uint8_t or uint16_t is recommended
# define APX_PORT_COUNT_MAX UINT16_MAX // This define must match limit of selected data type APX_PORT_COUNT_TYPE
#endif

#define APX_SERVER_MAX_CONCURRENT_CONNECTIONS 4000 // maximum number of connections the server will accept

#define APX_SMALL_DATA_SIZE 8u

#endif // APX_CFG_H
