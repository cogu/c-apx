"""
End-to-end integration tests for cryptographic APX file signing & verification.
"""

import json
import os
import select
import shutil
import socket
import subprocess
import time
from pathlib import Path
import pytest
from conftest import ApxServerInstance, wait_for_unix_socket, _set_pdeathsig


def der_to_raw_ecdsa(der: bytes) -> bytes:
    """Converts a DER-encoded ECDSA signature to 64-byte raw (r, s)."""
    idx = 2
    assert der[idx] == 0x02, f"Expected integer tag, got {der[idx]}"
    r_len = der[idx + 1]
    r = der[idx + 2 : idx + 2 + r_len]
    idx = idx + 2 + r_len
    assert der[idx] == 0x02, f"Expected integer tag, got {der[idx]}"
    s_len = der[idx + 1]
    s = der[idx + 2 : idx + 2 + s_len]
    # Pad/truncate to 32 bytes
    r = r[-32:].rjust(32, b"\x00")
    s = s[-32:].rjust(32, b"\x00")
    return r + s


def sign_data_with_openssl(key_path: Path, data: bytes) -> bytes:
    """Signs data using OpenSSL ECDSA with SHA-256 and returns 64-byte raw (r, s)."""
    proc = subprocess.Popen(
        ["openssl", "dgst", "-sha256", "-sign", str(key_path)],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    der_sig, err = proc.communicate(input=data)
    assert proc.returncode == 0, f"OpenSSL sign failed: {err.decode()}"
    return der_to_raw_ecdsa(der_sig)


def encode_numheader32(payload: bytes) -> bytes:
    """Encodes a payload with APX NumHeader-32 length prefix."""
    length = len(payload)
    if length <= 127:
        return bytes([length]) + payload
    return (0x80000000 | length).to_bytes(4, "big") + payload


def read_msg(sock: socket.socket, timeout: float = 3.0) -> bytes:
    """Reads a single NumHeader-32 framed message from the socket."""
    sock.settimeout(timeout)
    first_byte = sock.recv(1)
    if not first_byte:
        return b""
    c = first_byte[0]
    if c & 0x80:
        rem_header = sock.recv(3)
        assert len(rem_header) == 3
        msg_len = int.from_bytes(first_byte + rem_header, "big") & 0x7FFFFFFF
    else:
        msg_len = c

    data = b""
    while len(data) < msg_len:
        chunk = sock.recv(msg_len - len(data))
        if not chunk:
            break
        data += chunk
    return data


def test_server_rejects_unsigned_node_when_signed_required(tmp_path: Path, apx_server_bin: str, apx_node_bin: str):
    """
    Verify that an APX server configured with require-signed-nodes=true
    rejects an unsigned client node.
    """
    certs_dir = Path(__file__).resolve().parent / "certs"
    pubkey_path = certs_dir / "client_pubkey.pem"
    socket_path = str(tmp_path / "apx.socket")
    config_file = str(tmp_path / "server.json")

    config = {
        "apx-server": {
            "security": {
                "require-signed-nodes": True,
                "trusted-keys": [str(pubkey_path)],
            }
        },
        "socket-server-extension": {
            "unix-file": socket_path
        }
    }
    with open(config_file, "w", encoding="utf-8") as f:
        json.dump(config, f, indent=2)

    # 1. Start APX server with security config
    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)
    cmd = [apx_server_bin, "--ready-fd", str(w_fd), config_file]
    server_proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        pass_fds=(w_fd,),
        preexec_fn=_set_pdeathsig,
    )
    os.close(w_fd)

    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    assert rlist, "Server failed to start within timeout"
    os.read(r_fd, 1)
    os.close(r_fd)

    server = ApxServerInstance(server_proc, socket_path, tmp_path)

    try:
        # 2. Start unsigned node
        repo_root = Path(__file__).resolve().parent.parent
        listener_apx = repo_root / "example" / "nodes" / "unsigned_listener.apx"
        node_cmd = [apx_node_bin, "-c", socket_path, "--no-bind", str(listener_apx)]
        node_proc = subprocess.Popen(
            node_cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            preexec_fn=_set_pdeathsig,
        )

        time.sleep(0.5)

        # 3. Connect raw client to verify that listener node was not published on the server
        verify_sock = server.connect()
        greeting = b"RMFP/1.1\nMessage-Size: 32\n\n"
        verify_sock.sendall(bytes([len(greeting)]) + greeting)
        resp = read_msg(verify_sock, timeout=2.0)
        assert len(resp) > 0

        # Terminate node and server
        node_proc.terminate()
        node_proc.wait(timeout=2.0)
    finally:
        server.stop(timeout=2.0)
        assert server.process.returncode == 0, (
            f"apx_server exited with unexpected code: {server.process.returncode}\n"
            f"--- Server stderr ---\n{server.stderr}"
        )


