"""Definition admission, failure events, and logical definition capture."""

import hashlib
import json
import signal
import struct
import subprocess
import time
import zlib
from contextlib import ExitStack, contextmanager
from pathlib import Path

import pytest

from test_server_compressed_nodes import (
    encode_numheader32,
    read_msg,
    sign_data_with_openssl,
    start_server,
)

ADDRESS = 0x02000000
NAME = "DefinitionEvents"
DEFINITION = (
    f'APX/1.2\nN"{NAME}"\n'
    + "".join(f'R"Value{i}"C:=0\n' for i in range(64))
).encode()


@pytest.fixture
def definition_server(tmp_path, apx_server_bin, request):
    certs = Path(__file__).parent / "certs"
    server = start_server(tmp_path, apx_server_bin, {
        "apx-server": {"security": {
            "trusted-keys": [str(certs / "client_pubkey.pem")],
            "require-signed-nodes": getattr(request, "param", False),
        }},
        "socket-server-extension": {"unix-file": str(tmp_path / "apx.socket")},
        "monitor-extension": {"enabled": True, "socket-file": str(tmp_path / "monitor.socket")},
    })
    server.monitor_socket = str(tmp_path / "monitor.socket")
    yield server
    server.stop()
    assert server.process.returncode == 0, server.stderr


@contextmanager
def client(server):
    sock = server.connect()
    try:
        sock.sendall(encode_numheader32(b"RMFP/1.1\nMessage-Size: 32\n\n"))
        assert read_msg(sock)
        yield sock
    finally:
        sock.close()


def publish(sock, text=DEFINITION, *, name=NAME, compressed=True, payload=None,
            decoded_size=None, digest=None, signature=None, address=ADDRESS):
    if payload is None:
        payload = zlib.compress(text) if compressed else text
    digest = hashlib.sha256(text).digest() if digest is None else digest
    if compressed:
        header = struct.pack(
            "<IIIIHHHH", 23 if signature is not None else 22, address,
            len(text) if decoded_size is None else decoded_size, len(payload),
            0, 1, 1 if signature is not None else 2, 0,
        )
    else:
        header = struct.pack(
            "<IIIHH", 21 if signature is not None else 3, address, len(payload),
            0, 1 if signature is not None else 2,
        )
    command = header + (signature if signature is not None else digest) + f"{name}.apx\0".encode()
    sock.sendall(encode_numheader32(struct.pack(">I", 0xBFFFFC00) + command))
    return payload, read_msg(sock)


def write(sock, payload, *, address=ADDRESS, offset=0, more=False):
    encoded_address = 0x80000000 | (address + offset) | (0x40000000 if more else 0)
    sock.sendall(encode_numheader32(struct.pack(">I", encoded_address) + payload))


def opcode(message):
    assert len(message) >= 8
    return int.from_bytes(message[4:8], "little")


def assert_nack(message, error):
    assert opcode(message) == 1
    assert int.from_bytes(message[8:12], "little") == error


@contextmanager
def capture(server, apx_log_bin, path):
    recorder = subprocess.Popen(
        [apx_log_bin, "record", "-c", server.monitor_socket, str(path)],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    )
    try:
        time.sleep(0.8)
        assert recorder.poll() is None
        assert path.exists()
        yield
    finally:
        if recorder.poll() is None:
            recorder.send_signal(signal.SIGINT)
        stdout, stderr = recorder.communicate(timeout=5)
        assert recorder.returncode == 0, (stdout, stderr)


def events(apx_log_bin, path):
    result = subprocess.run(
        [apx_log_bin, "print", "--json", str(path)], capture_output=True, text=True, check=True,
    )
    return [json.loads(line) for line in result.stdout.splitlines()]


def assert_definition(records, text):
    publications = [e for e in records if e.get("file_name") == f"{NAME}.apx"
                    and e["event"] == "FILE_PUBLISH"]
    assert len(publications) == 1
    publication = publications[0]
    assert publication["file_size"] == len(text)
    writes = [e for e in records if e["event"] == "FILE_WRITE"
              and e.get("object_id") == publication["object_id"]
              and e.get("connection_id") == publication["connection_id"]]
    assert writes
    offset = 0
    reconstructed = b""
    for entry in writes:
        assert entry["seq"] > publication["seq"]
        assert entry["file_offset"] == offset
        data = bytes.fromhex(entry["data_hex"])
        reconstructed += data
        offset += len(data)
        assert offset <= publication["file_size"]
    assert reconstructed == text


