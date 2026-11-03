# APX CMake Integration Guide

This directory contains CMake helper modules, templates, and integration hooks for building and extending `c-apx`.

---

## 1. Overview

`c-apx` is designed for automotive and embedded Linux environments (such as Yocto / OpenEmbedded). It uses a modular extension architecture where:
- A compile-time static extension registry (`extensions_cfg.c`) is generated during the build, eliminating dynamic runtime plugin discovery (`dlopen`) to meet strict cybersecurity requirements.
- The `apx_server` binary only compiles and registers enabled in-tree and out-of-tree extensions.
- Library linkage is controlled via standard CMake options:
  - By default on Linux (`BUILD_SHARED_LIBS=ON`), libraries are built as shared libraries (`.so`).
  - For embedded/Yocto deployments requiring fully static binaries, configure with `-DBUILD_SHARED_LIBS=OFF` to build `apx_core` and extensions as static archives (`.a` / `.lib`) linked directly into `apx_server`.
  - On MSVC and during test builds (`UNIT_TEST=ON`), `BUILD_SHARED_LIBS` defaults to `OFF`.

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

Each extension provides an initialization function, and can optionally provide callbacks for configuration key validation and minimum configuration verification:

```c
/* my_sec_extension.h */
#ifndef MY_SEC_EXTENSION_H
#define MY_SEC_EXTENSION_H

#include "apx/server_extension.h"
#include "apx/types.h"
#include "dtl_type.h"

/* Mandatory registration function */
apx_error_t my_sec_extension_register(struct apx_server_tag *server, dtl_dv_t *config);

/* Optional: list of supported configuration keys for unrecognized key detection */
const char *const *my_sec_extension_config_keys(void);

/* Optional: minimum configuration validation check */
void my_sec_extension_check_config(dtl_hv_t *config, apx_config_warning_cb cb, void *arg);

#endif // MY_SEC_EXTENSION_H
```

```c
/* my_sec_extension.c */
#include "my_sec_extension.h"
#include "apx/server.h"

static const char *const m_config_keys[] = {
   "enabled",
   "auth-level",
   NULL
};

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
   apx_server_extension_handler_t handler = {my_sec_init, my_sec_shutdown};
   return apx_server_add_extension(server, "SECURITY", &handler, config);
}

const char *const *my_sec_extension_config_keys(void)
{
   return m_config_keys;
}

void my_sec_extension_check_config(dtl_hv_t *config, apx_config_warning_cb cb, void *arg)
{
   if (config == NULL || cb == NULL)
   {
      return;
   }
   /* Verify that minimum required configuration is present when enabled */
   if (dtl_hv_get_cstr(config, "auth-level") == NULL)
   {
      cb(arg, "Warning: 'security-extension' is enabled, but 'auth-level' is not configured");
   }
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

# Register with apx_server (NAME dictates the config key: "<NAME>-extension")
apx_register_server_extension(
    TARGET          my_sec_extension
    NAME            "security"
    HEADER          "my_sec_extension.h"
    REGISTER_FN     "my_sec_extension_register"
    CONFIG_KEYS_FN  "my_sec_extension_config_keys"
    CHECK_CONFIG_FN "my_sec_extension_check_config"
)
```

#### Parameters for `apx_register_server_extension`

| Parameter | Required | Description |
|---|---|---|
| `TARGET` | Yes | CMake library target name (e.g. `my_sec_extension`). |
| `NAME` | Yes | Extension name (dictates configuration section name `"<NAME>-extension"`). |
| `HEADER` | Yes | Header file path to `#include` in the generated `extensions_cfg.c`. |
| `REGISTER_FN` | Yes | C registration function name (`apx_error_t (*)(struct apx_server_tag *, dtl_dv_t *)`). |
| `CONFIG_KEYS_FN` | No | C function returning a `NULL`-terminated array of supported JSON keys (`const char *const *(*)(void)`). Used by `apx_server` to detect and warn about unrecognized keys. Defaults to `NULL`. |
| `CHECK_CONFIG_FN` | No | C function for pre-flight minimum configuration checks (`void (*)(dtl_hv_t *, apx_config_warning_cb, void *)`). Invoked during startup to emit warnings if mandatory options are missing. Defaults to `NULL`. |

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

Configuration for the APX server daemon and all extensions is centralized in a single JSON configuration file (e.g. `/etc/apx/server.json`):

```json
{
  "apx-server": {
    "multiple-provider-policy": "first"
  },
  "socket-server-extension": {
    "enabled": true,
    "unix-file": "/tmp/apx.socket",
    "unix-tag": "UNIX"
  },
  "security-extension": {
    "enabled": true,
    "auth-level": "admin"
  }
}
```

The systemd service unit executes:
```ini
[Service]
ExecStart=/usr/bin/apx_server /etc/apx/server.json
```
*(or specify `/etc/apx/` directly, where `apx_server` will automatically resolve `server.json`)*

Each extension entry in the registry (registered with `NAME "<NAME>"`) receives only its isolated sub-tree under `"<NAME>-extension"`.

During server startup, configuration is validated:
- **Enabled toggle**: The server daemon checks the optional `"enabled"` boolean attribute for each extension. If explicitly set to `false`, the extension is skipped and never initialized.
- **Unrecognized key detection**: When an extension specifies `CONFIG_KEYS_FN`, all keys in `"<NAME>-extension"` are checked against the returned supported key list. Any unrecognized keys or typos emit a descriptive warning on `stderr`.
- **Minimum configuration verification**: When an extension is enabled and specifies `CHECK_CONFIG_FN`, the server executes this validator during pre-flight checks. Missing mandatory settings emit a descriptive warning on `stderr` while allowing server startup to continue.

---

## 6. Build Instructions

For instructions on building the APX applications (`apx_server`, `apx_node`, `apx_control`) and running unit tests using GCC, Clang, or MSVC, please refer to the main [README.md](../README.md).
