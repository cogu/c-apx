"""
Tests for APX server lifecycle, readiness synchronization, and socket cleanup.
"""

import os
import socket
import pytest
from conftest import ApxServerInstance, wait_for_unix_socket


def test_server_startup_and_connect(apx_server: ApxServerInstance):
    """Verify that apx_server starts up cleanly and accepts client connections."""
    assert apx_server.is_running, "Server process should be running"
    assert os.path.exists(apx_server.socket_path), "UNIX socket file should exist"

    # Connect to the server socket
    client_sock = apx_server.connect()
    assert client_sock is not None
    client_sock.close()


def test_server_multiple_clients(apx_server: ApxServerInstance):
    """Verify that multiple clients can connect concurrently to the server."""
    clients = [apx_server.connect() for _ in range(5)]
    for client in clients:
        assert client.fileno() > 0
    for client in clients:
        client.close()


def test_server_graceful_shutdown_and_socket_cleanup(apx_server: ApxServerInstance):
    """Verify that SIGTERM triggers clean shutdown and removes the socket file."""
    socket_file = apx_server.socket_path
    assert os.path.exists(socket_file), "Socket file should exist while server is running"

    apx_server.stop()

    assert not apx_server.is_running, "Server should have stopped"
    assert apx_server.process.returncode == 0, f"Server exited with unexpected code: {apx_server.process.returncode}"
    assert not os.path.exists(socket_file), "Socket file should be unlinked/removed after server shutdown"


def test_wait_for_unix_socket_fallback(apx_server: ApxServerInstance):
    """Verify that active connect polling helper validates server readiness."""
    wait_for_unix_socket(apx_server.socket_path, timeout=1.0)


def test_server_socket_config_override(tmp_path, apx_server_bin: str):
    """Verify that --socket-config overrides socket settings in a base configuration file."""
    import json
    import subprocess
    import select
    from conftest import _set_pdeathsig

    base_socket = str(tmp_path / "base.socket")
    override_socket = str(tmp_path / "override.socket")
    config_file = str(tmp_path / "server.json")

    with open(config_file, "w", encoding="utf-8") as f:
        json.dump({
            "socket-server-extension": {
                "unix-file": base_socket
            }
        }, f)

    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)

    override_cfg = json.dumps({"unix-file": override_socket})
    cmd = [
        apx_server_bin,
        "--ready-fd", str(w_fd),
        "--socket-config", override_cfg,
        config_file
    ]
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        pass_fds=(w_fd,),
        preexec_fn=_set_pdeathsig
    )
    os.close(w_fd)

    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    assert rlist, "Server should signal ready"
    os.read(r_fd, 1)
    os.close(r_fd)

    assert os.path.exists(override_socket), "Override socket should have been created"
    assert not os.path.exists(base_socket), "Base socket should NOT have been created"

    # Connect to override socket
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.connect(override_socket)
    s.close()

    proc.terminate()
    proc.wait(timeout=3.0)
    assert not os.path.exists(override_socket), "Override socket should be cleaned up"


def test_server_survives_client_node_disconnect(
    apx_server: ApxServerInstance,
    spawn_apx_node,
):
    """
    Verify that when an APX client node disconnects (e.g. via SIGINT / SIGTERM),
    the server does not terminate (e.g. from unhandled SIGPIPE on socket writes)
    and continues serving remaining clients.
    """
    from pathlib import Path
    repo_root = Path(__file__).resolve().parent.parent
    listener_apx = repo_root / "example" / "nodes" / "unsigned_listener.apx"
    sender_apx = repo_root / "example" / "nodes" / "small_unsigned_sender.apx"

    assert listener_apx.is_file()
    assert sender_apx.is_file()

    # 1. Connect listener node
    listener = spawn_apx_node(listener_apx, bind=False)
    assert listener.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)

    # 2. Connect sender node
    sender = spawn_apx_node(sender_apx, bind=False)
    assert sender.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)

    # 3. Disconnect sender node via stop() (SIGINT)
    sender.stop()
    assert not sender.is_running

    # 4. Verify server is still running healthy
    assert apx_server.is_running, "Server terminated unexpectedly after client disconnect"
    assert apx_server.process.poll() is None

    # 5. Connect another client to confirm server still accepts and processes connections
    sender2 = spawn_apx_node(sender_apx, bind=False)
    assert sender2.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)
    assert apx_server.is_running

    sender2.stop()
    listener.stop()

