/*****************************************************************************
* \file      application.c
* \author    Conny Gustafsson
* \date      2019-10-13
* \brief     Performance test application
*
* Copyright (c) 2019-2022 Conny Gustafsson
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "apx/event_listener.h"
#include "apx/node_data.h"
#include "apx/client.h"
#include "application.h"
#include "pack.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void onClientConnected(void* arg, apx_clientConnection_t* client_connection);
static void onClientDisconnected(void* arg, apx_clientConnection_t* client_connection);
static void onRequirePortWrite(void *arg, apx_portInstance_t *port_instance, uint8_t const* data, apx_size_t size);
static double calculate_average_events_per_second(void);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
static apx_client_t *m_client = NULL;
static apx_nodeInstance_t *m_node_instance = NULL;
static uint32_t m_event_count;
static bool m_is_connected;
static bool m_has_pending_start_cmd;
static bool m_is_test_ongoing;
static bool m_is_requester;
static apx_portInstance_t *m_rqst_handle;
static apx_portInstance_t *m_rsp_handle;
static dtl_sv_t *m_sv;
static uint32_t m_timer = 0u;
static application_cfg_t m_cfg;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
/*
* Returns true on success, false otherwise
*/
bool application_init(const application_cfg_t *cfg)
{
   apx_clientEventListener_t handlerTable;
   apx_error_t result;
   memset(&handlerTable, 0, sizeof(handlerTable));
   handlerTable.client_connect1 = onClientConnected;
   handlerTable.client_disconnect1 = onClientDisconnected;
   handlerTable.require_port_write1 = onRequirePortWrite;
   if (cfg == NULL)
   {
      printf("cfg is NULL, aborting\n");
      return false;
   }
   memcpy(&m_cfg, cfg, sizeof(m_cfg));

   m_is_requester = false;
   m_event_count = 0u;
   m_is_connected = false;
   m_has_pending_start_cmd = true;
   m_is_test_ongoing = false;
   m_sv = dtl_sv_new();
   if (m_cfg.timer_init == 0)
   {
      m_cfg.timer_init = 1u; //Default to 1 second
   }
   m_client = apx_client_new();
   if (m_client != 0)
   {
      apx_client_register_event_listener(m_client, &handlerTable);
      result = apx_client_build_node(m_client, m_cfg.apx_definition);
      if (result != APX_NO_ERROR)
      {
         printf("apx_client_build_node failed with error %d\n", (int) result);
         return false;
      }
      m_node_instance = apx_client_get_last_attached_node(m_client);
      if ( (strcmp(apx_nodeInstance_get_name(m_node_instance), "RequestNode")==0) )
      {
         printf("Running in requester mode\n");
         m_is_requester = true;
         m_rqst_handle = apx_nodeInstance_get_provide_port(m_node_instance, (apx_portId_t) 0u);
         m_rsp_handle = apx_nodeInstance_get_require_port(m_node_instance, (apx_portId_t)0u);
      }
      else
      {
         printf("Running in responder mode\n");
         m_rqst_handle = apx_nodeInstance_get_require_port(m_node_instance, (apx_portId_t)0u);
         m_rsp_handle = apx_nodeInstance_get_provide_port(m_node_instance, (apx_portId_t)0u);
      }
      assert(m_rqst_handle != NULL);
      assert(m_rsp_handle != NULL);
      printf("Connecting to %s\n", m_cfg.server_address);
      switch(m_cfg.resource_type)
      {
      case APX_RESOURCE_TYPE_IPV4: //fall-trough
      case APX_RESOURCE_TYPE_IPV6:
         result = apx_client_connect_tcp(m_client, m_cfg.server_address, m_cfg.tcp_port);
         if (result != APX_NO_ERROR)
         {
            printf("apx_client_connect_tcp failed with error %d\n", (int) result);
            return false;
         }
         break;
      case APX_RESOURCE_TYPE_FILE:
#ifdef _WIN32
         printf("UNIX domain socket path not supported in Windows\n");
         return false;
#else
         result = apx_client_connect_unix(m_client, m_cfg.server_address);
         if (result != APX_NO_ERROR)
         {
            printf("apx_client_connect_unix failed with error %d\n", (int) result);
            return false;
         }
#endif
         break;
      case APX_RESOURCE_TYPE_NAME:
         if ( strcmp(m_cfg.server_address, "localhost") == 0 )
         {
            result = apx_client_connect_tcp(m_client, "127.0.0.1", m_cfg.tcp_port);
            if (result != APX_NO_ERROR)
            {
               printf("apx_client_connect_unix failed with error %d\n", (int) result);
               return false;
            }
         }
         else
         {
            fprintf(stderr, "Error: Unsupported connection name \"%s\"\n", m_cfg.server_address);
            return false;
         }
      break;
      }
      return true;
   }
   printf("apx_client_new failed\n");
   return false;
}

