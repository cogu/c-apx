/*****************************************************************************
* \file      parser.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX parser
*
* Copyright (c) 2017-2020 Conny Gustafsson
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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "apx/parser.h"
#include "apx/parser_base.h"
#include "bstr.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void state_init(apx_parse_state_t* self);
static void state_clear(apx_parse_state_t* self);
static apx_error_t state_make_node(apx_parse_state_t* self, uint8_t const* name_begin, uint8_t const* name_end);
static apx_error_t state_make_data_type(apx_parse_state_t* self, uint8_t const* name_begin, uint8_t const* name_end);
static apx_error_t state_make_provide_port(apx_parse_state_t* self, uint8_t const* name_begin, uint8_t const* name_end);
static apx_error_t state_make_require_port(apx_parse_state_t* self, uint8_t const* name_begin, uint8_t const* name_end);
static apx_error_t state_parse_data_signature(apx_parse_state_t* self, apx_signatureParser_t* parser, uint8_t const* begin, uint8_t const* end);
static apx_error_t state_parse_type_attributes(apx_parse_state_t* self, apx_attributeParser_t* parser, uint8_t const* begin, uint8_t const* end);
static apx_error_t state_parse_port_attributes(apx_parse_state_t* self, apx_attributeParser_t* parser, uint8_t const* begin, uint8_t const* end);

static void parser_clear_error(apx_parser_t *self);
static void parser_set_error(apx_parser_t* self, apx_error_t error_code, int32_t error_line);
static void parser_init_handler(apx_parser_t* self, apx_istream_handler_t* handler);
static void parser_reset(apx_parser_t* self);
static apx_error_t parser_on_open(void* arg);
static apx_error_t parser_on_close(void* arg);
static apx_error_t parser_on_new_line(void* arg, const char* line_begin, const char* line_end);
static apx_error_t parser_accept_version_line(apx_parser_t* self, uint8_t const* begin, uint8_t const* end);
static apx_error_t parser_accept_node_declaration(apx_parser_t* self, uint8_t const* begin, uint8_t const* end);
static apx_error_t parser_accept_type_or_port_declaration(apx_parser_t* self, uint8_t const* begin, uint8_t const* end);
static apx_error_t parser_accept_type_declaration(apx_parser_t* self, uint8_t const* begin, uint8_t const* end);
static apx_error_t parser_accept_port_declaration(apx_parser_t* self, uint8_t const* begin, uint8_t const* end);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_parser_create(apx_parser_t *self, apx_istream_t* stream)
{
   if ( (self != NULL) && (stream != NULL) )
   {
      apx_istream_handler_t handler;
      state_init(&self->state);
      parser_clear_error(self);
      parser_init_handler(self, &handler);
      apx_istream_set_handler(stream, &handler);
      self->stream = stream;
      apx_attributeParser_create(&self->attribute_parser);
      apx_signatureParser_create(&self->signature_parser);
   }
}

void apx_parser_destroy(apx_parser_t*self)
{
   if (self != NULL)
   {
      state_clear(&self->state);
      apx_attributeParser_destroy(&self->attribute_parser);
      apx_signatureParser_destroy(&self->signature_parser);
   }
}

apx_parser_t* apx_parser_new(apx_istream_t* stream)
{
   apx_parser_t *self = (apx_parser_t*) malloc(sizeof(apx_parser_t));
   if (self != 0)
   {
      apx_parser_create(self, stream);
   }
   return self;
}

void apx_parser_delete(apx_parser_t*self)
{
   if (self != 0)
   {
      apx_parser_destroy(self);
      free(self);
   }
}

apx_node_t* apx_parser_take_last_node(apx_parser_t* self)
{
   if (self != NULL)
   {
      apx_node_t* retval = self->state.node;
      self->state.node = NULL;
      return retval;
   }
   return (apx_node_t*) NULL;
}

apx_error_t apx_parser_get_last_error(apx_parser_t* self)
{
   if (self != NULL)
   {
      return self->last_error;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

int32_t apx_parser_get_error_line(apx_parser_t* self)
{
   if (self != NULL)
   {
      return self->last_error_line;
   }
   return -1;
}

apx_error_t apx_parser_parse_cstr(apx_parser_t* self, const char* apx_text)
{
   if ((self != NULL) && (apx_text != NULL))
   {
      size_t text_size = strlen(apx_text);
      return apx_parser_parse_bstr(self, (uint8_t const*)apx_text, (uint8_t const*)apx_text + text_size);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_parser_parse_bstr(apx_parser_t* self, uint8_t const* begin, uint8_t const* end)
{
   if ((self != NULL) && (begin != NULL) && (end != NULL) && (begin <= end))
   {
      size_t text_size = end - begin;
      parser_reset(self);
      apx_istream_open(self->stream);
      if (text_size > 0u)
      {
         apx_istream_write(self->stream, begin, (uint32_t)text_size);
      }
      apx_istream_close(self->stream);
      return apx_parser_get_last_error(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void state_init(apx_parse_state_t* self)
{
   self->accept_next = APX_DEFINITION_SECTION_VERSION;
   self->lineno = 0;
   self->major_version = 0;
   self->minor_version = 0;
   self->node = NULL;
   self->data_element = NULL;
   self->data_type = NULL;
   self->port = NULL;
}

static void state_clear(apx_parse_state_t* self)
{
   self->accept_next = APX_DEFINITION_SECTION_VERSION;
   self->lineno = 0;
   self->major_version = 0;
   self->minor_version = 0;
   if (self->node != NULL)
   {
      apx_node_delete(self->node);
      self->node = NULL;
   }
   self->data_element = NULL;
   self->data_type = NULL;
   self->port = NULL;
}

static apx_error_t state_make_node(apx_parse_state_t* self, uint8_t const* name_begin, uint8_t const* name_end)
{
   char node_name[APX_MAX_NAME_LEN + 1];
   size_t name_size = name_end - name_begin;
   if (name_size > APX_MAX_NAME_LEN)
   {
      return APX_NAME_TOO_LONG_ERROR;
   }
   memcpy(&node_name[0], name_begin, name_size);
   node_name[name_size] = '\0';
   self->node = apx_node_new(node_name);
   if (self->node == NULL)
   {
      return APX_MEM_ERROR;
   }
   return APX_NO_ERROR;
}

static apx_error_t state_make_data_type(apx_parse_state_t* self, uint8_t const* name_begin, uint8_t const* name_end)
{
   char node_name[APX_MAX_NAME_LEN + 1];
   size_t name_size = name_end - name_begin;
   if (name_size > APX_MAX_NAME_LEN)
   {
      return APX_NAME_TOO_LONG_ERROR;
   }
   memcpy(&node_name[0], name_begin, name_size);
   node_name[name_size] = '\0';
   self->data_type = apx_dataType_new(node_name, self->lineno);
   if (self->data_type == NULL)
   {
      return APX_MEM_ERROR;
   }
   return APX_NO_ERROR;
}

static apx_error_t state_make_provide_port(apx_parse_state_t* self, uint8_t const* name_begin, uint8_t const* name_end)
{
   char node_name[APX_MAX_NAME_LEN + 1];
   size_t name_size = name_end - name_begin;
   if (name_size > APX_MAX_NAME_LEN)
   {
      return APX_NAME_TOO_LONG_ERROR;
   }
   memcpy(&node_name[0], name_begin, name_size);
   node_name[name_size] = '\0';
   self->port = apx_port_new(APX_PROVIDE_PORT, node_name, self->lineno);
   if (self->port == NULL)
   {
      return APX_MEM_ERROR;
   }
   return APX_NO_ERROR;
}

static apx_error_t state_make_require_port(apx_parse_state_t* self, uint8_t const* name_begin, uint8_t const* name_end)
{
   char node_name[APX_MAX_NAME_LEN + 1];
   size_t name_size = name_end - name_begin;
   if (name_size > APX_MAX_NAME_LEN)
   {
      return APX_NAME_TOO_LONG_ERROR;
   }
   memcpy(&node_name[0], name_begin, name_size);
   node_name[name_size] = '\0';
   self->port = apx_port_new(APX_REQUIRE_PORT, node_name, self->lineno);
   if (self->port == NULL)
   {
      return APX_MEM_ERROR;
   }
   return APX_NO_ERROR;
}

static apx_error_t state_parse_data_signature(apx_parse_state_t* self, apx_signatureParser_t *parser, uint8_t const* begin, uint8_t const* end)
{
   uint8_t const* result = apx_signatureParser_parse_data_signature(parser, begin, end);
   if (result > begin)
   {
      if (result == end)
      {
         self->data_element = apx_signatureParser_take_data_element(parser);
         assert(self->data_element != NULL);
      }
      else
      {
         return APX_STRAY_CHARACTERS_AFTER_PARSE_ERROR;
      }
      return APX_NO_ERROR;
   }
   return apx_signatureParser_get_last_error(parser, NULL);
}

static apx_error_t state_parse_type_attributes(apx_parse_state_t* self, apx_attributeParser_t* parser, uint8_t const* begin, uint8_t const* end)
{
   assert(self->data_type != NULL);
   uint8_t const* result = apx_attributeParser_parse_type_attributes(parser, begin, end, apx_dataType_get_attributes(self->data_type));
   if (result > begin)
   {
      return APX_NO_ERROR;
   }
   return apx_attributeParser_get_last_error(parser, NULL);
}

static apx_error_t state_parse_port_attributes(apx_parse_state_t* self, apx_attributeParser_t* parser, uint8_t const* begin, uint8_t const* end)
{
   assert(self->port != NULL);
   uint8_t const* result = apx_attributeParser_parse_port_attributes(parser, begin, end, apx_port_get_attributes(self->port));
   if (result > begin)
   {
      return APX_NO_ERROR;
   }
   return apx_attributeParser_get_last_error(parser, NULL);
}

static void parser_reset(apx_parser_t* self)
{
   parser_clear_error(self);
   state_clear(&self->state);
}

static void parser_clear_error(apx_parser_t* self)
{
   self->last_error = APX_NO_ERROR;
   self->last_error_line = 0;
}

static void parser_set_error(apx_parser_t* self, apx_error_t error_code, int32_t error_line)
{
   self->last_error = error_code;
   self->last_error_line = error_line;
}

static void parser_init_handler(apx_parser_t* self, apx_istream_handler_t* handler)
{
   if (handler != NULL)
   {
      handler->arg = (void*)self;
      handler->open = parser_on_open;
      handler->close = parser_on_close;
      handler->new_line = parser_on_new_line;
   }
}

static apx_error_t parser_on_open(void* arg)
{
   apx_parser_t* self = (apx_parser_t*)arg;
   if (self == NULL)
   {
      return APX_NULL_PTR_ERROR;
   }
   parser_reset(self);
   return APX_NO_ERROR;
}

static apx_error_t parser_on_close(void* arg)
{
   apx_parser_t* self = (apx_parser_t*)arg;
   if (self != NULL)
   {
      apx_node_t* node = self->state.node;
      if (node != NULL)
      {
         apx_error_t result = apx_node_finalize(node);
         if (result != APX_NO_ERROR)
         {
            parser_set_error(self, result, apx_node_get_last_error_line(node));
         }
      }
      else
      {
         return self->last_error;
      }
   }
   return APX_NULL_PTR_ERROR;
}

static apx_error_t parser_on_new_line(void* arg, const char* line_begin, const char* line_end)
{
   apx_parser_t* self = (apx_parser_t*)arg;
   if (self != NULL)
   {
      size_t line_length = line_end - line_begin;
      self->state.lineno++;
      apx_error_t retval = APX_NO_ERROR;
      //TODO: Add support for line comments starting with the # character
      if (line_length > 0u)
      {
         switch (self->state.accept_next)
         {
         case APX_DEFINITION_SECTION_VERSION:
            retval = parser_accept_version_line(self, (uint8_t* const)line_begin, (uint8_t* const)line_end);
            break;
         case APX_DEFINITION_SECTION_NODE:
            retval = parser_accept_node_declaration(self, (uint8_t* const)line_begin, (uint8_t* const)line_end);
            break;
         case APX_DEFINITION_SECTION_TYPE:
            //As type declaration section is optional we also accept ports declarations
            retval = parser_accept_type_or_port_declaration(self, (uint8_t* const)line_begin, (uint8_t* const)line_end);
            break;
         case APX_DEFINITION_SECTION_PORT:
            retval = parser_accept_port_declaration(self, (uint8_t* const)line_begin, (uint8_t* const)line_end);
            break;
         default:
            retval = APX_PARSE_ERROR;
         }
         if (retval != APX_NO_ERROR)
         {
            parser_set_error(self, retval, self->state.lineno);
         }
      }
      else
      {
         //Skip empty lines
      }
      return retval;
   }
   return APX_NULL_PTR_ERROR;
}

static apx_error_t parser_accept_version_line(apx_parser_t* self, uint8_t const* begin, uint8_t const* end)
{

   uint8_t const* result = NULL;
   char const* prefix = "APX/";
   result = bstr_match_cstr(begin, end, prefix);
   if ((result > begin) && (result <= end))
   {
      uint8_t const* next = result;
      result = apx_parserBase_parse_i32(next, end, &self->state.major_version);
      if ( (result > next) && (result < end) )
      {
         uint8_t c;
         next = result;
         c = *next++;
         if ((c == '.') && (next < end))
         {
            result = apx_parserBase_parse_i32(next, end, &self->state.minor_version);
            if (result > next)
            {
               if (result == end)
               {
                  self->state.accept_next = APX_DEFINITION_SECTION_NODE;
                  return APX_NO_ERROR;
               }
               else
               {
                  return APX_STRAY_CHARACTERS_AFTER_PARSE_ERROR;
               }
            }
         }
      }
   }
   return APX_PARSE_ERROR;
}

static apx_error_t parser_accept_node_declaration(apx_parser_t* self, uint8_t const* begin, uint8_t const* end)
{
   if (begin < end)
   {
      uint8_t const* next = begin;
      uint8_t c = *next++;
      if ( (c == 'N') && (next < end) )
      {
         uint8_t const* result = apx_parserBase_parse_string_literal(next, end);
         if (result > next)
         {
            apx_error_t rc = state_make_node(&self->state, next + 1, result);
            if (rc != APX_NO_ERROR)
            {
               return rc;
            }
            next = result + 1;
            if (next == end)
            {
               self->state.accept_next = APX_DEFINITION_SECTION_TYPE;
               return APX_NO_ERROR;
            }
            else
            {
               return APX_STRAY_CHARACTERS_AFTER_PARSE_ERROR;
            }
         }
      }
   }
   return APX_PARSE_ERROR;
}

static apx_error_t parser_accept_type_or_port_declaration(apx_parser_t* self, uint8_t const* begin, uint8_t const* end)
{
   if (begin < end)
   {
      uint8_t const* next = begin;
      uint8_t c = *next++;
      if (next < end)
      {
         if (c == 'T')
         {
            return parser_accept_type_declaration(self, begin, end);
         }
         else
         {
            apx_error_t retval = parser_accept_port_declaration(self, begin, end);
            if (retval == APX_NO_ERROR)
            {
               self->state.accept_next = APX_DEFINITION_SECTION_PORT;
            }
            return retval;
         }
      }
   }
   return APX_PARSE_ERROR;
}

static apx_error_t parser_accept_type_declaration(apx_parser_t* self, uint8_t const* begin, uint8_t const* end)
{
   if (begin < end)
   {
      uint8_t const* next = begin;
      uint8_t c = *next++;
      assert(self->state.node != NULL);
      if ((c == 'T') && (next < end))
      {
         uint8_t const* result = apx_parserBase_parse_string_literal(next, end);
         if (result > next)
         {
            apx_error_t rc = state_make_data_type(&self->state, next + 1, result);
            if (rc != APX_NO_ERROR)
            {
               return rc;
            }
            assert(self->state.data_type != NULL);
            next = result + 1;
            result = bstr_search_val(next, end, ':');
            if (result > next)
            {
               //Type attributes exists (OK)
               uint8_t const* mark = result;
               rc = state_parse_data_signature(&self->state, &self->signature_parser, next, mark);
               if (rc != APX_NO_ERROR)
               {
                  apx_dataType_delete(self->state.data_type);
                  return apx_signatureParser_get_last_error(&self->signature_parser, NULL);
               }
               apx_dataSignature_set_element(&self->state.data_type->data_signature, self->state.data_element);
               self->state.data_element = NULL;
               next = mark + 1;
               if (next < end)
               {
                  rc = apx_dataType_init_attributes(self->state.data_type);
                  if (rc != APX_NO_ERROR)
                  {
                     apx_dataType_delete(self->state.data_type);
                     return rc;
                  }
                  rc = state_parse_type_attributes(&self->state, &self->attribute_parser, next, end);
                  if (rc != APX_NO_ERROR)
                  {
                     apx_dataType_delete(self->state.data_type);
                     return apx_signatureParser_get_last_error(&self->signature_parser, NULL);
                  }
               }
               else
               {
                  //No characters found after the ':' character
                  apx_dataType_delete(self->state.data_type);
                  return APX_PARSE_ERROR;
               }
            }
            else
            {
               //No attributes exists (also OK)
               rc = state_parse_data_signature(&self->state, &self->signature_parser, next, end);
               if (rc != APX_NO_ERROR)
               {
                  apx_dataType_delete(self->state.data_type);
                  return apx_signatureParser_get_last_error(&self->signature_parser, NULL);
               }
               apx_dataSignature_set_element(&self->state.data_type->data_signature, self->state.data_element);
               self->state.data_element = NULL;
            }
            apx_node_append_data_type(self->state.node, self->state.data_type);
            self->state.data_type = NULL;
            return APX_NO_ERROR;
         }
      }
   }
   return APX_PARSE_ERROR;
}


static apx_error_t parser_accept_port_declaration(apx_parser_t* self, uint8_t const* begin, uint8_t const* end)
{
   if (begin < end)
   {
      uint8_t const* next = begin;
      uint8_t c = *next++;
      if (next < end)
      {
         uint8_t const* result;
         apx_portType_t port_type;
         if (c == 'R')
         {
            port_type = APX_REQUIRE_PORT;
         }
         else if (c == 'P')
         {
            port_type = APX_PROVIDE_PORT;
         }
         else
         {
            return APX_PARSE_ERROR;
         }

         result = apx_parserBase_parse_string_literal(next, end);
         if (result > next)
         {
            apx_error_t rc = (port_type == APX_PROVIDE_PORT)? state_make_provide_port(&self->state, next + 1, result) :
               state_make_require_port(&self->state, next + 1, result);
            if (rc != APX_NO_ERROR)
            {
               return rc;
            }
            next = result + 1;
            assert(self->state.port != NULL);
            result = bstr_search_val(next, end, ':');
            if (result > next)
            {
               //Type attributes exists (OK)
               uint8_t const* mark = result;
               rc = state_parse_data_signature(&self->state, &self->signature_parser, next, mark);
               if (rc != APX_NO_ERROR)
               {
                  apx_port_delete(self->state.port);
                  return rc;
               }
               apx_dataSignature_set_element(&self->state.port->data_signature, self->state.data_element);
               self->state.data_element = NULL;
               next = mark + 1;
               if (next < end)
               {
                  rc = apx_port_init_attributes(self->state.port);
                  if (rc != APX_NO_ERROR)
                  {
                     apx_port_delete(self->state.port);
                     return rc;
                  }
                  rc = state_parse_port_attributes(&self->state, &self->attribute_parser, next, end);
                  if (rc != APX_NO_ERROR)
                  {
                     apx_port_delete(self->state.port);
                     return rc;
                  }
               }
               else
               {
                  //No characters found after the ':' character
                  apx_port_delete(self->state.port);
                  return APX_PARSE_ERROR;
               }
            }
            else
            {
               //No attributes exists (also OK)
               rc = state_parse_data_signature(&self->state, &self->signature_parser, next, end);
               if (rc != APX_NO_ERROR)
               {
                  apx_port_delete(self->state.port);
                  return rc;
               }
               apx_dataSignature_set_element(&self->state.port->data_signature, self->state.data_element);
               self->state.data_element = NULL;
            }
            apx_node_append_port(self->state.node, self->state.port);
            self->state.port = NULL;
            return APX_NO_ERROR;
         }
      }
   }
   return APX_PARSE_ERROR;
}

