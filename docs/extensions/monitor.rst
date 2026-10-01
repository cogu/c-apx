Server Monitor Extension
========================

The ``monitor-extension`` provides runtime state inspection and monitoring capabilities within the APX server daemon. It tracks client connections, observed virtual files, and signal transfer states.

Configuration Key
-----------------

In ``server.json``, the extension is configured under the ``"monitor-extension"`` key:

.. code-block:: json

   {
     "monitor-extension": {
       "enabled": true
     }
   }

Configuration Parameters
------------------------

.. list-table::
   :header-rows: 1
   :widths: 25 15 15 45

   * - Parameter
     - Type
     - Default
     - Description
   * - ``enabled``
     - Boolean
     - ``true``
     - Enables or disables the server monitoring extension.

Usage
-----

The monitor extension observes server lifecycle hooks without blocking signal routing performance. When enabled alongside interactive monitoring tools like :doc:`../apps/apx_info`, it allows administrators and test fixtures to query active connections and attached virtual file descriptors.
