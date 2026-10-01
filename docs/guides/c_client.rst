Writing an APX Client in C
==========================

This guide walks through creating an APX client application in C using ``apx_client_t``. An APX client instantiates one or more virtual nodes defined by APX IDL definitions, publishes provide-ports, subscribes to require-ports, and exchanges data with an :doc:`../apps/apx_server` daemon over streaming sockets.

Overview of Client Workflow
---------------------------

An APX client typically follows this lifecycle:

1. **Instantiate the client**: Create the ``apx_client_t`` container.
2. **Register event listeners**: Attach callbacks for connection state changes, incoming signal updates, and errors.
3. **Build the node**: Parse an APX IDL node definition text or file to register provide and require ports.
4. **Connect to server**: Connect to ``apx-server`` over a UNIX domain socket, TCP/IP, TLS, or Linux VSOCK.
5. **Exchange signals**: Write provide-port updates to send data to other nodes, and process incoming require-port writes in the registered callback.
6. **Disconnect and cleanup**: Tear down connections and free resources.

Step-by-Step Implementation
---------------------------

1. Including Headers & Client Creation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Include ``apx/client.h`` and allocate the client object:

.. code-block:: c

   #include "apx/client.h"
   #include "apx/event_listener.h"
   #include "dtl_sv.h"

   apx_client_t *client = apx_client_new();
   if (client == NULL)
   {
       /* Handle memory allocation error */
   }

2. Implementing Event Listeners
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Client applications receive asynchronous events via the ``apx_client_event_listener_t`` structure:

.. list-table::
   :header-rows: 1
   :widths: 25 35 40

   * - Callback
     - Signature
     - Trigger Condition
   * - ``connected``
     - ``void (*)(void *arg, apx_client_connection_t *conn)``
     - Invoked when the transport connection is established and the initial APX protocol handshake succeeds.
   * - ``disconnected``
     - ``void (*)(void *arg, apx_client_connection_t *conn)``
     - Invoked when the connection to the server is closed or lost.
   * - ``require_port_write``
     - ``void (*)(void *arg, apx_port_instance_t *port_instance, uint8_t const *data, apx_size_t size)``
     - Invoked whenever the APX server routes updated signal data to a require-port on an attached node.
   * - ``error_notify``
     - ``void (*)(void *arg, apx_client_connection_t *conn, apx_error_t error_code, const char *name)``
     - Invoked when a protocol, parse, or transmission error occurs.

Define callback functions and register the listener table with the client:

.. code-block:: c

   static void on_client_connected(void *arg, apx_client_connection_t *conn)
   {
       (void)arg;
       (void)conn;
       printf("[CLIENT] Connected to APX server\n");
   }

   static void on_client_disconnected(void *arg, apx_client_connection_t *conn)
   {
       (void)arg;
       (void)conn;
       printf("[CLIENT] Disconnected from APX server\n");
   }

   static void on_require_port_write(void *arg, apx_port_instance_t *port, uint8_t const *data, apx_size_t size)
   {
       (void)arg;
       const char *port_name = apx_port_instance_get_name(port);
       printf("[SIGNAL IN] Port '%s' updated (%u bytes)\n", port_name, (unsigned)size);
   }

   apx_client_event_listener_t listener;
   memset(&listener, 0, sizeof(listener));
   listener.connected = on_client_connected;
   listener.disconnected = on_client_disconnected;
   listener.require_port_write = on_require_port_write;

   apx_client_register_event_listener(client, &listener);

.. note::

   Callbacks are executed on internal worker threads. Do not perform blocking operations or acquire client mutexes in a way that could cause deadlocks.

3. Attaching an APX Node Definition
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Compile an APX IDL specification into the client runtime:

.. code-block:: c

   const char *node_def =
       "APX/1.3\n"
       "N\"VehicleNode\"\n"
       "P\"VehicleSpeed\"S:=0\n"
       "R\"EngineState\"C(0,3):=0\n";

   apx_error_t rc = apx_client_build_node(client, node_def);
   if (rc != APX_NO_ERROR)
   {
       fprintf(stderr, "Failed to compile node definition: error %d\n", (int)rc);
       apx_client_delete(client);
       return 1;
   }

