# Antigravity Workspace Guidelines & Context (`c-apx`)

Project context, architectural knowledge, coding conventions, and developer workflows for `c-apx`.

---

## 1. Project Overview

`c-apx` is the C reference implementation of [APX](https://github.com/cogu/apx) (AUTOSAR Port eXchange), a lightweight, low-latency streaming IPC and network middleware for automotive signals and embedded systems.

* **Target Version**: `v0.3.x` (Master branch / development).
* **Language Standard**: Strict C99 (`set(CMAKE_C_STANDARD 99)`).
* **Role in APX Ecosystem**: Reference implementation of APX; implements both client runtimes and the only APX server daemon (`apx_server`). Other implementations (Python, C++) only implement client runtimes.
* **Specification Support**: Conforms to APX IDL v1.3 and APX VM v2.1.
* **Primary Platform**: Linux (POSIX, Clang, Ninja, UNIX domain sockets, TCP/IP).

---

## 2. Repository Structure

```text
c-apx/
├── apx_core/        # Core APX protocol, IDL parser, compiler, VM, file manager, node cache
├── extension/       # Server extension architecture
│   ├── apx_socket_extension/    # TCP/IP and UNIX domain socket listeners
│   ├── apx_text_log_extension/  # Server text logging extension
│   └── apx_monitor_extension/   # Server runtime monitoring extension
├── app/             # Executable applications
│   ├── apx_server/     # APX server daemon
│   ├── apx_node/       # APX node runtime
│   ├── apx_control/    # Interactive CLI control utility
│   ├── apx_info/       # Server query and monitoring client
│   └── apx_perf_test/  # Performance benchmarking tool
├── tests/           # Integration & end-to-end tests (pytest + process fixtures)
├── docs/            # Sphinx + Furo + Doxygen documentation
├── adt/             # Submodule: Abstract Data Types (arrays, hash tables, lists, strings)
├── bstr/            # Submodule: Bounded binary string utilities
├── cutil/           # Submodule: C utilities & CuTest testing framework
├── dtl_type/        # Submodule: Dynamic Type Library (dtl_sv_t, dtl_av_t, dtl_hv_t)
├── dtl_json/        # Submodule: JSON parser and serializer
├── msocket/         # Submodule: Socket abstraction layer (TCP, UNIX domain, worker threads)
├── cmake/           # CMake modules and configuration
└── CMakePresets.json # Presets for Clang, Ninja, sanitizers, and clang-tidy
```

---

## 3. Architecture & Core Concepts

1. **Remote File Protocol (RMFP)**:
   * Data exchange occurs via memory-mapped virtual files (`NodeName.apx` definition, `NodeName.out` output signals, `NodeName.in` input signals).
   * Managed by `apx_fileManager_t`, `apx_file_t`, and `apx_remotefile_t`.
2. **Bytecode Compilation & Virtual Machine (VM)**:
   * `apx_compiler_t` compiles APX type and data definitions into compact bytecode programs (`apx_program_t`).
   * `apx_vm_t` executes these bytecode programs to serialize and deserialize structured C data with zero code generation overhead.
3. **Data Types & Dynamic Structures**:
   * Standardized dynamic data containers (`dtl_sv_t`, `dtl_av_t`, `dtl_hv_t`) are used for configuration, schema inspection, and generic payload handling.
   * `adt` provides high-performance internal data structures (`adt_ary_t`, `adt_hash_t`, `adt_list_t`, `adt_rbfh_t`).
4. **Server & Extension Architecture**:
   * The APX server (`apx_server_t`) routes signal changes across connected client nodes.
   * Extensions register hooks into server lifecycles using `apx_serverExtension_t`.

---

## 4. Build & Test Workflows

### Building Applications (Clang 18 + Ninja)
```bash
# Debug build (apx_server, apx_node, apx_control, apx_info, apx_perf_test)
cmake --preset clang-debug
cmake --build --preset clang-debug

# Release build
cmake --preset clang-release
cmake --build --preset clang-release
```

### Running C Unit Tests (CuTest)
```bash
# Build and run C unit tests (Target: apx_unit)
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

### Running Integration & End-to-End Tests (pytest)
```bash
pip install -r tests/requirements.txt
pytest
# Or in parallel:
pytest -n auto
```

### Building Documentation (Sphinx + Furo + Doxygen)
```bash
pip install -r docs/requirements-dev.txt
python3 -m sphinx -b html docs docs/_build/html

# Live preview server (Sphinx Autobuild):
sphinx-autobuild docs docs/_build/html
```

---

## 5. Coding Standards & Conventions

### C99 & Code Quality
* Write strict, portable C99.
* Always use `NULL` for null pointers; never use legacy `0` or `(type*) 0`.
* Keep headers self-contained and guarded with `#ifndef / #define / #endif`.

### Type & Struct Conventions
* **Struct Names**: All `struct` names must end with `_tag` (e.g., `struct apx_node_tag`, `struct apx_server_tag`).
* **Typedef Names**: The corresponding `typedef` for a struct must end with `_t` (e.g., `typedef struct apx_node_tag apx_node_t;`).
* **Forward Declarations in Headers**:
  * Only forward-declare the struct tag (`struct <name>_tag;`), never a forward `typedef` (e.g., do **not** write `typedef struct apx_node_tag apx_node_t;` as a forward declaration).
  * The `typedef` is defined **only** in the header or module where the struct itself is fully declared.
  * Struct members pointing to opaque or forward-declared structs in external headers should use the struct tag pointer (e.g., `struct apx_file_manager_tag *file_manager;`).
* **Usage in Source Files (`.c` / `.cpp`)**:
  * In source files where all required headers are included, **do not** use `struct <name>_tag` in the code.
  * Always use the typedef `<name>_t` instead (e.g., `apx_server_t *server`, `apx_node_t *node`, `apx_client_t *self`).

### Variable Naming Conventions
* **File-Scoped Variables (`static`)**: File-scoped variables declared using `static` must be prefixed with `m_` (e.g., `static volatile int m_running = 1;`).
* **Program-Scoped Variables (`extern`)**: Program-scoped variables intended to be accessed across compilation units using `extern` must be prefixed with `g_`.

### Callback Conventions
* **Callback Function Names**: Name callbacks with the pattern `on_<subject>_<event>` (e.g., `on_client_connected`, `on_client_disconnected`).
* **Callback Signatures**: Pass user context first (`void *arg`, `void *arg, void *socket, ...`).

### Memory Ownership & Allocation
* Structs generally follow the `_create()` / `_destroy()` (stack/struct) or `_new()` / `_delete()` (heap) lifecycle pattern.
* Code must maintain zero memory leaks.
* Use `cutil_memleak.h` / `-DLEAK_CHECK=ON` and AddressSanitizer (`clang-asan`) to verify leak-free execution.

### Concurrency & Thread Safety
* APX uses POSIX threads (`pthread`) and mutexes.
* **Deadlock Prevention Rule**: Never invoke registered user callbacks or event listeners while holding an internal mutex lock.
  * **Pattern**: Take mutex $\to$ copy listener function pointers / state snapshot into local variables $\to$ release mutex $\to$ invoke callback.

### Networking
* Use the `msocket` abstraction layer for all networking; do not invoke raw BSD socket APIs directly in application code.

---

## 6. Testing Practices

* **C Unit Tests**: Built with `cutest` (from `cutil`). Test suites live in `apx_core/test/`, `extension/*/test/`, and `app/*/test/`. Registered in `CMakeLists.txt` and built into `apx_unit`.
* **Integration Tests**: Written in Python using `pytest` under `tests/`. Uses process management fixtures in `tests/conftest.py` to test multi-process communication across `apx_server`, `apx_node`, and `apx_control`.
