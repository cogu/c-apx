//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "transmit_handler_spy.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_transmitHandlerSpy_create(apx_transmitHandlerSpy_t *self)
{
   if (self != 0)
   {
      self->buf = 0;
      self->transmitted = adt_ary_new(adt_bytearray_vdelete);
   }
}

void apx_transmitHandlerSpy_destroy(apx_transmitHandlerSpy_t *self)
{
   if (self != 0)
   {
      if (self->buf != 0)
      {
         adt_bytearray_delete(self->buf);
      }
      adt_ary_delete(self->transmitted);
   }
}

int32_t apx_transmitHandlerSpy_length(apx_transmitHandlerSpy_t *self)
{
   if (self != 0)
   {
      return adt_ary_length(self->transmitted);
   }
   return -1;
}

adt_bytearray_t *apx_transmitHandlerSpy_next(apx_transmitHandlerSpy_t *self)
{
   if (self != 0)
   {
      return (adt_bytearray_t*) adt_ary_shift(self->transmitted);
   }
   return (adt_bytearray_t*) 0;
}

uint8_t* apx_transmitHandlerSpy_getSendBuffer(void *arg, int32_t msgLen)
{
   apx_transmitHandlerSpy_t* self = (apx_transmitHandlerSpy_t*) arg;
   if (self != 0)
   {
      self->buf = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
      adt_bytearray_resize(self->buf, msgLen);
      return (adt_bytearray_data(self->buf));
   }
   return 0;
}

int32_t apx_transmitHandlerSpy_send(void *arg, int32_t offset, int32_t msgLen)
{
   apx_transmitHandlerSpy_t* self = (apx_transmitHandlerSpy_t*) arg;
   if ( (self != 0) && (adt_bytearray_length(self->buf) >= (uint32_t) msgLen) )
   {
      adt_ary_push(self->transmitted, self->buf);
      self->buf = 0;
      return msgLen;
   }
   return -1;
}



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


