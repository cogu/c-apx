Getting Started
===============

This guide builds the `c-apx reference implementation <https://github.com/cogu/c-apx>`_,
starts an APX server, connects two example nodes, and sends a signal between
them. Keep each process running while you complete the following steps.

Prerequisites
-------------

.. tab-set::

   .. tab-item:: Linux
      :sync: linux

      Install Git, CMake, and GCC or Clang using your distribution's package manager.

   .. tab-item:: Windows
      :sync: windows

      Install Git and Visual Studio with the **Desktop development with C++**
      workload. In the Visual Studio Installer, make sure the CMake tools for Windows
      component is selected.

      Run the commands in this guide from an **x64 Native Tools Command Prompt for
      Visual Studio**.

Clone and Build c-apx
---------------------

Clone the repository, then initialize its Git submodules before building it:

.. code-block:: console

   git clone https://github.com/cogu/c-apx.git
   cd c-apx
   git submodule update --init --recursive
   cmake -S . -B build
   cmake --build build

Start the APX Server
--------------------

From the ``c-apx`` directory, start the server with the example configuration:

.. tab-set::

   .. tab-item:: Linux
      :sync: linux

      .. code-block:: bash

         build/app/apx_server/apx-server example/config/server.json

   .. tab-item:: Windows
      :sync: windows

      .. code-block:: bat

         build\app\apx_server\Debug\apx-server.exe example\config

Leave the server running. Notice that with the example configuration, the
server text log extension outputs real-time server events (such as extension
initialization and socket bindings) directly to standard output.

Connect the Example Nodes
-------------------------

Open a second terminal in the ``c-apx`` directory and start the listener node:

.. tab-set::

   .. tab-item:: Linux
      :sync: linux

      .. code-block:: bash

         build/app/apx_node/apx-node --no-bind example/nodes/unsigned_listener.apx

   .. tab-item:: Windows
      :sync: windows

      .. code-block:: bat

         build\app\apx_node\Debug\apx-node.exe --no-bind example\nodes\unsigned_listener.apx

Open a third terminal in the ``c-apx`` directory and start the sender node:

.. tab-set::

   .. tab-item:: Linux
      :sync: linux

      .. code-block:: bash

         build/app/apx_node/apx-node example/nodes/unsigned_sender.apx

   .. tab-item:: Windows
      :sync: windows

      .. code-block:: bat

         build\app\apx_node\Debug\apx-node.exe example\nodes\unsigned_sender.apx

Send a Value
------------

Open a fourth terminal in the ``c-apx`` directory and set ``VehicleSpeed`` to
``100``:

.. tab-set::

   .. tab-item:: Linux
      :sync: linux

      .. code-block:: bash

         build/app/apx_control/apx-control VehicleSpeed 100

   .. tab-item:: Windows
      :sync: windows

      .. code-block:: bat

         build\app\apx_control\Debug\apx-control.exe VehicleSpeed 100

The value travels from ``apx-control`` to ``unsigned_sender``, then through the
APX server to ``unsigned_listener``. The terminal running the listener prints:

.. code-block:: text

   "VehicleSpeed": 100

Next Steps
----------

* Learn how to embed APX clients into C applications in :doc:`guides/c_client`.
* Program and embed custom servers and extensions in :doc:`guides/server_programming`.
* Explore command-line tools: :doc:`apps/apx_server`, :doc:`apps/apx_node`, and :doc:`apps/apx_control`.
* Configure logging levels and timestamps in :doc:`extensions/text_log`.
