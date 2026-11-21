"""End-to-end integration tests for Level 1 and Level 2 server node definition cache."""

import concurrent.futures
import hashlib
import os
import socket
import struct
import subprocess
import time
import zlib
from pathlib import Path

import pytest

from conftest import ApxNodeInstance, _set_pdeathsig, wait_for_unix_socket
from test_server_compressed_nodes import (
    encode_numheader32,
    read_msg,
    sign_data_with_openssl,
    start_server,
)
from test_server_definition_events import (
    assert_nack,
    capture,
    client,
    events,
    opcode,
    publish,
    write,
)


def make_req_definition(name: str, num_ports: int = 4, version: str = "1.3") -> bytes:
    lines = [f"APX/{version}", f'N"{name}"']
    for i in range(num_ports):
        lines.append(f'R"Sig{i}"C:={i}')
    return ("\n".join(lines) + "\n").encode("ascii")


def publish_raw_digest(sock, text: bytes, *, name: str, digest_type: int, digest_bytes: bytes, address: int = 0x02000000):
    header = struct.pack("<IIIHH", 3, address, len(text), 0, digest_type)
    cmd = header + digest_bytes.ljust(32, b"\x00")[:32] + f"{name}.apx\0".encode("ascii")
    sock.sendall(encode_numheader32(struct.pack(">I", 0xBFFFFC00) + cmd))
    return read_msg(sock)


def assert_definition_events(records, name: str, text: bytes):
    publications = [
        e for e in records if e.get("file_name") == f"{name}.apx" and e["event"] == "FILE_PUBLISH"
    ]
    assert len(publications) == 1
    publication = publications[0]
    assert publication["file_size"] == len(text)
    writes = [
        e
        for e in records
        if e["event"] == "FILE_WRITE"
        and e.get("object_id") == publication["object_id"]
        and e.get("connection_id") == publication["connection_id"]
    ]
    assert writes
    reconstructed = b"".join(bytes.fromhex(e["data_hex"]) for e in writes)
    assert reconstructed == text


def test_cache_cold_miss_and_warm_hit_skips_file_open(tmp_path: Path, apx_server_bin: str):
    cache_dir = tmp_path / "cache"
    log_file = tmp_path / "server.log"
    server = start_server(
        tmp_path,
        apx_server_bin,
        {
            "apx-server": {
                "cache": {
                    "level": 1,
                    "path": str(cache_dir),
                }
            },
            "socket-server-extension": {"unix-file": str(tmp_path / "apx.socket")},
            "textlog-extension": {
                "enabled": True,
                "log-level": "DEBUG",
                "file-enabled": True,
                "file-path": str(log_file),
            },
        },
    )
    try:
        name = "CacheHitNode"
        definition = make_req_definition(name, num_ports=8)
        sha_hex = hashlib.sha256(definition).hexdigest()
        cached_file = cache_dir / "text" / f"{name}_{sha_hex}.apx"

        # 1. Cold boot: server opens .apx, receives data, populates cache
        with client(server) as sock:
            payload, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 10  # RMF_CMD_FILE_OPEN for CacheHitNode.apx
            write(sock, payload)
            server_pub = read_msg(sock)
            assert opcode(server_pub) == 3
            assert f"{name}.".encode() in server_pub

        assert cached_file.is_file()
        assert cached_file.read_bytes() == definition

        # 2. Warm reconnect: server skips FILE_OPEN for .apx and immediately publishes .in
        with client(server) as sock:
            _, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 3  # Direct PUBLISH_FILE for .in/.cin, NOT FILE_OPEN (10)
            assert f"{name}.".encode() in resp
    finally:
        server.stop()

    assert server.process.returncode == 0, server.stderr
    log_text = log_file.read_text(encoding="utf-8")
    assert "(cache hit: text)" in log_text


@pytest.mark.parametrize("first_compressed,second_compressed", [(False, True), (True, False)])
def test_cache_cross_representation_plain_and_compressed(
    tmp_path: Path, apx_server_bin: str, first_compressed: bool, second_compressed: bool
):
    cache_dir = tmp_path / "cache"
    server = start_server(
        tmp_path,
        apx_server_bin,
        {
            "apx-server": {
                "cache": {
                    "level": 1,
                    "path": str(cache_dir),
                }
            },
            "socket-server-extension": {"unix-file": str(tmp_path / "apx.socket")},
        },
    )
    try:
        name = "CrossRepNode"
        definition = make_req_definition(name, num_ports=16)

        with client(server) as sock:
            payload, resp = publish(sock, definition, name=name, compressed=first_compressed)
            assert opcode(resp) == 10
            write(sock, payload)
            assert f"{name}.".encode() in read_msg(sock)

        with client(server) as sock:
            _, resp = publish(sock, definition, name=name, compressed=second_compressed)
            assert opcode(resp) == 3  # Cache hit skips transfer & decompression
            assert f"{name}.".encode() in resp
    finally:
        server.stop()

    assert server.process.returncode == 0, server.stderr


