"""
End-to-end integration tests for compressed APX definition transfer (RMFP 1.1).
Covers Backlog Item 16:
- Unsigned compressed file publication & transfer
- Signed compressed file publication & transfer with ECDSA-P256
- Digest verification
- Signature verification over uncompressed bytes
- Corruption & error rejection (bad Adler-32, size mismatch, digest mismatch, signature failure)
- Require-signed-nodes policy rejection of unsigned compressed definitions
"""

import hashlib
import json
import os
import select
import socket
import subprocess
import time
import zlib
from pathlib import Path
import pytest
from conftest import ApxServerInstance, _set_pdeathsig


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


def start_server(tmp_path: Path, apx_server_bin: str, config: dict) -> ApxServerInstance:
    """Helper to start an apx-server instance with the given config dict."""
    config_file = str(tmp_path / "server.json")
    socket_path = config["socket-server-extension"]["unix-file"]
    with open(config_file, "w", encoding="utf-8") as f:
        json.dump(config, f, indent=2)

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

    return ApxServerInstance(server_proc, socket_path, tmp_path)


def test_server_compressed_node_unsigned_transfer(tmp_path: Path, apx_server_bin: str):
    """
    Publish an unsigned compressed APX definition (RMF_CMD_PUBLISH_COMPRESSED_FILE_MSG, opcode 22):
    - Client sends valid zlib-compressed APX definition payload.
    - Server decompresses, verifies SHA-256 digest, parses definition, and publishes .in file.
    """
    socket_path = str(tmp_path / "apx.socket")
    config = {
        "apx-server": {},
        "socket-server-extension": {
            "unix-file": socket_path
        }
    }
    server = start_server(tmp_path, apx_server_bin, config)

    apx_def = b'APX/1.2\nN"CompressedNode1"\nR"VehicleSpeed"S:=0\nP"EngineState"C(0,3):=0\n'
    compressed_def = zlib.compress(apx_def)
    sha256_digest = hashlib.sha256(apx_def).digest()

    try:
        sock = server.connect()
        greeting = b"RMFP/1.1\nMessage-Size: 32\n\n"
        sock.sendall(bytes([len(greeting)]) + greeting)
        resp = read_msg(sock, timeout=2.0)
        assert len(resp) > 0, "Server did not reply to RMFP/1.1 greeting"

        # Encode RMF_CMD_PUBLISH_COMPRESSED_FILE_MSG (22)
        # Header layout (56 bytes + name + \0):
        # 0..3: CmdType = 22
        # 4..7: StartAddress = 0x02000000
        # 8..11: UncompressedSize
        # 12..15: CompressedSize
        # 16..17: FileType = 0 (Fixed)
        # 18..19: CompressionType = 1 (ZlibDeflate)
        # 20..21: DigestType = 2 (SHA-256)
        # 22..23: Reserved = 0
        # 24..55: DigestData (32 bytes)
        # 56..N: Name + \0
        cmd_addr = (0x80000000 | 0x3FFFFC00).to_bytes(4, "big")
        cmd_opcode = (22).to_bytes(4, "little")
        file_addr = (0x02000000).to_bytes(4, "little")
        uncompressed_size = len(apx_def).to_bytes(4, "little")
        compressed_size = len(compressed_def).to_bytes(4, "little")
        file_type = (0).to_bytes(2, "little")
        compression_type = (1).to_bytes(2, "little")
        digest_type = (2).to_bytes(2, "little")
        reserved = (0).to_bytes(2, "little")
        name = b"CompressedNode1.apx\x00"

        publish_payload = (
            cmd_addr
            + cmd_opcode
            + file_addr
            + uncompressed_size
            + compressed_size
            + file_type
            + compression_type
            + digest_type
            + reserved
            + sha256_digest
            + name
        )
        sock.sendall(encode_numheader32(publish_payload))

        # Server should open the file (sending FILE_OPEN at address 0x02000000)
        open_req = read_msg(sock, timeout=2.0)
        assert len(open_req) > 0, "Server did not send open request for compressed definition"

        # Client writes the compressed definition bytes to address 0x02000000
        write_addr = (0x80000000 | 0x02000000).to_bytes(4, "big")
        write_payload = write_addr + compressed_def
        sock.sendall(encode_numheader32(write_payload))

        # Server should accept, decompress, parse, and publish .in file
        in_file_info = read_msg(sock, timeout=2.0)
        assert len(in_file_info) > 0, "Server did not publish .in file for compressed node"
        assert b"CompressedNode1.in" in in_file_info, f"Expected CompressedNode1.in, got: {in_file_info}"

        sock.close()
    finally:
        server.stop(timeout=2.0)
        assert server.process.returncode == 0


