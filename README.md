# APX for C

This is the C implementation of [APX](https://apx.readthedocs.io).

Online documentation and API reference: **[c-apx.readthedocs.io](https://c-apx.readthedocs.io/)**

## v0.2.x (Stable)

All maintenance work for v0.2 is done on the [maintenance_0.2](https://github.com/cogu/c-apx/tree/maintenance_0.2) branch.
Latest release is [v0.2.9](https://github.com/cogu/c-apx/releases/tag/v0.2.9).

## v0.3.x (Development)

A brand new implementation is being developed on branch `master`. It will remain in alpha stage until release v0.4.0.

### APX Core Features

Reference implementation of APX IDL v1.3 and APX VM v2.1.

- Parser
- Bytecode compiler
- Virtual Machine
- Client

### APX Server Base Features

Reference implementation of the [APX server](https://c-apx.readthedocs.io/en/latest/apps/apx_server.html). It is highly configurable using a single JSON file.

**Optional features:**

- **Node definition caching**: Disk-backed Level 1 (`.apx` text) and Level 2 (`.apxc` precompiled binary) caching keyed by SHA-256 to eliminate definition transfer and compilation on reconnect.
- **Cryptographically signed APX node verification** (NIST P-256 with SHA-256):
  - Use the tool [`apx-sign`](https://c-apx.readthedocs.io/en/latest/apps/apx_sign.html) to create keys and sign your APX nodes.

### APX Server Extensions

Socket support, logging, and more are enabled in the APX server by adding compile-time extensions.
See the [Server Extensions documentation](https://c-apx.readthedocs.io/en/latest/extensions/index.html) for the full extension catalog, default settings, and configuration details.

## Dynamic vs. Static Clients

### Static Clients (not yet supported)

Static clients use a code generator (see [Python APX](https://github.com/cogu/py-apx)) to generate C code from APX definition files.
The generated code is fast and integrates well with type definitions shared with an AUTOSAR RTE generator.
Statically generated clients are supposed to be used together with APX-ES in order to run on small devices that run an RTOS.

### Dynamic Clients (supported)

Dynamic clients parse an APX definition file at runtime and build small bytecode programs (in-memory) which then execute through a virtual machine (VM). This method has more flexibility since it doesn't require C code to be generated or compiled as an intermediate step.
Client-side caching mechanisms are currently being developed for C and C++ (more information later).
Dynamic clients are best used on Windows and Linux systems.

## Building with CMake

`c-apx` provides CMake support for both Windows (MSVC) and Linux (GCC and Clang).

### Build Targets and Modes

- **Applications**: Standard builds compile the core library (`apx_core`), enabled extensions, and the executable applications in `app/`:
  - [`apx-server`](https://c-apx.readthedocs.io/en/latest/apps/apx_server.html): The APX server daemon.
  - [`apx-node`](https://c-apx.readthedocs.io/en/latest/apps/apx_node.html): APX node application.
  - [`apx-control`](https://c-apx.readthedocs.io/en/latest/apps/apx_control.html): APX control application.
  - [`apx-mon`](https://c-apx.readthedocs.io/en/latest/apps/apx_mon.html): Interactive diagnostic and monitoring client.
  - [`apx-log`](https://c-apx.readthedocs.io/en/latest/apps/apx_log.html): Event stream recording and inspection utility.
  - [`apx-perf-test`](https://c-apx.readthedocs.io/en/latest/apps/apx_perf_test.html): Dedicated APX performance test.
  - [`apx-sign`](https://c-apx.readthedocs.io/en/latest/apps/apx_sign.html): Utility for signing APX nodes using ECDSA.

  Applications can be built in either `Debug` or `Release` configuration.

- **Unit Tests**: A dedicated unit test suite target `apx_unit` is available.
  - Enabled by setting `-DUNIT_TEST=ON` at CMake configure time.
  - Built specifically via `--target apx_unit` and executed with `ctest`.

### Linux CMake Presets (Clang 21 + Ninja)

```bash
# Run unit tests
cmake --preset clang-test && cmake --build --preset clang-test && ctest --preset clang-test

# Address and Undefined Behavior Sanitizers (ASan + UBSan)
# Note: This preset requires a preconfigured pytest virtual environment
cmake --preset clang-asan && cmake --build --preset clang-asan && ctest -V --preset clang-asan

# Thread Sanitizer (TSan)
# Note: This preset requires a preconfigured pytest virtual environment
cmake --preset clang-tsan && cmake --build --preset clang-tsan && ctest -V --preset clang-tsan

# Static Analysis
cmake --preset clang-tidy && cmake --build --preset clang-tidy

# Debug build
cmake --preset clang-debug && cmake --build --preset clang-debug

# Release build
cmake --preset clang-release && cmake --build --preset clang-release
```

### Installing on Linux

Build with static libraries for a standalone installation:

```bash
cmake --preset clang-release -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=OFF
cmake --build --preset clang-release
sudo cmake --install build/clang-release

# Check installed version
apx-server --version
```

### Windows CMake Presets (Visual Studio 2026)

Run these commands from an **x64 Native Tools Command Prompt** for Visual Studio.

```powershell
# Unit test build
cmake --preset msvc-test
cmake --build --preset msvc-test
ctest --preset msvc-test

# Debug build
cmake --preset msvc-debug
cmake --build --preset msvc-debug

# Release build
cmake --preset msvc-release
cmake --build --preset msvc-release

# Static analysis
cmake --preset msvc-analyze
cmake --build --preset msvc-analyze
```

### Manual CMake Workflow with Linux and GCC

```bash
cmake -S . -B build-test -GNinja -DUNIT_TEST=ON
cmake --build build-test
ctest --test-dir build-test --output-on-failure
```

### Running pytests (Linux Only)

See [tests/README.md](tests/README.md) for virtual environment setup.

Requires either `clang-debug` or `clang-release` presets to have been prebuilt.

```bash
pytest -v -n auto
```
