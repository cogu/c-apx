/*****************************************************************************
* \file      ringbuf.h
* \author    Conny Gustafsson
* \date      2013-12-19
* \brief     Ringbuffer data structure
*
* Copyright (c) 2013-2016 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
#ifndef RINGBUF_H__
#define RINGBUF_H__
#include <stdint.h>

#include "ringbuf_cfg.h"

#ifndef E_BUF_OK
#define E_BUF_OK        0
#endif
#ifndef E_BUF_NOT_OK
#define E_BUF_NOT_OK    1
#endif
#ifndef E_BUF_OVERFLOW
#define E_BUF_OVERFLOW  2
#endif
#ifndef E_BUF_UNDERFLOW
#define E_BUF_UNDERFLOW 3
#endif


//RBFS: Ringbuffers containing elements of equal size
#if(RBFS_ENABLE)
typedef struct rbfs_tag   //ring buffer (fixed block size) -
{
   uint8_t* u8Buffer;
   uint8_t* u8WritePtr;
   uint8_t* u8ReadPtr;
   uint16_t u16MaxNumElem;
   uint16_t u16NumElem;
   uint16_t u16PeakNumElem; // = 0 unless debug enabled
   uint8_t u8ElemSize;
} rbfs_t;
#endif

//RBFD: Ringbuffers containing elements of dynamic size
#if(RBFD_ENABLE)
typedef struct rbfd_tag   //ring buffer (dynamic block size)
{
   uint8_t* u8Buffer;
   uint8_t* u8WritePtr;
   uint8_t* u8ReadPtr;
   uint16_t u16BufferSize;
   uint16_t u16BytesAvail;
} rbfd_t;
#endif

//RBFU16 Ringbuffers containing elements of uint16_t
#if (RBFU16_ENABLE)
typedef struct rbfu16_t   //ring buffer (fixed block size) -
{
   uint16_t* u16Buffer;
   uint16_t* u16WritePtr;
   uint16_t* u16ReadPtr;
   uint16_t u16MaxNumElem;
   uint16_t u16NumElem;
} rbfu16_t;
#endif

/***************** Public Function Declarations *******************/
#if(RBFS_ENABLE)
void rbfs_create(rbfs_t* rbf, uint8_t* u8Buffer, uint16_t u32NumElem, uint8_t u8ElemSize);
// returns E_BUF_OK or E_BUF_OVERFLOW
uint8_t rbfs_insert(rbfs_t* rbf, const uint8_t* u8Data);
// returns E_BUF_OK or E_BUF_UNDERFLOW
uint8_t rbfs_remove(rbfs_t* rbf, uint8_t* u8Data);
// returns E_BUF_OK if the u8Data existed in rbf else E_BUF_UNDERFLOW.
// Warning: Consider struct elements with padding can not use this function
uint8_t rbfs_exists(const rbfs_t* rbf, const uint8_t* u8Data);
uint8_t rbfs_peek(const rbfs_t* rbf, uint8_t* u8Data);
uint16_t rbfs_size(const rbfs_t* rbf);
uint16_t rbfs_free(const rbfs_t* rbf);
void rbfs_clear(rbfs_t* rbf);
#endif

#if(RBFD_ENABLE)
uint8_t rbfd_create(rbfd_t* rbfd, uint8_t* u8Buffer, uint16_t u16BufferSize);
uint8_t rbfd_insert(rbfd_t* rbfd, uint8_t* u8Data, uint8_t u8Len);
uint8_t rbfd_remove(rbfd_t* rbfd, uint8_t* u8Data, uint8_t u8Len);
uint16_t rbfd_size(rbfd_t* rbfd);
uint8_t rbfd_peekU8(rbfd_t* rbfd, uint8_t* u8Value);
uint8_t rbfd_peekU16(rbfd_t* rbfd, uint16_t* u16Value);
uint8_t rbfd_peekU32(rbfd_t* rbfd, uint32_t* u32Value);
#endif

#if(RBFU16_ENABLE)
uint8_t rbfu16_create(rbfu16_t* rbf, uint16_t* u16Buffer, uint16_t u16NumElem);
uint8_t rbfu16_insert(rbfu16_t* rbf, uint16_t u16Data);
uint8_t rbfu16_remove(rbfu16_t* rbf, uint16_t* u16Data);
uint8_t rbfu16_peek(rbfu16_t* rbf, uint16_t* u16Data);
uint16_t rbfu16_length(rbfu16_t* rbf);
#endif

#endif

