#ifndef APX_MSG_H
#define APX_MSG_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_msg_tag
{
   uint32_t msgType;
   uint32_t msgData1; //generic uint32 value
   uint32_t msgData2; //generic uint32 value
   void *msgData3;    //generic void* pointer value
#ifndef APX_EMBEDDED
   void *msgData4;    //generic void* pointer value
#endif
} apx_msg_t;


#define RMF_MSG_SIZE ((uint32_t) sizeof(apx_msg_t))
                                           //data used in apx_msg_t
#define APX_MSG_EXIT                       0
#define APX_MSG_SEND_ACKNOWLEDGE           1 //no extra info
#define APX_MSG_SEND_FILEINFO              2 //msgData3=apx_file_t *file
#define APX_MSG_SEND_FILE_OPEN             3 //msgData1=startAddress
#define APX_MSG_SEND_FILE_CLOSE            4 //msgData1=startAddress
#define APX_MSG_READ_FILE                  5 //msgData1=offset, msgData2=length, msgData3=apx_file_t *file
#define APX_MSG_WRITE_FILE                 6 //msgData1=offset, msgData3=apx_file_t *file, msgData4=data
#define APX_MSG_ERROR_CODE                 7 //msgData1=errorCode
#define APX_MSG_ERROR_INVALID_CMD          8 //msgData1=commandId
#define APX_MSG_ERROR_INVALID_WRITE        9 //msgData1=address, msgData2=length
#define APX_MSG_ERROR_INVALID_READ_HANDLER 10 //msgData1=address
#define APX_MSG_ERROR_FILE_ALREADY_EXISTS  11 //msgData1=address, msgData3=char* (name of file)







//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

#endif //APX_MSG_H
