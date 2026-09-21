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
import time
from pathlib import Path
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

    def stop(self, timeout: float = 3.0):
        """Gracefully terminate server and verify clean exit."""
        if self.is_running:
            self.process.terminate()
            try:
                self.process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()


@pytest.fixture(scope="session")
def apx_server_bin() -> str:
    """Finds the apx_server executable from environment or build directories."""
    env_bin = os.environ.get("APX_SERVER_BIN")
    if env_bin and os.path.isfile(env_bin) and os.access(env_bin, os.X_OK):
        return env_bin

    repo_root = Path(__file__).resolve().parent.parent
    candidate_paths = [
        repo_root / "build" / "clang-debug" / "app" / "apx_server" / "apx_server",
        repo_root / "build" / "clang-release" / "app" / "apx_server" / "apx_server",
        repo_root / "build" / "gcc-release" / "app" / "apx_server" / "apx_server",
        repo_root / "build" / "gcc-debug" / "app" / "apx_server" / "apx_server",
        repo_root / "build" / "app" / "apx_server" / "apx_server",
        repo_root / "build" / "clang-test" / "app" / "apx_server" / "apx_server",
    ]

    for path in candidate_paths:
        if path.is_file() and os.access(path, os.X_OK):
            return str(path)

    system_bin = shutil.which("apx_server")
    if system_bin:
        return system_bin

    raise FileNotFoundError(
        "Could not find 'apx_server' binary. Please build it first (e.g. cmake --build --preset clang-test --target apx_server) "
        "or set the APX_SERVER_BIN environment variable."
    )


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
