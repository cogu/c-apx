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
       "file-path": "/var/log/apx/server.log",
       "log-level": "INFO",
       "use-timestamp": false
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
   * - ``log-level``
     - String
     - ``"INFO"``
     - Minimum log severity level to output. Supported values (in order of severity): ``"CRITICAL"``, ``"ERROR"``, ``"WARNING"``, ``"INFO"``, ``"DEBUG"``. Case-insensitive. Messages below the configured level are filtered out.
   * - ``use-timestamp``
     - Boolean
     - ``false``
     - Toggles prepending a millisecond-precision timestamp (``[YYYY-MM-DD HH:MM:SS.mmm]``) before each log message.
   * - ``syslog-enabled``
     - Boolean
     - ``false``
     - *(Placeholder / Not yet implemented)* Reserved for future system syslog daemon output.

Message Formatting
------------------

Log entries follow a standardized prefix format:

* **With timestamp enabled:**

  .. code-block:: text

     [YYYY-MM-DD HH:MM:SS.mmm] [<LEVEL>] [<LABEL>] <message>

  Example:

  .. code-block:: text

     [2026-10-06 21:30:00.123] [INFO] [SERVER] Client connected (id: 1)

* **With timestamp disabled (default):**

  .. code-block:: text

     [<LEVEL>] [<LABEL>] <message>

  Example:

  .. code-block:: text

     [INFO] [SERVER] Client connected (id: 1)

Usage Configurations
--------------------

1. Console Output with Timestamps and Debug Level
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To direct detailed debug logs with timestamps to the terminal console:

.. code-block:: json

   {
     "textlog-extension": {
       "enabled": true,
       "file-enabled": true,
       "file-path": "",
       "log-level": "DEBUG",
       "use-timestamp": true
     }
   }

2. Dedicated File Logging (Default Info Level)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To direct server events to a permanent log file on disk:

.. code-block:: json

   {
     "textlog-extension": {
       "enabled": true,
       "file-enabled": true,
       "file-path": "/var/log/apx/server.log",
       "log-level": "INFO",
       "use-timestamp": false
     }
   }