def test_cache_definition_update_and_per_node_rotation(tmp_path: Path, apx_server_bin: str):
    cache_dir = tmp_path / "cache"
    server = start_server(
        tmp_path,
        apx_server_bin,
        {
            "apx-server": {
                "cache": {
                    "level": 1,
                    "path": str(cache_dir),
                    "max-versions-per-node": 2,
                    "max-size-mb": 0,
                }
            },
            "socket-server-extension": {"unix-file": str(tmp_path / "apx.socket")},
        },
    )
    try:
        name = "RotNode"
        defs = [make_req_definition(name, num_ports=n) for n in (2, 3, 4)]
        hashes = [hashlib.sha256(d).hexdigest() for d in defs]

        for idx, d in enumerate(defs):
            with client(server) as sock:
                payload, resp = publish(sock, d, name=name, compressed=False)
                assert opcode(resp) == 10
                write(sock, payload)
                assert f"{name}.".encode() in read_msg(sock)
            # Ensure strictly increasing mtimes across versions
            p = cache_dir / "text" / f"{name}_{hashes[idx]}.apx"
            assert p.is_file()
            os.utime(p, (1700000000 + idx * 10, 1700000000 + idx * 10))

        # Oldest version (v0) evicted; newest 2 versions remain
        assert not (cache_dir / "text" / f"{name}_{hashes[0]}.apx").exists()
        assert (cache_dir / "text" / f"{name}_{hashes[1]}.apx").is_file()
        assert (cache_dir / "text" / f"{name}_{hashes[2]}.apx").is_file()

        # Reconnecting with newest version hits cache
        with client(server) as sock:
            _, resp = publish(sock, defs[2], name=name, compressed=False)
            assert opcode(resp) == 3
    finally:
        server.stop()

    assert server.process.returncode == 0, server.stderr


def test_cache_corruption_recovery_heals_file(tmp_path: Path, apx_server_bin: str):
    cache_dir = tmp_path / "cache"
    server = start_server(
        tmp_path,
        apx_server_bin,
        {
            "apx-server": {
                "cache": {
                    "level": 1,
                    "path": str(cache_dir),
                }
            },
            "socket-server-extension": {"unix-file": str(tmp_path / "apx.socket")},
        },
    )
    try:
        name = "HealNode"
        definition = make_req_definition(name, num_ports=6)
        sha_hex = hashlib.sha256(definition).hexdigest()
        cached_file = cache_dir / "text" / f"{name}_{sha_hex}.apx"

        # Warm cache
        with client(server) as sock:
            payload, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 10
            write(sock, payload)
            assert f"{name}.".encode() in read_msg(sock)
        assert cached_file.read_bytes() == definition

        # 1. Same-length byte corruption -> server falls back to FILE_OPEN and heals file
        corrupted = bytearray(definition)
        corrupted[-2] = ord("9")
        cached_file.write_bytes(bytes(corrupted))
        with client(server) as sock:
            payload, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 10
            write(sock, payload)
            assert f"{name}.".encode() in read_msg(sock)
        assert cached_file.read_bytes() == definition

        # 2. Truncated file -> server falls back to FILE_OPEN and heals file
        cached_file.write_bytes(definition[:-5])
        with client(server) as sock:
            payload, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 10
            write(sock, payload)
            assert f"{name}.".encode() in read_msg(sock)
        assert cached_file.read_bytes() == definition
    finally:
        server.stop()

    assert server.process.returncode == 0, server.stderr


def test_cache_signed_nodes_never_bypass_verification(tmp_path: Path, apx_server_bin: str):
    certs = Path(__file__).parent / "certs"
    cache_dir = tmp_path / "cache"
    (cache_dir / "text").mkdir(parents=True)

    name = "SignedCacheNode"
    definition = make_req_definition(name, num_ports=5)
    sha_hex = hashlib.sha256(definition).hexdigest()
    (cache_dir / "text" / f"{name}_{sha_hex}.apx").write_bytes(definition)

    server = start_server(
        tmp_path,
        apx_server_bin,
        {
            "apx-server": {
                "security": {
                    "require-signed-nodes": True,
                    "trusted-keys": [str(certs / "client_pubkey.pem")],
                },
                "cache": {
                    "level": 1,
                    "path": str(cache_dir),
                },
            },
            "socket-server-extension": {"unix-file": str(tmp_path / "apx.socket")},
        },
    )
    try:
        # 1. Unsigned publish with matching cached SHA-256 is still rejected
        with client(server) as sock:
            _, resp = publish(sock, definition, name=name, compressed=False)
            assert_nack(resp, 79)  # APX_SIGNATURE_MISSING_ERROR

        # 2. Signed plain and compressed publishes ALWAYS send FILE_OPEN and verify signature
        sig = sign_data_with_openssl(certs / "client_key.pem", definition)
        for compressed in (False, True):
            with client(server) as sock:
                payload, resp = publish(sock, definition, name=name, compressed=compressed, signature=sig)
                assert opcode(resp) == 10  # Must open and verify over wire
                write(sock, payload)
                assert f"{name}.".encode() in read_msg(sock)
    finally:
        server.stop()

    assert server.process.returncode == 0, server.stderr


