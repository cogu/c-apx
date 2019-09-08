/*****************************************************************************
* \file      apx_vm.c
* \author    Conny Gustafsson
* \date      2019-02-24
* \brief     APX virtual machine (implements v2 of APX byte code language)
*
* Copyright (c) 2019 Conny Gustafsson
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_vm.h"
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
void apx_vm_create(apx_vm_t *self)
{
   if (self != 0)
   {
      self->portDataAttr = (apx_portDataAttributes_t*) 0;
      self->data = (uint8_t *) 0;
      self->dataLen =  0u;
      self->dataOffset = 0u;
      self->progBegin = (uint8_t*) 0;
      self->progEnd = (uint8_t*) 0;
      self->progNext = (uint8_t*) 0;
   }
}

void apx_vm_destroy(apx_vm_t *self)
{
   //Nothing to do
}

apx_vm_t* apx_vm_new(void)
{
   apx_vm_t *self = (apx_vm_t*) malloc(sizeof(apx_vm_t));
   if (self != 0)
   {
      apx_vm_create(self);
   }
   return self;
}

void apx_vm_delete(apx_vm_t *self)
{
   if (self != 0)
   {
      apx_vm_destroy(self);
      free(self);
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


