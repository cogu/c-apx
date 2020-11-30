#ifndef TRANSMIT_HANDLER_SPY_H
#define TRANSMIT_HANDLER_SPY_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_bytearray.h"
#include "adt_ary.h"
#include "apx/transmit_handler.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_transmitHandlerSpy_tag
{
   adt_bytearray_t *buf;
   adt_ary_t *transmitted; //strong references to adt_bytearray_t
}apx_transmitHandlerSpy_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_transmitHandlerSpy_create(apx_transmitHandlerSpy_t *self);
void apx_transmitHandlerSpy_destroy(apx_transmitHandlerSpy_t *self);

int32_t apx_transmitHandlerSpy_length(apx_transmitHandlerSpy_t *self);
adt_bytearray_t *apx_transmitHandlerSpy_next(apx_transmitHandlerSpy_t *self);

uint8_t* apx_transmitHandlerSpy_getSendBuffer(void *arg, int32_t msgLen);
int32_t apx_transmitHandlerSpy_send(void *arg, int32_t offset, int32_t msgLen);

#endif //TRANSMIT_HANDLER_SPY_H