def test_cache_sha1_rejected_and_no_digest_not_cached(tmp_path: Path, apx_server_bin: str):
    cache_dir = tmp_path / "cache"
    server = start_server(
        tmp_path,
        apx_server_bin,
        {
            "apx-server": {
                "cache": {
                    "level": 1,
                    "path": str(cache_dir),
                }
            },
            "socket-server-extension": {"unix-file": str(tmp_path / "apx.socket")},
        },
    )
    try:
        # 1. SHA-1 digest rejected with APX_UNSUPPORTED_ERROR (10)
        sha1_def = make_req_definition("Sha1Node", num_ports=2)
        with client(server) as sock:
            resp = publish_raw_digest(
                sock, sha1_def, name="Sha1Node", digest_type=1, digest_bytes=hashlib.sha1(sha1_def).digest()
            )
            assert_nack(resp, 10)

        # 2. RMF_DIGEST_TYPE_NONE succeeds via transfer and does NOT write to text cache
        none_def = make_req_definition("NoDigestNode", num_ports=2)
        with client(server) as sock:
            resp = publish_raw_digest(sock, none_def, name="NoDigestNode", digest_type=0, digest_bytes=b"")
            assert opcode(resp) == 10
            write(sock, none_def)
            assert b"NoDigestNode." in read_msg(sock)

        assert list((cache_dir / "text").glob("*.apx")) == []
    finally:
        server.stop()

    assert server.process.returncode == 0, server.stderr


def test_cache_readonly_directory_fallback(tmp_path: Path, apx_server_bin: str):
    if os.geteuid() == 0:
        pytest.skip("Root bypasses POSIX directory write permissions")

    cache_dir = tmp_path / "cache"
    text_dir = cache_dir / "text"
    text_dir.mkdir(parents=True)

    prewarm_name = "PrewarmedNode"
    prewarm_def = make_req_definition(prewarm_name, num_ports=4)
    prewarm_sha = hashlib.sha256(prewarm_def).hexdigest()
    (text_dir / f"{prewarm_name}_{prewarm_sha}.apx").write_bytes(prewarm_def)

    os.chmod(text_dir, 0o555)
    server = None
    try:
        server = start_server(
            tmp_path,
            apx_server_bin,
            {
                "apx-server": {
                    "cache": {
                        "level": 1,
                        "path": str(cache_dir),
                    }
                },
                "socket-server-extension": {"unix-file": str(tmp_path / "apx.socket")},
            },
        )

        # Pre-warmed entry still serves a cache hit (no FILE_OPEN)
        with client(server) as sock:
            _, resp = publish(sock, prewarm_def, name=prewarm_name, compressed=False)
            assert opcode(resp) == 3

        # Uncached node still connects via wire transfer without failing
        fresh_name = "FreshNode"
        fresh_def = make_req_definition(fresh_name, num_ports=3)
        with client(server) as sock:
            payload, resp = publish(sock, fresh_def, name=fresh_name, compressed=False)
            assert opcode(resp) == 10
            write(sock, payload)
            assert f"{fresh_name}.".encode() in read_msg(sock)
    finally:
        if server is not None:
            server.stop()
        os.chmod(text_dir, 0o755)

    assert server.process.returncode == 0, server.stderr
    assert "read-only" in server.stderr


