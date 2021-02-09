# APX for C

This is the C implementation of [APX](https://cogu.github.com/apx).

## v0.2.x (Stable)

All maintenance work for v0.2 is done on the [maintenance_0.2](https://github.com/cogu/c-apx/tree/maintenance_0.2) branch.
Latest release is [v0.2.8](https://github.com/cogu/c-apx/releases/tag/v0.2.8).

## v0.3.x (Experimental)

A brand new implementation is being developed on master branch. Its current state is experimental at best.

- New APX server and client with support for APX IDL v1.3 (upcoming specification).
  - Native support for TCP/IP and UNIX sockets.
  - Extend the server with custom extensions to allow new connection types.
- Dynamic client fully supported.
- APX-ES clients not yet supported (needs rewrite from v0.2 to v0.3).
- Static clients not yet supported (needs rewrite from v0.2 to v0.3).
- CMake build support for Linux and Windows.
- Visual Studio 2019 projects for Windows.
- Eclipse projects for Linux.

## Dynamic vs. Static Clients

### Static Clients

Static clients uses a code generator (see [Python APX](https://github.com/cogu/py-apx)) to generate C code from APX definitions files.
The generated code is fast and integrates well with type definitions shared with an AUTOSAR RTE generator.
Statically generated clients are supposed to be used together with APX-ES in order to run on small devices that run an RTOS.

### Dynamic Clients

Dynamic clients parses an APX definition file in runtime and builds small byte code programs (in-memory) which then executes through a virtual machine (VM). This method has more flexibility since it doesn't require C code to be generated or compiled as an intermediate step.
Caching mechanisms are currently being developed for C and C++ (More information later).
Dynamic clients is best used on Windows and Linux systems.

## What is APX?

APX is a software solution designed for the automotive industry. It is used to stream automotive signals in real-time
to (or from) Linux or Windows systems. APX is designed to work well for high-frequency signal changes (low latency updates) over short distances (SPI buses, local area ethernet or local host).

APX is a client-server solution. In a typical setup, one apx-server instance executes on Linux (or Windows).
APX clients connects to the server instance and starts streaming signal changes (to the server).
The server automatically builds data routing tables and continously forwards the signal changes
sent from client output (or provide) port(s) to client input (or require) port(s).

## Where can APX be used?

APX can be integrated on systems that run AUTOSAR classic (see APX-ES) as well as any Linux or Windows systems.

APX clients can be implemented in any programming language and can run on any platform. The APX protocol is designed to work well on small embedded systems (where RAM and ROM availability is a usual constraint).

## What is APX-ES?

APX for embedded systems (APX-ES) is client source code written in C for very small devices.

- It does not require an operating system (Using a small RTOS is recommended)
- It does not require any heap memory.
- It is intended to be MISRA-compliant (at some point).

Note that APX-ES on master branch is not yet updated to work with the new v0.3.x code base (that work will start soon).

## Building with CMake

CMake build has been tested for Windows (with Visual Studio) as well as Linux (with GCC).

For Windows, use a "Native tools command prompt" provided by your Visual Studio installation. It comes with a cmake binary that
by default chooses the right version of Visusl Studio compiler.

### Running unit tests (Linux and Windows)

Configure:

```sh
cmake -S . -B build -DUNIT_TEST=ON
```

Build:

```sh
cmake --build build --target apx_unit
```

Run test cases:

```cmd
cd build && ctest
```

### Building APX Binaries (Linux)

**Configure:**

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

Option 1: If you're going to run install target and want the binaries to end up in `/usr/bin` instead of `/usr/local/bin`:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr
```

Option 2: if you have ninja-build installed (recommended) remember to add the `-GNinja` argument during configuration.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -GNinja
```

**Build binaries:**

```sh
cmake --build build
```

**Install binaries:**

Also builds binaries if not previously built.

```sh
sudo cmake --build build --target install
```

### Building APX Binaries (Windows and Visual Studio)

Launch "x64 Native Tools Command Prompt for Visual Studio 2019" from start menu.

**Configure:**

```cmd
cmake -S . -B build
```

**Build binaries:**

```cmd
cmake --build build --config Release
```

**Install binaries:**

Run with admininistrative privilege.

```cmd
cmake --build build --target install
```
