Server Text Log Extension
==========================

The ``textlog-extension`` provides diagnostic and runtime event logging for the APX server daemon. It formats server events—such as client connections, disconnections, node attachments, and port routing—into human-readable text output.

Configuration Key
-----------------

In ``server.json``, the extension is configured under the ``"textlog-extension"`` key:

.. code-block:: json

   {
     "textlog-extension": {
       "enabled": true,
       "file-enabled": true,
       "file-path": "/var/log/apx/server.log"
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
     - Toggles extension registration and execution.
   * - ``file-enabled``
     - Boolean
     - ``false``
     - Enables or disables text stream logging output.
   * - ``file-path``
     - String
     - ``""``
     - Path to the destination log file. When set to an empty string (``""``), log events are directed to standard output (``stdout``). When set to a valid filepath, log events are written to the specified file.
   * - ``syslog-enabled``
     - Boolean
     - ``false``
     - *(Placeholder / Not yet implemented)* Reserved for future system syslog daemon output.

Usage Configurations
--------------------

1. Console Output (Stdout)
~~~~~~~~~~~~~~~~~~~~~~~~~~

To direct all server log entries to the terminal console:

.. code-block:: json

   {
     "textlog-extension": {
       "enabled": true,
       "file-enabled": true,
       "file-path": ""
     }
   }

2. Dedicated File Logging
~~~~~~~~~~~~~~~~~~~~~~~~~

To direct server events to a permanent log file on disk:

.. code-block:: json

   {
     "textlog-extension": {
       "enabled": true,
       "file-enabled": true,
       "file-path": "/var/log/apx/server.log"
     }
   }
