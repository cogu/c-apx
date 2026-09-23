Internal Design: Files and Memory Layout
=========================================

This document describes the internal architecture of virtual files in APX, their memory layout, and the Remote Memory File Protocol (RMFP).

Overview
--------

APX exchanges all signal data and definition schemas using memory-mapped virtual files. Rather than relying on traditional message queues or remote procedure calls (RPC), APX models an automotive or embedded software component as a collection of virtual files exposed over a transport stream (TCP/IP or UNIX domain socket).

There are four primary data files associated with each APX node:

* ``.apx``: Node definition file (APX IDL).
* ``.out``: Provide port data file (outgoing signals).
* ``.in``: Require port data file (incoming signals).
* ``.cout``: Provide port connection count file (active subscriber counts).
* ``.cin``: Require port connection count file (active provider counts).

Remote File Protocol (RMFP) Address Space
-----------------------------------------

The Remote Memory File Protocol reserves a 30-bit virtual address space (up to 1 GB), partitioned into dedicated functional regions:

.. list-table::
   :header-rows: 1
   :widths: 25 20 20 35

   * - Region
     - Start Address
     - Alignment
     - Description
   * - **Port Data**
     - ``0x00000000``
     - 1 KB (``0x400``)
     - Virtual port data files (``.out``, ``.in``).
   * - **Definition**
     - ``0x04000000``
     - 256 KB (``0x40000``)
     - APX IDL specification text files (``.apx``).
   * - **Port Count**
     - ``0x08000000``
     - 1 KB (``0x400``)
     - Dynamic port counter files (``.cout``, ``.cin``).
   * - **User Defined**
     - ``0x20000000``
     - 4 KB (``0x1000``)
     - Custom extension files or diagnostics.
   * - **Command Area**
     - ``0x3FFFFC00``
     - N/A
     - 1 KB reserved for RMF control commands (open, close, publish, revoke).

Address Assignment & Flags
~~~~~~~~~~~~~~~~~~~~~~~~~~

* **Address Bit 31 (``RMF_REMOTE_ADDRESS_BIT`` / ``0x80000000``)**: Distinguishes between local and remote file addresses within file maps.
* **Invalid Address (``0x7FFFFFFF``)**: Marks unassigned or dynamic file addresses before publication.

File Types and Specifications
-----------------------------

Definition File (``.apx``)
~~~~~~~~~~~~~~~~~~~~~~~~~~

* **Naming Convention**: ``<NodeName>.apx``
* **Role**: Publishes the APX IDL text declaring node name, type definitions, provide ports, and require ports.
* **Creator / Owner**: Client node runtime.
* **Direction**: Client publishes; Server subscribes.
* **Format**: Plaintext UTF-8 / ASCII text following the APX IDL specification (v1.2 or v1.3).

Output Signal Data (``.out``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* **Naming Convention**: ``<NodeName>.out``
* **Role**: Memory buffer containing the current values of all provide ports (output signals).
* **Creator / Owner**: Client node runtime.
* **Direction**: Client creates and publishes as a local file; Server subscribes as a remote file.
* **Layout**:
  * Contiguous byte buffer packed according to the compiled APX bytecode programs.
  * Byte offsets for each provide port are calculated sequentially during node building.
  * When a provide port value is updated by the application, the client sends a write message containing only the modified byte slice.

Input Signal Data (``.in``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

* **Naming Convention**: ``<NodeName>.in``
* **Role**: Memory buffer containing the current values of all require ports (input signals).
* **Creator / Owner**: APX Server.
* **Direction**: Server creates and publishes as a local file; Client opens and subscribes as a remote file.
* **Layout**:
  * Contiguous byte buffer matching the compiled layout of all require ports for the node.
  * When the server routes signal updates from a provider node, it writes the updated bytes into the corresponding require port offset of this file and streams the update to the client.

Provide Port Connection Counters (``.cout``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* **Naming Convention**: ``<NodeName>.cout``
* **Availability**: APX IDL v1.3+ only.
* **Role**: Informs provider nodes how many consumers are actively connected to each provide port.
* **Creator / Owner**: APX Server.
* **Direction**: Server creates and publishes as a local file; Client opens and subscribes as a remote file.
* **Layout**:
  * Array of 16-bit unsigned integers in little-endian byte order (``uint16_le``).
  * Element count equals the number of provide ports :math:`N`.
  * Total file size: :math:`2 \times N` bytes.
  * The :math:`i`-th element (offset :math:`2 \times i`) represents the connection count for provide port ID :math:`i`.
* **Behavior & Usage**:
  * When a client connects or disconnects, the server updates its internal connector table and sends a 2-byte write to the corresponding port offset in ``<NodeName>.cout``.
  * Provider applications can inspect this count (e.g. via ``apx_node_data_get_provide_port_connection_count``) to determine if any consumer is listening. If count is 0, signal generation can be throttled or suspended to conserve system resources.

Require Port Connection Counters (``.cin``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* **Naming Convention**: ``<NodeName>.cin``
* **Availability**: APX IDL v1.3+ only.
* **Role**: Informs requester nodes whether a provide port is actively routing data into each require port.
* **Creator / Owner**: APX Server.
* **Direction**: Server creates and publishes as a local file; Client opens and subscribes as a remote file.
* **Layout**:
  * Array of 16-bit unsigned integers in little-endian byte order (``uint16_le``).
  * Element count equals the number of require ports :math:`M`.
  * Total file size: :math:`2 \times M` bytes.
  * The :math:`j`-th element (offset :math:`2 \times j`) represents the connection count for require port ID :math:`j`.
* **Behavior & Usage**:
  * Standard 1-to-1 connections will show count 1 when connected and 0 when disconnected.
  * In configurations with multiple providers or failover sources, the count can exceed 1.

Backwards Compatibility & Version Negotiation
---------------------------------------------

To maintain full interoperability across heterogeneous client versions:

1. **APX IDL v1.2 Nodes**:
   * Older clients (such as ``py-apx`` :math:`\le` 0.4.5 or legacy ``c-apx`` v0.2.x) declare ``APX/1.2`` at the start of their definition files.
   * For any node declaring ``APX/1.2``, the APX Server **never** creates or publishes ``.cout`` or ``.cin`` files.
   * This prevents legacy clients from encountering unknown virtual files.

2. **APX IDL v1.3+ Nodes**:
   * Nodes declaring ``APX/1.3`` (or higher) enable connection counter support.
   * The server automatically publishes ``<NodeName>.cout`` if the node declares provide ports, and ``<NodeName>.cin`` if the node declares require ports.
