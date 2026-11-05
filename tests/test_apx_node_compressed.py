"""Explicit C-client definition publication through apx-node."""

import hashlib
import socket
import struct
import subprocess
import zlib
from pathlib import Path

import pytest

from conftest import _set_pdeathsig
from test_server_compressed_nodes import encode_numheader32, read_msg, sign_data_with_openssl


def decode_address(message):
    header_size = 4 if message[0] & 0x80 else 2
    encoded = int.from_bytes(message[:header_size], "big")
    mask = 0x3FFFFFFF if header_size == 4 else 0x3FFF
    more_bit = 0x40000000 if header_size == 4 else 0x4000
    return encoded & mask, bool(encoded & more_bit), message[header_size:]


def read_definition(peer, compressed, signed):
    greeting = read_msg(peer)
    assert greeting.startswith(b"RMFP/1.1")
    accepted = struct.pack(">I", 0xBFFFFC00) + struct.pack("<II", 20, 1)
    peer.sendall(encode_numheader32(accepted))
    message = read_msg(peer)
    address, more, command = decode_address(message)
    assert address == 0x3FFFFC00
    assert not more
    opcode, start, first_size = struct.unpack_from("<III", command)
    expected_opcode = (23 if signed else 22) if compressed else (21 if signed else 3)
    assert opcode == expected_opcode
    if compressed:
        logical_size = first_size
        encoded_size = struct.unpack_from("<I", command, 12)[0]
        assert struct.unpack_from("<H", command, 18)[0] == 1
        integrity_type = struct.unpack_from("<H", command, 20)[0]
        integrity = command[24:88 if signed else 56]
        name = command[88 if signed else 56:]
    else:
        logical_size = encoded_size = first_size
        integrity_type = struct.unpack_from("<H", command, 14)[0]
        integrity = command[16:80 if signed else 48]
        name = command[80 if signed else 48:]
    assert integrity_type == (1 if signed else 2)
    assert name == b"Sender.apx\x00"
    opened = struct.pack(">I", 0xBFFFFC00) + struct.pack("<II", 10, start)
    peer.sendall(encode_numheader32(opened))
    payload = b""
    fragments = 0
    while True:
        address, more, data = decode_address(read_msg(peer))
        assert address == start + len(payload)
        payload += data
        fragments += 1
        assert len(payload) <= encoded_size
        if not more:
            break
    assert len(payload) == encoded_size
    return payload, logical_size, integrity, fragments


@pytest.mark.parametrize("compressed", [False, True])
@pytest.mark.parametrize("signed", [False, True])
@pytest.mark.parametrize("small", [False, True])
def test_apx_node_definition_representation(tmp_path, apx_node_bin, compressed, signed, small):
    text = b'APX/1.2\nN"Sender"\n'
    if not small:
        text += "".join(
            f'R"Value{i}_{hashlib.sha256(str(i).encode()).hexdigest()[:16]}"C:=0\n'
            for i in range(800)
        ).encode()
    definition = tmp_path / "Sender.apx"
    definition.write_bytes(text)
    signature = None
    if signed:
        signature = sign_data_with_openssl(Path(__file__).parent / "certs/client_key.pem", text)
        Path(str(definition) + ".sig").write_bytes(signature)
    path = str(tmp_path / "peer.socket")
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as listener:
        listener.bind(path)
        listener.listen(1)
        listener.settimeout(5)
        arguments = [apx_node_bin, "--no-bind", "-c", path]
        if compressed:
            arguments.append("--compressed")
        arguments.append(str(definition))
        process = subprocess.Popen(
            arguments, stdout=subprocess.PIPE, stderr=subprocess.PIPE, preexec_fn=_set_pdeathsig,
        )
        try:
            peer, _ = listener.accept()
            with peer:
                payload, logical_size, integrity, fragments = read_definition(peer, compressed, signed)
                assert logical_size == len(text)
                assert (zlib.decompress(payload) if compressed else payload) == text
                assert integrity == (signature if signed else hashlib.sha256(text).digest())
                if small and compressed:
                    assert len(payload) > len(text), "Explicit compression must not use a size heuristic"
                if not small:
                    assert fragments > 1, "Large definitions must exercise transfer fragmentation"
        finally:
            if process.poll() is None:
                process.terminate()
            stdout, stderr = process.communicate(timeout=5)
            assert process.returncode == 0, (stdout, stderr)


def test_apx_node_compressed_flag_has_no_argument(apx_node_bin):
    result = subprocess.run([apx_node_bin, "--help"], capture_output=True, text=True, check=True)
    assert "--compressed" in result.stdout
    result = subprocess.run([apx_node_bin, "--compressed=9", "--help"], capture_output=True, text=True)
    assert result.returncode != 0
    result = subprocess.run([apx_node_bin, "--compression-level=9", "--help"], capture_output=True, text=True)
    assert result.returncode != 0


@pytest.mark.parametrize("compressed", [False, True])
def test_apx_node_compressed_server_roundtrip(tmp_path, spawn_apx_node, compressed):
    sender_file = tmp_path / "Sender.apx"
    listener_file = tmp_path / "Listener.apx"
    sender_file.write_text('APX/1.3\nN"Sender"\nP"Speed"S:=123\n', encoding="utf-8")
    listener_file.write_text('APX/1.3\nN"Listener"\nR"Speed"S:=0\n', encoding="utf-8")
    listener = spawn_apx_node(listener_file, bind=False)
    sender = spawn_apx_node(sender_file, bind=True, extra_args=["--compressed"] if compressed else [])
    assert listener.wait_for_output('"Speed": 123'), listener.output
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as control:
        control.connect(sender.bind_path)
        control.sendall(b'{"Speed": 321}\n')
    assert listener.wait_for_output('"Speed": 321'), listener.output