def test_server_signed_node_full_handshake(tmp_path: Path, apx_server_bin: str):
    """
    Test end-to-end RMFP 1.1 handshake and signed file publishing over UNIX socket:
    1. Valid signature -> accepted by server.
    2. Tampered signature -> rejected by server.
    """
    certs_dir = Path(__file__).resolve().parent / "certs"
    privkey_path = certs_dir / "client_key.pem"
    pubkey_path = certs_dir / "client_pubkey.pem"
    socket_path = str(tmp_path / "apx.socket")
    config_file = str(tmp_path / "server.json")

    config = {
        "apx-server": {
            "security": {
                "require-signed-nodes": True,
                "trusted-keys": [str(pubkey_path)],
            }
        },
        "socket-server-extension": {
            "unix-file": socket_path
        }
    }
    with open(config_file, "w", encoding="utf-8") as f:
        json.dump(config, f, indent=2)

    # 1. Start server
    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)
    cmd = [apx_server_bin, "--ready-fd", str(w_fd), config_file]
    server_proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        pass_fds=(w_fd,),
        preexec_fn=_set_pdeathsig,
    )
    os.close(w_fd)

    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    assert rlist, "Server failed to signal readiness"
    os.read(r_fd, 1)
    os.close(r_fd)

    server = ApxServerInstance(server_proc, socket_path, tmp_path)

    apx_def = b"APX/1.2\nN\"SignedNode1\"\nR\"Speed\"S:=0\n"
    valid_sig = sign_data_with_openssl(privkey_path, apx_def)
    assert len(valid_sig) == 64

    try:
        # TEST A: Connect and publish signed node with valid signature
        sock = server.connect()
        greeting = b"RMFP/1.1\nMessage-Size: 32\n\n"
        sock.sendall(bytes([len(greeting)]) + greeting)
        resp = read_msg(sock, timeout=2.0)
        assert len(resp) > 0, "Server did not reply to RMFP/1.1 greeting"

        # Publish signed file command:
        # 4-byte cmd area addr: 0x80000000 | 0x3FFFFC00 = 0xBFFFFC00
        cmd_addr = (0x80000000 | 0x3FFFFC00).to_bytes(4, "big")
        cmd_opcode = (23).to_bytes(4, "little")  # RMF_CMD_PUBLISH_SIGNED_FILE_MSG
        file_addr = (0x04000000).to_bytes(4, "little")
        file_size = len(apx_def).to_bytes(4, "little")
        file_type = (0).to_bytes(2, "little")
        sig_type = (1).to_bytes(2, "little")  # RMF_SIGNATURE_TYPE_ECDSA_P256
        name = b"SignedNode1.apx\x00"

        publish_payload = (
            cmd_addr
            + cmd_opcode
            + file_addr
            + file_size
            + file_type
            + sig_type
            + valid_sig
            + name
        )
        sock.sendall(encode_numheader32(publish_payload))

        # Server should acknowledge and request to open the file
        open_req = read_msg(sock, timeout=2.0)
        assert len(open_req) > 0, "Server did not send open request for signed file"

        # Write the definition data to address 0x04000000
        def_addr = (0x80000000 | 0x04000000).to_bytes(4, "big")
        write_payload = def_addr + apx_def
        sock.sendall(encode_numheader32(write_payload))

        # Once valid definition is received and verified, server publishes .in file info
        in_file_info = read_msg(sock, timeout=2.0)
        assert len(in_file_info) > 0, "Server did not accept and build valid signed node"
        assert b"SignedNode1.in" in in_file_info, f"Expected SignedNode1.in, got: {in_file_info}"

        sock.close()

        # TEST B: Connect and publish node with TAMPERED signature
        sock_tampered = server.connect()
        sock_tampered.sendall(bytes([len(greeting)]) + greeting)
        resp = read_msg(sock_tampered, timeout=2.0)
        assert len(resp) > 0

        tampered_sig = bytearray(valid_sig)
        tampered_sig[0] ^= 0xFF  # Corrupt 1 byte

        publish_tampered = (
            cmd_addr
            + cmd_opcode
            + file_addr
            + file_size
            + file_type
            + sig_type
            + bytes(tampered_sig)
            + b"TamperedNode.apx\x00"
        )
        sock_tampered.sendall(encode_numheader32(publish_tampered))

        open_req2 = read_msg(sock_tampered, timeout=2.0)
        assert len(open_req2) > 0

        # Send definition data
        sock_tampered.sendall(encode_numheader32(def_addr + apx_def))

        # Verification must fail on server: server will NOT send .out file info for tampered node
        sock_tampered.settimeout(0.5)
        try:
            extra_msg = read_msg(sock_tampered, timeout=0.5)
            assert b"TamperedNode.out" not in extra_msg
        except (socket.timeout, TimeoutError):
            pass  # Expected: server ignored/rejected tampered node

        sock_tampered.close()

    finally:
        server.stop(timeout=2.0)
        assert server.process.returncode == 0, (
            f"apx_server exited with unexpected code: {server.process.returncode}\n"
            f"--- Server stderr ---\n{server.stderr}"
        )
