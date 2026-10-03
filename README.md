# APX for C

This is the C implementation of [APX](https://apx.readthedocs.io).

Online documentation and API reference: **[c-apx.readthedocs.io](https://c-apx.readthedocs.io/)**

## v0.2.x (Stable)

All maintenance work for v0.2 is done on the [maintenance_0.2](https://github.com/cogu/c-apx/tree/maintenance_0.2) branch.
Latest release is [v0.2.8](https://github.com/cogu/c-apx/releases/tag/v0.2.8).

## v0.3.x (Development)

A brand new implementation is being developed on branch `master`. It will remain in alpha stage until release v0.4.0.

### Current implementation status

#### APX Core Features

- Reference implementation of APX IDL v1.3 and APX VM v2.1.
  - Parser
  - Compiler
  - Virtual Machine
- `New:` Port connection count support (server and client).
- Dynamic client support
  - Use `apx-node` together with `apx-control` to dynamically create APX nodes.

#### APX Server Base Features

Reference implementation of the APX server. It is highly configurable using a single JSON file.

**Security features (optional):**

- `New:` Cryptographically signed APX node verification (NIST P-256 with SHA-256)
  - Use the tool `apx-sign` to create keys and sign your APX nodes.

#### APX Server Extensions

Socket support, logging, and more are enabled in the APX server by adding compile-time extensions.
Each extension can be individually configured using JSON (including enable/disable options).

##### Socket Extension

Basic socket server support.

- TCP sockets.
- UNIX Domain sockets.
- `New:` Virtual Machine Sockets (VSOCK).

Default settings:

- Listening port: `5000`
- Listening file path: `/tmp/apx.socket`

Extension Type: `Source`

##### TLS Extension

`New:` TLS socket server.

Default settings:

- Listening port: `5020`

Extension Type: `Source`

##### TextLog Extension

Demonstrates the built-in event listener mechanism in `apx_core` and logs server events to stdout or a log file.

Default settings:

- Logs to stdout.

Extension Type: `Sink`

##### Monitor Extension

An unfinished extension that has the ambition of streaming real-time APX event data to remote loggers.
It currently does nothing.

Extension Type: `Sink`

#### Features not yet started

- APX-ES clients not yet supported (needs a rewrite from v0.2).
- Static clients not yet supported (needs a rewrite from v0.2).


## Dynamic vs. Static Clients

### Static Clients (not yet supported)

Static clients use a code generator (see [Python APX](https://github.com/cogu/py-apx)) to generate C code from APX definition files.
The generated code is fast and integrates well with type definitions shared with an AUTOSAR RTE generator.
Statically generated clients are supposed to be used together with APX-ES in order to run on small devices that run an RTOS.

### Dynamic Clients (supported)

Dynamic clients parse an APX definition file at runtime and build small bytecode programs (in-memory) which then execute through a virtual machine (VM). This method has more flexibility since it doesn't require C code to be generated or compiled as an intermediate step.
Caching mechanisms are currently being developed for C and C++ (more information later).
Dynamic clients are best used on Windows and Linux systems.

## Building with CMake

`c-apx` provides CMake support for both Windows (MSVC) and Linux (GCC and Clang).

### Build Targets and Modes

- **Applications**: Standard builds compile the core library (`apx_core`), enabled extensions, and the executable applications in `app/`:
  - `apx-server`: The APX server daemon.
  - `apx-node`: APX node application.
  - `apx-control`: APX control application.
  - `apx-perf-test`: Dedicated APX performance test.
  - `apx-sign`: Utility for signing APX nodes using ECDSA.

  Applications can be built in either `Debug` or `Release` configuration.

- **Unit Tests**: A dedicated unit test suite target `apx_unit` is available.
  - Enabled by setting `-DUNIT_TEST=ON` at CMake configure time.
  - Built specifically via `--target apx_unit` and executed with `ctest`.

For Windows, use a "Native tools command prompt" from your Visual Studio installation.

### Linux CMake Presets (Clang 21 + Ninja)

```bash
# Run unit tests
cmake --preset clang-test && cmake --build --preset clang-test && ctest --preset clang-test

# Address and Undefined Behavior Sanitizers (ASan + UBSan)
# Note: This preset requires a preconfigured pytest virtual environment
cmake --preset clang-asan && cmake --build --preset clang-asan && ctest -V --preset clang-asan

# Thread Sanitizers (TSan)
# Note: This preset requires a preconfigured pytest virtual environment
cmake --preset clang-tsan && cmake --build --preset clang-tsan && ctest -V --preset clang-tsan

# Static Analysis
cmake --preset clang-tidy && cmake --build --preset clang-tidy

# Debug build
cmake --preset clang-debug && cmake --build --preset clang-debug

# Release build
cmake --preset clang-release && cmake --build --preset clang-release
```

### Windows CMake Presets (Visual Studio 2026)

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
```

### Manual CMake Workflow with Linux and GCC

```sh
cmake -S . -B build-test -GNinja -DUNIT_TEST=ON
cmake --build build-test
ctest --test-dir build-test --output-on-failure
```

#### Running pytests (Linux Only)

See [tests/README.md](tests/README.md) for virtual environment setup.

Requires either `clang-debug` or `clang-release` presets to have been prebuilt.

```bash
pytest -v -n auto
```