def test_cache_monitor_and_apxlog_parity_on_hit(tmp_path: Path, apx_server_bin: str, apx_log_bin: str):
    cache_dir = tmp_path / "cache"
    monitor_socket = str(tmp_path / "monitor.socket")
    server = start_server(
        tmp_path,
        apx_server_bin,
        {
            "apx-server": {
                "cache": {
                    "level": 1,
                    "path": str(cache_dir),
                }
            },
            "socket-server-extension": {"unix-file": str(tmp_path / "apx.socket")},
            "monitor-extension": {"enabled": True, "socket-file": monitor_socket},
        },
    )
    server.monitor_socket = monitor_socket
    try:
        name = "MonitoredCachedNode"
        definition = make_req_definition(name, num_ports=12)

        # 1. Warm the cache before starting capture
        with client(server) as sock:
            payload, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 10
            write(sock, payload)
            assert f"{name}.".encode() in read_msg(sock)
        time.sleep(0.2)  # Allow warm-up connection disconnect to complete before capture

        # 2. Capture live + snapshot for a cache-hit client (compressed publication)
        live = tmp_path / "live.apxlog"
        snapshot = tmp_path / "snapshot.apxlog"
        with client(server) as sock:
            with capture(server, apx_log_bin, live):
                _, resp = publish(sock, definition, name=name, compressed=True)
                assert opcode(resp) == 3  # Cache hit!
                time.sleep(0.2)
            with capture(server, apx_log_bin, snapshot):
                pass

        assert_definition_events(events(apx_log_bin, live), name, definition)
        assert_definition_events(events(apx_log_bin, snapshot), name, definition)
    finally:
        server.stop()

    assert server.process.returncode == 0, server.stderr


def test_cache_concurrent_clients_cold_and_warm(tmp_path: Path, apx_server_bin: str):
    cache_dir = tmp_path / "cache"
    server = start_server(
        tmp_path,
        apx_server_bin,
        {
            "apx-server": {
                "cache": {
                    "level": 1,
                    "path": str(cache_dir),
                }
            },
            "socket-server-extension": {"unix-file": str(tmp_path / "apx.socket")},
        },
    )
    try:
        name = "ConcurrentNode"
        definition = make_req_definition(name, num_ports=10)

        def run_client(expect_hit: bool) -> int:
            with client(server) as sock:
                payload, resp = publish(sock, definition, name=name, compressed=False)
                op = opcode(resp)
                if op == 10:
                    assert not expect_hit
                    write(sock, payload)
                    pub_resp = read_msg(sock)
                    assert opcode(pub_resp) == 3
                else:
                    assert op == 3
                return op

        # Cold burst: all 6 clients connect concurrently
        with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
            futs = [pool.submit(run_client, False) for _ in range(6)]
            ops = [f.result(timeout=10.0) for f in futs]
        assert all(op in (3, 10) for op in ops)

        # Warm burst: all 6 clients must get cache hits (opcode 3)
        with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
            futs = [pool.submit(run_client, True) for _ in range(6)]
            ops = [f.result(timeout=10.0) for f in futs]
        assert ops == [3] * 6
    finally:
        server.stop()

    assert server.process.returncode == 0, server.stderr


def test_cache_apx_node_signal_routing_across_hit(tmp_path: Path, apx_server_bin: str, apx_node_bin: str):
    cache_dir = tmp_path / "cache"
    log_file = tmp_path / "server.log"
    sock_path = str(tmp_path / "apx.socket")
    server = start_server(
        tmp_path,
        apx_server_bin,
        {
            "apx-server": {
                "cache": {
                    "level": 1,
                    "path": str(cache_dir),
                }
            },
            "socket-server-extension": {"unix-file": sock_path},
            "textlog-extension": {
                "enabled": True,
                "log-level": "DEBUG",
                "file-enabled": True,
                "file-path": str(log_file),
            },
        },
    )

    sender_file = tmp_path / "Sender.apx"
    listener_file = tmp_path / "Listener.apx"
    sender_file.write_text('APX/1.3\nN"Sender"\nP"Speed"S:=123\n', encoding="utf-8")
    listener_file.write_text('APX/1.3\nN"Listener"\nR"Speed"S:=0\n', encoding="utf-8")

    def launch_node(apx_file: Path, bind_sock: str | None = None, compressed: bool = False) -> ApxNodeInstance:
        cmd = [apx_node_bin, "-c", sock_path]
        if bind_sock is not None:
            cmd += ["-b", bind_sock]
        else:
            cmd.append("--no-bind")
        if compressed:
            cmd.append("--compressed")
        cmd.append(str(apx_file))
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            preexec_fn=_set_pdeathsig,
        )
        inst = ApxNodeInstance(proc, apx_file, bind_sock)
        if bind_sock is not None:
            wait_for_unix_socket(bind_sock, timeout=3.0)
        return inst

    nodes: list[ApxNodeInstance] = []
    try:
        # 1. Cold start: listener + uncompressed sender warm the cache
        listener1 = launch_node(listener_file)
        nodes.append(listener1)
        sender1 = launch_node(sender_file, str(tmp_path / "sender1.socket"), compressed=False)
        nodes.append(sender1)
        assert listener1.wait_for_output('"Speed": 123'), listener1.output
        sender1.stop()
        listener1.stop()
        nodes.clear()
        time.sleep(0.2)

        # 2. Warm reconnect: listener + compressed sender both hit text cache and route signals
        listener2 = launch_node(listener_file)
        nodes.append(listener2)
        sender2_sock = str(tmp_path / "sender2.socket")
        sender2 = launch_node(sender_file, sender2_sock, compressed=True)
        nodes.append(sender2)
        assert listener2.wait_for_output('"Speed": 123'), listener2.output
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as control:
            control.connect(sender2_sock)
            control.sendall(b'{"Speed": 456}\n')
        assert listener2.wait_for_output('"Speed": 456'), listener2.output
    finally:
        for n in nodes:
            n.stop()
        server.stop()

    assert server.process.returncode == 0, server.stderr
    log_text = log_file.read_text(encoding="utf-8")
    assert log_text.count("Server open file: Sender.apx") == 1
    assert log_text.count("Server open file: Listener.apx") == 1
    assert "Server created node Sender (cache hit: text)" in log_text
    assert "Server created node Listener (cache hit: text)" in log_text