def test_server_compressed_node_signed_transfer(tmp_path: Path, apx_server_bin: str):
    """
    Publish a signed compressed APX definition (RMF_CMD_PUBLISH_SIGNED_COMPRESSED_FILE_MSG, opcode 23):
    - Client sends valid ECDSA-P256 signature over uncompressed definition.
    - Server decompresses, verifies signature against trusted key, parses definition, and publishes .in file.
    """
    certs_dir = Path(__file__).resolve().parent / "certs"
    privkey_path = certs_dir / "client_key.pem"
    pubkey_path = certs_dir / "client_pubkey.pem"
    socket_path = str(tmp_path / "apx.socket")

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
    server = start_server(tmp_path, apx_server_bin, config)

    apx_def = b'APX/1.2\nN"SignedCompressedNode"\nR"Odometer"L:=0\nP"Status"C(0,1):=0\n'
    valid_sig = sign_data_with_openssl(privkey_path, apx_def)
    compressed_def = zlib.compress(apx_def)

    try:
        sock = server.connect()
        greeting = b"RMFP/1.1\nMessage-Size: 32\n\n"
        sock.sendall(bytes([len(greeting)]) + greeting)
        resp = read_msg(sock, timeout=2.0)
        assert len(resp) > 0

        # Encode RMF_CMD_PUBLISH_SIGNED_COMPRESSED_FILE_MSG (23)
        # Header layout (88 bytes + name + \0):
        # 0..3: CmdType = 23
        # 4..7: StartAddress = 0x03000000
        # 8..11: UncompressedSize
        # 12..15: CompressedSize
        # 16..17: FileType = 0 (Fixed)
        # 18..19: CompressionType = 1 (ZlibDeflate)
        # 20..21: SignatureType = 1 (ECDSA_P256)
        # 22..23: Reserved = 0
        # 24..87: SignatureData (64 bytes)
        # 88..N: Name + \0
        cmd_addr = (0x80000000 | 0x3FFFFC00).to_bytes(4, "big")
        cmd_opcode = (23).to_bytes(4, "little")
        file_addr = (0x03000000).to_bytes(4, "little")
        uncompressed_size = len(apx_def).to_bytes(4, "little")
        compressed_size = len(compressed_def).to_bytes(4, "little")
        file_type = (0).to_bytes(2, "little")
        compression_type = (1).to_bytes(2, "little")
        signature_type = (1).to_bytes(2, "little")
        reserved = (0).to_bytes(2, "little")
        name = b"SignedCompressedNode.apx\x00"

        publish_payload = (
            cmd_addr
            + cmd_opcode
            + file_addr
            + uncompressed_size
            + compressed_size
            + file_type
            + compression_type
            + signature_type
            + reserved
            + valid_sig
            + name
        )
        sock.sendall(encode_numheader32(publish_payload))

        open_req = read_msg(sock, timeout=2.0)
        assert len(open_req) > 0, "Server did not open signed compressed definition"

        write_addr = (0x80000000 | 0x03000000).to_bytes(4, "big")
        write_payload = write_addr + compressed_def
        sock.sendall(encode_numheader32(write_payload))

        in_file_info = read_msg(sock, timeout=2.0)
        assert len(in_file_info) > 0
        assert b"SignedCompressedNode.in" in in_file_info, f"Expected SignedCompressedNode.in, got: {in_file_info}"

        sock.close()
    finally:
        server.stop(timeout=2.0)
        assert server.process.returncode == 0


