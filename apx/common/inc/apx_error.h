#ifndef APX_ERROR_H
#define APX_ERROR_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_GENERIC_ERROR             -1
#define APX_NO_ERROR                  0
#define APX_INVALID_ARGUMENT_ERROR    1
#define APX_MEM_ERROR                 2
#define APX_PARSE_ERROR               3
#define APX_DATA_SIGNATURE_ERROR      4
#define APX_VALUE_ERROR               5
#define APX_LENGTH_ERROR              6
#define APX_ELEMENT_TYPE_ERROR        7
#define APX_DV_TYPE_ERROR             8
#define APX_UNSUPPORTED_ERROR         9
#define APX_NOT_IMPLEMENTED_ERROR     10
#define APX_NOT_FOUND_ERROR           11
#define APX_UNMATCHED_BRACE_ERROR     12
#define APX_UNMATCHED_BRACKET_ERROR   13
#define APX_UNMATCHED_STRING_ERROR    14
#define APX_INVALID_TYPE_REF_ERROR    15
#define APX_EXPECTED_BRACKET_ERROR    16
#define APX_INVALID_ATTRIBUTE_ERROR   17
#define APX_TOO_MANY_NODES_ERROR      18
#define APX_NODE_MISSING_ERROR        19
#define APX_NODE_ALREADY_EXISTS_ERROR 20
#define APX_MISSING_BUFFER_ERROR      21
#define APX_MISSING_FILE_ERROR        22
#define APX_NAME_MISSING_ERROR        23
#define APX_NAME_TOO_LONG_ERROR       24
#define APX_THREAD_CREATE_ERROR       25
#define APX_MSG_TOO_LARGE_ERROR       26
#define APX_CONNECTION_ERROR          27
#define APX_TRANSMIT_ERROR            28
#define APX_NULL_PTR_ERROR            29
#define APX_BUFFER_BOUNDARY_ERROR     30
#define APX_BUFFER_FULL_ERROR         31
#define APX_QUEUE_FULL_ERROR          32
#define APX_DATA_NOT_PROCESSED_ERROR  33
#define APX_PACK_ERROR                34
#define APX_READ_ERROR                35
#define APX_INVALID_MSG_ERROR         36

#define RMF_APX_NO_ERROR                  500
#define RMF_APX_INVALID_ARGUMENT_ERROR    (RMF_APX_NO_ERROR+APX_INVALID_ARGUMENT_ERROR)
#define RMF_APX_MEM_ERROR                 (RMF_APX_NO_ERROR+APX_MEM_ERROR)
#define RMF_APX_PARSE_ERROR               (RMF_APX_NO_ERROR+APX_PARSE_ERROR)
#define RMF_APX_DATA_SIGNATURE_ERROR      (RMF_APX_NO_ERROR+APX_DATA_SIGNATURE_ERROR)
#define RMF_APX_VALUE_ERROR               505
#define RMF_APX_LENGTH_ERROR              506
#define RMF_APX_ELEMENT_TYPE_ERROR        507
#define RMF_APX_DV_TYPE_ERROR             508
#define RMF_APX_UNSUPPORTED_ERROR         509
#define RMF_APX_NOT_IMPLEMENTED_ERROR     510
#define RMF_APX_NOT_FOUND_ERROR           (RMF_APX_NO_ERROR+APX_NOT_FOUND_ERROR)
#define RMF_APX_UNMATCHED_BRACE_ERROR     512
#define RMF_APX_UNMATCHED_BRACKET_ERROR   513
#define RMF_APX_UNMATCHED_STRING_ERROR    514
#define RMF_APX_INVALID_TYPE_REF_ERROR    515
#define RMF_APX_EXPECTED_BRACKET_ERROR    516
#define RMF_APX_INVALID_ATTRIBUTE_ERROR   517
#define RMF_APX_TOO_MANY_NODES_ERROR      518
#define RMF_APX_NODE_MISSING_ERROR        519
#define RMF_APX_NODE_ALREADY_EXISTS_ERROR 520
#define RMF_APX_MISSING_BUFFER_ERROR      521
#define RMF_APX_MISSING_FILE_ERROR        522
#define RMF_APX_NAME_MISSING_ERROR        523
#define RMF_APX_NAME_TOO_LONG_ERROR       524
#define RMF_APX_THREAD_CREATE_ERROR       525
#define RMF_APX_MSG_TOO_LONG_ERROR        526
                                          //527 RESERVED
#define RMF_APX_TRANSMIT_ERROR            528
#define RMF_APX_NULL_PTR_ERROR            529
#define RMF_APX_BUFFER_BOUNDARY_ERROR     (RMF_APX_NO_ERROR+30)
#define RMF_APX_BUFFER_FULL_ERROR         (RMF_APX_NO_ERROR+31)
#define RMF_APX_QUEUE_FULL_ERROR          (RMF_APX_NO_ERROR+32)
#define RMF_APX_DATA_NOT_PROCESSED_ERROR  (RMF_APX_NO_ERROR+33)
#define RMF_APX_PACK_ERROR                (RMF_APX_NO_ERROR+34)
#define RMF_APX_READ_ERROR                (RMF_APX_NO_ERROR+35)
#define RMF_APX_INVALID_MSG_ERROR         (RMF_APX_NO_ERROR+36)



typedef int32_t apx_error_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

#endif //APX_ERROR_H
