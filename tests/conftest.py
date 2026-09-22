"""
Pytest fixtures and helpers for APX server and client testing.
"""

import ctypes
import json
import os
import select
import shutil
import socket
import subprocess
import sys
import threading
import time
from pathlib import Path
from typing import Any
import pytest

PR_SET_PDEATHSIG = 1
SIGTERM = 15


def _set_pdeathsig():
    """Linux kernel helper: deliver SIGTERM to child if parent process dies."""
    if sys.platform.startswith("linux"):
        try:
            libc = ctypes.CDLL("libc.so.6")
            libc.prctl(PR_SET_PDEATHSIG, SIGTERM)
        except Exception:
            pass


def wait_for_unix_socket(socket_path: str, timeout: float = 3.0, poll_interval: float = 0.005):
    """
    Polls a UNIX domain socket by attempting non-blocking connects.
    Succeeds only when server has called bind() AND listen().
    Rejects stale or non-listening socket files with ECONNREFUSED.
    """
    start_time = time.monotonic()
    while time.monotonic() - start_time < timeout:
        if os.path.exists(socket_path):
            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                s.settimeout(0.1)
                s.connect(socket_path)
                s.close()
                return
            except (ConnectionRefusedError, FileNotFoundError):
                s.close()
        time.sleep(poll_interval)

    raise TimeoutError(f"APX Server failed to accept connections on {socket_path} within {timeout}s")


class ApxServerInstance:
    """Represents a running apx_server instance for testing."""

    def __init__(self, process: subprocess.Popen, socket_path: str, tmp_dir: Path, config_path: str = ""):
        self.process = process
        self.socket_path = socket_path
        self.config_path = config_path
        self.tmp_dir = tmp_dir

    @property
    def is_running(self) -> bool:
        return self.process.poll() is None

    def connect(self) -> socket.socket:
        """Create a client socket connected to this APX server."""
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(self.socket_path)
        return sock

    def client_args(self, *extra_args: str) -> list[str]:
        """Returns standard CLI connect arguments for APX clients: ['-c', socket_path, *extra_args]."""
        return ["-c", self.socket_path, *extra_args]

    def stop(self, timeout: float = 3.0):
        """Gracefully terminate server and verify clean exit."""
        if self.is_running:
            self.process.terminate()
            try:
                self.process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()


class ApxNodeInstance:
    """Represents a running apx_node instance for testing."""

    def __init__(self, process: subprocess.Popen, definition_file: Path | str, bind_path: str | None = None):
        self.process = process
        self.definition_file = Path(definition_file)
        self.bind_path = bind_path
        self._output_lines: list[str] = []
        self._stderr_lines: list[str] = []
        self._lock = threading.Lock()
        self._line_event = threading.Event()
        self._stdout_thread = threading.Thread(target=self._read_stdout, daemon=True)
        self._stderr_thread = threading.Thread(target=self._read_stderr, daemon=True)
        self._stdout_thread.start()
        self._stderr_thread.start()

    def _read_stdout(self):
        try:
            if self.process.stdout:
                for line in iter(self.process.stdout.readline, ''):
                    if not line:
                        break
                    clean_line = line.rstrip('\r\n')
                    with self._lock:
                        self._output_lines.append(clean_line)
                        self._line_event.set()
        except Exception:
            pass

    def _read_stderr(self):
        try:
            if self.process.stderr:
                for line in iter(self.process.stderr.readline, ''):
                    if not line:
                        break
                    clean_line = line.rstrip('\r\n')
                    with self._lock:
                        self._stderr_lines.append(clean_line)
        except Exception:
            pass

    @property
    def is_running(self) -> bool:
        return self.process.poll() is None

    @property
    def lines(self) -> list[str]:
        with self._lock:
            return list(self._output_lines)

    @property
    def stderr_lines(self) -> list[str]:
        with self._lock:
            return list(self._stderr_lines)

    @property
    def output(self) -> str:
        with self._lock:
            return "\n".join(self._output_lines)

    @property
    def stderr(self) -> str:
        with self._lock:
            return "\n".join(self._stderr_lines)

    def wait_for_output(self, pattern: str, timeout: float = 3.0) -> bool:
        """Wait until pattern appears in any output line."""
        start_time = time.monotonic()
        while time.monotonic() - start_time < timeout:
            with self._lock:
                for line in self._output_lines:
                    if pattern in line:
                        return True
            self._line_event.wait(timeout=0.05)
            self._line_event.clear()
        with self._lock:
            return any(pattern in line for line in self._output_lines)

    def stop(self, timeout: float = 3.0):
        """Gracefully terminate apx_node process and verify clean exit."""
        if self.is_running:
            self.process.terminate()
            try:
                self.process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()


class ApxControl:
    """Helper for invoking the apx_control CLI tool."""

    def __init__(self, bin_path: str):
        self.bin_path = bin_path

    def run(self, *args: str, connect_path: str | None = None, timeout: float = 3.0) -> subprocess.CompletedProcess:
        cmd = [self.bin_path]
        if connect_path:
            cmd.extend(["-c", str(connect_path)])
        cmd.extend(str(a) for a in args)
        return subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=timeout,
            check=True
        )

    def set_signal(self, name: str, value: Any, *, connect_path: str | None = None, timeout: float = 3.0) -> subprocess.CompletedProcess:
        """Convenience method to set a signal value: apx_control -c <connect_path> <name> <value>."""
        return self.run(name, str(value), connect_path=connect_path, timeout=timeout)


