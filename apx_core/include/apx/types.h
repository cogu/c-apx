/*****************************************************************************
* \file      types.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     Shared data types and definitions
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_TYPES_H
#define APX_TYPES_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#if APX_DEBUG_ENABLE
#include <stdio.h>
#endif
#include "apx/cfg.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
struct adt_str_tag;

typedef void (apx_void_ptr_func_t)(void *arg);
typedef struct adt_str_tag* (apx_to_string_func_t)(void* arg);

typedef int32_t apx_offset_t;
typedef uint32_t apx_size_t; //use uint16_t  to send up to 64KB, use uint32_t for 4GB.
typedef APX_PORT_ID_TYPE apx_port_id_t; //uint32_t is default. Use uint16_t for smaller memory footprint
typedef uint32_t apx_unique_port_id_t; //highest significant bit is 0 when it contains a require port ID and 1 when it contains a provide port ID
typedef uint32_t apx_type_id_t;
typedef uint32_t apx_computation_list_id_t;
typedef uint32_t apx_element_id_t;
typedef uint32_t apx_connection_id_t;

#define MAX_TYPE_REF_FOLLOW_COUNT 255u

#define APX_INVALID_PORT_ID ((apx_port_id_t) 0xFFFFFFFFu)
#define APX_INVALID_TYPE_ID ((apx_type_id_t) 0xFFFFFFFFu)
#define APX_INVALID_COMPUTATION_LIST_ID ((apx_computation_list_id_t) 0xFFFFFFFFu)
#define APX_INVALID_ELEMENT_ID ((apx_element_id_t) 0xFFFFFFFFu)

#define APX_ADDRESS_MASK_INTERNAL ((uint32_t) 0x7FFFFFFF)
#define APX_INVALID_ADDRESS ((uint32_t) 0x7FFFFFFF)

typedef APX_PORT_COUNT_TYPE apx_port_count_t;

typedef uint16_t apx_event_id_t;

typedef struct apx_data_write_cmd_tag
{
   apx_offset_t offset;
   apx_size_t len;
} apx_data_write_cmd_t;

#define APX_DATA_WRITE_CMD_SIZE sizeof(apx_data_write_cmd_t)

typedef uint8_t apx_connection_type_t;
#define APX_CONNECTION_TYPE_DEFAULT           0
#define APX_CONNECTION_TYPE_MONITOR           1
#define APX_CONNECTION_TYPE_EVENT             2

typedef uint8_t apx_file_type_t;
#define APX_UNKNOWN_FILE_TYPE             ((apx_file_type_t) 0u)
#define APX_DEFINITION_FILE_TYPE          ((apx_file_type_t) 1u) //".apx"
#define APX_PROVIDE_PORT_DATA_FILE_TYPE   ((apx_file_type_t) 2u) //".out"
#define APX_REQUIRE_PORT_DATA_FILE_TYPE   ((apx_file_type_t) 3u) //".in"
#define APX_PROVIDE_PORT_COUNT_FILE_TYPE  ((apx_file_type_t) 4u) //".cout"
#define APX_REQUIRE_PORT_COUNT_FILE_TYPE  ((apx_file_type_t) 5u) //".cin"

//6-63 Reserved for APX future growth
#define APX_USER_DEFINED_FILE_TYPE_BEGIN  ((apx_file_type_t) 64u)

#define APX_PROVIDE_PORT_DATA_EXT ".out"
#define APX_REQUIRE_PORT_DATA_EXT ".in"
#define APX_PROVIDE_PORT_COUNT_EXT ".cout"
#define APX_REQUIRE_PORT_COUNT_EXT ".cin"
#define APX_DEFINITION_FILE_EXT   ".apx"



typedef uint8_t apx_mode_t;
#define APX_NO_MODE              ((apx_mode_t) 0u)
#define APX_CLIENT_MODE          ((apx_mode_t) 1u)
#define APX_SERVER_MODE          ((apx_mode_t) 2u)
#define APX_MONITOR_MODE         ((apx_mode_t) 3u)

typedef uint8_t apx_port_type_t;
#define APX_REQUIRE_PORT ((apx_port_type_t) 0u)
#define APX_PROVIDE_PORT ((apx_port_type_t) 1u)

#define APX_PORT_ID_PROVIDE_PORT 0x80000000 //used inside an uint32_t to carry either a provide port ID and a require port ID.
#define APX_PORT_ID_MASK         0x7FFFFFFF //used to clear the port flag (ready to cast it into an int32_t)

typedef uint8_t apx_type_code_t;
#define APX_TYPE_CODE_NONE     ((apx_type_code_t) 0u)
#define APX_TYPE_CODE_UINT8    ((apx_type_code_t) 1u)   //'C'
#define APX_TYPE_CODE_UINT16   ((apx_type_code_t) 2u)   //'S'
#define APX_TYPE_CODE_UINT32   ((apx_type_code_t) 3u)   //'L'
#define APX_TYPE_CODE_UINT64   ((apx_type_code_t) 4u)   //'Q'
#define APX_TYPE_CODE_INT8     ((apx_type_code_t) 5u)   //'c'
#define APX_TYPE_CODE_INT16    ((apx_type_code_t) 6u)   //'s'
#define APX_TYPE_CODE_INT32    ((apx_type_code_t) 7u)   //'l'
#define APX_TYPE_CODE_INT64    ((apx_type_code_t) 8u)   //'q'
#define APX_TYPE_CODE_CHAR     ((apx_type_code_t) 9u)   //'a' (Latin1-encoded character)
#define APX_TYPE_CODE_CHAR8    ((apx_type_code_t) 10u)  //'A' (UTF-8-encoded characted)
#define APX_TYPE_CODE_CHAR16   ((apx_type_code_t) 11u)  //'u' (UTF-16-encoded characted, reserved for APX IDL v1.4)
#define APX_TYPE_CODE_CHAR32   ((apx_type_code_t) 12u)  //'U' (UTF-16-encoded characted, reserved for APX IDL v1.4)
#define APX_TYPE_CODE_BOOL     ((apx_type_code_t) 13u)  //"b"
#define APX_TYPE_CODE_BYTE     ((apx_type_code_t) 14u)  //"B"
#define APX_TYPE_CODE_RECORD   ((apx_type_code_t) 15u)  //"{}"
#define APX_TYPE_CODE_REF_ID   ((apx_type_code_t) 16u)  //type reference by ID
#define APX_TYPE_CODE_REF_NAME ((apx_type_code_t) 17u)  //type reference by name
#define APX_TYPE_CODE_REF_PTR  ((apx_type_code_t) 18u)  //pointer to type (this is possible only after a port/node has been finalized)

typedef uint8_t apx_token_class_t;
#define APX_TOKEN_CLASS_NONE            ((apx_token_class_t) 0u)
#define APX_TOKEN_CLASS_DATA_ELEMENT    ((apx_token_class_t) 1u)
#define APX_TOKEN_GROUP_DECLARATION     ((apx_token_class_t) 2u) //RESERVED FOR APX IDL v1.4
#define APX_TOKEN_FUNCTION_DECLARATION  ((apx_token_class_t) 3u) //RESERVED FOR APX IDL v1.4

#define UINT8_SIZE   1u
#define CHAR_SIZE    1u
#define CHAR8_SIZE   1u
#define BYTE_SIZE    1u
#define BOOL_SIZE    1u
#define UINT16_SIZE  2u
#define CHAR16_SIZE  2u
#define UINT32_SIZE  4u
#define CHAR32_SIZE  4u
#define UINT64_SIZE  8u
#define INT8_SIZE    1u
#define INT16_SIZE   2u
#define INT32_SIZE   4u
#define INT64_SIZE   8u

typedef uint8_t apx_size_type_t;
#define APX_SIZE_TYPE_NONE   ((apx_size_type_t) 0u)
#define APX_SIZE_TYPE_UINT8  ((apx_size_type_t) 1u)
#define APX_SIZE_TYPE_UINT16 ((apx_size_type_t) 2u)
#define APX_SIZE_TYPE_UINT32 ((apx_size_type_t) 3u)

#ifdef _MSC_VER
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif

#define APX_NODE_DEFAULT_VERSION_MAJOR 1
#define APX_NODE_DEFAULT_VERSION_MINOR 3

typedef uint8_t apx_log_level_t;
#define APX_LOG_LEVEL_CRITICAL 0    //SYSLOG_LEVEL 2 (+2)
#define APX_LOG_LEVEL_ERROR    1    //SYSLOG_LEVEL 3 (+2)
#define APX_LOG_LEVEL_WARNING  2    //SYSLOG_LEVEL 4 (+2)
#define APX_LOG_LEVEL_INFO     3    //SYSLOG_LEVEL 6 (+3)
#define APX_LOG_LEVEL_DEBUG    4    //SYSLOG_LEVEL 7 (+3)
#define APX_MAX_LOG_LEVEL      APX_LOG_LEVEL_DEBUG

typedef uint8_t scalar_storage_type_t;
#define APX_VM_SCALAR_STORAGE_TYPE_NONE   ((scalar_storage_type_t) 0u)
#define APX_VM_SCALAR_STORAGE_TYPE_INT32  ((scalar_storage_type_t) 1u)
#define APX_VM_SCALAR_STORAGE_TYPE_UINT32 ((scalar_storage_type_t) 2u)
#define APX_VM_SCALAR_STORAGE_TYPE_INT64  ((scalar_storage_type_t) 3u)
#define APX_VM_SCALAR_STORAGE_TYPE_UINT64 ((scalar_storage_type_t) 4u)
#define APX_VM_SCALAR_STORAGE_TYPE_BOOL   ((scalar_storage_type_t) 5u)
#define APX_VM_SCALAR_STORAGE_TYPE_CHAR   ((scalar_storage_type_t) 6u)
#define APX_VM_SCALAR_STORAGE_TYPE_BYTE   ((scalar_storage_type_t) 7u)

typedef uint8_t apx_range_check_state_t;
#define APX_RANGE_CHECK_STATE_NOT_CHECKED  ((apx_range_check_state_t) 0u)
#define APX_RANGE_CHECK_STATE_OK           ((apx_range_check_state_t) 1u)
#define APX_RANGE_CHECK_STATE_FAIL         ((apx_range_check_state_t) 2u)

#define APX_MAX_LOG_LEN 1024

#define APX_LOG_LABEL_MAX_LEN 16

#define APX_INVALID_CONNECTION_ID 0xFFFFFFFF

#define APX_PORT_DATA_ADDRESS_START        0x0u
#define APX_PORT_DATA_ADDRESS_ALIGNMENT    0x400u      //1KB
#define APX_DEFINITION_ADDRESS_START       0x4000000u  //64MB
#define APX_DEFINITION_ADDRESS_ALIGNMENT   0x40000u    //256kB
#define APX_PORT_COUNT_ADDRESS_START       0x8000000u  //128MB
#define APX_PORT_COUNT_ADDRESS_ALIGNMENT   0x400u      //1KB
#define APX_USER_DEFINED_ADDRESS_START     0x20000000u //128MB
#define APX_USER_DEFINED_ADDRESS_ALIGNMENT 0x1000u     //4KB

typedef uint8_t apx_data_state_t;
#define APX_DATA_STATE_INIT                           ((apx_data_state_t) 0u)
#define APX_DATA_STATE_WAITING_FOR_FILE_PUBLICATION   ((apx_data_state_t) 1u) //Used in client mode
#define APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST  ((apx_data_state_t) 2u) //Used in server mode
#define APX_DATA_STATE_WAITING_FOR_FILE_DATA          ((apx_data_state_t) 3u) //used in client and server mode
#define APX_DATA_STATE_SYNCHRONIZED                   ((apx_data_state_t) 4u) //Used in client and server mode
#define APX_DATA_STATE_FILE_REVOKED                   ((apx_data_state_t) 5u) //Used in client and server mode
#define APX_DATA_STATE_DISCONNECTED                   ((apx_data_state_t) 6u) //Used in client and server mode


typedef uint8_t apx_file_request_type_t;
#define APX_FILE_REQUEST_TYPE_MAPPING                 ((apx_file_request_type_t) 0u)
#define APX_FILE_REQUEST_TYPE_SNAPSHOT                ((apx_file_request_type_t) 1u)

typedef uint8_t apx_resource_type_t;
#define APX_RESOURCE_TYPE_UNKNOWN ((apx_resource_type_t) 0) //Unknown
#define APX_RESOURCE_TYPE_IPV4    ((apx_resource_type_t) 1) //Seems to be an IPv4 address
#define APX_RESOURCE_TYPE_IPV6    ((apx_resource_type_t) 2) //Seems to be an IPv6 address
#define APX_RESOURCE_TYPE_FILE    ((apx_resource_type_t) 3) //Seems to be a unix file path)
#define APX_RESOURCE_TYPE_NAME    ((apx_resource_type_t) 4) //Seems to be a name
#define APX_RESOURCE_TYPE_ERROR   ((apx_resource_type_t) 5) //An error has occured

typedef uint8_t apx_program_type_t;
#define APX_UNPACK_PROGRAM   ((apx_program_type_t) 0u)
#define APX_PACK_PROGRAM     ((apx_program_type_t) 1u)

typedef uint8_t apx_attribute_parse_type_t;
#define APX_ATTRIBUTE_PARSE_TYPE_NONE             ((apx_attribute_parse_type_t) 0u)
#define APX_ATTRIBUTE_PARSE_TYPE_VALUE_TABLE      ((apx_attribute_parse_type_t) 1u)
#define APX_ATTRIBUTE_PARSE_TYPE_RATIONAL_SCALING ((apx_attribute_parse_type_t) 2u)
#define APX_ATTRIBUTE_PARSE_TYPE_INIT_VALUE       ((apx_attribute_parse_type_t) 3u)
#define APX_ATTRIBUTE_PARSE_TYPE_PARAMETER        ((apx_attribute_parse_type_t) 4u)
#define APX_ATTRIBUTE_PARSE_TYPE_QUEUE_LENGTH     ((apx_attribute_parse_type_t) 5u)

typedef uint8_t apx_argument_type_t;
#define APX_ARGUMENT_TYPE_INVALID         ((apx_argument_type_t) 0u)
#define APX_ARGUMENT_TYPE_INTEGER_LITERAL ((apx_argument_type_t) 1u)
#define APX_ARGUMENT_TYPE_STRING_LITERAL  ((apx_argument_type_t) 2u)

typedef uint8_t apx_computation_type_t;
#define APX_COMPUTATION_TYPE_VALUE_TABLE      ((apx_computation_type_t) 0u)
#define APX_COMPUTATION_TYPE_RATIONAL_SCALING ((apx_computation_type_t) 1u)

#define APX_RATIONAL_ARG_INDEX_0 0
#define APX_RATIONAL_ARG_INDEX_1 1
#define APX_RATIONAL_ARG_INDEX_2 2
#define APX_RATIONAL_ARG_INDEX_3 3
#define APX_RATIONAL_ARG_INDEX_4 4
#define APX_RATIONAL_ARG_INDEX_5 5

typedef uint8_t apx_port_connector_event_t;
#define APX_PORT_CONNECTED_EVENT       ((apx_port_connector_event_t) 0u)
#define APX_PORT_DISCONNECTED_EVENT    ((apx_port_connector_event_t) 1u)

typedef uint8_t apx_connection_state_t;
#define APX_CONNECTION_STATE_CREATED      ((apx_connection_state_t) 0u)
#define APX_CONNECTION_STATE_CONNECTING   ((apx_connection_state_t) 1u)
#define APX_CONNECTION_STATE_ACCEPTED     ((apx_connection_state_t) 2u)
#define APX_CONNECTION_STATE_CLOSED       ((apx_connection_state_t) 3u)


// Shared library visibility (needs more work)

# if __GNUC__ >= 4
   #define DLL_PUBLIC __attribute__ ((visibility ("default")))
   #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
# else
   #define DLL_PUBLIC
   #define DLL_LOCAL
# endif

#endif //APX_TYPES_H
