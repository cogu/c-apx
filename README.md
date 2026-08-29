# APX for C

This is the C implementation of [APX](https://cogu.github.com/apx).

## v0.2.x (Stable)

All maintenance work for v0.2 is done on the [maintenance_0.2](https://github.com/cogu/c-apx/tree/maintenance_0.2) branch.
Latest release is [v0.2.8](https://github.com/cogu/c-apx/releases/tag/v0.2.8).

## v0.3.x (Development)

A brand new implementation is being developed on master branch. Its current state is experimental at best.

### Current implementation status ([v0.3.2](https://github.com/cogu/c-apx/releases/tag/v0.3.2))

- New APX server and client with support for APX IDL v1.3 (upcoming APX specification).
  - Native support for TCP/IP and UNIX sockets.
  - Possible to extend the server with custom extensions to allow new connection types.
- Dynamic clients are fully supported.
- APX-ES clients not yet supported (needs rewrite from v0.2 to v0.3).
- Static clients not yet supported (needs rewrite from v0.2 to v0.3).
- CMake build support for Linux and Windows.
- Visual Studio 2019 projects for Windows.

## Dynamic vs. Static Clients

### Static Clients (not yet supported)

Static clients uses a code generator (see [Python APX](https://github.com/cogu/py-apx)) to generate C code from APX definitions files.
The generated code is fast and integrates well with type definitions shared with an AUTOSAR RTE generator.
Statically generated clients are supposed to be used together with APX-ES in order to run on small devices that run an RTOS.

### Dynamic Clients (supported)

Dynamic clients parses an APX definition file in runtime and builds small byte code programs (in-memory) which then executes through a virtual machine (VM). This method has more flexibility since it doesn't require C code to be generated or compiled as an intermediate step.
Caching mechanisms are currently being developed for C and C++ (More information later).
Dynamic clients is best used on Windows and Linux systems.

## What is APX?

APX (AUTOSAR Port eXchange) is a software solution designed for the automotive industry. It is used to stream automotive signals in real-time
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

Note that APX-ES on master branch is not yet updated to work with the new v0.3.x code base.

## Building with CMake

`c-apx` provides CMake support for both Windows (MSVC) and Linux (GCC and Clang).

### Build Targets and Modes

- **Applications**: Standard builds compile the core library (`apx_core`), enabled extensions, and the executable applications in `app/`:
  - `apx_server`: The APX server daemon.
  - `apx_node`: APX node application.
  - `apx_control`: APX control application.

  Applications can be built in either `Debug` or `Release` configuration.

- **Unit Tests**: A dedicated unit test suite target `apx_unit` is available.
  - Enabled by setting `-DUNIT_TEST=ON` at CMake configure time.
  - Built specifically via `--target apx_unit` and executed with `ctest`.

---

### Building on Windows (Visual Studio / MSVC)

Open the **x64 Native Tools Command Prompt for Visual Studio** (e.g. Visual Studio 2019 or later) to ensure CMake configures the MSVC compiler environment.

#### 1. Building Applications

**Configure:**

```cmd
cmake -S . -B build
```

**Build (Release):**

```cmd
cmake --build build --config Release
```
*(Or use `--config Debug` for a debug build).*


#### 2. Building and Running Unit Tests

**Configure with unit tests enabled:**

```cmd
cmake -S . -B build-test -DUNIT_TEST=ON
```

**Build test target:**

```cmd
cmake --build build-test --target apx_unit --config Debug
```

**Run unit tests:**

```cmd
ctest --test-dir build-test -C Debug --output-on-failure
```
*(Or use `--config Release` / `-C Release` for release configuration).*

---

### Building on Linux

#### 1. Using GCC

##### Building Applications

**Configure:**

```bash
cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Release
```

If you plan to run the install target and want binaries installed to `/usr/bin` instead of `/usr/local/bin`:

```bash
cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr
```

**Build:**

```bash
cmake --build build
```

**Install binaries (optional):**

```bash
sudo cmake --build build --target install
```

##### Building and Running Unit Tests

**Configure:**

```bash
cmake -S . -B build-test -GNinja -DUNIT_TEST=ON
```

**Build `apx_unit`:**

```bash
cmake --build build-test --target apx_unit
```

**Run unit tests:**

```bash
ctest --test-dir build-test --output-on-failure
```

---

#### 2. Using Clang

`c-apx` includes CMake Presets (`CMakePresets.json`) targeting Clang 18 with Ninja, as well as support for direct CMake invocations.

##### Using CMake Presets (Recommended)

**Building Applications (Debug or Release):**

```bash
# Debug build (apx_server, apx_node, apx_control)
cmake --preset clang-debug
cmake --build --preset clang-debug

# Release build
cmake --preset clang-release
cmake --build --preset clang-release
```

The debug preset also generates a compilation database at `build/clang-debug/compile_commands.json` for `clangd` / IDE integration.

**Running Unit Tests & Sanitizers:**

```bash
# Standard Unit Tests
cmake --preset clang-test
cmake --build --preset clang-test
ctest --preset clang-test

# Address and Undefined Behavior Sanitizers (ASan + UBSan)
cmake --preset clang-asan
cmake --build --preset clang-asan
ctest --preset clang-asan

# Thread Sanitizer (TSan)
cmake --preset clang-tsan
cmake --build --preset clang-tsan
ctest --preset clang-tsan

# Static Analysis (Clang-Tidy)
cmake --preset clang-tidy
cmake --build --preset clang-tidy
```

##### Using Direct CMake Commands

**Building Applications:**

```bash
cmake -S . -B build-clang -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -GNinja
cmake --build build-clang
```

**Building & Running Unit Tests:**

```bash
cmake -S . -B build-clang-test -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DUNIT_TEST=ON -GNinja
cmake --build build-clang-test --target apx_unit
ctest --test-dir build-clang-test --output-on-failure
```

