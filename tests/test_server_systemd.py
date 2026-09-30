"""
Integration tests for systemd socket activation in apx_server.
Tests pre-bound file descriptor adoption and sd_listen_fds behavior.
"""

import json
import os
import select
import socket
import subprocess
from pathlib import Path
import pytest
from conftest import _set_pdeathsig


def test_systemd_socket_activation_success(tmp_path: Path, apx_server_bin: str):
    """Verify that apx_server adopts an inherited socket on fd 3 when LISTEN_FDS=1."""
    socket_path = str(tmp_path / "systemd_activated.socket")

    # 1. Pre-bind and listen in the parent process (mimicking systemd)
    listen_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    listen_sock.bind(socket_path)
    listen_sock.listen(128)
    listen_sock.set_inheritable(True)
    listen_fd = listen_sock.fileno()

    # 2. Setup readiness pipe
    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)

    def preexec_systemd():
        _set_pdeathsig()
        if listen_fd != 3:
            os.dup2(listen_fd, 3)
        os.environ["LISTEN_FDS"] = "1"
        os.environ["LISTEN_PID"] = str(os.getpid())

    socket_cfg = json.dumps({"unix-systemd": True, "unix-tag": "systemd"})
    cmd = [apx_server_bin, "--ready-fd", str(w_fd), "--socket-config", socket_cfg]

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        pass_fds=(w_fd, listen_fd, 3),
        preexec_fn=preexec_systemd
    )
    os.close(w_fd)

    # 3. Wait for readiness
    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    assert rlist, "Server should signal ready after adopting systemd socket"
    os.read(r_fd, 1)
    os.close(r_fd)

    # 4. Verify client can connect to the pre-bound socket
    client_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client_sock.settimeout(2.0)
    client_sock.connect(socket_path)
    assert client_sock.fileno() > 0
    client_sock.close()

    # 5. Teardown
    listen_sock.close()
    proc.terminate()
    stdout, stderr = proc.communicate(timeout=3.0)
    assert proc.returncode == 0, f"Server failed with stderr: {stderr.decode()}"


def test_systemd_socket_activation_auto_fallback(tmp_path: Path, apx_server_bin: str):
    """Verify that 'unix-systemd': 'auto' falls back to 'unix-file' when no systemd fds exist."""
    fallback_socket = str(tmp_path / "fallback.socket")

    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)

    socket_cfg = json.dumps({
        "unix-systemd": "auto",
        "unix-file": fallback_socket,
        "unix-tag": "local"
    })
    cmd = [apx_server_bin, "--ready-fd", str(w_fd), "--socket-config", socket_cfg]

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        pass_fds=(w_fd,),
        preexec_fn=_set_pdeathsig
    )
    os.close(w_fd)

    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    assert rlist, "Server should signal ready using fallback socket file"
    os.read(r_fd, 1)
    os.close(r_fd)

    assert os.path.exists(fallback_socket), "Fallback socket file should have been created"

    client_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client_sock.settimeout(2.0)
    client_sock.connect(fallback_socket)
    assert client_sock.fileno() > 0
    client_sock.close()

    proc.terminate()
    stdout, stderr = proc.communicate(timeout=3.0)
    assert proc.returncode == 0


def test_systemd_socket_activation_missing_env_fails(apx_server_bin: str):
    """Verify that 'unix-systemd': true fails cleanly when no systemd fds are passed."""
    socket_cfg = json.dumps({"unix-systemd": True})
    cmd = [apx_server_bin, "--socket-config", socket_cfg]

    # Explicitly clear LISTEN_PID and LISTEN_FDS
    env = os.environ.copy()
    env.pop("LISTEN_PID", None)
    env.pop("LISTEN_FDS", None)

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
        preexec_fn=_set_pdeathsig
    )
    stdout, stderr = proc.communicate(timeout=3.0)

    assert proc.returncode != 0, "Server must fail to start when required systemd fds are missing"
    stderr_text = stderr.decode()
    assert "Systemd socket activation requested, but no file descriptors inherited" in stderr_text


def test_systemd_socket_activation_pid_mismatch_fails(apx_server_bin: str):
    """Verify that a mismatched LISTEN_PID prevents socket adoption and fails."""
    socket_cfg = json.dumps({"unix-systemd": True})
    cmd = [apx_server_bin, "--socket-config", socket_cfg]

    env = os.environ.copy()
    env["LISTEN_PID"] = "999999"
    env["LISTEN_FDS"] = "1"

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
        preexec_fn=_set_pdeathsig
    )
    stdout, stderr = proc.communicate(timeout=3.0)

    assert proc.returncode != 0, "Server must fail to start when LISTEN_PID does not match"
    stderr_text = stderr.decode()
    assert "Systemd socket activation requested, but no file descriptors inherited" in stderr_text
