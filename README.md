# APX for C

This is the C implementation of [APX](https://github.com/cogu/apx-doc).

## v0.2.x (Stable)

All maintenance work for v0.2 is done on the [maintenance_0.2](https://github.com/cogu/c-apx/tree/maintenance_0.2) branch.
Latest release is [v0.2.6](https://github.com/cogu/c-apx/releases/tag/v0.2.6).

## v0.3.x (In development)

All development work for v0.3 is done on the master branch. As such, the master branch should for now be considered
highly experimental.

### Current status of master branch (v0.3.0a)

- Full support for APX v1.2 specification.
- APX server should have the same feature set as seen in [v0.2.6](https://github.com/cogu/c-apx/releases/tag/v0.2.6).
- CMake build support for Linux and Windows.
- Dynamic clients is fully supported.
- APX-ES clients not yet supported (needs rewrite from v0.2 to v0.3).
- Static clients not yet supported (needs rewrite from v0.2 to v0.3).

## Dynamic vs. Static Clients

### Static Clients

Static clients uses a code generator (see [Python APX](https://github.com/cogu/py-apx)) to generate C code from APX definitions files.
The generated code is fast and integrates well with type definitions shared with an AUTOSAR RTE generator.

### Dynamic Clients

Dynamic clients parses an APX definition file in runtime and builds small byte code programs (in-memory) which then executes through a virtual machine (VM). This method has more flexibility since it doesn't require C code to be generated or compiled as an intermediate step.

## What is APX?

APX is a software solution designed for the automotive industry. It is used to stream automotive signals in real-time
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

## Building with CMake

CMake build has been tested for Windows (with Visual Studio) as well as Linux (with GCC).

### Running unit tests (Linux)

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

### Running unit tests (Windows with Visual Studio)

Use a command prompt provided by your Visual Studio installation.
For example, I use "x64 Native Tools Command Prompt for VS2019" which is found on the start menu.
It conveniently comes with CMake pre-installed which generates Visual Studio projects by default.

Configure:

```cmd
cmake -S . -B VisualStudio -DUNIT_TEST=ON
```

Build:

```cmd
cmake --build VisualStudio --config Debug --target apx_unit
```

Run test cases:

```cmd
cd VisualStudio && ctest
```