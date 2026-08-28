# APX CMake Integration Guide

This directory contains CMake helper modules, templates, and integration hooks for building and extending `c-apx`.

---

## 1. Overview

`c-apx` is designed for automotive and embedded Linux environments (such as Yocto / OpenEmbedded). It uses a modular extension architecture where:
- Core APX and extensions are compiled as static libraries (`.a` / `.lib`).
- The `apx_server` binary statically links only the enabled in-tree and out-of-tree extensions.
- No dynamic runtime loading (`dlopen`) is used, meeting strict automotive cybersecurity standards.

---

## 2. In-Tree Extension Configuration

Standard in-tree extensions can be enabled or disabled at configure time using standard CMake boolean flags:

```bash
cmake -S . -B build \
  -DAPX_BUILD_SERVER=ON \
  -DAPX_SERVER_ENABLE_SOCKET_EXTENSION=ON \
  -DAPX_SERVER_ENABLE_TEXT_LOG_EXTENSION=ON \
  -DAPX_SERVER_ENABLE_MONITOR_EXTENSION=ON
```

### Yocto `PACKAGECONFIG` Integration
In a BitBake recipe (e.g. `c-apx_git.bb`), these flags map directly to `PACKAGECONFIG`:

```bitbake
PACKAGECONFIG ??= "server socket-server text-log monitor"

PACKAGECONFIG[server]        = "-DAPX_BUILD_SERVER=ON,-DAPX_BUILD_SERVER=OFF"
PACKAGECONFIG[socket-server] = "-DAPX_SERVER_ENABLE_SOCKET_EXTENSION=ON,-DAPX_SERVER_ENABLE_SOCKET_EXTENSION=OFF"
PACKAGECONFIG[text-log]      = "-DAPX_SERVER_ENABLE_TEXT_LOG_EXTENSION=ON,-DAPX_SERVER_ENABLE_TEXT_LOG_EXTENSION=OFF"
PACKAGECONFIG[monitor]       = "-DAPX_SERVER_ENABLE_MONITOR_EXTENSION=ON,-DAPX_SERVER_ENABLE_MONITOR_EXTENSION=OFF"
```

---

## 3. Developing Custom / Out-of-Tree Extensions

Integrators can develop proprietary or custom extensions in separate source repositories without maintaining patch files against upstream `c-apx`.

### Step 1: Implement the Extension C Registration Interface

Each extension provides an initialization function matching the standard signature:

```c
/* my_sec_extension.h */
#ifndef MY_SEC_EXTENSION_H
#define MY_SEC_EXTENSION_H

#include "apx/server_extension.h"
#include "dtl_type.h"

apx_error_t my_sec_extension_register(struct apx_server_tag *server, dtl_dv_t *config);

#endif // MY_SEC_EXTENSION_H
```

```c
/* my_sec_extension.c */
#include "my_sec_extension.h"
#include "apx/server.h"

static apx_error_t my_sec_init(struct apx_server_tag *apx_server, dtl_dv_t *config)
{
   /* Initialize custom security extension using config (dtl_hv_t) */
   return APX_NO_ERROR;
}

static void my_sec_shutdown(void)
{
   /* Clean up extension resources */
}

apx_error_t my_sec_extension_register(struct apx_server_tag *server, dtl_dv_t *config)
{
   apx_serverExtensionHandler_t handler = {my_sec_init, my_sec_shutdown};
   return apx_server_add_extension(server, "SECURITY", &handler, config);
}
```

### Step 2: Create the Extension `CMakeLists.txt`

In the extension source directory:

```cmake
# my_sec_extension/CMakeLists.txt
add_library(my_sec_extension STATIC
    src/my_sec_extension.c
)

target_include_directories(my_sec_extension PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(my_sec_extension PRIVATE
    apx_core
)

# Register with apx_server (NAME dictates the config filename: <NAME>.json)
apx_register_server_extension(
    TARGET      my_sec_extension
    NAME        "security"
    HEADER      "my_sec_extension.h"
    REGISTER_FN "my_sec_extension_register"
)
```

---

## 4. Integrating Out-of-Tree Extensions via Yocto

To include custom extensions in a Yocto build, create a `.bbappend` file in your custom layer (e.g. `meta-customer/recipes-core/c-apx/c-apx_%.bbappend`):

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://my_sec_extension/"

EXTRA_OECMAKE += "\
    -DAPX_SERVER_EXTENSION_DIRS=${WORKDIR}/my_sec_extension \
"
```

When CMake runs, it automatically includes the custom extension directory, compiles it as a static library, and registers it into the `apx_server` static extension registry.

---

## 5. Runtime Configuration for Extensions

When deploying to a target system (e.g. via systemd), configuration files can be placed in `/etc/apx/`:

```
/etc/apx/
├── server.json            # Core server configuration
├── socket-server.json     # Built-in socket extension configuration
└── security.json          # Custom security extension configuration (<NAME>.json)
```

The systemd service unit executes:
```ini
[Service]
ExecStart=/usr/bin/apx_server /etc/apx/
```

`apx_server` will automatically load `/etc/apx/server.json` and any matching `<NAME>.json` files for each statically linked extension.

---

## 6. Build Instructions

For instructions on building the APX applications (`apx_server`, `apx_node`, `apx_control`) and running unit tests using GCC, Clang, or MSVC, please refer to the main [README.md](../README.md).
