#ifndef APX_MSG_H
#define APX_MSG_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#ifdef APX_EMBEDDED
#include "apx_es_cfg.h"
#else
#include "apx_cfg.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_msg_tag
{
   uint32_t msgType;
   uint32_t msgData1; //generic uint32 value
   uint32_t msgData2; //generic uint32 value
   union msgData3_tag{
      void *ptr;                         //generic void* pointer value
#if APX_SMALL_DATA_SIZE > 0
      uint8_t data[APX_SMALL_DATA_SIZE]; //port data (when port data length is small)
#endif
   } msgData3;
#ifndef APX_EMBEDDED
   void *msgData4;    //generic void* pointer value
#endif
} apx_msg_t;


#define RMF_MSG_SIZE ((uint32_t) sizeof(apx_msg_t))
                                      //data used in apx_msg_t
#define RMF_MSG_EXIT                  0
#define RMF_MSG_CONNECT               1
#define RMF_MSG_DISCONNECT            2
#define RMF_MSG_FILEINFO              3 //msgData1=size, msgData3=apx_file_t *file
#define RMF_MSG_FILE_OPEN             4 //msgData1=file startAddress
#define RMF_MSG_FILE_CLOSE            5 //msgData1=file startAddress
#define RMF_MSG_WRITE_NOTIFY          6 //msgData1=offset, msgData2=length, msgData3.ptr=apx_file_t *file
#define RMF_MSG_FILE_WRITE            7 //msgData1=writeAddress, msgData2=length, msgData3.ptr=apx_file_t *file, msgData4=data
#define RMF_MSG_FILE_SEND             8 //msgData3=apx_file_t *file
#define RMF_MSG_DIRECT_WRITE          9 //msgData1=writeAddress, msgData2=length, msgData3.data=port data


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

#endif //APX_MSG_H
