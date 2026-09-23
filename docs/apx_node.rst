APX Node and Data Management (apx_node)
=======================================

The ``apx_node`` module represents an APX node parse tree, holding data types, require ports, and provide ports.

Overview
--------

An APX node definition contains:

* **Data Types**: Custom type definitions and aliases used by ports.
* **Require Ports**: Input ports expecting signal data from other nodes.
* **Provide Ports**: Output ports publishing signal data to the APX network.

API Reference
-------------

Data Types
~~~~~~~~~~

.. doxygentypedef:: apx_node_t

Lifecycle Functions
~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: apx_node_create
.. doxygenfunction:: apx_node_destroy
.. doxygenfunction:: apx_node_new
.. doxygenfunction:: apx_node_delete

Node Configuration & Accessors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: apx_node_append_data_type
.. doxygenfunction:: apx_node_append_port
.. doxygenfunction:: apx_node_set_name
.. doxygenfunction:: apx_node_get_name
.. doxygenfunction:: apx_node_num_data_types
.. doxygenfunction:: apx_node_num_require_ports
.. doxygenfunction:: apx_node_num_provide_ports
.. doxygenfunction:: apx_node_finalize
