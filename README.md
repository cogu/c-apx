# APX for C

This is the C implementation of [APX](https://apx.readthedocs.io).

Online documentation and API reference: **[c-apx.readthedocs.io](https://c-apx.readthedocs.io/)**

## v0.2.x (Stable)

All maintenance work for v0.2 is done on the [maintenance_0.2](https://github.com/cogu/c-apx/tree/maintenance_0.2) branch.
Latest release is [v0.2.8](https://github.com/cogu/c-apx/releases/tag/v0.2.8).

## v0.3.x (Development)

A brand new implementation is being developed on master branch. Its current state is in pre-alpha stage.

### Current implementation status

- New APX server and client with support for APX IDL v1.3.
  - Native support for TCP/IP and UNIX sockets.
  - Possible to extend the server with custom extensions to allow new connection types.
- Dynamic clients are fully supported.
- APX-ES clients not yet supported (needs rewrite from v0.2).
- Static clients not yet supported (needs rewrite from v0.2).
- CMake build support for Linux and Windows.

## Dynamic vs. Static Clients

### Static Clients (not yet supported)

Static clients uses a code generator (see [Python APX](https://github.com/cogu/py-apx)) to generate C code from APX definitions files.
The generated code is fast and integrates well with type definitions shared with an AUTOSAR RTE generator.
Statically generated clients are supposed to be used together with APX-ES in order to run on small devices that run an RTOS.

### Dynamic Clients (supported)

Dynamic clients parses an APX definition file in runtime and builds small byte code programs (in-memory) which then executes through a virtual machine (VM). This method has more flexibility since it doesn't require C code to be generated or compiled as an intermediate step.
Caching mechanisms are currently being developed for C and C++ (More information later).
Dynamic clients is best used on Windows and Linux systems.

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

## Building with CMake

For Windows, use a "Native tools command prompt" from your Visual Studio installation.

### Using CMake Presets (Clang 18 + Ninja)

```bash
# Run unit tests
cmake --preset clang-test
cmake --build --preset clang-test
ctest --preset clang-test

# Address and Undefined Behavior Sanitizers (ASan + UBSan)
cmake --preset clang-asan
cmake --build --preset clang-asan
ctest --preset clang-asan

# Static Analysis
cmake --preset clang-tidy
cmake --build --preset clang-tidy

# Debug build
cmake --preset clang-debug
cmake --build --preset clang-debug

# Release build
cmake --preset clang-release
cmake --build --preset clang-release
```

### Manual CMake Workflows (Linux and Windows)

For Windows, use a "Native tools command prompt" from your Visual Studio installation. It comes with a cmake binary that
by default chooses the appropriate compiler version.

#### Running unit tests

Configure:

```sh
cmake -S . -B build-test -GNinja -DUNIT_TEST=ON
```

Build:

```sh
cmake --build build-test
```

Run test cases:

```sh
ctest --test-dir build-test --output-on-failure
```

##### Release Build

**Configure:**

```bash
cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Release
```

**Build:**

```bash
cmake --build build
```

#### Release build with install

If you plan to run the install target and want binaries installed to `/usr/bin` instead of `/usr/local/bin`:

```bash
cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr
sudo cmake --build build --target install
```