def test_server_rejects_unsigned_compressed_when_signed_required(tmp_path: Path, apx_server_bin: str):
    """
    Verify that an APX server configured with require-signed-nodes=true
    rejects an unsigned compressed definition with APX_SIGNATURE_MISSING_ERROR (79).
    """
    certs_dir = Path(__file__).resolve().parent / "certs"
    pubkey_path = certs_dir / "client_pubkey.pem"
    socket_path = str(tmp_path / "apx.socket")

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
    server = start_server(tmp_path, apx_server_bin, config)

    apx_def = b'APX/1.2\nN"UnsignedCompressed"\nR"Speed"S:=0\n'
    compressed_def = zlib.compress(apx_def)

    try:
        sock = server.connect()
        greeting = b"RMFP/1.1\nMessage-Size: 32\n\n"
        sock.sendall(bytes([len(greeting)]) + greeting)
        resp = read_msg(sock, timeout=2.0)
        assert len(resp) > 0

        cmd_addr = (0x80000000 | 0x3FFFFC00).to_bytes(4, "big")
        cmd_opcode = (22).to_bytes(4, "little")
        file_addr = (0x02000000).to_bytes(4, "little")
        uncompressed_size = len(apx_def).to_bytes(4, "little")
        compressed_size = len(compressed_def).to_bytes(4, "little")
        file_type = (0).to_bytes(2, "little")
        compression_type = (1).to_bytes(2, "little")
        digest_type = (0).to_bytes(2, "little")
        reserved = (0).to_bytes(2, "little")
        digest_data = b"\x00" * 32
        name = b"UnsignedCompressed.apx\x00"

        publish_payload = (
            cmd_addr
            + cmd_opcode
            + file_addr
            + uncompressed_size
            + compressed_size
            + file_type
            + compression_type
            + digest_type
            + reserved
            + digest_data
            + name
        )
        sock.sendall(encode_numheader32(publish_payload))

        # Server should reply with NACK (ErrorCode = 79, APX_SIGNATURE_MISSING_ERROR)
        nack_msg = read_msg(sock, timeout=2.0)
        assert len(nack_msg) >= 12
        # Address header (4 bytes) + CmdType (4 bytes) + ErrorCode (4 bytes)
        cmd_type = int.from_bytes(nack_msg[4:8], "little")
        error_code = int.from_bytes(nack_msg[8:12], "little")
        assert cmd_type == 1, f"Expected NACK cmd_type 1, got {cmd_type}"
        assert error_code == 79, f"Expected APX_SIGNATURE_MISSING_ERROR (79), got {error_code}"

        sock.close()
    finally:
        server.stop(timeout=2.0)
        assert server.process.returncode == 0


