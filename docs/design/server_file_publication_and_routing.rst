Server File Publication and Signal Routing Architecture
========================================================

This document describes the architectural design of RemoteFile virtual file publication,
in-memory buffer pre-population, and batched signal routing in ``apx-server``.
This architecture minimizes wire framing overhead, eliminates per-port message fragmentation,
and guarantees strict dependency ordering during client connect and disconnect lifecycles.

Overview and Architecture Goals
--------------------------------

In APX sessions, the server daemon (``apx-server``) manages two coupled responsibilities:

1. **Remote File Protocol (RMFP) Lifecycle**: Hosting and synchronizing memory-mapped virtual
   files across client sessions.
2. **Signal Routing Engine**: Dynamically matching provide and require ports by name and signature,
   updating connection count buffers, and routing live signal data.

In large embedded and automotive deployments with nodes containing over 1,000 signals, a naive
per-port routing implementation causes message storms—dispatching thousands of tiny 2-byte write
packets across the transport. Because each message carries transport framing (``NumHeader``) and
RemoteFile address headers (4 to 8 bytes of header per message), uncoalesced per-port updates lead to:

* Over 400% wire framing overhead.
* Socket write syscall amplification.
* Worker ringbuffer queue contention and lock thrashing.
* Out-of-order signal processing if data arrives before client port counts are updated.

To solve this, ``c-apx`` implements three foundational architectural mechanisms:

1. **Deferred Server File Publication**: Server-owned files (``.cout``, ``.cin``, ``.in``) are announced
   to the connecting client only **after** definitions are parsed, initial provide data is received,
   and bidirectional in-memory routing is resolved.
2. **In-Memory Pre-Population & Bulk Initial Snapshots**: When the connecting client opens server-owned
   files, each file is transferred as a **single, fully populated bulk snapshot**. Zero delta-updates
   are sent to the connecting client.
3. **Batched Range-List Coalescing**: When existing peer nodes are affected by connection or disconnection
   events, updates to their open files are tracked in disjoint range lists (``adt_u32_range_list_t``)
   and flushed in coalesced byte ranges following a strict dependency order (``.cin`` |rarr| ``.cout`` |rarr| ``.in``).

.. |rarr| unicode:: U+2192 .. right arrow

The Five RemoteFile Virtual Files
---------------------------------

As defined in the APX specification, each connected node participates in up to five virtual files:

.. list-table::
   :widths: 18 15 22 20 25
   :header-rows: 1

   * - File Name
     - Owner
     - Memory Region
     - Transfer Direction
     - Contents
   * - ``<Node>.apx``
     - Client
     - High (64 MB+)
     - Client |rarr| Server
     - APX IDL Interface Definition
   * - ``<Node>.out``
     - Client
     - Low (0–16 KB)
     - Client |rarr| Server
     - Packed Provide-Port Data
   * - ``<Node>.in``
     - Server
     - Low (0–16 KB)
     - Server |rarr| Client
     - Packed Require-Port Data
   * - ``<Node>.cout``
     - Server
     - High (128 MB+)
     - Server |rarr| Client
     - Provide-Port Connection Counts (uint16 per port)
   * - ``<Node>.cin``
     - Server
     - High (128 MB+)
     - Server |rarr| Client
     - Require-Port Connection Counts (uint16 per port)

Connection Lifecycle and Publication Order
------------------------------------------

When a new client connects to ``apx-server``, the server coordinates ingestion, routing, and
publication in distinct, sequential phases:

1. **Transport Connection & Handshake**:
   The client opens a socket (UNIX domain socket, TCP/IP, or VSOCK) and transmits the RMFP greeting
   (e.g., ``RMFP/1.1\nNumHeader: 32\n\n``). The server verifies the greeting and responds with
   ``RMF_CMD_ACK`` or ``RMF_CMD_HEADER_ACCEPTED``.

2. **Definition Ingestion & Allocation**:
   The client announces its local files: ``<Node>.apx`` and ``<Node>.out``. The server opens and
   reads ``<Node>.apx``, parses the IDL text, compiles VM bytecode programs, and instantiates
   shadow buffers for all five files.

3. **Initial Provide Data Ingestion**:
   The server requests ``<Node>.out`` via ``OpenFile``. The client responds with its initial
   provide-port data snapshot.

4. **In-Memory Pre-Population (Zero Wire Traffic)**:
   With definitions and initial provide values in memory, the server executes bidirectional
   port matching against all active sessions:

   * **Require Ports**: Matches ``Node_A`` require ports with existing providers (e.g. ``Node_B``),
     copying current provider values directly into ``Node_A.in`` and calculating require counts
     for ``Node_A.cin``.
   * **Provide Ports**: Matches ``Node_A`` provide ports with existing requesters, calculating
     provide connection counts for ``Node_A.cout``.

   All calculations update the local in-memory buffers of ``Node_A`` directly.

5. **Orderly Server File Publication**:
   Only after pre-population is complete does the server publish its files to the client, in strict
   dependency order:

   1. ``<Node>.cout``
   2. ``<Node>.cin``
   3. ``<Node>.in``

