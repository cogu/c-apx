"""
Integration tests for UNIX domain socket peer credential inspection and allowed-groups whitelisting.
"""

import grp
import json
import os
import select
import socket
import subprocess
import time
from pathlib import Path
import pytest
from conftest import _set_pdeathsig, ApxServerInstance


def _start_server_with_config(tmp_path: Path, apx_server_bin: str, socket_cfg: dict) -> ApxServerInstance:
    socket_path = socket_cfg.get("unix-file", str(tmp_path / "apx.socket"))
    socket_cfg["unix-file"] = socket_path

    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)

    cfg_str = json.dumps(socket_cfg)
    cmd = [apx_server_bin, "--ready-fd", str(w_fd), "--socket-config", cfg_str]
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        pass_fds=(w_fd,),
        preexec_fn=_set_pdeathsig,
    )
    os.close(w_fd)

    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    assert rlist, "Server should signal ready"
    ready_byte = os.read(r_fd, 1)
    os.close(r_fd)
    assert ready_byte, "Server exited before signaling readiness"

    return ApxServerInstance(proc, socket_path, tmp_path)


def test_peer_credentials_audited_on_connect(tmp_path: Path, apx_server_bin: str):
    """Verify that when no allowed-groups restriction is set, client connection is accepted."""
    server = _start_server_with_config(tmp_path, apx_server_bin, {})
    try:
        sock = server.connect()
        time.sleep(0.1)
        assert sock.fileno() > 0
        sock.close()
    finally:
        server.stop()

    assert "Access denied" not in server.stderr


def test_allowed_groups_permitted(tmp_path: Path, apx_server_bin: str):
    """Verify that a client whose group is in 'allowed-groups' is accepted."""
    current_group = grp.getgrgid(os.getgid()).gr_name
    server = _start_server_with_config(
        tmp_path,
        apx_server_bin,
        {"allowed-groups": [current_group]},
    )
    try:
        sock = server.connect()
        sock.settimeout(1.0)
        time.sleep(0.1)
        assert sock.fileno() > 0
        sock.close()
    finally:
        server.stop()

    assert "Access denied" not in server.stderr


def test_allowed_groups_rejected(tmp_path: Path, apx_server_bin: str):
    """Verify that a client whose group is NOT in 'allowed-groups' is rejected."""
    current_gid = os.getgid()
    # Find a group that current user is NOT in
    disallowed_group = "daemon"
    for g in grp.getgrall():
        if g.gr_gid != current_gid and "cogu" not in g.gr_mem:
            disallowed_group = g.gr_name
            break

    server = _start_server_with_config(
        tmp_path,
        apx_server_bin,
        {"allowed-groups": [disallowed_group]},
    )
    try:
        sock = server.connect()
        sock.settimeout(1.0)
        # Server closes connection immediately upon credential rejection
        data = sock.recv(1024)
        assert data == b"", "Rejected connection should be closed by server immediately"
        sock.close()
    finally:
        server.stop()

    assert "Access denied: client" in server.stderr
    assert "is not in allowed groups" in server.stderr


def test_allowed_groups_multiple_with_match(tmp_path: Path, apx_server_bin: str):
    """Verify that if one of multiple allowed groups matches, connection is accepted."""
    current_group = grp.getgrgid(os.getgid()).gr_name
    server = _start_server_with_config(
        tmp_path,
        apx_server_bin,
        {"allowed-groups": ["nonexistent_grp_xyz", current_group]},
    )
    try:
        sock = server.connect()
        time.sleep(0.1)
        assert sock.fileno() > 0
        sock.close()
    finally:
        server.stop()

    assert "Access denied" not in server.stderr
    assert "Warning: configured group 'nonexistent_grp_xyz' not found on system" in server.stderr


def test_allowed_groups_single_string_config(tmp_path: Path, apx_server_bin: str):
    """Verify that 'allowed-groups' specified as a single string works."""
    current_group = grp.getgrgid(os.getgid()).gr_name
    server = _start_server_with_config(
        tmp_path,
        apx_server_bin,
        {"allowed-groups": current_group},
    )
    try:
        sock = server.connect()
        time.sleep(0.1)
        assert sock.fileno() > 0
        sock.close()
    finally:
        server.stop()

    assert "Access denied" not in server.stderr
