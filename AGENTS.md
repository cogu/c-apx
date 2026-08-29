# Antigravity Workspace Guidelines & Context (`c-apx`)

This document provides project-wide context, architectural knowledge, coding conventions, and build instructions for Google Antigravity when working on `c-apx`.

---

## 1. Project Overview

`c-apx` is the C implementation of **APX** (AUTOSAR Port eXchange), a lightweight, low-latency streaming IPC/network middleware designed for automotive signals and embedded systems.

* **Target Version**: `v0.3.x` (Master branch / development).
* **Language Standard**: C99 (`set(CMAKE_C_STANDARD 99)`).
* **Architecture**: Client-Server with in-memory bytecode compilation and virtual machine execution for dynamic clients, alongside support for static clients and embedded targets (APX-ES).
* **Supported Platforms**: Linux (POSIX, GCC, Clang, Ninja, UNIX domain sockets, TCP/IP) and Windows (MSVC, Winsock2, TCP/IP).

---

## 2. Repository Structure & Subsystems

```text
c-apx/
├── apx_core/        # Core APX protocol, IDL parser, compiler, VM, file manager, node cache, event loops
├── extension/       # Server extension architecture
│   ├── apx_socket_extension/    # TCP/IP and UNIX domain socket listeners
│   ├── apx_text_log_extension/  # Server text logging extension
│   └── apx_monitor_extension/   # Server runtime monitoring extension
├── app/             # Executable applications
│   ├── apx_server/  # APX server daemon
│   ├── apx_node/    # APX node application
│   ├── apx_control/ # Interactive control CLI
│   └── apx_info/    # Server query and monitor client
├── adt/             # Abstract Data Types (dynamic arrays, hash tables, lists, ring buffers, strings)
├── bstr/            # Binary string utilities and byte-level manipulation
├── cutil/           # Cross-platform C utilities (endianness, osutil, memory leak instrumentation)
├── dtl_type/        # Dynamic Type Library (dtl_sv_t scalar, dtl_av_t array, dtl_hv_t hash/map)
├── dtl_json/        # JSON parser and serializer integrating with dtl_type
├── msocket/         # Socket abstraction layer (TCP, UNIX domain, client/server worker threads)
├── Testing/         # Unit testing framework based on CuTest (cutest)
├── cmake/           # CMake helper modules and extension generators
└── CMakePresets.json # Preset configurations for Clang, sanitizers, and static analysis
```

---

## 3. Architecture & Core Concepts

1. **Remote File Protocol (RMFP)**:
   * Data exchange occurs via memory-mapped virtual files (e.g. `NodeName.apx` definition files, `NodeName.out` signal data, `NodeName.in` input buffers).
   * File state and transfers are managed by `apx_fileManager_t`, `apx_file_t`, and `apx_remotefile_t`.
2. **Bytecode Compilation & Virtual Machine (VM)**:
   * `apx_compiler_t` compiles APX type and data definitions into bytecode programs (`apx_program_t`).
   * `apx_vm_t` executes these programs to serialize and deserialize structured C data to/from raw wire formats with low overhead.
3. **Data Types & Dynamic Structures**:
   * Standardized dynamic data containers (`dtl_sv_t`, `dtl_av_t`, `dtl_hv_t`) are used for configuration, schema inspection, and generic payload handling.
   * `adt` provides high-performance internal data structures (`adt_ary_t`, `adt_hash_t`, `adt_list_t`, `adt_rbfh_t`).
4. **Server & Extension Architecture**:
   * The APX server (`apx_server_t`) routes signal changes across connected client nodes.
   * Extensions register hooks into server lifecycles using `apx_serverExtension_t` and are dynamically linked or configured via `extensions_cfg.c`.

---

## 4. Build & Test Workflows

### Linux Workflows (Recommended)

#### Option A: Using CMake Presets (Clang 18 + Ninja)
```bash
# Debug build (Applications: apx_server, apx_node, apx_control)
cmake --preset clang-debug
cmake --build --preset clang-debug

# Release build
cmake --preset clang-release
cmake --build --preset clang-release

# Run Unit Tests (Target: apx_unit)
cmake --preset clang-test
cmake --build --preset clang-test
ctest --preset clang-test

# Address & Undefined Behavior Sanitizers (ASan + UBSan)
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

#### Option B: Using GCC / Manual CMake
```bash
# Build Applications
cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Build and Run Unit Tests
cmake -S . -B build-test -GNinja -DUNIT_TEST=ON
cmake --build build-test --target apx_unit
ctest --test-dir build-test --output-on-failure
```

### Windows Workflows (MSVC)
```cmd
:: Build Applications
cmake -S . -B build
cmake --build build --config Release

:: Build and Run Unit Tests
cmake -S . -B build-test -DUNIT_TEST=ON
cmake --build build-test --target apx_unit --config Debug
ctest --test-dir build-test -C Debug --output-on-failure
```

---

## 5. Coding Standards & Conventions

### C99 and Pointer Conventions
* Write strict, portable C99.
* Always use `NULL` for null pointers; never use legacy `0` or `(type*) 0`.
* Keep headers self-contained and guarded with `#ifndef / #define / #endif`.

### Memory Ownership & Allocation
* Structs generally follow the `_create()` / `_delete()` (heap) or `_init()` / `_destroy()` (stack/embedded) lifecycle pattern.
* Code must maintain zero memory leaks.
* Use `cutil_memleak.h` / `-DLEAK_CHECK=ON` and AddressSanitizer (`clang-asan`) to verify leak-free execution.

### Concurrency & Thread Safety
* APX uses POSIX threads (`pthread`) on Linux and Windows thread primitives.
* **Deadlock Prevention Rule**: Never invoke registered user callbacks or event listeners while holding an internal mutex lock.
  * **Pattern**: Take mutex $\to$ copy listener function pointers / state snapshot into local variables $\to$ release mutex $\to$ invoke callback.

### Cross-Platform POSIX / Windows Support
* Use `msocket` abstraction for all networking (do not invoke raw BSD / Winsock APIs directly in application code).
* Path separators and directory utilities must handle both forward slash `/` and backslash `\`.
* Time routines: Use `clock_gettime(CLOCK_REALTIME)` on Linux and `GetSystemTimePreciseAsFileTime` on Windows.

---

## 6. Testing Practices

* Unit tests are built with `cutest` (`Testing/`).
* Common test suites live in `apx_core/test/`, `extension/*/test/`, and `app/*/test/`.
* Test files are named `testsuite_<module>.c` and registered in `CMakeLists.txt` (`APX_COMMON_TEST_SUITE` or `apx_unit`).
* New features, protocol handlers, and bug fixes must include accompanying CuTest test suites.
