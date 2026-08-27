/*****************************************************************************
* \file      command.c
* \author    Conny Gustafsson
* \date      2021-01-21
* \brief     Command data structure
*
* Copyright (c) 2021 Conny Gustafsson
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
#include <string.h>
#include "apx/command.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_build_command_with_ptr(apx_command_t* self, apx_cmdType_t cmd_type, uint32_t d1, uint32_t d2, void* d3, void* d4)
{
   if (self != NULL)
   {
      self->cmd_type = cmd_type;
      self->data1 = d1;
      self->data2 = d2;
      self->data3.ptr = d3;
      self->data4 = d4;
   }
}

void apx_build_command_with_data(apx_command_t* self, apx_cmdType_t cmd_type, uint32_t d1, uint32_t d2, uint8_t const* d3, void* d4)
{
   if (self != NULL)
   {
      self->cmd_type = cmd_type;
      self->data1 = d1;
      self->data2 = d2;
      memcpy(&self->data3.data[0], d3, APX_SMALL_DATA_SIZE);
      self->data4 = d4;
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