6. **Single Bulk Transfers to Connecting Client**:
   When the client requests and opens these files, the server transmits each file in a single,
   fully populated bulk transfer. Zero delta packets are sent to the newly connected client.

7. **Coalesced Updates to Surviving Peers**:
   Surviving peer clients that already hold open files receive updates through the routing batching
   mechanism described below.

Connect Sequence Diagram
~~~~~~~~~~~~~~~~~~~~~~~~

.. uml:: diagrams/server_connect_publication_and_routing.puml

Routing Batching and Range-List Coalescing
------------------------------------------

Architecture and Data Structures
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

During routing events, multiple port instances on a node may change state simultaneously.
Rather than transmitting individual updates immediately, ``apx_node_instance_t`` manages a batching
lifecycle protected by an internal ``batch_lock``:

.. code-block:: c

   void apx_node_instance_begin_batch(apx_node_instance_t *self);
   apx_error_t apx_node_instance_end_batch(apx_node_instance_t *self);

Each node instance maintains three disjoint range lists (``adt_u32_range_list_t``):

* ``self->dirty_in``: Modified byte ranges in the require-port data file (``.in``).
* ``self->dirty_cin``: Modified byte ranges in the require-port connection count file (``.cin``).
* ``self->dirty_cout``: Modified byte ranges in the provide-port connection count file (``.cout``).

When a batch is active (``batch_depth > 0``), signal and port count writes do not transmit across
the socket immediately. Instead, they record modified byte intervals into the appropriate range list.

Coalescing Threshold (``APX_ROUTING_COALESCE_MAX_GAP``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Range insertions use a coalescing gap threshold defined in ``node_instance.c``:

.. code-block:: c

   #define APX_ROUTING_COALESCE_MAX_GAP 6u

When two dirty intervals are separated by an unmodified gap of :math:`\le 6` bytes, the range list
merges them into a single contiguous interval.

**Wire Overhead Rationale**:

* **Low Address Files (``.in`` / ``.out``)**: Mapped in the lower 16 KB (``0x00000000``). They use the
  2-byte RemoteFile Low Address Form. Combined with ``NumHeader`` (1–2 bytes), each separate message
  requires 3 to 4 bytes of header overhead.
* **High Address Files (``.cin`` / ``.cout``)**: Mapped at 128 MB (``0x08000000``). They use the
  4-byte High Address Form, requiring 5 to 6 bytes of header overhead per message.
* Transmitting up to 6 unchanged bytes amortizes cleanly against header overhead, while eliminating
  socket write system calls, packet buffer fragmentation, and receiver VM dispatch overhead.

Strict Flush Dependency Ordering
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When ``apx_node_instance_end_batch()`` reaches depth 0, dirty range lists are flushed to the
transport in a deterministic sequence:

1. **Flush ``dirty_cin``**: Require port counts are updated first. This ensures client runtimes
   and user notification callbacks observe that a valid provider exists before signal data changes.
2. **Flush ``dirty_cout``**: Provide port counts are updated second.
3. **Flush ``dirty_in``**: Live signal payload data is transmitted last.

Concurrency and Deadlock Prevention
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Batch operations use a dedicated ``batch_lock`` mutex, cleanly decoupled from the
``connector_table`` lock and connection send mutexes. This prevents lock order inversion deadlocks
when routing events traverse bidirectional node relationships.

Disconnection Lifecycle
-----------------------

When a client disconnects (socket closed, process terminated, or network timeout), the server
cleans up routing tables and notifies surviving peers:

1. The server marks the departing session disconnected and invokes ``detach_all_nodes()``.
2. All surviving peer nodes affected by departing connectors enter an explicit routing batch (``begin_batch``).
3. The routing engine tears down port connectors:
   * Require connection counts on surviving nodes are decremented.
   * Provide connection counts on surviving nodes are decremented.
   * If require ports have configured initial values, signal buffers are reset.
4. All modifications are captured in the surviving nodes' range lists.
5. The server completes the batch (``end_batch``), flushing decremented counts and reset data to
   surviving clients in coalesced, bulk transfers.

Disconnect Sequence Diagram
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. uml:: diagrams/server_disconnect_routing.puml

Performance and Benchmark Verification
--------------------------------------

The combination of deferred publication, pre-population, and range-list coalescing was benchmarked
on representative large nodes (over 1,000 signals and 140–1,220 ports per node):

.. list-table::
   :widths: 35 25 25 15
   :header-rows: 1

   * - Phase
     - Uncoalesced Deltas
     - Implemented Architecture
     - Reduction
   * - Connect: Connecting Client
     - > 2,000 individual deltas
     - 3 bulk transfers
     - **> 99.8%**
   * - Connect: Peer Client Updates
     - > 2,000 individual deltas
     - 11–14 coalesced writes
     - **> 99.3%**
   * - Disconnect: Surviving Peer Updates
     - > 2,000 individual deltas
     - 10–12 coalesced writes
     - **> 99.4%**
   * - Total Session Lifecycle Writes
     - > 4,500 socket messages
     - 23–30 socket messages
     - **> 99.4%**
