"""
Integration tests for APX Server and apx-node over Linux VSOCK (AF_VSOCK) transport.
Includes portability checks to skip loopback tests on systems where VSOCK or the
kernel vsock_loopback transport driver is unavailable or not loaded.
"""

import json
import os
import random
import select
import socket
import subprocess
import sys
import time
from pathlib import Path
import pytest
from conftest import _set_pdeathsig


def is_vsock_loopback_supported() -> tuple[bool, str]:
    """
    Checks if the host system and kernel support VSOCK loopback communication.
    Returns (True, 'OK') if loopback works, or (False, reason) if unsupported or disabled.
    """
    if not sys.platform.startswith("linux"):
        return False, "Only supported on Linux"

    if not hasattr(socket, "AF_VSOCK"):
        return False, "Python socket module lacks AF_VSOCK"

    # Attempt to load the kernel loopback module if permissions allow
    try:
        subprocess.run(["modprobe", "vsock_loopback"], check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except Exception:
        pass

    cid_local = getattr(socket, "VMADDR_CID_LOCAL", 1)

    # 1. Verify AF_VSOCK socket can be created
    try:
        sock = socket.socket(socket.AF_VSOCK, socket.SOCK_STREAM)
    except Exception as e:
        return False, f"Socket creation failed: {e}"

    # 2. Verify binding to VMADDR_CID_LOCAL (1) succeeds.
    # On Linux systems without vsock_loopback loaded, binding to CID 1 fails with EADDRNOTAVAIL (Errno 99).
    test_port = random.randint(52000, 55000)
    try:
        sock.bind((cid_local, test_port))
        sock.listen(1)
    except Exception as e:
        sock.close()
        return False, "Driver 'vsock_loopback' not loaded (run: sudo modprobe vsock_loopback)"

    # 3. Verify loopback client connection succeeds
    try:
        client = socket.socket(socket.AF_VSOCK, socket.SOCK_STREAM)
        client.settimeout(0.3)
        client.connect((cid_local, test_port))
        client.close()
        sock.close()
        return True, "OK"
    except Exception as e:
        sock.close()
        return False, f"Loopback connect failed: {e}"


VSOCK_LOOPBACK_AVAILABLE, VSOCK_SKIP_REASON = is_vsock_loopback_supported()

require_vsock_loopback = pytest.mark.skipif(
    not VSOCK_LOOPBACK_AVAILABLE,
    reason=f"VSOCK loopback unavailable: {VSOCK_SKIP_REASON}"
)


def run_vsock_server(tmp_path: Path, apx_server_bin: str, vsock_port: int, vsock_cid: int = 4294967295) -> subprocess.Popen:
    """Helper to start an apx-server instance listening on VSOCK with deterministic ready-fd synchronization."""
    config = {
        "socket-server-extension": {
            "vsock-port": vsock_port,
            "vsock-cid": vsock_cid
        }
    }
    config_file = str(tmp_path / "server_vsock.json")
    with open(config_file, "w", encoding="utf-8") as f:
        json.dump(config, f)

    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)

    cmd = [apx_server_bin, "--ready-fd", str(w_fd), config_file]
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        pass_fds=(w_fd,),
        preexec_fn=_set_pdeathsig
    )
    os.close(w_fd)

    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    if not rlist:
        os.close(r_fd)
        proc.kill()
        out, err = proc.communicate()
        raise TimeoutError(f"Server failed to start in time. stderr: {err.decode('utf-8', errors='replace')}")
    os.read(r_fd, 1)
    os.close(r_fd)

    return proc


# ============================================================================
# Portable Tests (Run on all systems, no kernel vsock_loopback required)
# ============================================================================

def test_apx_node_vsock_cli_arg_validation(apx_node_bin: str, tmp_path: Path):
    """Verify that apx-node cleanly validates and rejects malformed --vsock arguments."""
    repo_root = Path(__file__).resolve().parent.parent
    node_file = str(repo_root / "example" / "nodes" / "small_unsigned_sender.apx")

    invalid_cases = [
        # Missing value
        ["--vsock"],
        # Missing colon separator
        ["--vsock", "5000"],
        # Empty CID or Port
        ["--vsock", ":5000"],
        ["--vsock", "2:"],
        # Non-numeric, non-alias CID
        ["--vsock", "invalid_alias:5000"],
        # Non-numeric Port
        ["--vsock", "2:port5000"],
        # Port 0 (unsupported/invalid)
        ["--vsock", "2:0"],
        # Conflicting TLS flag
        ["--vsock", "2:5000", "--tls"],
    ]

    for extra_args in invalid_cases:
        cmd = [apx_node_bin, *extra_args, "--no-bind", node_file]
        res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        assert res.returncode != 0, f"Expected non-zero exit code for args {extra_args}, got {res.returncode}"