4. Connecting to the Server
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Depending on the transport configured on ``apx-server``:

.. code-block:: c

   // Connect over UNIX domain socket (Linux default)
   apx_client_connect_unix(client, "/tmp/apx.socket");

   // Or connect over plain TCP
   // apx_client_connect_tcp(client, "127.0.0.1", 5000);

   // Or connect over Linux VSOCK
   // apx_client_connect_vsock(client, 2, 52000);

5. Writing Provide-Port Signals
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To publish an updated value to a provide-port:

1. Lookup the ``apx_port_instance_t`` handle by node and port name.
2. Construct the value using the dynamic type library (``dtl_sv_t``).
3. Call ``apx_client_write_port_data()``:

.. code-block:: c

   apx_port_instance_t *port = apx_client_get_port_instance_by_name(client, "VehicleNode", "VehicleSpeed");
   if (port != NULL)
   {
       dtl_sv_t *speed_val = dtl_sv_new_i32(75);
       apx_client_write_port_data(client, port, (dtl_dv_t*)speed_val);
       dtl_sv_delete(speed_val);
   }

Complete Example
----------------

Below is a complete, standalone C99 example demonstrating an APX client that publishes vehicle speed and observes engine status:

.. code-block:: c

   #include <stdio.h>
   #include <string.h>
   #include <unistd.h>
   #include "apx/client.h"
   #include "apx/event_listener.h"
   #include "dtl_sv.h"

   static volatile int m_running = 1;

   static void on_connected(void *arg, apx_client_connection_t *conn)
   {
       (void)arg; (void)conn;
       printf("[CLIENT] Connected to APX server\n");
   }

   static void on_disconnected(void *arg, apx_client_connection_t *conn)
   {
       (void)arg; (void)conn;
       printf("[CLIENT] Disconnected\n");
       m_running = 0;
   }

   static void on_require_port_write(void *arg, apx_port_instance_t *port, uint8_t const *data, apx_size_t size)
   {
       (void)arg;
       printf("[CLIENT] Port '%s' received update (%u bytes)\n",
              apx_port_instance_get_name(port), (unsigned)size);
   }

   int main(void)
   {
       apx_client_t *client = apx_client_new();
       if (client == NULL) return 1;

       apx_client_event_listener_t listener;
       memset(&listener, 0, sizeof(listener));
       listener.connected = on_connected;
       listener.disconnected = on_disconnected;
       listener.require_port_write = on_require_port_write;
       apx_client_register_event_listener(client, &listener);

       const char *node_def =
           "APX/1.3\n"
           "N\"DashboardNode\"\n"
           "P\"VehicleSpeed\"S:=0\n"
           "R\"EngineState\"C(0,3):=0\n";

       if (apx_client_build_node(client, node_def) != APX_NO_ERROR)
       {
           fprintf(stderr, "Failed to parse APX definition\n");
           apx_client_delete(client);
           return 1;
       }

       if (apx_client_connect_unix(client, "/tmp/apx.socket") != APX_NO_ERROR)
       {
           fprintf(stderr, "Failed to connect to /tmp/apx.socket\n");
           apx_client_delete(client);
           return 1;
       }

       apx_port_instance_t *speed_port =
           apx_client_get_port_instance_by_name(client, "DashboardNode", "VehicleSpeed");

       int speed = 0;
       while (m_running && speed <= 100)
       {
           sleep(1);
           speed += 10;
           dtl_sv_t *val = dtl_sv_new_i32(speed);
           apx_client_write_port_data(client, speed_port, (dtl_dv_t*)val);
           dtl_sv_delete(val);
           printf("[CLIENT] Published VehicleSpeed = %d\n", speed);
       }

       apx_client_disconnect(client);
       apx_client_delete(client);
       return 0;
   }
