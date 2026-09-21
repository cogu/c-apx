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