# ============================================================================
# Loopback Functional Tests (Skipped when vsock_loopback is unavailable)
# ============================================================================

@require_vsock_loopback
def test_server_vsock_loopback_raw_socket(tmp_path: Path, apx_server_bin: str):
    """Test connecting to APX server over VSOCK loopback using a raw AF_VSOCK socket and performing RMFP handshake."""
    port = random.randint(52000, 54000)
    proc = run_vsock_server(tmp_path, apx_server_bin, vsock_port=port)
    try:
        sock = socket.socket(socket.AF_VSOCK, socket.SOCK_STREAM)
        sock.settimeout(3.0)
        cid_local = getattr(socket, "VMADDR_CID_LOCAL", 1)
        sock.connect((cid_local, port))

        # Send RMFP/1.1 greeting
        greeting = b"RMFP/1.1\nMessage-Size: 32\n\n"
        sock.sendall(bytes([len(greeting)]) + greeting)

        # Read server greeting response
        first_byte = sock.recv(1)
        assert len(first_byte) == 1
        c = first_byte[0]
        if c & 0x80:
            rem_header = sock.recv(3)
            msg_len = int.from_bytes(first_byte + rem_header, "big") & 0x7FFFFFFF
        else:
            msg_len = c

        resp = b""
        while len(resp) < msg_len:
            chunk = sock.recv(msg_len - len(resp))
            assert chunk
            resp += chunk

        # Server replies with RMF_CMD_ACCEPT_HEADER (opcode 20 = 0x14)
        assert resp[:8] == b"\xbf\xff\xfc\x00\x14\x00\x00\x00"
        sock.close()
    finally:
        proc.terminate()
        proc.wait(timeout=3.0)


@require_vsock_loopback
def test_server_vsock_loopback_apx_node_numeric_cid(tmp_path: Path, apx_server_bin: str, apx_node_bin: str):
    """Test apx-node connecting to APX server over VSOCK loopback using numeric CID (1:<port>)."""
    repo_root = Path(__file__).resolve().parent.parent
    node_file = str(repo_root / "example" / "nodes" / "small_unsigned_sender.apx")

    port = random.randint(52000, 54000)
    server_proc = run_vsock_server(tmp_path, apx_server_bin, vsock_port=port)
    try:
        cmd = [
            apx_node_bin,
            "--vsock", f"1:{port}",
            "--no-bind",
            node_file
        ]
        node_proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            preexec_fn=_set_pdeathsig
        )
        time.sleep(0.8)
        node_proc.terminate()
        stdout, stderr = node_proc.communicate(timeout=2.0)
        assert "connected to APX server" in stdout, f"Node stdout: {stdout}\nNode stderr: {stderr}"
    finally:
        server_proc.terminate()
        server_proc.wait(timeout=3.0)


@require_vsock_loopback
def test_server_vsock_loopback_apx_node_symbolic_alias(tmp_path: Path, apx_server_bin: str, apx_node_bin: str):
    """Test apx-node connecting to APX server over VSOCK loopback using the symbolic alias (local:<port>)."""
    repo_root = Path(__file__).resolve().parent.parent
    node_file = str(repo_root / "example" / "nodes" / "small_unsigned_sender.apx")

    port = random.randint(52000, 54000)
    server_proc = run_vsock_server(tmp_path, apx_server_bin, vsock_port=port)
    try:
        cmd = [
            apx_node_bin,
            "--vsock", f"local:{port}",
            "--no-bind",
            node_file
        ]
        node_proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            preexec_fn=_set_pdeathsig
        )
        time.sleep(0.8)
        node_proc.terminate()
        stdout, stderr = node_proc.communicate(timeout=2.0)
        assert "connected to APX server" in stdout, f"Node stdout: {stdout}\nNode stderr: {stderr}"
    finally:
        server_proc.terminate()
        server_proc.wait(timeout=3.0)