def test_cache_lock_prevents_two_servers_sharing_cache_dir(tmp_path: Path, apx_server_bin: str):
    cache_dir = tmp_path / "shared_cache"
    dir1 = tmp_path / "srv1"
    dir2 = tmp_path / "srv2"
    dir3 = tmp_path / "srv3"
    dir1.mkdir()
    dir2.mkdir()
    dir3.mkdir()

    name = "LockedNode"
    definition = make_req_definition(name, num_ports=4)

    server1 = start_server(
        dir1,
        apx_server_bin,
        {
            "apx-server": {"cache": {"level": 1, "path": str(cache_dir)}},
            "socket-server-extension": {"unix-file": str(dir1 / "apx1.socket")},
        },
    )
    server2 = None
    server3 = None
    try:
        # 1. Server 1 warms the cache and holds <cache_dir>/.lock
        assert (cache_dir / ".lock").is_file()
        with client(server1) as sock:
            payload, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 10
            write(sock, payload)
            assert f"{name}.".encode() in read_msg(sock)

        # 2. Server 2 starts with the SAME cache.path -> starts cleanly, warns with Server 1 PID, disables cache
        server2 = start_server(
            dir2,
            apx_server_bin,
            {
                "apx-server": {"cache": {"level": 1, "path": str(cache_dir)}},
                "socket-server-extension": {"unix-file": str(dir2 / "apx2.socket")},
            },
        )
        # Because Server 2 disabled its cache, it must still send FILE_OPEN (10) even though file is cached
        with client(server2) as sock:
            payload, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 10
            write(sock, payload)
            assert f"{name}.".encode() in read_msg(sock)

        server2.stop()
        assert server2.process.returncode == 0, server2.stderr
        assert "already locked" in server2.stderr
        assert str(server1.process.pid) in server2.stderr
        server2 = None

        # 3. Stop Server 1 -> kernel releases lock -> Server 3 acquires lock and gets a cache hit
        server1.stop()
        assert server1.process.returncode == 0, server1.stderr
        server1 = None

        server3 = start_server(
            dir3,
            apx_server_bin,
            {
                "apx-server": {"cache": {"level": 1, "path": str(cache_dir)}},
                "socket-server-extension": {"unix-file": str(dir3 / "apx3.socket")},
            },
        )
        with client(server3) as sock:
            _, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 3  # Cache hit!
    finally:
        if server3 is not None:
            server3.stop()
        if server2 is not None:
            server2.stop()
        if server1 is not None:
            server1.stop()

    assert server3 is not None and server3.process.returncode == 0, server3.stderr


