/**
 * abstract base class for all APX transmit handlers used by apx_fileManager. These needs to be adapted to fit each protocol (TCP, SPI, etc.)
 */
#ifndef APX_TRANSMIT_HANDLER_H
#define APX_TRANSMIT_HANDLER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

typedef struct apx_transmitHandler_tag
{
   void *arg; //user argument


   //Old API (to be deleted)
   int32_t (*getSendAvail)(void *arg); //this is used to query the transmitHandler how many bytes that can be provided by getSendBuffer
   uint8_t* (*getSendBuffer)(void *arg, int32_t msgLen); //transmitHandler shall attempt to allocate a buffer of appropriate length
   int32_t (*send)(void *arg, int32_t offset, int32_t msgLen); //buffer is provided by transmit handler

   //New API
   uint8_t* (*getMsgBuffer)(void *arg, int32_t *maxMsgLen, int32_t *sendAvail); //Returns a pointer to a message buffer, maxMsgLen is the maximum allowed message length, sendAvail is the number of bytes free in the underlying send buffer
   int32_t (*sendMsg)(void *arg, int32_t offset, int32_t msgLen); //Sends one message. Returns number of bytes consumed from underlying send buffer. MsgBuffer is free to use again after this call.
} apx_transmitHandler_t;
//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////




#endif //APX_TRANSMIT_HANDLER_H
