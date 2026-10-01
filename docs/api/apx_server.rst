Server Daemon and Lifecycle (apx_server)
========================================

The ``apx_server`` module manages the APX server instance, routing signal updates across client nodes, orchestrating child connections, and coordinating server extensions.

Overview
--------

The APX server acts as the central router in an APX network:

* Accepts incoming connections from client nodes over sockets (TCP/IP or UNIX domain sockets via extensions).
* Parses and matches provide and require port signatures across connected nodes.
* Routes signal updates published by provide ports to interested require ports.
* Manages server extension lifecycles and background worker threads.

API Reference
-------------

Data Types
~~~~~~~~~~

.. doxygentypedef:: apx_server_t

Lifecycle Functions
~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: apx_server_create
.. doxygenfunction:: apx_server_destroy
.. doxygenfunction:: apx_server_new
.. doxygenfunction:: apx_server_delete
.. doxygenfunction:: apx_server_start
.. doxygenfunction:: apx_server_stop

Connection & Extension Management
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: apx_server_accept_connection
.. doxygenfunction:: apx_server_detach_connection
.. doxygenfunction:: apx_server_add_extension
.. doxygenfunction:: apx_server_register_event_listener
.. doxygenfunction:: apx_server_unregister_event_listener

Routing & Port Management
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: apx_server_connect_node_instance_provide_ports
.. doxygenfunction:: apx_server_connect_node_instance_require_ports
.. doxygenfunction:: apx_server_disconnect_node_instance_provide_ports
.. doxygenfunction:: apx_server_disconnect_node_instance_require_ports