@pytest.mark.parametrize("compressed", [False, True])
@pytest.mark.parametrize("signed", [False, True])
def test_logical_definition_live_and_snapshot(
    definition_server, apx_log_bin, tmp_path, compressed, signed,
):
    signature = None
    if signed:
        signature = sign_data_with_openssl(Path(__file__).parent / "certs/client_key.pem", DEFINITION)
    live = tmp_path / "live.apxlog"
    snapshot = tmp_path / "snapshot.apxlog"
    with client(definition_server) as sock:
        with capture(definition_server, apx_log_bin, live):
            payload, response = publish(sock, compressed=compressed, signature=signature)
            assert opcode(response) == 10
            for offset in range(0, len(payload), 7):
                fragment = payload[offset:offset + 7]
                write(sock, fragment, offset=offset, more=offset + len(fragment) < len(payload))
            assert f"{NAME}.in".encode() in read_msg(sock)
            time.sleep(0.2)
        with capture(definition_server, apx_log_bin, snapshot):
            pass
    assert_definition(events(apx_log_bin, live), DEFINITION)
    assert_definition(events(apx_log_bin, snapshot), DEFINITION)


@pytest.mark.parametrize("failure,compressed,stage,error,response_code", [
    ("checksum", True, "decompression", 86, 3),
    ("trailing", True, "decompression", 45, 3),
    ("size", True, "decompression", 7, 3),
    ("digest", True, "digest", 86, 3),
    ("digest", False, "digest", 86, 3),
    ("parsing", True, "parsing", 3, 3),
    ("parsing", False, "parsing", 3, 3),
    ("signature", True, "signature", 80, 80),
])
def test_definition_rejection_event(
    definition_server, apx_log_bin, tmp_path, failure, compressed, stage, error, response_code,
):
    text = b"not an APX definition\n" if failure == "parsing" else DEFINITION
    payload = zlib.compress(text) if compressed else text
    options = {}
    if failure == "checksum":
        payload = payload[:-1] + bytes([payload[-1] ^ 1])
    elif failure == "trailing":
        payload += b"trailing"
    elif failure == "size":
        options["decoded_size"] = len(text) + 1
    elif failure == "digest":
        options["digest"] = b"\x55" * 32
    elif failure == "signature":
        options["signature"] = b"\x55" * 64
    path = tmp_path / "rejection.apxlog"
    with client(definition_server) as sock:
        with capture(definition_server, apx_log_bin, path):
            _, response = publish(sock, text, compressed=compressed, payload=payload, **options)
            assert opcode(response) == 10
            write(sock, payload)
            assert_nack(read_msg(sock), response_code)
            write(sock, payload)
            # A subsequent valid node proves rejection did not damage the connection.
            name = "Recovered"
            valid = b'APX/1.2\nN"Recovered"\nR"Value"C:=0\n'
            encoded, response = publish(sock, valid, name=name, address=ADDRESS + 0x10000)
            assert opcode(response) == 10
            write(sock, encoded, address=ADDRESS + 0x10000)
            assert b"Recovered.in" in read_msg(sock)
            time.sleep(0.2)
    records = events(apx_log_bin, path)
    rejected = [e for e in records if e["event"] == "NODE_DEFINITION_REJECTED"]
    assert len(rejected) == 1
    entry = rejected[0]
    assert entry["file_name"] == f"{NAME}.apx"
    assert entry["connection_id"] >= 0
    assert entry["transfer_encoding"] == ("zlib-deflate" if compressed else "uncompressed")
    assert entry["failure_stage"] == stage
    assert entry["error_code"] == error
    assert entry["response_code"] == response_code
    assert entry["encoded_size"] == len(payload)
    assert not any(e["event"] == "FILE_PUBLISH" and e.get("file_name") == f"{NAME}.apx" for e in records)
    printed = subprocess.run(
        [apx_log_bin, "print", str(path)], capture_output=True, text=True, check=True,
    ).stdout
    assert f"stage={stage}" in printed
    assert "NODE_DEFINITION_REJECTED" in printed


