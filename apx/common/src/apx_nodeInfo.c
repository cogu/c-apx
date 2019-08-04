/*****************************************************************************
* \file      apx_nodeInfo.c
* \author    Conny Gustafsson
* \date      2019-01-04
* \brief     A simple data container
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
#include <malloc.h>
#include <string.h>
#include "apx_nodeInfo.h"
#include "apx_parser.h"
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
void apx_nodeInfo_create(apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      self->node = (apx_node_t*) 0;
      self->text = (char*) 0;
      self->textLen = 0u;
   }
}

void apx_nodeInfo_destroy(apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      if (self->node != 0)
      {
         apx_node_delete(self->node);
      }
      if(self->text != 0)
      {
         free(self->text);
      }
   }
}

apx_nodeInfo_t* apx_nodeInfo_new(void)
{
   apx_nodeInfo_t *self = (apx_nodeInfo_t*) malloc(sizeof(apx_nodeInfo_t));
   if (self != 0)
   {
      apx_nodeInfo_create(self);
   }
   return self;
}

void apx_nodeInfo_delete(apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      apx_nodeInfo_destroy(self);
      free(self);
   }
}

void apx_nodeInfo_vdelete(void *arg)
{
   apx_nodeInfo_delete((apx_nodeInfo_t*) arg);
}

apx_error_t apx_nodeInfo_updateFromString(apx_nodeInfo_t *self, struct apx_parser_tag *parser, const char *apx_text)
{
   if ( (self != 0) && (parser != 0))
   {
      apx_nodeInfo_destroy(self);
      apx_nodeInfo_create(self);
      self->node = apx_parser_parseString(parser, apx_text);
      if (self->node != 0)
      {
         apx_parser_clearNodes(parser);
         self->textLen = (uint32_t) strlen(apx_text);
         self->text = (char*) malloc(self->textLen+1);
         if (self->text != 0)
         {
            memcpy(self->text, apx_text, self->textLen+1);
         }
         else
         {
            return APX_MEM_ERROR;
         }
      }
      else
      {
         return apx_parser_getLastError(parser);
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


