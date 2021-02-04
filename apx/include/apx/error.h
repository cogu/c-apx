#ifndef APX_ERROR_H
#define APX_ERROR_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef int32_t apx_error_t;

#define APX_GENERIC_ERROR             -1
#define APX_NO_ERROR                  0
#define APX_INVALID_ARGUMENT_ERROR    1
#define APX_MEM_ERROR                 2
#define APX_PARSE_ERROR               3
#define APX_DATA_SIGNATURE_ERROR      4
#define APX_PORT_SIGNATURE_ERROR      5
#define APX_INTERNAL_ERROR            6
#define APX_LENGTH_ERROR              7
#define APX_ELEMENT_TYPE_ERROR        8
#define APX_UNSUPPORTED_ERROR         10
#define APX_NOT_IMPLEMENTED_ERROR     11
#define APX_NOT_FOUND_ERROR           12
#define APX_UNMATCHED_BRACE_ERROR     13
#define APX_UNMATCHED_BRACKET_ERROR   14
#define APX_UNMATCHED_STRING_ERROR    15
#define APX_INVALID_TYPE_REF_ERROR    16
#define APX_EXPECTED_BRACKET_ERROR    17
#define APX_INVALID_ATTRIBUTE_ERROR   18
#define APX_TOO_MANY_NODES_ERROR      19
#define APX_NODE_MISSING_ERROR        20
#define APX_NODE_ALREADY_EXISTS_ERROR 21
#define APX_TYPE_ALREADY_EXIST_ERROR  22
#define APX_PORT_ALREADY_EXIST_ERROR  23
#define APX_FILE_ALREADY_EXISTS_ERROR 24
#define APX_MISSING_BUFFER_ERROR      25
#define APX_MISSING_FILE_ERROR        26
#define APX_NAME_MISSING_ERROR        27
#define APX_NAME_TOO_LONG_ERROR       28
#define APX_THREAD_CREATE_ERROR       29
#define APX_THREAD_JOIN_ERROR         30
#define APX_THREAD_JOIN_TIMEOUT_ERROR 31
#define APX_FILE_TOO_LARGE_ERROR      32
#define APX_MSG_TOO_LARGE_ERROR       33
#define APX_CONNECTION_ERROR          34
#define APX_TRANSMIT_ERROR            35
#define APX_NULL_PTR_ERROR            36
#define APX_BUFFER_BOUNDARY_ERROR     37
#define APX_BUFFER_FULL_ERROR         38
#define APX_QUEUE_FULL_ERROR          39
#define APX_DATA_NOT_PROCESSED_ERROR  40
#define APX_PACK_ERROR                41
#define APX_UNPACK_ERROR              42
#define APX_READ_ERROR                43
#define APX_INVALID_MSG_ERROR         44
#define APX_UNEXPECTED_DATA_ERROR     45
#define APX_INVALID_PROGRAM_ERROR     46
#define APX_INVALID_STATE_ERROR       47
#define APX_INVALID_INSTRUCTION_ERROR 48
#define APX_FILE_NOT_FOUND_ERROR      49
#define APX_MISSING_KEY_ERROR           50
#define APX_INVALID_OPEN_HANDLER_ERROR  51
#define APX_INVALID_WRITE_HANDLER_ERROR 52
#define APX_INVALID_WRITE_ERROR         53
#define APX_INVALID_FILE_ERROR          54
#define APX_INIT_VALUE_ERROR            55
#define APX_INVALID_ADDRESS_ERROR       56
#define APX_FILE_NOT_OPEN_ERROR         57
#define APX_BUSY_ERROR                  58
#define APX_DATA_NOT_COMPLETE_ERROR     59
#define APX_NOT_CONNECTED_ERROR         60
#define APX_INVALID_NAME_ERROR          61
#define APX_INVALID_PORT_HANDLE_ERROR   62
#define APX_STRAY_CHARACTERS_AFTER_PARSE_ERROR 63
#define APX_EMPTY_RECORD_ERROR                 64
#define APX_INVALID_HEADER_ERROR               65
#define APX_UNEXPECTED_END_ERROR               66
#define APX_VALUE_TYPE_ERROR                   67
#define APX_VALUE_RANGE_ERROR                  68
#define APX_VALUE_CONVERSION_ERROR             69
#define APX_VALUE_LENGTH_ERROR                 70
#define APX_NUMBER_TOO_LARGE_ERROR             71
#define APX_VERSION_ERROR                      72
#define APX_TOO_MANY_PORTS_ERROR               73
#define APX_FILE_CREATE_ERROR                  74
#define APX_TOO_MANY_REFERENCES_ERROR          75
#define APX_INDEX_ERROR                        76
#define APX_SEMAPHORE_ERROR                    77

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

#endif //APX_ERROR_H
