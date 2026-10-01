Server Programming & Event Listeners
======================================

This guide explains how to embed and program the APX server daemon in C using ``apx_server_t``, register server-level and connection-level event listeners, and author custom server extensions.

Server Lifecycle & Embedding
----------------------------

The APX server can be embedded directly into custom C applications, test suites, and platform middleware:

.. code-block:: c

   #include "apx/server.h"

   apx_server_t server;
   apx_server_create(&server);

   // Configure and attach extensions...

   apx_error_t rc = apx_server_start(&server);
   if (rc == APX_NO_ERROR)
   {
       // Main application loop...
   }

   apx_server_stop(&server);
   apx_server_destroy(&server);

Server Event Listeners
----------------------

The server notifies registered listeners of global lifecycle events and internal diagnostic messages through ``apx_server_event_listener_t``:

.. list-table::
   :header-rows: 1
   :widths: 25 35 40

   * - Callback
     - Signature
     - Trigger Condition
   * - ``new_connection``
     - ``void (*)(void *arg, apx_server_connection_t *conn)``
     - Invoked when a new client establishes a connection and completes initial transport handshakes.
   * - ``connection_closed``
     - ``void (*)(void *arg, apx_server_connection_t *conn)``
     - Invoked when an established client connection terminates or encounters a fatal transport error.
   * - ``server_write_log``
     - ``void (*)(void *arg, apx_log_level_t level, const char *label, const char *msg)``
     - Invoked when the server daemon emits an internal log entry (routing errors, port mismatches, security rejections).

Registering Server Listeners
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static void on_new_connection(void *arg, apx_server_connection_t *conn)
   {
       (void)arg;
       (void)conn;
       printf("[SERVER-EVENT] New client connected\n");
   }

   static void on_connection_closed(void *arg, apx_server_connection_t *conn)
   {
       (void)arg;
       (void)conn;
       printf("[SERVER-EVENT] Client disconnected\n");
   }

   static void on_server_log(void *arg, apx_log_level_t level, const char *label, const char *msg)
   {
       (void)arg;
       printf("[LOG][%d][%s] %s\n", (int)level, label, msg);
   }

   apx_server_event_listener_t listener;
   memset(&listener, 0, sizeof(listener));
   listener.new_connection = on_new_connection;
   listener.connection_closed = on_connection_closed;
   listener.server_write_log = on_server_log;

   void *handle = apx_server_register_event_listener(&server, &listener);

Connection & File Event Listeners
---------------------------------

To observe virtual file publications (such as node definitions or memory-mapped signal files) on individual client connections, use ``apx_server_connection_event_listener_t``:

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Callback
     - Signature
     - Trigger Condition
   * - ``protocol_header_accepted``
     - ``void (*)(void *arg, apx_connection_base_t *conn)``
     - Invoked when Remote File Protocol headers are successfully negotiated.
   * - ``file_published``
     - ``void (*)(void *arg, apx_connection_base_t *conn, const rmf_file_info_t *file_info)``
     - Invoked when a client advertises a new virtual file (e.g. ``Node.apx``, ``Node.out``).
   * - ``file_revoked``
     - ``void (*)(void *arg, apx_connection_base_t *conn, const rmf_file_info_t *file_info)``
     - Invoked when an existing virtual file is revoked by a client.

Developing Custom Server Extensions
-----------------------------------

Custom server extensions allow third parties to integrate bespoke transports (such as CAN, shared memory, or IPC mechanisms) and logging targets without modifying upstream APX core code.

1. Implement Lifecycle Handlers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Every extension implements an initialization function and a shutdown function:

.. code-block:: c

   #include "apx/server.h"
   #include "apx/server_extension.h"

   static apx_error_t my_extension_init(struct apx_server_tag *server, dtl_dv_t *config)
   {
       // Parse custom JSON configuration from config (dtl_hv_t)
       // Initialize listeners, background worker threads, or resources
       return APX_NO_ERROR;
   }

   static void my_extension_shutdown(void)
   {
       // Clean up extension resources and stop worker threads
   }

   apx_error_t my_extension_register(struct apx_server_tag *server, dtl_dv_t *config)
   {
       apx_server_extension_handler_t handler = {my_extension_init, my_extension_shutdown};
       return apx_server_add_extension(server, "MY_EXTENSION", &handler, config);
   }

2. CMake Integration
~~~~~~~~~~~~~~~~~~~~

Register the extension using CMake's build hook:

.. code-block:: cmake

   apx_register_server_extension(
       TARGET my_extension
       NAME "my-extension"
       HEADER "my_extension.h"
       REGISTER_FN "my_extension_register"
   )

This automatically generates the static registry entry in ``extensions_cfg.c``, linking the extension directly into the ``apx-server`` binary.

For complete out-of-tree extension guides and BitBake integration examples, see the [APX CMake Integration Guide](https://github.com/cogu/c-apx/blob/master/cmake/README.md) (`cmake/README.md`).
