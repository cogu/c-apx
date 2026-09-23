Client Connection and Lifecycle (apx_client)
============================================

The ``apx_client`` module provides the runtime client interface for connecting to an APX server, managing local nodes, and transferring signal data.

Overview
--------

The client runtime encapsulates:

* Connecting to an APX server over TCP or UNIX domain sockets.
* Parsing and registering local APX node definitions.
* Publishing node definition files (``.apx``) and signal data buffers (``.out``) to the server.
* Subscribing to input buffers (``.in``) updated by the server.

API Reference
-------------

Data Types
~~~~~~~~~~

.. doxygentypedef:: apx_client_t

Lifecycle Functions
~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: apx_client_create
.. doxygenfunction:: apx_client_destroy
.. doxygenfunction:: apx_client_new
.. doxygenfunction:: apx_client_delete

Connection Management
~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: apx_client_disconnect
.. doxygenfunction:: apx_client_register_event_listener
.. doxygenfunction:: apx_client_unregister_event_listener

Node & Manager Access
~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: apx_client_build_node
.. doxygenfunction:: apx_client_get_last_attached_node
.. doxygenfunction:: apx_client_get_file_manager
.. doxygenfunction:: apx_client_get_node_manager