def test_l2_cold_miss_warm_hit_signal_routing_and_apxlog_parity(
    tmp_path: Path, apx_server_bin: str, apx_node_bin: str, apx_log_bin: str
):
    cache_dir = tmp_path / "cache"
    log_file = tmp_path / "server.log"
    sock_path = str(tmp_path / "apx.socket")
    monitor_socket = str(tmp_path / "monitor.socket")
    server = start_server(
        tmp_path,
        apx_server_bin,
        {
            "apx-server": {"cache": {"level": 2, "path": str(cache_dir)}},
            "socket-server-extension": {"unix-file": sock_path},
            "monitor-extension": {"enabled": True, "socket-file": monitor_socket},
            "textlog-extension": {
                "enabled": True,
                "log-level": "DEBUG",
                "file-enabled": True,
                "file-path": str(log_file),
            },
        },
    )
    server.monitor_socket = monitor_socket

    sender_text = 'APX/1.3\nN"SenderL2"\nP"Speed"S:=123\n'
    listener_text = 'APX/1.3\nN"ListenerL2"\nR"Speed"S:=0\n'
    sender_file = tmp_path / "SenderL2.apx"
    listener_file = tmp_path / "ListenerL2.apx"
    sender_file.write_text(sender_text, encoding="utf-8")
    listener_file.write_text(listener_text, encoding="utf-8")

    sender_sha = hashlib.sha256(sender_text.encode("ascii")).hexdigest()
    listener_sha = hashlib.sha256(listener_text.encode("ascii")).hexdigest()

    def launch_node(apx_file: Path, bind_sock: str | None = None, compressed: bool = False) -> ApxNodeInstance:
        cmd = [apx_node_bin, "-c", sock_path]
        if bind_sock is not None:
            cmd += ["-b", bind_sock]
        else:
            cmd.append("--no-bind")
        if compressed:
            cmd.append("--compressed")
        cmd.append(str(apx_file))
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            preexec_fn=_set_pdeathsig,
        )
        inst = ApxNodeInstance(proc, apx_file, bind_sock)
        if bind_sock is not None:
            wait_for_unix_socket(bind_sock, timeout=3.0)
        return inst

    nodes: list[ApxNodeInstance] = []
    try:
        assert (cache_dir / "bin" / ".format_version").is_file()

        # 1. Cold boot populates both text/ and bin/
        listener1 = launch_node(listener_file)
        nodes.append(listener1)
        sender1 = launch_node(sender_file, str(tmp_path / "sender1.socket"), compressed=False)
        nodes.append(sender1)
        assert listener1.wait_for_output('"Speed": 123'), listener1.output
        sender1.stop()
        listener1.stop()
        nodes.clear()
        time.sleep(0.2)

        assert (cache_dir / "text" / f"SenderL2_{sender_sha}.apx").is_file()
        assert (cache_dir / "bin" / f"SenderL2_{sender_sha}.apxc").is_file()
        assert (cache_dir / "text" / f"ListenerL2_{listener_sha}.apx").is_file()
        assert (cache_dir / "bin" / f"ListenerL2_{listener_sha}.apxc").is_file()

        # 2. Warm boot hits Level 2 binary cache, routes signals, and streams logical .apx to apx-log
        live = tmp_path / "live_l2.apxlog"
        snapshot = tmp_path / "snap_l2.apxlog"
        with capture(server, apx_log_bin, live):
            listener2 = launch_node(listener_file)
            nodes.append(listener2)
            sender2_sock = str(tmp_path / "sender2.socket")
            sender2 = launch_node(sender_file, sender2_sock, compressed=True)
            nodes.append(sender2)
            assert listener2.wait_for_output('"Speed": 123'), listener2.output
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as control:
                control.connect(sender2_sock)
                control.sendall(b'{"Speed": 456}\n')
            assert listener2.wait_for_output('"Speed": 456'), listener2.output
            time.sleep(0.2)

        with capture(server, apx_log_bin, snapshot):
            pass

        assert_definition_events(events(apx_log_bin, live), "SenderL2", sender_text.encode("ascii"))
        assert_definition_events(events(apx_log_bin, snapshot), "SenderL2", sender_text.encode("ascii"))
    finally:
        for n in nodes:
            n.stop()
        server.stop()

    assert server.process.returncode == 0, server.stderr
    assert "not yet implemented" not in server.stderr
    log_text = log_file.read_text(encoding="utf-8")
    assert log_text.count("Server open file: SenderL2.apx") == 1
    assert log_text.count("Server open file: ListenerL2.apx") == 1
    assert "Server created node SenderL2 (cache hit: binary)" in log_text
    assert "Server created node ListenerL2 (cache hit: binary)" in log_text


def test_l2_corrupt_binary_falls_back_to_l1_text_without_wire_transfer(tmp_path: Path, apx_server_bin: str):
    cache_dir = tmp_path / "cache"
    log_file = tmp_path / "server.log"
    server = start_server(
        tmp_path,
        apx_server_bin,
        {
            "apx-server": {"cache": {"level": 2, "path": str(cache_dir)}},
            "socket-server-extension": {"unix-file": str(tmp_path / "apx.socket")},
            "textlog-extension": {
                "enabled": True,
                "log-level": "DEBUG",
                "file-enabled": True,
                "file-path": str(log_file),
            },
        },
    )
    try:
        name = "FallbackL2Node"
        definition = make_req_definition(name, num_ports=6)
        sha_hex = hashlib.sha256(definition).hexdigest()
        bin_file = cache_dir / "bin" / f"{name}_{sha_hex}.apxc"

        # 1. Cold boot warms both L1 and L2
        with client(server) as sock:
            payload, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 10
            write(sock, payload)
            assert f"{name}.".encode() in read_msg(sock)
        assert bin_file.is_file()

        # 2. Corrupt .apxc on disk -> server falls back to L1 text (zero wire transfer!) and heals .apxc
        corrupted = bytearray(bin_file.read_bytes())
        corrupted[-1] ^= 0xFF
        bin_file.write_bytes(bytes(corrupted))

        with client(server) as sock:
            _, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 3  # Still zero wire transfer via L1 text hit!
            assert f"{name}.".encode() in resp
        assert bin_file.is_file()

        # 3. Third connect hits regenerated L2 binary cache
        with client(server) as sock:
            _, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 3
    finally:
        server.stop()

    assert server.process.returncode == 0, server.stderr
    log_text = log_file.read_text(encoding="utf-8")
    assert "(cache hit: text)" in log_text
    assert "(cache hit: binary)" in log_text