/*
* Returns true if the application want to continue to run, false otherwise
*/
bool application_run(void)
{
   if (m_is_connected)
   {
      if (m_is_requester)
      {
         if (m_has_pending_start_cmd)
         {
            apx_error_t result;
            m_event_count = 1u;
            m_has_pending_start_cmd = false;
            m_is_test_ongoing = true;
            m_timer = m_cfg.timer_init;
            dtl_sv_set_u32(m_sv, m_event_count);
            result = apx_client_write_port_data(m_client, m_rqst_handle, (dtl_dv_t*) m_sv);
            assert(result == APX_NO_ERROR);
            printf("Test started\n");
         }
         else
         {
            if (m_is_test_ongoing)
            {
               m_timer--;
            }
            if (m_timer == 0u)
            {
               m_is_test_ongoing = false;
               double result = calculate_average_events_per_second();
               printf("Test completed.\nTotal Events: %u. Events/s: %.2f.\n", m_event_count, result);
               return false;
            }
         }
      }
   }
   return true;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void onClientConnected(void* arg, apx_clientConnection_t* client_connection)
{
   (void)arg;
   (void)client_connection;
   m_is_connected = true;
   printf("Connected to server\n");
}

static void onClientDisconnected(void* arg, apx_clientConnection_t* client_connection)
{
   (void)arg;
   (void)client_connection;

   m_is_connected = false;
   m_has_pending_start_cmd = true;
   m_is_test_ongoing = false;
   printf("Disconnected from server\n");
}


static void onRequirePortWrite(void* arg, apx_portInstance_t* port_instance, uint8_t const* data, apx_size_t size)
{
   (void)arg;
   (void)data;
   (void)size;
   if (m_is_requester)
   {
      if ((m_is_test_ongoing) && (port_instance == m_rsp_handle))
      {
         apx_error_t result;
         uint32_t value;
         bool ok;
         dtl_sv_t* sv = NULL;
         result = apx_client_read_port_data(m_client, m_rsp_handle, (dtl_dv_t**)&sv);
         assert(result == APX_NO_ERROR);
         assert(sv != NULL);
         value = dtl_sv_to_u32(sv, &ok);
         if (ok)
         {
            m_event_count = value;
            dtl_sv_set_u32(sv, value + 1);
            result = apx_client_write_port_data(m_client, m_rqst_handle, (dtl_dv_t*)sv);
            assert(result == APX_NO_ERROR);
         }
         dtl_dv_dec_ref((dtl_dv_t*)sv);
      }
   }
   else
   {
      if ((m_rqst_handle != NULL) && (m_rsp_handle != NULL))
      {
         apx_error_t result;
         dtl_sv_t* sv = NULL;
         uint32_t value;
         bool ok;
         result = apx_client_read_port_data(m_client, m_rqst_handle, (dtl_dv_t**)&sv);
         assert(result == APX_NO_ERROR);
         assert(sv != NULL);
         value = dtl_sv_to_u32(sv, &ok);
         if (ok)
         {
            dtl_sv_set_u32(sv, value + 1);
            result = apx_client_write_port_data(m_client, m_rsp_handle, (dtl_dv_t*)sv);
            assert(result == APX_NO_ERROR);
         }
         dtl_dv_dec_ref((dtl_dv_t*)sv);
      }
   }
}

static double calculate_average_events_per_second(void)
{
   double events = (double)m_event_count;
   double time = (double)m_cfg.timer_init;
   return events / time;
}
