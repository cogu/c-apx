# Integration and End-to-End Tests

This directory contains Python-based integration and end-to-end tests for `c-apx` using `pytest`. These tests validate multi-process interactions between the compiled binaries (`apx-server`, `apx-node`, and `apx-control`) over UNIX domain sockets and JSON router interfaces.

---

## Prerequisites

Before running the tests, compile the project binaries and install test dependencies:

```bash
# 1. Build the application binaries (apx-server, apx-node, apx-control)
cmake --preset clang-debug
cmake --build --preset clang-debug

# 2. Install Python test dependencies
pip install -r tests/requirements.txt
```

Run tests from the repository root:

```bash
pytest
# Or run with parallel workers via pytest-xdist:
pytest -n auto
```

---

## Test Fixtures

All shared fixtures and helper classes are defined in [`conftest.py`](file:///home/cogu/repo/c-apx/tests/conftest.py).

### 1. Binary Discovery Fixtures (Session Scope)

These fixtures locate the compiled executables automatically. They search environment variables first, then standard CMake build output directories (such as `build/clang-debug/`, `build/clang-release/`, etc.), and finally the system `PATH`.

| Fixture | Return Type | Environment Override | Description |
|---|---|---|---|
| `apx_server_bin` | `str` | `APX_SERVER_BIN` | Absolute path to the `apx-server` executable. |
| `apx_node_bin` | `str` | `APX_NODE_BIN` | Absolute path to the `apx-node` executable. |
| `apx_control_bin` | `str` | `APX_CONTROL_BIN` | Absolute path to the `apx-control` executable. |

---

### 2. Server Lifecycle Fixtures (Function Scope)

#### `apx_server`
Starts an isolated `apx_server` daemon process dedicated to the executing test.

* **Readiness synchronization**: Uses Linux pipe notification (`--ready-fd`) to wait deterministically for server initialization without sleep delays.
* **Isolated configuration**: Generates an isolated UNIX domain socket (`apx.socket`) in a temporary directory (`tmp_path`) passed via `--socket-config`.
* **Process lifecycle**: Configures `PR_SET_PDEATHSIG` so the daemon is terminated if the test process dies, and performs graceful `SIGTERM` shutdown and socket cleanup during teardown.
* **Yields**: An `ApxServerInstance` object.

---

### 3. Node Factory Fixtures (Function Scope)

#### `spawn_apx_node`
A factory fixture yielding a callable `_spawn(...)` to launch one or more `apx_node` processes connected to the running `apx_server`.

```python
def _spawn(
    definition_file: str | Path,
    *,
    bind: bool | str | Path = False,
    extra_args: list[str] | None = None
) -> ApxNodeInstance: ...
```

* **Connection**: Automatically passes `-c <apx_server.socket_path>` to connect the node to the active test server.
* **JSON Router Binding (`bind`)**:
  * `bind=False`: Runs with `--no-bind` (pure signal listener/sender with no local control socket).
  * `bind=True`: Automatically assigns an isolated UNIX domain socket in `tmp_path` (`-b <tmp_path>/node_N.socket`) and waits for the socket to accept connections.
  * `bind="path/to/socket"`: Binds to a specific socket path.
* **Output monitoring**: Starts background reader threads for stdout and stderr with line buffering (`stdbuf -oL` when available).
* **Teardown**: Automatically terminates and reaps all spawned nodes upon test completion.
* **Returns**: An `ApxNodeInstance` object.

---

### 4. Control CLI Fixtures (Function Scope)

#### `apx_control`
Provides a helper wrapper around the `apx_control` command-line utility.

* **Yields**: An `ApxControl` instance wrapping `apx_control_bin`.
* **Methods**:
  * `run(*args, connect_path=None, timeout=3.0)`: Invokes `apx_control` with CLI arguments, optionally connecting to a specific UNIX socket (`-c`).
  * `set_signal(name, value, *, connect_path=None, timeout=3.0)`: Convenience wrapper to set a signal value: `apx_control -c <connect_path> <name> <value>`.

---

## Helper Classes and Utilities

| Class / Function | Description |
|---|---|
| `ApxServerInstance` | Encapsulates the running server process, socket path, and temporary directory. Provides `.connect()` to create raw client sockets, `.client_args()` for CLI argument formatting, and `.stop()` for teardown. |
| `ApxNodeInstance` | Encapsulates a running node process and real-time output streams. Provides `.wait_for_output(pattern, timeout)` to block until expected text appears in stdout, as well as `.output`, `.lines`, `.stderr`, and `.stop()`. |
| `ApxControl` | Helper class for executing the `apx_control` CLI tool against running APX nodes or servers. |
| `wait_for_unix_socket(path, timeout, poll_interval)` | Polls a UNIX domain socket until `connect()` succeeds or timeout expires, verifying `bind()` and `listen()` readiness. |

---

## Example Usage

```python
from pathlib import Path
from conftest import ApxServerInstance, ApxControl

def test_signal_routing_example(
    apx_server: ApxServerInstance,
    spawn_apx_node,
    apx_control: ApxControl,
):
    repo_root = Path(__file__).resolve().parent.parent
    listener_apx = repo_root / "example" / "nodes" / "unsigned_listener.apx"
    sender_apx = repo_root / "example" / "nodes" / "unsigned_sender.apx"

    # Start listener node
    listener = spawn_apx_node(listener_apx, bind=False)
    assert listener.wait_for_output("[APX-CONNECTION] connected to APX server")

    # Start sender node with JSON control socket
    sender = spawn_apx_node(sender_apx, bind=True)
    assert sender.wait_for_output("[APX-CONNECTION] connected to APX server")

    # Inject signal via apx_control into sender's JSON socket
    result = apx_control.set_signal("VehicleSpeed", 100, connect_path=sender.bind_path)
    assert result.returncode == 0

    # Verify listener receives routed signal
    assert listener.wait_for_output('"VehicleSpeed": 100')
```