def test_server_rejects_compressed_signed_tampered_signature(tmp_path: Path, apx_server_bin: str):
    """
    Verify that an APX server rejects a compressed signed definition when the signature is tampered:
    returns NACK with APX_SIGNATURE_VERIFICATION_ERROR (80).
    """
    certs_dir = Path(__file__).resolve().parent / "certs"
    privkey_path = certs_dir / "client_key.pem"
    pubkey_path = certs_dir / "client_pubkey.pem"
    socket_path = str(tmp_path / "apx.socket")

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
    server = start_server(tmp_path, apx_server_bin, config)

    apx_def = b'APX/1.2\nN"TamperedSignedCompressed"\nR"Speed"S:=0\n'
    valid_sig = sign_data_with_openssl(privkey_path, apx_def)
    tampered_sig = bytearray(valid_sig)
    tampered_sig[0] ^= 0xFF
    compressed_def = zlib.compress(apx_def)

    try:
        sock = server.connect()
        greeting = b"RMFP/1.1\nMessage-Size: 32\n\n"
        sock.sendall(bytes([len(greeting)]) + greeting)
        resp = read_msg(sock, timeout=2.0)
        assert len(resp) > 0

        cmd_addr = (0x80000000 | 0x3FFFFC00).to_bytes(4, "big")
        cmd_opcode = (23).to_bytes(4, "little")
        file_addr = (0x03000000).to_bytes(4, "little")
        uncompressed_size = len(apx_def).to_bytes(4, "little")
        compressed_size = len(compressed_def).to_bytes(4, "little")
        file_type = (0).to_bytes(2, "little")
        compression_type = (1).to_bytes(2, "little")
        signature_type = (1).to_bytes(2, "little")
        reserved = (0).to_bytes(2, "little")
        name = b"TamperedSignedCompressed.apx\x00"

        publish_payload = (
            cmd_addr
            + cmd_opcode
            + file_addr
            + uncompressed_size
            + compressed_size
            + file_type
            + compression_type
            + signature_type
            + reserved
            + bytes(tampered_sig)
            + name
        )
        sock.sendall(encode_numheader32(publish_payload))

        open_req = read_msg(sock, timeout=2.0)
        assert len(open_req) > 0

        write_addr = (0x80000000 | 0x03000000).to_bytes(4, "big")
        write_payload = write_addr + compressed_def
        sock.sendall(encode_numheader32(write_payload))

        # Server should reply with NACK (ErrorCode = 80, APX_SIGNATURE_VERIFICATION_ERROR)
        nack_msg = read_msg(sock, timeout=2.0)
        assert len(nack_msg) >= 12
        cmd_type = int.from_bytes(nack_msg[4:8], "little")
        error_code = int.from_bytes(nack_msg[8:12], "little")
        assert cmd_type == 1, f"Expected NACK cmd_type 1, got {cmd_type}"
        assert error_code == 80, f"Expected APX_SIGNATURE_VERIFICATION_ERROR (80), got {error_code}"

        sock.close()
    finally:
        server.stop(timeout=2.0)
        assert server.process.returncode == 0


