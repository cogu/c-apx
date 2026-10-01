apx_perf_test
=============

``apx_perf_test`` is a benchmarking utility that measures APX signal routing throughput and latency across an active :doc:`apx_server` instance. It executes a full-duplex ping-pong round-trip communication pattern between two virtual nodes.

Synopsis
--------

.. code-block:: text

   apx_perf_test [-c | --connect <connect_path>]
                 [-s | --slave]
                 [-t | --time <seconds>]
                 [--version] [-h | --help]

Architecture & Benchmark Flow
-----------------------------

The benchmark uses two nodes cooperating in a ping-pong exchange:

1. **Requester (Master / Default Mode)**:
   * Instantiates ``RequestNode`` with provide-port ``PerfTest_rqst`` (uint32) and require-port ``PerfTest_rsp`` (uint32).
   * Initiates the benchmark by writing ``1`` to ``PerfTest_rqst``.
   * Measures continuous round-trip replies from the responder for the duration specified by ``-t``.
   * Computes and displays total round-trip transactions, duration, and average throughput in ``events/second``.

2. **Responder (Slave Mode, ``-s``)**:
   * Instantiates ``RespondNode`` with require-port ``PerfTest_rqst`` (uint32) and provide-port ``PerfTest_rsp`` (uint32).
   * Listens passively for incoming updates to ``PerfTest_rqst``.
   * Increments the received value and writes it back to ``PerfTest_rsp``.

Command-Line Options
--------------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``-c <path>``, ``--connect <path>``
     - Address or path to the ``apx-server``. Accepts a UNIX domain socket path or a TCP address with port (e.g. ``localhost:5000``). Default on Linux: ``/tmp/apx.socket``. Default on Windows: ``127.0.0.1:5000``.
   * - ``-s``, ``--slave``
     - Launch in responder (slave) mode. When omitted, runs in requester (master) mode.
   * - ``-t <seconds>``, ``--time <seconds>``
     - Duration in seconds to run the performance test (master mode only). Default: ``5``.
   * - ``--version``
     - Display version information and exit.
   * - ``-h``, ``--help``
     - Display usage help and exit.

Running a Benchmark
-------------------

Step 1: Start the APX Server
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apx-server --socket-config '{"unix-file": "/tmp/apx.socket"}'

Step 2: Start the Responder (Slave)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In a separate terminal, launch the responder:

.. code-block:: bash

   apx_perf_test -s -c /tmp/apx.socket

Step 3: Run the Requester (Master)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In another terminal, run the requester for 10 seconds:

.. code-block:: bash

   apx_perf_test -t 10 -c /tmp/apx.socket

Expected Output
~~~~~~~~~~~~~~~

.. code-block:: text

   Connected to APX server
   Running test for 10 seconds...
   Completed 425,120 round-trip transactions in 10.00 seconds.
   Throughput: 42,512 events/sec
