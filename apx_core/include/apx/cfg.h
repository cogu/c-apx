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
# define APX_MAX_PSG_LEN 2048
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

#ifndef APX_SERVER_DEFAULT_MAX_CONCURRENT_CONNECTIONS
# define APX_SERVER_DEFAULT_MAX_CONCURRENT_CONNECTIONS 256u // Default maximum concurrent server connections
#endif
#ifndef APX_SERVER_MAX_CONCURRENT_CONNECTIONS_LIMIT
# define APX_SERVER_MAX_CONCURRENT_CONNECTIONS_LIMIT 65535u // Upper limit for concurrent server connections
#endif
#define APX_SERVER_MAX_CONCURRENT_CONNECTIONS APX_SERVER_DEFAULT_MAX_CONCURRENT_CONNECTIONS

#ifndef APX_MAX_DEFINITION_TRANSFER_MEMORY
# define APX_MAX_DEFINITION_TRANSFER_MEMORY (UINT64_C(128) * UINT64_C(1024) * UINT64_C(1024))
#endif

#ifndef APX_DEFINITION_CODEC_MEMORY_ALLOWANCE
# define APX_DEFINITION_CODEC_MEMORY_ALLOWANCE (UINT64_C(256) * UINT64_C(1024))
#endif

#define APX_SMALL_DATA_SIZE 8u

#endif // APX_CFG_H
