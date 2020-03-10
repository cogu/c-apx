# APX for C

This is the C implementation of [APX](https://github.com/cogu/apx-doc).

## v0.2.x (Stable)

All maintenance work for v0.2 is done on the [maintenance_0.2](https://github.com/cogu/c-apx/tree/maintenance_0.2) branch.
Latest release is [v0.2.5](https://github.com/cogu/c-apx/releases/tag/v0.2.5).

## v0.3.x (In development)

All development work for v0.3 is done on the master branch. As such, the master branch should for now be considered
highly experimental.

### Current status of master branch (v0.3.0a)

- apx-server should be considered "minimum viable product".
- GCC support only (Visual Studio project is currently out of sync).
- APX-ES is not yet supported (needs refactoring/rework).
- Dynamic nodes only (code-generated nodes not yet implemented).
- Limited data type support:
  - uint8
  - uint16
  - uint32
  - uint8 array (fixed length)
  - uint16 array (fixed length)
  - uint32 array (fixed length)

## What is APX?

APX is a software technology designed for the automotive industry. It is used to stream automotive signals in real-time
to (or from) Linux or Windows systems. APX is designed to work well for high-frequency signal changes (low latency updates) over short distances (SPI buses, local area ethernet or local host).

APX is a client-server solution. In a typical setup, one apx-server instance executes on Linux (or Windows).
APX clients connects to the server instance and starts streaming signal changes (to the server).
The server automatically builds data routing tables and continously forwards the signal changes
sent from client output (or provide) port(s) to client input (or require) port(s).

## Where can APX be used?

APX can be integrated on systems that run AUTOSAR classic (see APX-ES) as well as on any Linux or Windows system.

APX clients can be implemented in any programming language and can run on any platform. The APX protocol is designed to work well on small embedded systems (where RAM and ROM availability is a usual constraint).

## What is APX-ES?

APX for embedded systems (APX-ES) is client source code written in C for very small devices.

- It does not require an operating system (Using a small RTOS is recomended)
- It does not require any heap memory.
- It is intended to be MISRA-compliant (at some point).

Note that APX-ES on master branch is not yet updated to work with the new v0.3.x code base (that work will start soon).