def _find_binary(env_var: str, app_name: str) -> str:
    """Finds an executable binary from environment variable, build directories, or PATH."""
    env_bin = os.environ.get(env_var)
    if env_bin and os.path.isfile(env_bin) and os.access(env_bin, os.X_OK):
        return env_bin

    repo_root = Path(__file__).resolve().parent.parent
    candidate_paths = [
        repo_root / "build" / "clang-debug" / "app" / app_name / app_name,
        repo_root / "build" / "clang-release" / "app" / app_name / app_name,
        repo_root / "build" / "gcc-release" / "app" / app_name / app_name,
        repo_root / "build" / "gcc-debug" / "app" / app_name / app_name,
        repo_root / "build" / "app" / app_name / app_name,
        repo_root / "build" / "clang-test" / "app" / app_name / app_name,
    ]

    for path in candidate_paths:
        if path.is_file() and os.access(path, os.X_OK):
            return str(path)

    system_bin = shutil.which(app_name)
    if system_bin:
        return system_bin

    raise FileNotFoundError(
        f"Could not find '{app_name}' binary. Please build it first "
        f"(e.g. cmake --build --preset clang-debug --target {app_name}) "
        f"or set the {env_var} environment variable."
    )


@pytest.fixture(scope="session")
def apx_server_bin() -> str:
    """Finds the apx_server executable from environment or build directories."""
    return _find_binary("APX_SERVER_BIN", "apx_server")


@pytest.fixture(scope="session")
def apx_node_bin() -> str:
    """Finds the apx_node executable from environment or build directories."""
    return _find_binary("APX_NODE_BIN", "apx_node")


@pytest.fixture(scope="session")
def apx_control_bin() -> str:
    """Finds the apx_control executable from environment or build directories."""
    return _find_binary("APX_CONTROL_BIN", "apx_control")


@pytest.fixture
def apx_control(apx_control_bin: str) -> ApxControl:
    """Fixture providing an ApxControl helper instance."""
    return ApxControl(apx_control_bin)


@pytest.fixture
def spawn_apx_node(apx_server: ApxServerInstance, apx_node_bin: str, tmp_path: Path):
    """
    Factory fixture to launch apx_node instances connected to apx_server.
    Handles:
    - Connecting to apx_server via UNIX domain socket (-c apx_server.socket_path).
    - Isolated bind sockets (-b <tmp_path/node_N.socket>) or --no-bind.
    - Deterministic waiting for JSON server listening readiness when bound.
    - Automatic cleanup (SIGTERM) on test teardown.
    - Kernel cleanup via PR_SET_PDEATHSIG.
    """
    nodes: list[ApxNodeInstance] = []
    node_counter = 0

    def _spawn(
        definition_file: str | Path,
        *,
        bind: bool | str | Path = False,
        extra_args: list[str] | None = None
    ) -> ApxNodeInstance:
        nonlocal node_counter
        node_counter += 1

        bind_path = None
        cmd = [apx_node_bin, *apx_server.client_args()]

        if bind is False:
            cmd.append("--no-bind")
        elif bind is True:
            bind_socket = str(tmp_path / f"node_{node_counter}.socket")
            cmd.extend(["-b", bind_socket])
            bind_path = bind_socket
        elif isinstance(bind, (str, Path)):
            bind_socket = str(bind)
            cmd.extend(["-b", bind_socket])
            bind_path = bind_socket

        if extra_args:
            cmd.extend(extra_args)

        cmd.append(str(definition_file))

        stdbuf_bin = shutil.which("stdbuf")
        if stdbuf_bin:
            cmd = [stdbuf_bin, "-oL", *cmd]

        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
            preexec_fn=_set_pdeathsig
        )

        node = ApxNodeInstance(proc, definition_file, bind_path=bind_path)
        nodes.append(node)

        if bind_path:
            wait_for_unix_socket(bind_path, timeout=3.0)

        return node

    yield _spawn

    for node in nodes:
        node.stop()



@pytest.fixture(scope="function")
def apx_server(tmp_path: Path, apx_server_bin: str):
    """
    Starts an isolated APX server daemon using --ready-fd synchronization.
    - Zero arbitrary sleep() calls.
    - Deterministic startup notification via pipe.
    - Automatic cleanup of child processes on parent crash (PR_SET_PDEATHSIG).
    - Graceful SIGTERM teardown.
    """
    socket_path = str(tmp_path / "apx.socket")

    # 1. Create readiness pipe
    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)

    # 2. Launch server using inline --socket-config (no config file on disk needed)
    socket_cfg = json.dumps({"unix-file": socket_path})
    cmd = [apx_server_bin, "--ready-fd", str(w_fd), "--socket-config", socket_cfg]
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        pass_fds=(w_fd,),
        preexec_fn=_set_pdeathsig
    )
    # Close write end in parent
    os.close(w_fd)

    # 3. Wait for readiness notification from server
    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    if not rlist:
        os.close(r_fd)
        proc.kill()
        stdout, stderr = proc.communicate()
        raise TimeoutError(
            f"apx_server timed out waiting for ready notification.\nStdout: {stdout.decode()}\nStderr: {stderr.decode()}"
        )

    ready_byte = os.read(r_fd, 1)
    os.close(r_fd)

    if not ready_byte:
        proc.kill()
        stdout, stderr = proc.communicate()
        raise RuntimeError(
            f"apx_server exited before signaling readiness.\nStdout: {stdout.decode()}\nStderr: {stderr.decode()}"
        )

    server = ApxServerInstance(proc, socket_path, tmp_path)
    yield server

    # 4. Graceful Teardown
    server.stop(timeout=3.0)