def test_l2_startup_format_version_sweep_preserves_text_cache(tmp_path: Path, apx_server_bin: str):
    cache_dir = tmp_path / "cache"
    dir1 = tmp_path / "run1"
    dir2 = tmp_path / "run2"
    dir1.mkdir()
    dir2.mkdir()
    log2 = dir2 / "server.log"

    name = "UpgradeNode"
    definition = make_req_definition(name, num_ports=5)
    sha_hex = hashlib.sha256(definition).hexdigest()
    text_file = cache_dir / "text" / f"{name}_{sha_hex}.apx"
    bin_file = cache_dir / "bin" / f"{name}_{sha_hex}.apxc"

    # 1. Run 1 warms both text/ and bin/
    server1 = start_server(
        dir1,
        apx_server_bin,
        {
            "apx-server": {"cache": {"level": 2, "path": str(cache_dir)}},
            "socket-server-extension": {"unix-file": str(dir1 / "apx.socket")},
        },
    )
    try:
        with client(server1) as sock:
            payload, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 10
            write(sock, payload)
            assert f"{name}.".encode() in read_msg(sock)
    finally:
        server1.stop()
    assert text_file.is_file()
    assert bin_file.is_file()

    # 2. Simulate server upgrade by writing an old .format_version stamp
    (cache_dir / "bin" / ".format_version").write_text("0/vm2.0/abi0\n", encoding="ascii")

    server2 = start_server(
        dir2,
        apx_server_bin,
        {
            "apx-server": {"cache": {"level": 2, "path": str(cache_dir)}},
            "socket-server-extension": {"unix-file": str(dir2 / "apx.socket")},
            "textlog-extension": {
                "enabled": True,
                "log-level": "DEBUG",
                "file-enabled": True,
                "file-path": str(log2),
            },
        },
    )
    try:
        # Startup sweep cleared bin/ while keeping text/
        assert not bin_file.exists()
        assert text_file.is_file()

        # First connect after upgrade hits text/ (no wire transfer) and regenerates bin/
        with client(server2) as sock:
            _, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 3
        assert bin_file.is_file()

        # Second connect hits regenerated bin/
        with client(server2) as sock:
            _, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 3
    finally:
        server2.stop()

    assert server2.process.returncode == 0, server2.stderr
    assert "Binary cache format changed" in server2.stderr
    log_text = log2.read_text(encoding="utf-8")
    assert "(cache hit: text)" in log_text
    assert "(cache hit: binary)" in log_text


def test_l2_signed_node_post_transfer_binary_hit(tmp_path: Path, apx_server_bin: str):
    certs = Path(__file__).parent / "certs"
    cache_dir = tmp_path / "cache"
    log_file = tmp_path / "server.log"

    name = "SignedL2Node"
    definition = make_req_definition(name, num_ports=6)
    sha_hex = hashlib.sha256(definition).hexdigest()
    sig = sign_data_with_openssl(certs / "client_key.pem", definition)

    server = start_server(
        tmp_path,
        apx_server_bin,
        {
            "apx-server": {
                "security": {
                    "require-signed-nodes": True,
                    "trusted-keys": [str(certs / "client_pubkey.pem")],
                },
                "cache": {"level": 2, "path": str(cache_dir)},
            },
            "socket-server-extension": {"unix-file": str(tmp_path / "apx.socket")},
            "textlog-extension": {
                "enabled": True,
                "log-level": "INFO",
                "file-enabled": True,
                "file-path": str(log_file),
            },
        },
    )
    try:
        # 1. Cold signed publish: transfers, verifies signature, compiles, stores bin/ ONLY (not text/)
        with client(server) as sock:
            payload, resp = publish(sock, definition, name=name, compressed=False, signature=sig)
            assert opcode(resp) == 10
            write(sock, payload)
            assert f"{name}.".encode() in read_msg(sock)

        assert (cache_dir / "bin" / f"{name}_{sha_hex}.apxc").is_file()
        assert list((cache_dir / "text").glob("*.apx")) == []

        # 2. Warm signed compressed publish: still transfers & verifies signature, hits bin/ post-transfer
        with client(server) as sock:
            payload, resp = publish(sock, definition, name=name, compressed=True, signature=sig)
            assert opcode(resp) == 10
            write(sock, payload)
            assert f"{name}.".encode() in read_msg(sock)

        # 3. Bad signature MUST still be rejected even though .apxc is cached!
        bad_sig = bytes([sig[0] ^ 0xFF]) + sig[1:]
        with client(server) as sock:
            payload, resp = publish(sock, definition, name=name, compressed=False, signature=bad_sig)
            assert opcode(resp) == 10
            write(sock, payload)
            assert_nack(read_msg(sock), 80)  # APX_SIGNATURE_VERIFICATION_ERROR
    finally:
        server.stop()

    assert server.process.returncode == 0, server.stderr
    log_text = log_file.read_text(encoding="utf-8")
    assert f"Server created node {name} (cache hit: binary)" in log_text