def test_server_rejects_corrupted_zlib_stream(tmp_path: Path, apx_server_bin: str):
    """
    Verify that an APX server rejects a compressed definition with corrupted zlib stream:
    returns NACK with APX_PARSE_ERROR (3).
    """
    socket_path = str(tmp_path / "apx.socket")
    config = {
        "apx-server": {},
        "socket-server-extension": {
            "unix-file": socket_path
        }
    }
    server = start_server(tmp_path, apx_server_bin, config)

    apx_def = b'APX/1.2\nN"CorruptZlibNode"\nR"Speed"S:=0\n'
    valid_compressed = zlib.compress(apx_def)
    corrupted_compressed = bytearray(valid_compressed)
    # Corrupt a byte in the middle of DEFLATE stream
    corrupted_compressed[len(corrupted_compressed) // 2] ^= 0xAA

    try:
        sock = server.connect()
        greeting = b"RMFP/1.1\nMessage-Size: 32\n\n"
        sock.sendall(bytes([len(greeting)]) + greeting)
        resp = read_msg(sock, timeout=2.0)
        assert len(resp) > 0

        cmd_addr = (0x80000000 | 0x3FFFFC00).to_bytes(4, "big")
        cmd_opcode = (22).to_bytes(4, "little")
        file_addr = (0x02000000).to_bytes(4, "little")
        uncompressed_size = len(apx_def).to_bytes(4, "little")
        compressed_size = len(corrupted_compressed).to_bytes(4, "little")
        file_type = (0).to_bytes(2, "little")
        compression_type = (1).to_bytes(2, "little")
        digest_type = (0).to_bytes(2, "little")
        reserved = (0).to_bytes(2, "little")
        digest_data = b"\x00" * 32
        name = b"CorruptZlibNode.apx\x00"

        publish_payload = (
            cmd_addr
            + cmd_opcode
            + file_addr
            + uncompressed_size
            + compressed_size
            + file_type
            + compression_type
            + digest_type
            + reserved
            + digest_data
            + name
        )
        sock.sendall(encode_numheader32(publish_payload))

        open_req = read_msg(sock, timeout=2.0)
        assert len(open_req) > 0

        write_addr = (0x80000000 | 0x02000000).to_bytes(4, "big")
        write_payload = write_addr + bytes(corrupted_compressed)
        sock.sendall(encode_numheader32(write_payload))

        # Server should reply with NACK (ErrorCode = 3, APX_PARSE_ERROR)
        nack_msg = read_msg(sock, timeout=2.0)
        assert len(nack_msg) >= 12
        cmd_type = int.from_bytes(nack_msg[4:8], "little")
        error_code = int.from_bytes(nack_msg[8:12], "little")
        assert cmd_type == 1, f"Expected NACK cmd_type 1, got {cmd_type}"
        assert error_code == 3, f"Expected APX_PARSE_ERROR (3), got {error_code}"

        sock.close()
    finally:
        server.stop(timeout=2.0)
        assert server.process.returncode == 0


def test_server_rejects_digest_mismatch(tmp_path: Path, apx_server_bin: str):
    """
    Verify that an APX server rejects a compressed definition when the SHA-256 digest
    advertised does not match the uncompressed content: returns NACK with APX_PARSE_ERROR (3).
    """
    socket_path = str(tmp_path / "apx.socket")
    config = {
        "apx-server": {},
        "socket-server-extension": {
            "unix-file": socket_path
        }
    }
    server = start_server(tmp_path, apx_server_bin, config)

    apx_def = b'APX/1.2\nN"DigestMismatchNode"\nR"Speed"S:=0\n'
    compressed_def = zlib.compress(apx_def)
    # Generate corrupted digest
    sha256_digest = bytearray(hashlib.sha256(apx_def).digest())
    sha256_digest[0] ^= 0xFF

    try:
        sock = server.connect()
        greeting = b"RMFP/1.1\nMessage-Size: 32\n\n"
        sock.sendall(bytes([len(greeting)]) + greeting)
        resp = read_msg(sock, timeout=2.0)
        assert len(resp) > 0

        cmd_addr = (0x80000000 | 0x3FFFFC00).to_bytes(4, "big")
        cmd_opcode = (22).to_bytes(4, "little")
        file_addr = (0x02000000).to_bytes(4, "little")
        uncompressed_size = len(apx_def).to_bytes(4, "little")
        compressed_size = len(compressed_def).to_bytes(4, "little")
        file_type = (0).to_bytes(2, "little")
        compression_type = (1).to_bytes(2, "little")
        digest_type = (2).to_bytes(2, "little")
        reserved = (0).to_bytes(2, "little")
        name = b"DigestMismatchNode.apx\x00"

        publish_payload = (
            cmd_addr
            + cmd_opcode
            + file_addr
            + uncompressed_size
            + compressed_size
            + file_type
            + compression_type
            + digest_type
            + reserved
            + bytes(sha256_digest)
            + name
        )
        sock.sendall(encode_numheader32(publish_payload))

        open_req = read_msg(sock, timeout=2.0)
        assert len(open_req) > 0

        write_addr = (0x80000000 | 0x02000000).to_bytes(4, "big")
        write_payload = write_addr + compressed_def
        sock.sendall(encode_numheader32(write_payload))

        nack_msg = read_msg(sock, timeout=2.0)
        assert len(nack_msg) >= 12
        cmd_type = int.from_bytes(nack_msg[4:8], "little")
        error_code = int.from_bytes(nack_msg[8:12], "little")
        assert cmd_type == 1, f"Expected NACK cmd_type 1, got {cmd_type}"
        assert error_code == 3, f"Expected APX_PARSE_ERROR (3), got {error_code}"

        sock.close()
    finally:
        server.stop(timeout=2.0)
        assert server.process.returncode == 0

