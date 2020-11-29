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
   union msgData3_tag{
      void *ptr;                         //generic pointer value
      uint8_t data[APX_SMALL_DATA_SIZE]; //port data (when port data length is small)
   } msgData3;
   void *msgData4; //generic pointer value
} apx_msg_t;


#define RMF_MSG_SIZE ((uint32_t) sizeof(apx_msg_t))
                                           //data used in apx_msg_t
#define APX_MSG_EXIT                       0
#define APX_MSG_SEND_ACKNOWLEDGE           1 //no extra info
#define APX_MSG_SEND_FILEINFO              2 //msgData3=apx_fileInfo_t *fileInfo
#define APX_MSG_SEND_FILE_OPEN             3 //msgData1=startAddress
#define APX_MSG_SEND_FILE_CLOSE            4 //msgData1=startAddress
#define APX_MSG_SEND_FILE_CONST_DATA       5 //msgData1=address, msgData2=length, msgData3.ptr= apx_file_read_const_data_func
#define APX_MSG_SEND_FILE_DYN_DATA         6 //msgData1=address, msgData2=length, msgData3.ptr=data (allocated through SOA, needs to be freed)
#define APX_MSG_SEND_FILE_DATA_DIRECT      7 //msgData1=address, msgData2=length, msgData3.data=data (buffer memory)
#define APX_MSG_SEND_ERROR_CODE            8 //msgData1=errorCode


/*
Errors:
   RMF_APX_FILE_ALREADY_EXISTS_ERROR: msgData2=fileAddress, msgData3.ptr = STRDUPed file name (needs to be freed)
   RMF_APX_INVALID_READ_HANDLER: msgData2=fileAddress
   RMF_APX_INVALID_WRITE_ERROR: TBD


*/




//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

#endif //APX_MSG_H
