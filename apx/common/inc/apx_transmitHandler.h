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
   //primary interface (used by embedded devices to send a single message)
   int32_t (*getSendAvail)(void *arg); //this is used to query the transmitHandler how many bytes that can be provided by getSendBuffer
   uint8_t* (*getSendBuffer)(void *arg, int32_t msgLen); //transmitHandler shall attempt to allocate a buffer of appropriate length
   int32_t (*send)(void *arg, int32_t offset, int32_t msgLen); //buffer is provided by transmit handler

   //secondary interface (used when running in Linux/Windows, can send multiple messages in single block)
   uint8_t* (*getSendBufferRaw)(void *arg, int32_t dataLen);
   int32_t (*sendRaw)(void *arg, int32_t dataLen);
} apx_transmitHandler_t;
//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////




#endif //APX_TRANSMIT_HANDLER_H