@pytest.mark.parametrize("kind,error", [
    ("premature-final", 59), ("final-more", 59), ("overlap", 56), ("overflow", 32),
    ("empty-fragment", 70),
])
def test_invalid_compressed_fragment_rejected(definition_server, kind, error):
    with client(definition_server) as sock:
        payload, response = publish(sock)
        assert opcode(response) == 10
        if kind == "premature-final":
            write(sock, payload[:3])
        elif kind == "final-more":
            write(sock, payload, more=True)
        elif kind == "overlap":
            write(sock, payload[:3], more=True)
            write(sock, payload[3:], offset=2)
        elif kind == "overflow":
            write(sock, payload + b"\x00")
        else:
            write(sock, b"", more=True)
        assert_nack(read_msg(sock), error)
        write(sock, payload)
        assert definition_server.is_running


def test_oversize_definition_rejected_before_open(definition_server):
    with client(definition_server) as sock:
        _, response = publish(sock, decoded_size=0x1000001)
        assert_nack(response, 32)


def test_pending_definition_is_not_snapshotted(definition_server, apx_log_bin, tmp_path):
    path = tmp_path / "pending.apxlog"
    with client(definition_server) as sock:
        payload, response = publish(sock)
        assert opcode(response) == 10
        write(sock, payload[:3], more=True)
        with capture(definition_server, apx_log_bin, path):
            pass
        # Revocation releases the pending node and its reservation.
        command = struct.pack("<II", 4, ADDRESS)
        sock.sendall(encode_numheader32(struct.pack(">I", 0xBFFFFC00) + command))
        assert_nack(read_msg(sock), 59)
    records = events(apx_log_bin, path)
    assert not any(e.get("file_name") == f"{NAME}.apx" for e in records)
    assert not any(e["event"] == "NODE_ATTACH" and e.get("node_name") == NAME for e in records)


@pytest.mark.parametrize("definition_server", [True], indirect=True)
@pytest.mark.parametrize("compressed", [False, True])
def test_signature_policy_rejection_is_logged(definition_server, apx_log_bin, tmp_path, compressed):
    path = tmp_path / "policy.apxlog"
    with client(definition_server) as sock:
        with capture(definition_server, apx_log_bin, path):
            _, response = publish(sock, compressed=compressed)
            assert_nack(response, 79)
            time.sleep(0.2)
    rejected = [e for e in events(apx_log_bin, path) if e["event"] == "NODE_DEFINITION_REJECTED"]
    assert len(rejected) == 1
    assert rejected[0]["failure_stage"] == "admission"
    assert rejected[0]["error_code"] == 79
    assert rejected[0]["transfer_encoding"] == ("zlib-deflate" if compressed else "uncompressed")


@pytest.mark.parametrize("release", ["rejection", "revocation", "disconnect"])
def test_aggregate_budget_released(definition_server, release):
    advertised_size = 12 * 1024 * 1024
    with ExitStack() as stack:
        clients = [stack.enter_context(client(definition_server)) for _ in range(4)]
        payloads = []
        for index, sock in enumerate(clients[:3]):
            payload, response = publish(sock, name=f"Pending{index}", decoded_size=advertised_size)
            payloads.append(payload)
            assert opcode(response) == 10
        _, response = publish(clients[3], name="OverBudget", decoded_size=advertised_size)
        assert_nack(response, 58)
        if release == "rejection":
            write(clients[0], payloads[0])
            assert_nack(read_msg(clients[0]), 3)
        elif release == "revocation":
            clients[0].sendall(encode_numheader32(
                struct.pack(">I", 0xBFFFFC00) + struct.pack("<II", 4, ADDRESS)))
            assert_nack(read_msg(clients[0]), 59)
        else:
            clients[0].close()
        deadline = time.monotonic() + 5.0
        attempt = 0
        while True:
            _, response = publish(
                clients[3], name=f"Retry{attempt}", address=ADDRESS + 0x10000 * (attempt + 1),
                decoded_size=advertised_size,
            )
            if opcode(response) == 10:
                break
            assert_nack(response, 58)
            assert time.monotonic() < deadline, "Disconnected transfer retained its memory reservation"
            time.sleep(0.05)
            attempt += 1