def test_l2_no_digest_post_transfer_binary_hit(tmp_path: Path, apx_server_bin: str):
    cache_dir = tmp_path / "cache"
    log_file = tmp_path / "server.log"
    server = start_server(
        tmp_path,
        apx_server_bin,
        {
            "apx-server": {"cache": {"level": 2, "path": str(cache_dir)}},
            "socket-server-extension": {"unix-file": str(tmp_path / "apx.socket")},
            "textlog-extension": {
                "enabled": True,
                "log-level": "INFO",
                "file-enabled": True,
                "file-path": str(log_file),
            },
        },
    )
    try:
        name = "NoDigestL2Node"
        definition = make_req_definition(name, num_ports=4)
        sha_hex = hashlib.sha256(definition).hexdigest()

        # 1. Cold connect without digest: stores bin/ ONLY (not text/)
        with client(server) as sock:
            resp = publish_raw_digest(sock, definition, name=name, digest_type=0, digest_bytes=b"")
            assert opcode(resp) == 10
            write(sock, definition)
            assert f"{name}.".encode() in read_msg(sock)
        assert (cache_dir / "bin" / f"{name}_{sha_hex}.apxc").is_file()
        assert list((cache_dir / "text").glob("*.apx")) == []

        # 2. Second connect without digest: transfers and hits bin/ post-transfer
        with client(server) as sock:
            resp = publish_raw_digest(sock, definition, name=name, digest_type=0, digest_bytes=b"")
            assert opcode(resp) == 10
            write(sock, definition)
            assert f"{name}.".encode() in read_msg(sock)
    finally:
        server.stop()

    assert server.process.returncode == 0, server.stderr
    log_text = log_file.read_text(encoding="utf-8")
    assert f"Server created node {name} (cache hit: binary)" in log_text


def test_l2_prewarmed_readonly_bin_only_with_monitor_enabled(tmp_path: Path, apx_server_bin: str):
    if os.geteuid() == 0:
        pytest.skip("Root bypasses POSIX directory write permissions")

    cache_dir = tmp_path / "cache"
    dir1 = tmp_path / "warm"
    dir2 = tmp_path / "ro"
    dir1.mkdir()
    dir2.mkdir()
    log2 = dir2 / "server.log"

    name = "RoBinOnlyNode"
    definition = make_req_definition(name, num_ports=4)
    sha_hex = hashlib.sha256(definition).hexdigest()

    # 1. Warm bin/ then delete text/ to simulate bin-only pre-warmed read-only image
    server1 = start_server(
        dir1,
        apx_server_bin,
        {
            "apx-server": {"cache": {"level": 2, "path": str(cache_dir)}},
            "socket-server-extension": {"unix-file": str(dir1 / "apx.socket")},
        },
    )
    try:
        with client(server1) as sock:
            payload, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 10
            write(sock, payload)
            assert f"{name}.".encode() in read_msg(sock)
    finally:
        server1.stop()

    (cache_dir / "text" / f"{name}_{sha_hex}.apx").unlink()
    os.chmod(cache_dir / "bin", 0o555)
    server2 = None
    try:
        server2 = start_server(
            dir2,
            apx_server_bin,
            {
                "apx-server": {"cache": {"level": 2, "path": str(cache_dir)}},
                "socket-server-extension": {"unix-file": str(dir2 / "apx.socket")},
                "monitor-extension": {"enabled": True, "socket-file": str(dir2 / "monitor.socket")},
                "textlog-extension": {
                    "enabled": True,
                    "log-level": "INFO",
                    "file-enabled": True,
                    "file-path": str(log2),
                },
            },
        )
        with client(server2) as sock:
            _, resp = publish(sock, definition, name=name, compressed=False)
            assert opcode(resp) == 3  # Direct L2 binary hit even without text/!
    finally:
        if server2 is not None:
            server2.stop()
        os.chmod(cache_dir / "bin", 0o755)

    assert server2.process.returncode == 0, server2.stderr
    assert "read-only" in server2.stderr
    assert f"Server created node {name} (cache hit: binary)" in log2.read_text(encoding="utf-8")



